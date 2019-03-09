/* 
 * MIR html downloading/parsing demo version 2019
 *
 * Goal of this demo: 
   1. Download the html from the URL using libcurl;
   2. Extract the weblinks information from the URL using the Haut-html API;
   2. Extract the image links information from the URL using the Haut-html API;

   Compile the demo program with this command:
   
   make

   or

   gcc htmlprocess.c -o htmlprocess -Ihaut-html/include -Lhaut-html/lib -lcurl -lhaut
   
   Run the program with this command:
   ./htmlprocess www.liacs.nl

   (Have in mind that you first need to 'make clean && make' the parser)
*/

#include <stdio.h> 
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include <curl/curl.h>
#include <haut/string_util.h>
#include <haut/haut.h>
#include <haut/tag.h>

#define URL_BUFFER_SIZE 10000
#define MIN(a,b) (((a)<(b))?(a):(b))

char END_LINE[1] = {"\n"};
char SLASH[1] = {"/"};

/* We use this structure to encapsulate any additional data we might need during parsing.
 * An instance of this structure is passed along through the parser's state object, 
 * which we can access at any time from one of the callback functions.
 * (Think of it as a safe alternative of using global variables) */
typedef struct {
    CURL* curl;
    size_t base_length; // Length of the base url in the effective url
    strbuffer_t base_url; // Effective url of the query as provided by CURL
    strbuffer_t base_tag_url; // Base url of the query as provided by the parser (attribute "base")
    strbuffer_t protocol; //The protocol of the base url
    strbuffer_t webpage; // webpage contents
    strbuffer_t attributeList; //A list with all content of given attribute (In our case the links)
    int tag; //The tag we are looking ofr
    char * attribute; //The attribute we are looking ofr
} state_t;

typedef state_t* state_tp;
/*** Added for solution ***/
static const char* ABS_PREFIX[] = { "http:", "https:", "file:", "ftp:", "\0" };

/* Checks whether @url is absolute, i.e. begins with one of ABS_PREFIX */
bool
is_absolute_url( strfragment_t *url ) {
    const char **prefix = ABS_PREFIX;
    /* Iterate over strings in ABS_PREFIX */
    while( 1 ) {
        if( *prefix[0] == '\0' ) break;
        if( strfragment_nicmp( url, *prefix, strlen( *prefix ) ) )
            return true;
        prefix++;
    }
    return false;
}

size_t absolute_length(char *url){
    size_t length = 0;
    int count = 0;
    while(*url!='\0'){
        if(*url++ == '/') count++;
        if(count==3) break;
        length++;
    }
    return ++length;
}

void get_protocol(state_tp document, char *url){
    size_t length = 0;
    while(url[length]!='\0')
        if(url[length++] == ':') break;
    strbuffer_append(&(document->protocol), url, length);
}

strbuffer_t convert_to_link_absolut(strfragment_t *value, state_tp document){
    //Catch the full absolute urls
    const char *link = value->data;
    strbuffer_t absolute_link;
    strbuffer_init(&absolute_link);
    if (link[0] == '#'){ //Anchors are links on the page itself and don't need to be processed
        strbuffer_free(&absolute_link);
        return absolute_link;
    }
    else if (link[0] == '/'){
        if (link[1] == '/'){//Everything starting with // uses the same protocol, but is interpreted as an absolute url
            if(document->protocol.size==0){
                if(document->base_tag_url.size > 0)
                    get_protocol(document, document->base_tag_url.data);
                else
                    get_protocol(document, document->base_url.data);
            }
            strbuffer_append(&absolute_link, document->protocol.data, document->protocol.size);
            strbuffer_append(&absolute_link, SLASH, 1);
        } else {//Everything starting with a single '/' is an absolute path on the current host
            if(document->base_tag_url.size>0)
                strbuffer_append(&absolute_link, document->base_tag_url.data, document->base_tag_url.size);
            else
                strbuffer_append(&absolute_link, document->base_url.data, document->base_length);
        }
        strbuffer_append(&absolute_link, link+1, value->size-1);
        strbuffer_append(&absolute_link, END_LINE, 1);
        return absolute_link;
    }
    else if(strncasecmp(link,"javascript:", MIN(value->size, 11)) == 0 || 
        strncasecmp(link,"mailto:", MIN(value->size, 7) == 0)){
        strbuffer_free(&absolute_link);
        return absolute_link;
    }
    else {//Interpret this as a relative url on the current path
        if(document->base_tag_url.size > 0){
            if(document->base_tag_url.size != document->base_url.size || 
                strncmp(document->base_tag_url.data, document->base_url.data, document->base_tag_url.size) != 0){
                strbuffer_append(&absolute_link, document->base_tag_url.data, document->base_tag_url.size);
                strbuffer_append(&absolute_link, link, value->size);
                strbuffer_append(&absolute_link, END_LINE, 1);
                return absolute_link;
            }
        }
        strbuffer_append(&absolute_link, document->base_url.data, document->base_url.size);
        if(document->base_url.data[document->base_url.size-1] != '/') strbuffer_append(&absolute_link, SLASH, 1);
        strbuffer_append(&absolute_link, link, value->size);
        strbuffer_append(&absolute_link, END_LINE, 1);
        return absolute_link;
    }
    strbuffer_free(&absolute_link);
    return absolute_link;
}

