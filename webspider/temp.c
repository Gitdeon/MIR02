char * webpage = getWebPage(link);
    char * q = GetLinksFromWebPage(webpage, link);
    char * h = NULL;
    char * newlinks = NULL;
    printf("All links:\n%s\n", q);
    int iterator = 0;
    while (iterator <= 300) {
       int LineSize = strcspn(q, "\n");
       char * p = (char *)malloc(URL_BUFFER_SIZE*sizeof(char));
       p  = strncpy(p, q, LineSize);
       q = replace_str(q, p, "");
       // IF q is empty, go to next link in list
       h = getWebPage(p);
       newlinks = GetLinksFromWebPage(h,p);
       printf("All new links:\n%s\n", newlinks);
       iterator++;
       char * temp_q;
       temp_q = q;
       if((q = malloc(strlen(temp_q)+strlen(newlinks)+1)) != NULL){
          q[0] = '\0';   // ensures the memory is an empty string
          strcat(q, temp_q);
          strcat(q, newlinks);
       }
       else {
          printf("malloc failed!\n");
       }
       printf("List: \n %s", q);
    }