/* In this function, the HTML-file is written and HTML-code is saved. 
 * This function is called by CURL whenever data becomes available, which can be a chunk or the whole file at once*/
static size_t 
write_callback(char *buffer, size_t size, size_t nmemb, state_tp webpageData)
{ 
    /*** Added for solution ***/
    /* On the first run of this function, query CURL for the base url (after possible redirections) */
    if( webpageData->base_url.size == 0 ){
        CURL* curl = webpageData->curl;
        char* url_ptr;
        curl_easy_getinfo( curl, CURLINFO_EFFECTIVE_URL, &url_ptr );
        strbuffer_append( &(webpageData->base_url), url_ptr, strlen( url_ptr ) );
        printf("Using url:\n%s\n", url_ptr);
    }
    
    /* the size of the received data */
    size_t realsize = size * nmemb; 

    /*Append data to the part of the webpage we already have*/
    strbuffer_append(&(webpageData->webpage), buffer, realsize);
    
    return realsize;
}

/* This function is called everytime the parser has processed an attribute.
 * Arguments are a pointer to the parser object, the key and the value of the attribute.
 * In case of a void-attribute, value may contain a null-pointer.
 */
void
attribute_callback( haut_t* p, strfragment_t* key, strfragment_t* value ) {

    /* First of all, retrieve our custom state object */
    state_t* document =(state_tp)p->userdata;
    /* We need to know what element this attribute belongs to,
     * for this we use haut_currentElementTag(), which returns an
     * enumerated value of one of the standard HTML5 tags.
     */

    /*Get the base href url in order to be able and properly construct absolute URLs*/
    if( haut_currentElementTag( p ) == TAG_BASE ) {
        if( strfragment_icmp( key, "href" ) && value && value->data && value->size > 0 )
            strbuffer_append(&(document->base_tag_url), value->data, value->size);
    }

    /*Grep all objects with the TAG we want*/
    if( haut_currentElementTag( p ) == document->tag ) {
        // So this is a link, now we're interested in the attribute
        if( strfragment_icmp( key, document->attribute ) && value && value->data && value->size > 0 ) {
            strbuffer_t link;
            /*** Added for solution ***/
            /*If link is relative construct the absolute else add it to the list*/
            if( !is_absolute_url( value ) )
                link = convert_to_link_absolut(value, document);
            else{
                strbuffer_init(&link);
                strbuffer_append(&link, value->data, value->size);
                strbuffer_append(&link, END_LINE, 1);
            }

            /*Sanity check and save link*/
            if (link.size == 0) return;
            char *p = link.data;
            for ( ; *p; ++p) *p = tolower(*p);
            strbuffer_append(&(document->attributeList), link.data, link.size);
            strbuffer_free(&link);
        }

        /* Note that key and value are both char-pointer
         * in a wrapper structure strfragment_t. This structure contains
         * a data and a size field. See also haut/string_util.h 
         * 
         * The pointers in key and value are only valid during this function,
         * if you need the data later, it should be copied! 
         */
    }

}

char *extractAttributesOfTag(state_tp document, int tag, char *attribute){

    /*Initialize buffers*/
    document->attribute = attribute;
    document->tag = tag;
    strbuffer_init(&(document->attributeList));
    strbuffer_init(&(document->base_tag_url));
    strbuffer_init(&(document->protocol));

    document->base_length = absolute_length(document->base_url.data);

    /* Create an instance of the parser's state structure and initialize */
    haut_t parser;
    haut_init(&parser);

    /* Setup our event handler.
     * The Haut parser uses a SAX-like interface, which means it will not generate a DOM-tree for us.
     * Instead, we supply the parser with callbacks of events that we are interested in.
     * If the parser encounters one of these events it will let us know by using the callback.
     * In this case, we are only interesting in attributes (<a href=...>)
     */


    //Add callback to the parser and set the userdata
    parser.events.attribute = attribute_callback;
    parser.userdata = document;

    //Set the input buffer and let the parser run
    haut_setInput(&parser, document->webpage.data, document->webpage.size);
    haut_parse(&parser);

    /*If there are links copy them to the output pointer (Better clean up)*/
    char *result = NULL;
    if(document->attributeList.size > 0){
        result = (char *)malloc(document->attributeList.size*sizeof(char));
        strcpy(result, document->attributeList.data);
    }

    //clean up

    /* Release the memory allocated by Haut */
    haut_destroy(&parser);
    strbuffer_free(&(document->protocol));
    strbuffer_free(&(document->base_tag_url));
    strbuffer_free(&(document->attributeList));
    return result;
}


char* getWebPage(char *myUrl){
    /* curl_easy_init() initializes CURL and this call must have a corresponding call to curl_easy_cleanup() */  
    state_tp webpageData = (state_tp)malloc(sizeof(state_t));
    CURL *curl = curl_easy_init();
    webpageData->curl = curl;
    strbuffer_init( &(webpageData->webpage) );
    strbuffer_init( &(webpageData->base_url) );
    /* Tell CURL the URL address we are going to download */
    curl_easy_setopt( curl, CURLOPT_URL, myUrl );

    /* Pass a pointer to the function write_callback( char *ptr, size_t size, size_t nmemb, void *userdata)
     * write_callback() gets called by libcurl as soon as there is data received, 
     * and we can process the received data, such as saving and weblinks extraction. */
    curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, write_callback );

    /* Set the `userdata' argument of the write_callback function to contain our parser pointer */
    curl_easy_setopt( curl, CURLOPT_WRITEDATA, webpageData );

    /* CURL_FOLLOWLOCATION set to 1 tells the CURL to follow any `Location:'-header that the server sends as part of a HTTP header.
     * This means that the library will re-send the same request on the new location 
     * and follow new `Location:'-headers all the way until no more such headers are returned.*/
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);

    /* Tell CURL to perform its operation (download the url) */
    CURLcode curl_res = curl_easy_perform(curl);
    char * webpage = NULL;
    if( curl_res == 0 ) { 
        printf( "HTML file downloaded successfully\n" );
        strncpy(myUrl, webpageData->base_url.data, webpageData->base_url.size);
        myUrl[webpageData->base_url.size] = '\0';
        webpage = webpageData->webpage.data;
    }
    else{
        printf( "Could not dowload webpage\n" );
        strbuffer_free(&(webpageData->webpage));
    }
    /* Tell CURL to deallocate its resources */
    curl_easy_cleanup(curl);
    strbuffer_free(&(webpageData->base_url));
    webpageData->curl = NULL;
    free(webpageData);
    /*** Added for solution ***/
    return webpage;
}


char *GetLinksFromWebPage(char *myhtmlpage, char *myUrl){
    /*Define the attribute we are interested in*/
    char *attribute = (char *)malloc(5*sizeof(char));
    strncpy(attribute, "href\0", 5);
    state_tp document = (state_tp)malloc(sizeof(state_t));

    /*Initialize buffers*/
    strbuffer_init( &(document->webpage) );
    strbuffer_init( &(document->base_url) );

    /*Add proper information to buffers*/
    strbuffer_append(&(document->webpage), myhtmlpage, strlen(myhtmlpage));
    strbuffer_append(&(document->base_url), myUrl, strlen(myUrl));

    /*Extract the links*/
    char * result = extractAttributesOfTag(document, TAG_A, attribute);

    /*Clean up*/
    free(attribute);
    strbuffer_free( &(document->webpage) );
    strbuffer_free( &(document->base_url) );
    free(document);

    /*Return result */
    return result;
}

char *replace_str(char *str, char *orig, char *rep)
{
  static char buffer[4096];
  char *p;

  if(!(p = strstr(str, orig)))  // Is 'orig' even in 'str'?
    return str;

  strncpy(buffer, str, p-str); // Copy characters from 'str' start to 'orig' st$
  buffer[p-str] = '\0';

  sprintf(buffer+(p-str), "%s%s", rep, p+strlen(orig));

  return buffer;
}

int main(int argc, char *argv[])
{   
    /* We need one argument */
    if( argc != 2 ) return 1;

    
    /* Allocate memory for the link (needed bacause we might change it later on) ... */
    size_t qLen = strlen(argv[1]);
    char *q = (char *)malloc(URL_BUFFER_SIZE*sizeof(char));
    strcpy(q, argv[1]);
    q[qLen] = '\0';
   
    /*Download webpage at q*/
    char * webpage = getWebPage(q);
    char * webLinks = GetLinksFromWebPage(webpage, q);
    int iterator = 0;
    while (iterator <= 300) {
       iterator++;
    }
    int LineSize = strcspn(webLinks, "\n");
    char * p = (char *)malloc(URL_BUFFER_SIZE*sizeof(char));
    p  = strncpy(p, webLinks, LineSize);
    webLinks = replace_str(webLinks, p, "");
    /*Print everything*/
    printf("All links:\n%s\n", webLinks);
    printf("First line:%s", p);
    // haut_destroy(&parser);
        
    /* Release the memory allocated for myState */
    /* Release the memory allocated for our data */
    // if(webpage!=NULL) free(webpage);
    // if(webLinks!=NULL) free(webLinks);
    // free(q); 

    return 0;
}
