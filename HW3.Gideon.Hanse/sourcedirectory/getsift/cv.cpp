#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <assert.h>
#include "cv.h"
#include "error.h"
#include "global.h"
#include "alloc.h"
#include <float.h>
#include <math.h>
#include "jpeglib.h"

/////////////////////////libjpeg:read image and save image////////////////////////////

struct my_error_mgr 
{
	struct jpeg_error_mgr pub; /* "public" fields */
	jmp_buf setjmp_buffer; /* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

void my_error_exit(j_common_ptr cinfo)
{
	/* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
	my_error_ptr myerr = (my_error_ptr) cinfo->err;
	/* Always display the message. */
	/* We could postpone this until after returning, if we chose. */
	(*cinfo->err->output_message) (cinfo);
	/* Return control to the setjmp point */
	longjmp(myerr->setjmp_buffer, 1);
}

IplImage* read_jpeg_file(char * filename)
{
	IplImage* image=0;		
	struct jpeg_decompress_struct cinfo;
	struct my_error_mgr jerr;
	FILE* infile;
	infile=fopen(filename,"rb");
	if(infile==NULL)
	{
        printf("can't open jpeg image/n");
		return 0;
	}	
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;	
	if(setjmp(jerr.setjmp_buffer))
	{
		jpeg_destroy_decompress(&cinfo);
		fclose(infile);
		return 0;
	}	
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo,infile);
	jpeg_read_header(&cinfo,TRUE);
	jpeg_start_decompress(&cinfo);
    
    image = cvCreateImage(cvSize(cinfo.output_width, cinfo.output_height),IPL_DEPTH_8U, cinfo.output_components);        
	JSAMPARRAY imageBuffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, cinfo.output_width*cinfo.output_components, 1);
	for (int y = 0; y < cinfo.output_height; y++) 
	{
		jpeg_read_scanlines(&cinfo, imageBuffer, 1);
		unsigned char* dstRow = (unsigned char*)image->imageData + image->widthStep*y;
		memcpy(dstRow, imageBuffer[0], cinfo.output_width*cinfo.output_components);
	}
	
	jpeg_finish_decompress(&cinfo);

    // clean up.
    jpeg_destroy_decompress(&cinfo); 

    return image;
}

void write_jpeg_file(char *filename, const IplImage* img_)
{
    const IplImage* img = img_;
    IplImage* rgb = 0;

    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr       jerr;
 
    // set up compression structure
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

	FILE* outfile;
	outfile=fopen(filename,"wb");
	if(outfile==NULL)
	{
        printf("can't compress jpeg image/n");
		exit(1);
	}
	jpeg_stdio_dest(&cinfo, outfile);
    
    cinfo.image_width      = img->width;
    cinfo.image_height     = img->height;
    cinfo.input_components = img->nChannels;
    if (img->nChannels == 3) 
	{
      cinfo.in_color_space   = JCS_RGB;
    }
	else 
	{
      cinfo.in_color_space   = JCS_GRAYSCALE;
    }
    
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality (&cinfo, 99, true); // set max quality. [0..100]
    jpeg_start_compress(&cinfo, true);

    JSAMPROW row;
    while (cinfo.next_scanline < cinfo.image_height) {
      row = (JSAMPROW)((char*)img->imageData + cinfo.next_scanline*img->widthStep);
      jpeg_write_scanlines(&cinfo, &row, 1);
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

    cvReleaseImage(&rgb); // release rgb scratch if it was used.
}
  

float icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE+1)*2];

const float icv8x32fTab_cv[] =
{
    -256.f, -255.f, -254.f, -253.f, -252.f, -251.f, -250.f, -249.f,
    -248.f, -247.f, -246.f, -245.f, -244.f, -243.f, -242.f, -241.f,
    -240.f, -239.f, -238.f, -237.f, -236.f, -235.f, -234.f, -233.f,
    -232.f, -231.f, -230.f, -229.f, -228.f, -227.f, -226.f, -225.f,
    -224.f, -223.f, -222.f, -221.f, -220.f, -219.f, -218.f, -217.f,
    -216.f, -215.f, -214.f, -213.f, -212.f, -211.f, -210.f, -209.f,
    -208.f, -207.f, -206.f, -205.f, -204.f, -203.f, -202.f, -201.f,
    -200.f, -199.f, -198.f, -197.f, -196.f, -195.f, -194.f, -193.f,
    -192.f, -191.f, -190.f, -189.f, -188.f, -187.f, -186.f, -185.f,
    -184.f, -183.f, -182.f, -181.f, -180.f, -179.f, -178.f, -177.f,
    -176.f, -175.f, -174.f, -173.f, -172.f, -171.f, -170.f, -169.f,
    -168.f, -167.f, -166.f, -165.f, -164.f, -163.f, -162.f, -161.f,
    -160.f, -159.f, -158.f, -157.f, -156.f, -155.f, -154.f, -153.f,
    -152.f, -151.f, -150.f, -149.f, -148.f, -147.f, -146.f, -145.f,
    -144.f, -143.f, -142.f, -141.f, -140.f, -139.f, -138.f, -137.f,
    -136.f, -135.f, -134.f, -133.f, -132.f, -131.f, -130.f, -129.f,
    -128.f, -127.f, -126.f, -125.f, -124.f, -123.f, -122.f, -121.f,
    -120.f, -119.f, -118.f, -117.f, -116.f, -115.f, -114.f, -113.f,
    -112.f, -111.f, -110.f, -109.f, -108.f, -107.f, -106.f, -105.f,
    -104.f, -103.f, -102.f, -101.f, -100.f,  -99.f,  -98.f,  -97.f,
     -96.f,  -95.f,  -94.f,  -93.f,  -92.f,  -91.f,  -90.f,  -89.f,
     -88.f,  -87.f,  -86.f,  -85.f,  -84.f,  -83.f,  -82.f,  -81.f,
     -80.f,  -79.f,  -78.f,  -77.f,  -76.f,  -75.f,  -74.f,  -73.f,
     -72.f,  -71.f,  -70.f,  -69.f,  -68.f,  -67.f,  -66.f,  -65.f,
     -64.f,  -63.f,  -62.f,  -61.f,  -60.f,  -59.f,  -58.f,  -57.f,
     -56.f,  -55.f,  -54.f,  -53.f,  -52.f,  -51.f,  -50.f,  -49.f,
     -48.f,  -47.f,  -46.f,  -45.f,  -44.f,  -43.f,  -42.f,  -41.f,
     -40.f,  -39.f,  -38.f,  -37.f,  -36.f,  -35.f,  -34.f,  -33.f,
     -32.f,  -31.f,  -30.f,  -29.f,  -28.f,  -27.f,  -26.f,  -25.f,
     -24.f,  -23.f,  -22.f,  -21.f,  -20.f,  -19.f,  -18.f,  -17.f,
     -16.f,  -15.f,  -14.f,  -13.f,  -12.f,  -11.f,  -10.f,   -9.f,
      -8.f,   -7.f,   -6.f,   -5.f,   -4.f,   -3.f,   -2.f,   -1.f,
       0.f,    1.f,    2.f,    3.f,    4.f,    5.f,    6.f,    7.f,
       8.f,    9.f,   10.f,   11.f,   12.f,   13.f,   14.f,   15.f,
      16.f,   17.f,   18.f,   19.f,   20.f,   21.f,   22.f,   23.f,
      24.f,   25.f,   26.f,   27.f,   28.f,   29.f,   30.f,   31.f,
      32.f,   33.f,   34.f,   35.f,   36.f,   37.f,   38.f,   39.f,
      40.f,   41.f,   42.f,   43.f,   44.f,   45.f,   46.f,   47.f,
      48.f,   49.f,   50.f,   51.f,   52.f,   53.f,   54.f,   55.f,
      56.f,   57.f,   58.f,   59.f,   60.f,   61.f,   62.f,   63.f,
      64.f,   65.f,   66.f,   67.f,   68.f,   69.f,   70.f,   71.f,
      72.f,   73.f,   74.f,   75.f,   76.f,   77.f,   78.f,   79.f,
      80.f,   81.f,   82.f,   83.f,   84.f,   85.f,   86.f,   87.f,
      88.f,   89.f,   90.f,   91.f,   92.f,   93.f,   94.f,   95.f,
      96.f,   97.f,   98.f,   99.f,  100.f,  101.f,  102.f,  103.f,
     104.f,  105.f,  106.f,  107.f,  108.f,  109.f,  110.f,  111.f,
     112.f,  113.f,  114.f,  115.f,  116.f,  117.f,  118.f,  119.f,
     120.f,  121.f,  122.f,  123.f,  124.f,  125.f,  126.f,  127.f,
     128.f,  129.f,  130.f,  131.f,  132.f,  133.f,  134.f,  135.f,
     136.f,  137.f,  138.f,  139.f,  140.f,  141.f,  142.f,  143.f,
     144.f,  145.f,  146.f,  147.f,  148.f,  149.f,  150.f,  151.f,
     152.f,  153.f,  154.f,  155.f,  156.f,  157.f,  158.f,  159.f,
     160.f,  161.f,  162.f,  163.f,  164.f,  165.f,  166.f,  167.f,
     168.f,  169.f,  170.f,  171.f,  172.f,  173.f,  174.f,  175.f,
     176.f,  177.f,  178.f,  179.f,  180.f,  181.f,  182.f,  183.f,
     184.f,  185.f,  186.f,  187.f,  188.f,  189.f,  190.f,  191.f,
     192.f,  193.f,  194.f,  195.f,  196.f,  197.f,  198.f,  199.f,
     200.f,  201.f,  202.f,  203.f,  204.f,  205.f,  206.f,  207.f,
     208.f,  209.f,  210.f,  211.f,  212.f,  213.f,  214.f,  215.f,
     216.f,  217.f,  218.f,  219.f,  220.f,  221.f,  222.f,  223.f,
     224.f,  225.f,  226.f,  227.f,  228.f,  229.f,  230.f,  231.f,
     232.f,  233.f,  234.f,  235.f,  236.f,  237.f,  238.f,  239.f,
     240.f,  241.f,  242.f,  243.f,  244.f,  245.f,  246.f,  247.f,
     248.f,  249.f,  250.f,  251.f,  252.f,  253.f,  254.f,  255.f,
     256.f,  257.f,  258.f,  259.f,  260.f,  261.f,  262.f,  263.f,
     264.f,  265.f,  266.f,  267.f,  268.f,  269.f,  270.f,  271.f,
     272.f,  273.f,  274.f,  275.f,  276.f,  277.f,  278.f,  279.f,
     280.f,  281.f,  282.f,  283.f,  284.f,  285.f,  286.f,  287.f,
     288.f,  289.f,  290.f,  291.f,  292.f,  293.f,  294.f,  295.f,
     296.f,  297.f,  298.f,  299.f,  300.f,  301.f,  302.f,  303.f,
     304.f,  305.f,  306.f,  307.f,  308.f,  309.f,  310.f,  311.f,
     312.f,  313.f,  314.f,  315.f,  316.f,  317.f,  318.f,  319.f,
     320.f,  321.f,  322.f,  323.f,  324.f,  325.f,  326.f,  327.f,
     328.f,  329.f,  330.f,  331.f,  332.f,  333.f,  334.f,  335.f,
     336.f,  337.f,  338.f,  339.f,  340.f,  341.f,  342.f,  343.f,
     344.f,  345.f,  346.f,  347.f,  348.f,  349.f,  350.f,  351.f,
     352.f,  353.f,  354.f,  355.f,  356.f,  357.f,  358.f,  359.f,
     360.f,  361.f,  362.f,  363.f,  364.f,  365.f,  366.f,  367.f,
     368.f,  369.f,  370.f,  371.f,  372.f,  373.f,  374.f,  375.f,
     376.f,  377.f,  378.f,  379.f,  380.f,  381.f,  382.f,  383.f,
     384.f,  385.f,  386.f,  387.f,  388.f,  389.f,  390.f,  391.f,
     392.f,  393.f,  394.f,  395.f,  396.f,  397.f,  398.f,  399.f,
     400.f,  401.f,  402.f,  403.f,  404.f,  405.f,  406.f,  407.f,
     408.f,  409.f,  410.f,  411.f,  412.f,  413.f,  414.f,  415.f,
     416.f,  417.f,  418.f,  419.f,  420.f,  421.f,  422.f,  423.f,
     424.f,  425.f,  426.f,  427.f,  428.f,  429.f,  430.f,  431.f,
     432.f,  433.f,  434.f,  435.f,  436.f,  437.f,  438.f,  439.f,
     440.f,  441.f,  442.f,  443.f,  444.f,  445.f,  446.f,  447.f,
     448.f,  449.f,  450.f,  451.f,  452.f,  453.f,  454.f,  455.f,
     456.f,  457.f,  458.f,  459.f,  460.f,  461.f,  462.f,  463.f,
     464.f,  465.f,  466.f,  467.f,  468.f,  469.f,  470.f,  471.f,
     472.f,  473.f,  474.f,  475.f,  476.f,  477.f,  478.f,  479.f,
     480.f,  481.f,  482.f,  483.f,  484.f,  485.f,  486.f,  487.f,
     488.f,  489.f,  490.f,  491.f,  492.f,  493.f,  494.f,  495.f,
     496.f,  497.f,  498.f,  499.f,  500.f,  501.f,  502.f,  503.f,
     504.f,  505.f,  506.f,  507.f,  508.f,  509.f,  510.f,  511.f,
};

icvRGB2XYZ_16u_C3R_t icvRGB2XYZ_16u_C3R_p = 0;
icvRGB2XYZ_32f_C3R_t icvRGB2XYZ_32f_C3R_p = 0;
icvRGB2HSV_8u_C3R_t  icvRGB2HSV_8u_C3R_p = 0;
icvRGB2Luv_8u_C3R_t  icvRGB2Luv_8u_C3R_p = 0;
icvRGB2HLS_32f_C3R_t icvRGB2HLS_32f_C3R_p = 0;
icvRGB2HLS_8u_C3R_t  icvRGB2HLS_8u_C3R_p = 0;
icvXYZ2RGB_8u_C3R_t  icvXYZ2RGB_8u_C3R_p = 0;
icvXYZ2RGB_16u_C3R_t icvXYZ2RGB_16u_C3R_p = 0;
icvXYZ2RGB_32f_C3R_t icvXYZ2RGB_32f_C3R_p = 0;
icvHSV2RGB_8u_C3R_t  icvHSV2RGB_8u_C3R_p = 0;
icvHLS2RGB_32f_C3R_t icvHLS2RGB_32f_C3R_p = 0;
icvHLS2RGB_8u_C3R_t  icvHLS2RGB_8u_C3R_p = 0;
icvLab2BGR_8u_C3R_t  icvLab2BGR_8u_C3R_p = 0;
icvLuv2RGB_8u_C3R_t  icvLuv2RGB_8u_C3R_p = 0;

enum { GENERIC=0, ASYMMETRICAL=1, SYMMETRICAL=2, POSITIVE=4, SUM_TO_1=8, INTEGER=16 };

void cvCreateData( CvArr* arr )
{
    CV_FUNCNAME( "cvCreateData" );
    
    __BEGIN__;

    if( CV_IS_MAT_HDR( arr ))
    {
        size_t step, total_size;
        CvMat* mat = (CvMat*)arr;
        step = mat->step;

        if( mat->data.ptr != 0 )
            CV_ERROR( CV_StsError, "Data is already allocated" );

        if( step == 0 )
            step = CV_ELEM_SIZE(mat->type)*mat->cols;

        total_size = step*mat->rows + sizeof(int) + CV_MALLOC_ALIGN;
        CV_CALL( mat->refcount = (int*)cvAlloc( (size_t)total_size ));
        mat->data.ptr = (uchar*)cvAlignPtr( mat->refcount + 1, CV_MALLOC_ALIGN );
        *mat->refcount = 1;
    }
    else if( CV_IS_IMAGE_HDR(arr))
    {
        IplImage* img = (IplImage*)arr;

        if( img->imageData != 0 )
            CV_ERROR( CV_StsError, "Data is already allocated" );

        if( !CvIPL.allocateData )
        {
            CV_CALL( img->imageData = img->imageDataOrigin = 
                        (char*)cvAlloc( (size_t)img->imageSize ));
        }
        else
        {
            int depth = img->depth;
            int width = img->width;

            if( img->depth == IPL_DEPTH_32F || img->nChannels == 64 )
            {
                img->width *= img->depth == IPL_DEPTH_32F ? sizeof(float) : sizeof(double);
                img->depth = IPL_DEPTH_8U;
            }

            CvIPL.allocateData( img, 0, 0 );

            img->width = width;
            img->depth = depth;
        }
    }
    else if( CV_IS_MATND_HDR( arr ))
    {
        CvMatND* mat = (CvMatND*)arr;
        int i;
        size_t total_size = CV_ELEM_SIZE(mat->type);

        if( mat->data.ptr != 0 )
            CV_ERROR( CV_StsError, "Data is already allocated" );

        if( CV_IS_MAT_CONT( mat->type ))
        {
            total_size = (size_t)mat->dim[0].size*(mat->dim[0].step != 0 ?
                         mat->dim[0].step : total_size);
        }
        else
        {
            for( i = mat->dims - 1; i >= 0; i-- )
            {
                size_t size = (size_t)mat->dim[i].step*mat->dim[i].size;

                if( total_size < size )
                    total_size = size;
            }
        }
        
        CV_CALL( mat->refcount = (int*)cvAlloc( total_size +
                                        sizeof(int) + CV_MALLOC_ALIGN ));
        mat->data.ptr = (uchar*)cvAlignPtr( mat->refcount + 1, CV_MALLOC_ALIGN );
        *mat->refcount = 1;
    }
    else
    {
        CV_ERROR( CV_StsBadArg, "unrecognized or unsupported array type" );
    }

    __END__;
}

IplImage * cvCreateImage( CvSize size, int depth, int channels )
{
    IplImage *img = 0;

    CV_FUNCNAME( "cvCreateImage" );

    __BEGIN__;

    CV_CALL( img = cvCreateImageHeader( size, depth, channels ));
    assert( img );
    CV_CALL( cvCreateData( img ));

    __END__;

    if( cvGetErrStatus() < 0 )
        cvReleaseImage( &img );

    return img;
}

IplImage * cvCreateImageHeader( CvSize size, int depth, int channels )
{
    IplImage *img = 0;

    CV_FUNCNAME( "cvCreateImageHeader" );

    __BEGIN__;

    if( !CvIPL.createHeader )
    {
        CV_CALL( img = (IplImage *)cvAlloc( sizeof( *img )));
        CV_CALL( cvInitImageHeader( img, size, depth, channels, IPL_ORIGIN_TL,
                                    CV_DEFAULT_IMAGE_ROW_ALIGN ));
    }
    else
    {
        char *colorModel;
        char *channelSeq;

        icvGetColorModel( channels, &colorModel, &channelSeq );

        img = CvIPL.createHeader( channels, 0, depth, colorModel, channelSeq,
                                  IPL_DATA_ORDER_PIXEL, IPL_ORIGIN_TL,
                                  CV_DEFAULT_IMAGE_ROW_ALIGN,
                                  size.width, size.height, 0, 0, 0, 0 );
    }

    __END__;

    if( cvGetErrStatus() < 0 && img )
        cvReleaseImageHeader( &img );

    return img;
}

IplImage* cvInitImageHeader( IplImage * image, CvSize size, int depth,int channels, int origin, int align )
{
    IplImage* result = 0;

    CV_FUNCNAME( "cvInitImageHeader" );

    __BEGIN__;

    char *colorModel, *channelSeq;

    if( !image )
        CV_ERROR( CV_HeaderIsNull, "null pointer to header" );

    memset( image, 0, sizeof( *image ));
    image->nSize = sizeof( *image );

    CV_CALL( icvGetColorModel( channels, &colorModel, &channelSeq ));
    strncpy( image->colorModel, colorModel, 4 );
    strncpy( image->channelSeq, channelSeq, 4 );

    if( size.width < 0 || size.height < 0 )
        CV_ERROR( CV_BadROISize, "Bad input roi" );

    if( (depth != (int)IPL_DEPTH_1U && depth != (int)IPL_DEPTH_8U &&
         depth != (int)IPL_DEPTH_8S && depth != (int)IPL_DEPTH_16U &&
         depth != (int)IPL_DEPTH_16S && depth != (int)IPL_DEPTH_32S &&
         depth != (int)IPL_DEPTH_32F && depth != (int)IPL_DEPTH_64F) ||
         channels < 0 )
        CV_ERROR( CV_BadDepth, "Unsupported format" );
    if( origin != CV_ORIGIN_BL && origin != CV_ORIGIN_TL )
        CV_ERROR( CV_BadOrigin, "Bad input origin" );

    if( align != 4 && align != 8 )
        CV_ERROR( CV_BadAlign, "Bad input align" );

    image->width = size.width;
    image->height = size.height;

    if( image->roi )
    {
        image->roi->coi = 0;
        image->roi->xOffset = image->roi->yOffset = 0;
        image->roi->width = size.width;
        image->roi->height = size.height;
    }

    image->nChannels = MAX( channels, 1 );
    image->depth = depth;
    image->align = align;
    image->widthStep = (((image->width * image->nChannels *
         (image->depth & ~IPL_DEPTH_SIGN) + 7)/8)+ align - 1) & (~(align - 1));
    image->origin = origin;
    image->imageSize = image->widthStep * image->height;

    result = image;

    __END__;

    return result;
}

void cvReleaseImageHeader( IplImage** image )
{
    CV_FUNCNAME( "cvReleaseImageHeader" );

    __BEGIN__;

    if( !image )
        CV_ERROR( CV_StsNullPtr, "" );

    if( *image )
    {
        IplImage* img = *image;
        *image = 0;
        
        if( !CvIPL.deallocate )
        {
            cvFree( &img->roi );
            cvFree( &img );
        }
        else
        {
            CvIPL.deallocate( img, IPL_IMAGE_HEADER | IPL_IMAGE_ROI );
        }
    }
    __END__;
}


void cvReleaseImage( IplImage ** image )
{
    CV_FUNCNAME( "cvReleaseImage" );

    __BEGIN__

    if( !image )
        CV_ERROR( CV_StsNullPtr, "" );

    if( *image )
    {
        IplImage* img = *image;
        *image = 0;
        
        cvReleaseData( img );
        cvReleaseImageHeader( &img );
    }

    __END__;
}

void cvReleaseData( CvArr* arr )
{
    CV_FUNCNAME( "cvReleaseData" );
    
    __BEGIN__;

    if( CV_IS_MAT_HDR( arr ) || CV_IS_MATND_HDR( arr ))
    {
        CvMat* mat = (CvMat*)arr;
        cvDecRefData( mat );
    }
    else if( CV_IS_IMAGE_HDR( arr ))
    {
        IplImage* img = (IplImage*)arr;

        if( !CvIPL.deallocate )
        {
            char* ptr = img->imageDataOrigin;
            img->imageData = img->imageDataOrigin = 0;
            cvFree( &ptr );
        }
        else
        {
            CvIPL.deallocate( img, IPL_IMAGE_DATA );
        }
    }
    else
    {
        CV_ERROR( CV_StsBadArg, "unrecognized or unsupported array type" );
    }

    __END__;
}

static  void icvGetColorModel( int nchannels, char** colorModel, char** channelSeq )
{
    static char* tab[][2] =
    {
        {"GRAY", "GRAY"},
        {"",""},
        {"RGB","BGR"},
        {"RGB","BGRA"}
    };

    nchannels--;
    *colorModel = *channelSeq = "";

    if( (unsigned)nchannels <= 3 )
    {
        *colorModel = tab[nchannels][0];
        *channelSeq = tab[nchannels][1];
    }
}


void  cvDecRefData( CvArr* arr )
{
    if( CV_IS_MAT( arr ))
    {
        CvMat* mat = (CvMat*)arr;
        mat->data.ptr = NULL;
        if( mat->refcount != NULL && --*mat->refcount == 0 )
            cvFree( &mat->refcount );
        mat->refcount = NULL;
    }
    else if( CV_IS_MATND( arr ))
    {
        CvMatND* mat = (CvMatND*)arr;
        mat->data.ptr = NULL;
        if( mat->refcount != NULL && --*mat->refcount == 0 )
            cvFree( &mat->refcount );
        mat->refcount = NULL;
    }
}

CvSize  cvSize( int width, int height )
{
    CvSize s;

    s.width = width;
    s.height = height;

    return s;
}

CvMat* cvCreateMat( int height, int width, int type )
{
    CvMat* arr = 0;

    CV_FUNCNAME( "cvCreateMat" );
    
    __BEGIN__;

    CV_CALL( arr = cvCreateMatHeader( height, width, type ));
    CV_CALL( cvCreateData( arr ));

    __END__;

    if( cvGetErrStatus() < 0 )
        cvReleaseMat( &arr );

    return arr;
}

void cvReleaseMat( CvMat** array )
{
    CV_FUNCNAME( "cvReleaseMat" );
    
    __BEGIN__;

    if( !array )
        CV_ERROR_FROM_CODE( CV_HeaderIsNull );

    if( *array )
    {
        CvMat* arr = *array;
        
        if( !CV_IS_MAT_HDR(arr) && !CV_IS_MATND_HDR(arr) )
            CV_ERROR_FROM_CODE( CV_StsBadFlag );

        *array = 0;

        cvDecRefData( arr );
        cvFree( &arr );
    }

    __END__;
}

CvMat cvMat( int rows, int cols, int type, void* data)
{
    CvMat m;

    assert( (unsigned)CV_MAT_DEPTH(type) <= CV_64F );
    type = CV_MAT_TYPE(type);
    m.type = CV_MAT_MAGIC_VAL | CV_MAT_CONT_FLAG | type;
    m.cols = cols;
    m.rows = rows;
    m.step = rows > 1 ? m.cols*CV_ELEM_SIZE(type) : 0;
    m.data.ptr = (uchar*)data;
    m.refcount = NULL;
    m.hdr_refcount = 0;

    return m;
}

void cvSetZero( CvArr* arr )
{
    CV_FUNCNAME( "cvSetZero" );
    
    __BEGIN__;

    CvMat stub, *mat = (CvMat*)arr;
    CvSize size;
    int mat_step;

    if( !CV_IS_MAT( mat ))
    {
        if( CV_IS_MATND(mat))
        {
            CvMatND nstub;
            CvNArrayIterator iterator;
            
            CV_CALL( cvInitNArrayIterator( 1, &arr, 0, &nstub, &iterator,0 ));
            iterator.size.width *= CV_ELEM_SIZE(iterator.hdr[0]->type);

            if( iterator.size.width <= CV_MAX_INLINE_MAT_OP_SIZE*(int)sizeof(double) )
            {
                do
                {
                    memset( iterator.ptr[0], 0, iterator.size.width );
                }
                while( cvNextNArraySlice( &iterator ));
            }
            else
            {
                do
                {
                    icvSetZero_8u_C1R( iterator.ptr[0], CV_STUB_STEP, iterator.size );
                }
                while( cvNextNArraySlice( &iterator ));
            }
            EXIT;
        }    
        else if( CV_IS_SPARSE_MAT(mat))
        {
            CvSparseMat* mat1 = (CvSparseMat*)mat;
            cvClearSet( mat1->heap );
            if( mat1->hashtable )
                memset( mat1->hashtable, 0, mat1->hashsize*sizeof(mat1->hashtable[0]));
            EXIT;
        }
        else
        {
            int coi = 0;
            CV_CALL( mat = cvGetMat( mat, &stub, &coi,0 ));
            if( coi != 0 )
                CV_ERROR( CV_BadCOI, "coi is not supported" );
        }
    }

    size = cvGetMatSize( mat );
    size.width *= CV_ELEM_SIZE(mat->type);
    mat_step = mat->step;

    if( CV_IS_MAT_CONT( mat->type ))
    {
        size.width *= size.height;

        if( size.width <= CV_MAX_INLINE_MAT_OP_SIZE*(int)sizeof(double) )
        {
            memset( mat->data.ptr, 0, size.width );
            EXIT;
        }

        mat_step = CV_STUB_STEP;
        size.height = 1;
    }

    IPPI_CALL( icvSetZero_8u_C1R( mat->data.ptr, mat_step, size ));

    __END__;
}

void  cvmSet( CvMat* mat, int row, int col, double value )
{
    int type;
    type = CV_MAT_TYPE(mat->type);
    assert( (unsigned)row < (unsigned)mat->rows &&
            (unsigned)col < (unsigned)mat->cols );

    if( type == CV_32FC1 )
        ((float*)(mat->data.ptr + (size_t)mat->step*row))[col] = (float)value;
    else
    {
        assert( type == CV_64FC1 );
        ((double*)(mat->data.ptr + (size_t)mat->step*row))[col] = (double)value;
    }
}

void cvSVD( CvArr* aarr, CvArr* warr, CvArr* uarr, CvArr* varr, int flags )
{
    uchar* buffer = 0;
    int local_alloc = 0;

    CV_FUNCNAME( "cvSVD" );

    __BEGIN__;

    CvMat astub, *a = (CvMat*)aarr;
    CvMat wstub, *w = (CvMat*)warr;
    CvMat ustub, *u;
    CvMat vstub, *v;
    CvMat tmat;
    uchar* tw = 0;
    int type;
    int a_buf_offset = 0, u_buf_offset = 0, buf_size, pix_size;
    int temp_u = 0, /* temporary storage for U is needed */
        t_svd; /* special case: a->rows < a->cols */
    int m, n;
    int w_rows, w_cols;
    int u_rows = 0, u_cols = 0;
    int w_is_mat = 0;

    if( !CV_IS_MAT( a ))
        CV_CALL( a = cvGetMat( a, &astub,NULL,0 ));

    if( !CV_IS_MAT( w ))
        CV_CALL( w = cvGetMat( w, &wstub,NULL,0 ));

    if( !CV_ARE_TYPES_EQ( a, w ))
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    if( a->rows >= a->cols )
    {
        m = a->rows;
        n = a->cols;
        w_rows = w->rows;
        w_cols = w->cols;
        t_svd = 0;
    }
    else
    {
        CvArr* t;
        CV_SWAP( uarr, varr, t );

        flags = (flags & CV_SVD_U_T ? CV_SVD_V_T : 0)|
                (flags & CV_SVD_V_T ? CV_SVD_U_T : 0);
        m = a->cols;
        n = a->rows;
        w_rows = w->cols;
        w_cols = w->rows;
        t_svd = 1;
    }

    u = (CvMat*)uarr;
    v = (CvMat*)varr;

    w_is_mat = w_cols > 1 && w_rows > 1;

    if( !w_is_mat && CV_IS_MAT_CONT(w->type) && w_cols + w_rows - 1 == n )
        tw = w->data.ptr;

    if( u )
    {
        if( !CV_IS_MAT( u ))
            CV_CALL( u = cvGetMat( u, &ustub,NULL,0 ));

        if( !(flags & CV_SVD_U_T) )
        {
            u_rows = u->rows;
            u_cols = u->cols;
        }
        else
        {
            u_rows = u->cols;
            u_cols = u->rows;
        }

        if( !CV_ARE_TYPES_EQ( a, u ))
            CV_ERROR( CV_StsUnmatchedFormats, "" );

        if( u_rows != m || (u_cols != m && u_cols != n))
            CV_ERROR( CV_StsUnmatchedSizes, !t_svd ? "U matrix has unappropriate size" :
                                                     "V matrix has unappropriate size" );
            
        temp_u = (u_rows != u_cols && !(flags & CV_SVD_U_T)) || u->data.ptr==a->data.ptr;

        if( w_is_mat && u_cols != w_rows )
            CV_ERROR( CV_StsUnmatchedSizes, !t_svd ? "U and W have incompatible sizes" :
                                                     "V and W have incompatible sizes" );
    }
    else
    {
        u = &ustub;
        u->data.ptr = 0;
        u->step = 0;
    }

    if( v )
    {
        int v_rows, v_cols;

        if( !CV_IS_MAT( v ))
            CV_CALL( v = cvGetMat( v, &vstub,NULL,0 ));

        if( !(flags & CV_SVD_V_T) )
        {
            v_rows = v->rows;
            v_cols = v->cols;
        }
        else
        {
            v_rows = v->cols;
            v_cols = v->rows;
        }

        if( !CV_ARE_TYPES_EQ( a, v ))
            CV_ERROR( CV_StsUnmatchedFormats, "" );

        if( v_rows != n || v_cols != n )
            CV_ERROR( CV_StsUnmatchedSizes, t_svd ? "U matrix has unappropriate size" :
                                                    "V matrix has unappropriate size" );

        if( w_is_mat && w_cols != v_cols )
            CV_ERROR( CV_StsUnmatchedSizes, t_svd ? "U and W have incompatible sizes" :
                                                    "V and W have incompatible sizes" );
    }
    else
    {
        v = &vstub;
        v->data.ptr = 0;
        v->step = 0;
    }

    type = CV_MAT_TYPE( a->type );
    pix_size = CV_ELEM_SIZE(type);
    buf_size = n*2 + m;

    if( !(flags & CV_SVD_MODIFY_A) )
    {
        a_buf_offset = buf_size;
        buf_size += a->rows*a->cols;
    }

    if( temp_u )
    {
        u_buf_offset = buf_size;
        buf_size += u->rows*u->cols;
    }

    buf_size *= pix_size;

    if( buf_size <= CV_MAX_LOCAL_SIZE )
    {
        buffer = (uchar*)cvStackAlloc( buf_size );
        local_alloc = 1;
    }
    else
    {
        CV_CALL( buffer = (uchar*)cvAlloc( buf_size ));
    }
    
    if( !(flags & CV_SVD_MODIFY_A) )
    {
        cvInitMatHeader( &tmat, m, n, type,
                         buffer + a_buf_offset*pix_size,CV_AUTOSTEP );
        if( !t_svd )
            cvCopy( a, &tmat,NULL );
        else
            cvT( a, &tmat );
        a = &tmat;
    }

    if( temp_u )
    {
        cvInitMatHeader( &ustub, u_cols, u_rows, type, buffer + u_buf_offset*pix_size ,CV_AUTOSTEP);
        u = &ustub;
    }

    if( !tw )
        tw = buffer + (n + m)*pix_size;

    if( type == CV_32FC1 )
    {
        icvSVD_32f( a->data.fl, a->step/sizeof(float), a->rows, a->cols,
                   (float*)tw, u->data.fl, u->step/sizeof(float), u_cols,
                   v->data.fl, v->step/sizeof(float), (float*)buffer );
    }
    else if( type == CV_64FC1 )
    {
        icvSVD_64f( a->data.db, a->step/sizeof(double), a->rows, a->cols,
                    (double*)tw, u->data.db, u->step/sizeof(double), u_cols,
                    v->data.db, v->step/sizeof(double), (double*)buffer );
    }
    else
    {
        CV_ERROR( CV_StsUnsupportedFormat, "" );
    }

    if( tw != w->data.ptr )
    {
        int shift = w->cols != 1;
        cvSetZero( w );
        if( type == CV_32FC1 )
            for( int i = 0; i < n; i++ )
                ((float*)(w->data.ptr + i*w->step))[i*shift] = ((float*)tw)[i];
        else
            for( int i = 0; i < n; i++ )
                ((double*)(w->data.ptr + i*w->step))[i*shift] = ((double*)tw)[i];
    }

    if( uarr )
    {
        if( !(flags & CV_SVD_U_T))
            cvT( u, uarr );
        else if( temp_u )
            cvCopy( u, uarr,NULL );
        /*CV_CHECK_NANS( uarr );*/
    }

    if( varr )
    {
        if( !(flags & CV_SVD_V_T))
            cvT( v, varr );
        /*CV_CHECK_NANS( varr );*/
    }

    CV_CHECK_NANS( w );

    __END__;

    if( buffer && !local_alloc )
        cvFree( &buffer );
}

CvMat*  cvGetRow( const CvArr* arr, CvMat* submat, int row )
{
    return cvGetRows( arr, submat, row, row + 1, 1 );
}

void cvCopy( const void* srcarr, void* dstarr, const void* maskarr )
{
    CV_FUNCNAME( "cvCopy" );
    
    __BEGIN__;

    int pix_size;
    CvMat srcstub, *src = (CvMat*)srcarr;
    CvMat dststub, *dst = (CvMat*)dstarr;
    CvSize size;

    if( !CV_IS_MAT(src) || !CV_IS_MAT(dst) )
    {
        if( CV_IS_SPARSE_MAT(src) && CV_IS_SPARSE_MAT(dst))
        {
            CvSparseMat* src1 = (CvSparseMat*)src;
            CvSparseMat* dst1 = (CvSparseMat*)dst;
            CvSparseMatIterator iterator;
            CvSparseNode* node;

            dst1->dims = src1->dims;
            memcpy( dst1->size, src1->size, src1->dims*sizeof(src1->size[0]));
            dst1->valoffset = src1->valoffset;
            dst1->idxoffset = src1->idxoffset;
            cvClearSet( dst1->heap );

            if( src1->heap->active_count >= dst1->hashsize*CV_SPARSE_HASH_RATIO )
            {
                CV_CALL( cvFree( &dst1->hashtable ));
                dst1->hashsize = src1->hashsize;
                CV_CALL( dst1->hashtable =
                    (void**)cvAlloc( dst1->hashsize*sizeof(dst1->hashtable[0])));
            }

            memset( dst1->hashtable, 0, dst1->hashsize*sizeof(dst1->hashtable[0]));

            for( node = cvInitSparseMatIterator( src1, &iterator );
                 node != 0; node = cvGetNextSparseNode( &iterator ))
            {
                CvSparseNode* node_copy = (CvSparseNode*)cvSetNew( dst1->heap );
                int tabidx = node->hashval & (dst1->hashsize - 1);
                CV_MEMCPY_AUTO( node_copy, node, dst1->heap->elem_size );
                node_copy->next = (CvSparseNode*)dst1->hashtable[tabidx];
                dst1->hashtable[tabidx] = node_copy;
            }
            EXIT;
        }
        else if( CV_IS_MATND(src) || CV_IS_MATND(dst) )
        {
            CvArr* arrs[] = { src, dst };
            CvMatND stubs[3];
            CvNArrayIterator iterator;

            CV_CALL( cvInitNArrayIterator( 2, arrs, maskarr, stubs, &iterator,0 ));
            pix_size = CV_ELEM_SIZE(iterator.hdr[0]->type);

            if( !maskarr )
            {
                iterator.size.width *= pix_size;
                if( iterator.size.width <= CV_MAX_INLINE_MAT_OP_SIZE*(int)sizeof(double))
                {
                    do
                    {
                        memcpy( iterator.ptr[1], iterator.ptr[0], iterator.size.width );
                    }
                    while( cvNextNArraySlice( &iterator ));
                }
                else
                {
                    do
                    {
                        icvCopy_8u_C1R( iterator.ptr[0], CV_STUB_STEP,
                                        iterator.ptr[1], CV_STUB_STEP, iterator.size );
                    }
                    while( cvNextNArraySlice( &iterator ));
                }
            }
            else
            {
                CvCopyMaskFunc func = icvGetCopyMaskFunc( pix_size );
                if( !func )
                    CV_ERROR( CV_StsUnsupportedFormat, "" );

                do
                {
                    func( iterator.ptr[0], CV_STUB_STEP,
                          iterator.ptr[1], CV_STUB_STEP,
                          iterator.size,
                          iterator.ptr[2], CV_STUB_STEP );
                }
                while( cvNextNArraySlice( &iterator ));
            }
            EXIT;
        }
        else
        {
            int coi1 = 0, coi2 = 0;
            CV_CALL( src = cvGetMat( src, &srcstub, &coi1,0 ));
            CV_CALL( dst = cvGetMat( dst, &dststub, &coi2,0 ));

            if( coi1 )
            {
                CvArr* planes[] = { 0, 0, 0, 0 };

                if( maskarr )
                    CV_ERROR( CV_StsBadArg, "COI + mask are not supported" );

                planes[coi1-1] = dst;
                CV_CALL( cvSplit( src, planes[0], planes[1], planes[2], planes[3] ));
                EXIT;
            }
            else if( coi2 )
            {
                CvArr* planes[] = { 0, 0, 0, 0 };
            
                if( maskarr )
                    CV_ERROR( CV_StsBadArg, "COI + mask are not supported" );

                planes[coi2-1] = src;
                CV_CALL( cvMerge( planes[0], planes[1], planes[2], planes[3], dst ));
                EXIT;
            }
        }
    }

    if( !CV_ARE_TYPES_EQ( src, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedFormats );

    if( !CV_ARE_SIZES_EQ( src, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedSizes );

    size = cvGetMatSize( src );
    pix_size = CV_ELEM_SIZE(src->type);

    if( !maskarr )
    {
        int src_step = src->step, dst_step = dst->step;
        size.width *= pix_size;
        if( CV_IS_MAT_CONT( src->type & dst->type ) && (src_step == dst_step) && (src_step == src->width * pix_size))
        {
            size.width *= size.height;

            if( size.width <= CV_MAX_INLINE_MAT_OP_SIZE*
                              CV_MAX_INLINE_MAT_OP_SIZE*(int)sizeof(double))
            {
                memcpy( dst->data.ptr, src->data.ptr, size.width );
                EXIT;
            }

            size.height = 1;
            src_step = dst_step = CV_STUB_STEP;
        }

        icvCopy_8u_C1R( src->data.ptr, src_step,
                        dst->data.ptr, dst_step, size );
    }
    else
    {
        CvCopyMaskFunc func = icvGetCopyMaskFunc(pix_size);
        CvMat maskstub, *mask = (CvMat*)maskarr;
        int src_step = src->step;
        int dst_step = dst->step;
        int mask_step;

        if( !CV_IS_MAT( mask ))
            CV_CALL( mask = cvGetMat( mask, &maskstub,NULL,0 ));
        if( !CV_IS_MASK_ARR( mask ))
            CV_ERROR( CV_StsBadMask, "" );

        if( !CV_ARE_SIZES_EQ( src, mask ))
            CV_ERROR( CV_StsUnmatchedSizes, "" );

        mask_step = mask->step;
        
        if( CV_IS_MAT_CONT( src->type & dst->type & mask->type ))
        {
            size.width *= size.height;
            size.height = 1;
            src_step = dst_step = mask_step = CV_STUB_STEP;
        }

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        IPPI_CALL( func( src->data.ptr, src_step, dst->data.ptr, dst_step,
                         size, mask->data.ptr, mask_step ));
    }

    __END__;
}


#define ICV_DEF_CVT_CASE_2D( srctype, worktype,             \
                             cast_macro1, cast_macro2 )     \
{                                                           \
    const srctype* _src = (const srctype*)src;              \
    srcstep /= sizeof(_src[0]);                             \
                                                            \
    for( ; size.height--; _src += srcstep, dst += dststep ) \
    {                                                       \
        int i;                                              \
                                                            \
        for( i = 0; i <= size.width - 4; i += 4 )           \
        {                                                   \
            worktype t0 = cast_macro1(_src[i]);             \
            worktype t1 = cast_macro1(_src[i+1]);           \
                                                            \
            dst[i] = cast_macro2(t0);                       \
            dst[i+1] = cast_macro2(t1);                     \
                                                            \
            t0 = cast_macro1(_src[i+2]);                    \
            t1 = cast_macro1(_src[i+3]);                    \
                                                            \
            dst[i+2] = cast_macro2(t0);                     \
            dst[i+3] = cast_macro2(t1);                     \
        }                                                   \
                                                            \
        for( ; i < size.width; i++ )                        \
        {                                                   \
            worktype t0 = cast_macro1(_src[i]);             \
            dst[i] = cast_macro2(t0);                       \
        }                                                   \
    }                                                       \
}


#define ICV_DEF_CVT_FUNC_2D( flavor, dsttype, worktype, cast_macro2,    \
                             srcdepth1, srctype1, cast_macro11,         \
                             srcdepth2, srctype2, cast_macro12,         \
                             srcdepth3, srctype3, cast_macro13,         \
                             srcdepth4, srctype4, cast_macro14,         \
                             srcdepth5, srctype5, cast_macro15,         \
                             srcdepth6, srctype6, cast_macro16 )        \
static CvStatus CV_STDCALL                                              \
icvCvtTo_##flavor##_C1R( const uchar* src, int srcstep,                 \
                         dsttype* dst, int dststep,                     \
                         CvSize size, int param )                       \
{                                                                       \
    int srctype = param;                                                \
    dststep /= sizeof(dst[0]);                                          \
                                                                        \
    switch( CV_MAT_DEPTH(srctype) )                                     \
    {                                                                   \
    case srcdepth1:                                                     \
        ICV_DEF_CVT_CASE_2D( srctype1, worktype,                        \
                             cast_macro11, cast_macro2 );               \
        break;                                                          \
    case srcdepth2:                                                     \
        ICV_DEF_CVT_CASE_2D( srctype2, worktype,                        \
                             cast_macro12, cast_macro2 );               \
        break;                                                          \
    case srcdepth3:                                                     \
        ICV_DEF_CVT_CASE_2D( srctype3, worktype,                        \
                             cast_macro13, cast_macro2 );               \
        break;                                                          \
    case srcdepth4:                                                     \
        ICV_DEF_CVT_CASE_2D( srctype4, worktype,                        \
                             cast_macro14, cast_macro2 );               \
        break;                                                          \
    case srcdepth5:                                                     \
        ICV_DEF_CVT_CASE_2D( srctype5, worktype,                        \
                             cast_macro15, cast_macro2 );               \
        break;                                                          \
    case srcdepth6:                                                     \
        ICV_DEF_CVT_CASE_2D( srctype6, worktype,                        \
                             cast_macro16, cast_macro2 );               \
        break;                                                          \
    }                                                                   \
                                                                        \
    return  CV_OK;                                                      \
}


ICV_DEF_CVT_FUNC_2D( 8u, uchar, int, CV_CAST_8U,
                     CV_8S,  char,   CV_NOP,
                     CV_16U, ushort, CV_NOP,
                     CV_16S, short,  CV_NOP,
                     CV_32S, int,    CV_NOP,
                     CV_32F, float,  cvRound,
                     CV_64F, double, cvRound )

ICV_DEF_CVT_FUNC_2D( 8s, char, int, CV_CAST_8S,
                     CV_8U,  uchar,  CV_NOP,
                     CV_16U, ushort, CV_NOP,
                     CV_16S, short,  CV_NOP,
                     CV_32S, int,    CV_NOP,
                     CV_32F, float,  cvRound,
                     CV_64F, double, cvRound )

ICV_DEF_CVT_FUNC_2D( 16u, ushort, int, CV_CAST_16U,
                     CV_8U,  uchar,  CV_NOP,
                     CV_8S,  char,   CV_NOP,
                     CV_16S, short,  CV_NOP,
                     CV_32S, int,    CV_NOP,
                     CV_32F, float,  cvRound,
                     CV_64F, double, cvRound )

ICV_DEF_CVT_FUNC_2D( 16s, short, int, CV_CAST_16S,
                     CV_8U,  uchar,  CV_NOP,
                     CV_8S,  char,   CV_NOP,
                     CV_16U, ushort, CV_NOP,
                     CV_32S, int,    CV_NOP,
                     CV_32F, float,  cvRound,
                     CV_64F, double, cvRound )

ICV_DEF_CVT_FUNC_2D( 32s, int, int, CV_NOP,
                     CV_8U,  uchar,  CV_NOP,
                     CV_8S,  char,   CV_NOP,
                     CV_16U, ushort, CV_NOP,
                     CV_16S, short,  CV_NOP,
                     CV_32F, float,  cvRound,
                     CV_64F, double, cvRound )

ICV_DEF_CVT_FUNC_2D( 32f, float, float, CV_NOP,
                     CV_8U,  uchar,  CV_8TO32F,
                     CV_8S,  char,   CV_8TO32F,
                     CV_16U, ushort, CV_NOP,
                     CV_16S, short,  CV_NOP,
                     CV_32S, int,    CV_CAST_32F,
                     CV_64F, double, CV_CAST_32F )

ICV_DEF_CVT_FUNC_2D( 64f, double, double, CV_NOP,
                     CV_8U,  uchar,  CV_8TO32F,
                     CV_8S,  char,   CV_8TO32F,
                     CV_16U, ushort, CV_NOP,
                     CV_16S, short,  CV_NOP,
                     CV_32S, int,    CV_NOP,
                     CV_32F, float,  CV_NOP )

CV_DEF_INIT_FUNC_TAB_2D( CvtTo, C1R )

#define ICV_DEF_CVT_SCALE_CASE( srctype, worktype,          \
                            scale_macro, cast_macro, a, b ) \
                                                            \
{                                                           \
    const srctype* _src = (const srctype*)src;              \
    srcstep /= sizeof(_src[0]);                             \
                                                            \
    for( ; size.height--; _src += srcstep, dst += dststep ) \
    {                                                       \
        for( i = 0; i <= size.width - 4; i += 4 )           \
        {                                                   \
            worktype t0 = scale_macro((a)*_src[i]+(b));     \
            worktype t1 = scale_macro((a)*_src[i+1]+(b));   \
                                                            \
            dst[i] = cast_macro(t0);                        \
            dst[i+1] = cast_macro(t1);                      \
                                                            \
            t0 = scale_macro((a)*_src[i+2] + (b));          \
            t1 = scale_macro((a)*_src[i+3] + (b));          \
                                                            \
            dst[i+2] = cast_macro(t0);                      \
            dst[i+3] = cast_macro(t1);                      \
        }                                                   \
                                                            \
        for( ; i < size.width; i++ )                        \
        {                                                   \
            worktype t0 = scale_macro((a)*_src[i] + (b));   \
            dst[i] = cast_macro(t0);                        \
        }                                                   \
    }                                                       \
}

#define  ICV_DEF_CVT_SCALE_FUNC_INT( flavor, dsttype, cast_macro )      \
static  CvStatus  CV_STDCALL                                            \
icvCvtScaleTo_##flavor##_C1R( const uchar* src, int srcstep,            \
                              dsttype* dst, int dststep, CvSize size,   \
                              double scale, double shift, int param )   \
{                                                                       \
    int i, srctype = param;                                             \
    dsttype lut[256];                                                   \
    dststep /= sizeof(dst[0]);                                          \
                                                                        \
    switch( CV_MAT_DEPTH(srctype) )                                     \
    {                                                                   \
    case  CV_8U:                                                        \
        if( size.width*size.height >= 256 )                             \
        {                                                               \
            double val = shift;                                         \
            for( i = 0; i < 256; i++, val += scale )                    \
            {                                                           \
                int t = cvRound(val);                                   \
                lut[i] = cast_macro(t);                                 \
            }                                                           \
                                                                        \
            icvLUT_Transform8u_##flavor##_C1R( src, srcstep, dst,       \
                                dststep*sizeof(dst[0]), size, lut );    \
        }                                                               \
        else if( fabs( scale ) <= 128. &&                               \
                 fabs( shift ) <= (INT_MAX*0.5)/(1 << ICV_FIX_SHIFT))   \
        {                                                               \
            int iscale = cvRound(scale*(1 << ICV_FIX_SHIFT));           \
            int ishift = cvRound(shift*(1 << ICV_FIX_SHIFT));           \
                                                                        \
            ICV_DEF_CVT_SCALE_CASE( uchar, int, ICV_SCALE,              \
                                    cast_macro, iscale, ishift );       \
        }                                                               \
        else                                                            \
        {                                                               \
            ICV_DEF_CVT_SCALE_CASE( uchar, int, cvRound,                \
                                    cast_macro, scale, shift );         \
        }                                                               \
        break;                                                          \
    case  CV_8S:                                                        \
        if( size.width*size.height >= 256 )                             \
        {                                                               \
            for( i = 0; i < 256; i++ )                                  \
            {                                                           \
                int t = cvRound( (char)i*scale + shift );               \
                lut[i] = cast_macro(t);                                 \
            }                                                           \
                                                                        \
            icvLUT_Transform8u_##flavor##_C1R( src, srcstep, dst,       \
                                dststep*sizeof(dst[0]), size, lut );    \
        }                                                               \
        else if( fabs( scale ) <= 128. &&                               \
                 fabs( shift ) <= (INT_MAX*0.5)/(1 << ICV_FIX_SHIFT))   \
        {                                                               \
            int iscale = cvRound(scale*(1 << ICV_FIX_SHIFT));           \
            int ishift = cvRound(shift*(1 << ICV_FIX_SHIFT));           \
                                                                        \
            ICV_DEF_CVT_SCALE_CASE( char, int, ICV_SCALE,               \
                                    cast_macro, iscale, ishift );       \
        }                                                               \
        else                                                            \
        {                                                               \
            ICV_DEF_CVT_SCALE_CASE( char, int, cvRound,                 \
                                    cast_macro, scale, shift );         \
        }                                                               \
        break;                                                          \
    case  CV_16U:                                                       \
        if( fabs( scale ) <= 1. && fabs(shift) < DBL_EPSILON )          \
        {                                                               \
            int iscale = cvRound(scale*(1 << ICV_FIX_SHIFT));           \
                                                                        \
            ICV_DEF_CVT_SCALE_CASE( ushort, int, ICV_SCALE,             \
                                    cast_macro, iscale, 0 );            \
        }                                                               \
        else                                                            \
        {                                                               \
            ICV_DEF_CVT_SCALE_CASE( ushort, int, cvRound,               \
                                    cast_macro, scale, shift );         \
        }                                                               \
        break;                                                          \
    case  CV_16S:                                                       \
        if( fabs( scale ) <= 1. &&                                      \
            fabs( shift ) <= (INT_MAX*0.5)/(1 << ICV_FIX_SHIFT))        \
        {                                                               \
            int iscale = cvRound(scale*(1 << ICV_FIX_SHIFT));           \
            int ishift = cvRound(shift*(1 << ICV_FIX_SHIFT));           \
                                                                        \
            ICV_DEF_CVT_SCALE_CASE( short, int, ICV_SCALE,              \
                                    cast_macro, iscale, ishift );       \
        }                                                               \
        else                                                            \
        {                                                               \
            ICV_DEF_CVT_SCALE_CASE( short, int, cvRound,                \
                                    cast_macro, scale, shift );         \
        }                                                               \
        break;                                                          \
    case  CV_32S:                                                       \
        ICV_DEF_CVT_SCALE_CASE( int, int, cvRound,                      \
                                cast_macro, scale, shift );             \
        break;                                                          \
    case  CV_32F:                                                       \
        ICV_DEF_CVT_SCALE_CASE( float, int, cvRound,                    \
                                cast_macro, scale, shift );             \
        break;                                                          \
    case  CV_64F:                                                       \
        ICV_DEF_CVT_SCALE_CASE( double, int, cvRound,                   \
                                cast_macro, scale, shift );             \
        break;                                                          \
    default:                                                            \
        assert(0);                                                      \
        return CV_BADFLAG_ERR;                                          \
    }                                                                   \
                                                                        \
    return  CV_OK;                                                      \
}

#define  ICV_DEF_CVT_SCALE_FUNC_FLT( flavor, dsttype, cast_macro )      \
static  CvStatus  CV_STDCALL                                            \
icvCvtScaleTo_##flavor##_C1R( const uchar* src, int srcstep,            \
                              dsttype* dst, int dststep, CvSize size,   \
                              double scale, double shift, int param )   \
{                                                                       \
    int i, srctype = param;                                             \
    dsttype lut[256];                                                   \
    dststep /= sizeof(dst[0]);                                          \
                                                                        \
    switch( CV_MAT_DEPTH(srctype) )                                     \
    {                                                                   \
    case  CV_8U:                                                        \
        if( size.width*size.height >= 256 )                             \
        {                                                               \
            double val = shift;                                         \
            for( i = 0; i < 256; i++, val += scale )                    \
                lut[i] = (dsttype)val;                                  \
                                                                        \
            icvLUT_Transform8u_##flavor##_C1R( src, srcstep, dst,       \
                                dststep*sizeof(dst[0]), size, lut );    \
        }                                                               \
        else                                                            \
        {                                                               \
            ICV_DEF_CVT_SCALE_CASE( uchar, double, CV_NOP,              \
                                    cast_macro, scale, shift );         \
        }                                                               \
        break;                                                          \
    case  CV_8S:                                                        \
        if( size.width*size.height >= 256 )                             \
        {                                                               \
            for( i = 0; i < 256; i++ )                                  \
                lut[i] = (dsttype)((char)i*scale + shift);              \
                                                                        \
            icvLUT_Transform8u_##flavor##_C1R( src, srcstep, dst,       \
                                dststep*sizeof(dst[0]), size, lut );    \
        }                                                               \
        else                                                            \
        {                                                               \
            ICV_DEF_CVT_SCALE_CASE( char, double, CV_NOP,               \
                                    cast_macro, scale, shift );         \
        }                                                               \
        break;                                                          \
    case  CV_16U:                                                       \
        ICV_DEF_CVT_SCALE_CASE( ushort, double, CV_NOP,                 \
                                cast_macro, scale, shift );             \
        break;                                                          \
    case  CV_16S:                                                       \
        ICV_DEF_CVT_SCALE_CASE( short, double, CV_NOP,                  \
                                cast_macro, scale, shift );             \
        break;                                                          \
    case  CV_32S:                                                       \
        ICV_DEF_CVT_SCALE_CASE( int, double, CV_NOP,                    \
                                cast_macro, scale, shift );             \
        break;                                                          \
    case  CV_32F:                                                       \
        ICV_DEF_CVT_SCALE_CASE( float, double, CV_NOP,                  \
                                cast_macro, scale, shift );             \
        break;                                                          \
    case  CV_64F:                                                       \
        ICV_DEF_CVT_SCALE_CASE( double, double, CV_NOP,                 \
                                cast_macro, scale, shift );             \
        break;                                                          \
    default:                                                            \
        assert(0);                                                      \
        return CV_BADFLAG_ERR;                                          \
    }                                                                   \
                                                                        \
    return  CV_OK;                                                      \
}


ICV_DEF_CVT_SCALE_FUNC_INT( 8u, uchar, CV_CAST_8U )
ICV_DEF_CVT_SCALE_FUNC_INT( 8s, char, CV_CAST_8S )
ICV_DEF_CVT_SCALE_FUNC_INT( 16s, short, CV_CAST_16S )
ICV_DEF_CVT_SCALE_FUNC_INT( 16u, ushort, CV_CAST_16U )
ICV_DEF_CVT_SCALE_FUNC_INT( 32s, int, CV_CAST_32S )

ICV_DEF_CVT_SCALE_FUNC_FLT( 32f, float, CV_CAST_32F )
ICV_DEF_CVT_SCALE_FUNC_FLT( 64f, double, CV_CAST_64F )

CV_DEF_INIT_FUNC_TAB_2D( CvtScaleTo, C1R )

typedef  CvStatus (CV_STDCALL *CvCvtFunc)( const void* src, int srcstep,
                                           void* dst, int dststep, CvSize size,
                                           int param );

typedef  CvStatus (CV_STDCALL *CvCvtScaleFunc)( const void* src, int srcstep,
                                             void* dst, int dststep, CvSize size,
                                             double scale, double shift,
                                             int param );


void cvConvertScale( const void* srcarr, void* dstarr, double scale, double shift )
{
    static CvFuncTable cvt_tab, cvtscale_tab;
    static int inittab = 0;

    CV_FUNCNAME( "cvConvertScale" );

    __BEGIN__;

    int type;
    int is_nd = 0;
    CvMat  srcstub, *src = (CvMat*)srcarr;
    CvMat  dststub, *dst = (CvMat*)dstarr;
    CvSize size;
    int src_step, dst_step;
    int no_scale = scale == 1 && shift == 0;

    if( !CV_IS_MAT(src) )
    {
        if( CV_IS_MATND(src) )
            is_nd = 1;
        else
        {
            int coi = 0;
            CV_CALL( src = cvGetMat( src, &srcstub, &coi,0 ));

            if( coi != 0 )
                CV_ERROR( CV_BadCOI, "" );
        }
    }

    if( !CV_IS_MAT(dst) )
    {
        if( CV_IS_MATND(dst) )
            is_nd = 1;
        else
        {
            int coi = 0;
            CV_CALL( dst = cvGetMat( dst, &dststub, &coi,0 ));

            if( coi != 0 )
                CV_ERROR( CV_BadCOI, "" );
        }
    }

    if( is_nd )
    {
        CvArr* arrs[] = { src, dst };
        CvMatND stubs[2];
        CvNArrayIterator iterator;
        int dsttype;

        CV_CALL( cvInitNArrayIterator( 2, arrs, 0, stubs, &iterator, CV_NO_DEPTH_CHECK ));

        type = iterator.hdr[0]->type;
        dsttype = iterator.hdr[1]->type;
        iterator.size.width *= CV_MAT_CN(type);

        if( !inittab )
        {
            icvInitCvtToC1RTable( &cvt_tab );
            icvInitCvtScaleToC1RTable( &cvtscale_tab );
            inittab = 1;
        }

        if( no_scale )
        {
            CvCvtFunc func = (CvCvtFunc)(cvt_tab.fn_2d[CV_MAT_DEPTH(dsttype)]);
            if( !func )
                CV_ERROR( CV_StsUnsupportedFormat, "" );

            do
            {
                IPPI_CALL( func( iterator.ptr[0], CV_STUB_STEP,
                                 iterator.ptr[1], CV_STUB_STEP,
                                 iterator.size, type ));
            }
            while( cvNextNArraySlice( &iterator ));
        }
        else
        {
            CvCvtScaleFunc func =
                (CvCvtScaleFunc)(cvtscale_tab.fn_2d[CV_MAT_DEPTH(dsttype)]);
            if( !func )
                CV_ERROR( CV_StsUnsupportedFormat, "" );

            do
            {
                IPPI_CALL( func( iterator.ptr[0], CV_STUB_STEP,
                                 iterator.ptr[1], CV_STUB_STEP,
                                 iterator.size, scale, shift, type ));
            }
            while( cvNextNArraySlice( &iterator ));
        }
        EXIT;
    }

    if( no_scale && CV_ARE_TYPES_EQ( src, dst ) )
    {
        cvCopy( src, dst,0);
        EXIT;
    }

    if( !CV_ARE_SIZES_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    size = cvGetMatSize( src );
    type = CV_MAT_TYPE(src->type);
    src_step = src->step;
    dst_step = dst->step;

    if( CV_IS_MAT_CONT( src->type & dst->type ))
    {
        size.width *= size.height;
        src_step = dst_step = CV_STUB_STEP;
        size.height = 1;
    }

    size.width *= CV_MAT_CN( type );

    if( CV_ARE_TYPES_EQ( src, dst ) && size.height == 1 &&
        size.width <= CV_MAX_INLINE_MAT_OP_SIZE )
    {
        if( CV_MAT_DEPTH(type) == CV_32F )
        {
            const float* srcdata = (const float*)(src->data.ptr);
            float* dstdata = (float*)(dst->data.ptr);

            do
            {
                dstdata[size.width - 1] = (float)(srcdata[size.width-1]*scale + shift);
            }
            while( --size.width );

            EXIT;
        }

        if( CV_MAT_DEPTH(type) == CV_64F )
        {
            const double* srcdata = (const double*)(src->data.ptr);
            double* dstdata = (double*)(dst->data.ptr);

            do
            {
                dstdata[size.width - 1] = srcdata[size.width-1]*scale + shift;
            }
            while( --size.width );

            EXIT;
        }
    }

    if( !inittab )
    {
        icvInitCvtToC1RTable( &cvt_tab );
        icvInitCvtScaleToC1RTable( &cvtscale_tab );
        inittab = 1;
    }

    if( !CV_ARE_CNS_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    if( no_scale )
    {
        CvCvtFunc func = (CvCvtFunc)(cvt_tab.fn_2d[CV_MAT_DEPTH(dst->type)]);

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        IPPI_CALL( func( src->data.ptr, src_step,
                   dst->data.ptr, dst_step, size, type ));
    }
    else
    {
        CvCvtScaleFunc func = (CvCvtScaleFunc)
            (cvtscale_tab.fn_2d[CV_MAT_DEPTH(dst->type)]);

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        IPPI_CALL( func( src->data.ptr, src_step,
                   dst->data.ptr, dst_step, size,
                   scale, shift, type ));
    }

    __END__;
}

#define ICV_DEF_LU_DECOMP_FUNC( flavor, arrtype )                               \
static CvStatus CV_STDCALL                                                      \
icvLUDecomp_##flavor( double* A, int stepA, CvSize sizeA,                       \
                      arrtype* B, int stepB, CvSize sizeB, double* _det )       \
{                                                                               \
    int n = sizeA.width;                                                        \
    int m = 0, i;                                                               \
    double det = 1;                                                             \
                                                                                \
    assert( sizeA.width == sizeA.height );                                      \
                                                                                \
    if( B )                                                                     \
    {                                                                           \
        assert( sizeA.height == sizeB.height );                                 \
        m = sizeB.width;                                                        \
    }                                                                           \
    stepA /= sizeof(A[0]);                                                      \
    stepB /= sizeof(B[0]);                                                      \
                                                                                \
    for( i = 0; i < n; i++, A += stepA, B += stepB )                            \
    {                                                                           \
        int j, k = i;                                                           \
        double* tA = A;                                                         \
        arrtype* tB = 0;                                                        \
        double kval = fabs(A[i]), tval;                                         \
                                                                                \
        /* find the pivot element */                                            \
        for( j = i + 1; j < n; j++ )                                            \
        {                                                                       \
            tA += stepA;                                                        \
            tval = fabs(tA[i]);                                                 \
                                                                                \
            if( tval > kval )                                                   \
            {                                                                   \
                kval = tval;                                                    \
                k = j;                                                          \
            }                                                                   \
        }                                                                       \
                                                                                \
        if( kval == 0 )                                                         \
        {                                                                       \
            det = 0;                                                            \
            break;                                                              \
        }                                                                       \
                                                                                \
        /* swap rows */                                                         \
        if( k != i )                                                            \
        {                                                                       \
            tA = A + stepA*(k - i);                                             \
            det = -det;                                                         \
                                                                                \
            for( j = i; j < n; j++ )                                            \
            {                                                                   \
                double t;                                                       \
                CV_SWAP( A[j], tA[j], t );                                      \
            }                                                                   \
                                                                                \
            if( m > 0 )                                                         \
            {                                                                   \
                tB = B + stepB*(k - i);                                         \
                                                                                \
                for( j = 0; j < m; j++ )                                        \
                {                                                               \
                    arrtype t = B[j];                                           \
                    CV_SWAP( B[j], tB[j], t );                                  \
                }                                                               \
            }                                                                   \
        }                                                                       \
                                                                                \
        tval = 1./A[i];                                                         \
        det *= A[i];                                                            \
        tA = A;                                                                 \
        tB = B;                                                                 \
        A[i] = tval; /* to replace division with multiplication in LUBack */    \
                                                                                \
        /* update matrix and the right side of the system */                    \
        for( j = i + 1; j < n; j++ )                                            \
        {                                                                       \
            tA += stepA;                                                        \
            tB += stepB;                                                        \
            double alpha = -tA[i]*tval;                                         \
                                                                                \
            for( k = i + 1; k < n; k++ )                                        \
                tA[k] = tA[k] + alpha*A[k];                                     \
                                                                                \
            if( m > 0 )                                                         \
                for( k = 0; k < m; k++ )                                        \
                    tB[k] = (arrtype)(tB[k] + alpha*B[k]);                      \
        }                                                                       \
    }                                                                           \
                                                                                \
    if( _det )                                                                  \
        *_det = det;                                                            \
                                                                                \
    return CV_OK;                                                               \
}


ICV_DEF_LU_DECOMP_FUNC( 32f, float )
ICV_DEF_LU_DECOMP_FUNC( 64f, double )

#define ICV_DEF_LU_BACK_FUNC( flavor, arrtype )                                 \
static CvStatus CV_STDCALL                                                      \
icvLUBack_##flavor( double* A, int stepA, CvSize sizeA,                         \
                    arrtype* B, int stepB, CvSize sizeB )                       \
{                                                                               \
    int n = sizeA.width;                                                        \
    int m = sizeB.width, i;                                                     \
                                                                                \
    assert( m > 0 && sizeA.width == sizeA.height &&                             \
            sizeA.height == sizeB.height );                                     \
    stepA /= sizeof(A[0]);                                                      \
    stepB /= sizeof(B[0]);                                                      \
                                                                                \
    A += stepA*(n - 1);                                                         \
    B += stepB*(n - 1);                                                         \
                                                                                \
    for( i = n - 1; i >= 0; i--, A -= stepA )                                   \
    {                                                                           \
        int j, k;                                                               \
        for( j = 0; j < m; j++ )                                                \
        {                                                                       \
            arrtype* tB = B + j;                                                \
            double x = 0;                                                       \
                                                                                \
            for( k = n - 1; k > i; k--, tB -= stepB )                           \
                x += A[k]*tB[0];                                                \
                                                                                \
            tB[0] = (arrtype)((tB[0] - x)*A[i]);                                \
        }                                                                       \
    }                                                                           \
                                                                                \
    return CV_OK;                                                               \
}


ICV_DEF_LU_BACK_FUNC( 32f, float )
ICV_DEF_LU_BACK_FUNC( 64f, double )

static CvFuncTable lu_decomp_tab, lu_back_tab;
static int lu_inittab = 0;

static void icvInitLUTable( CvFuncTable* decomp_tab,
                            CvFuncTable* back_tab )
{
    decomp_tab->fn_2d[0] = (void*)icvLUDecomp_32f;
    decomp_tab->fn_2d[1] = (void*)icvLUDecomp_64f;
    back_tab->fn_2d[0] = (void*)icvLUBack_32f;
    back_tab->fn_2d[1] = (void*)icvLUBack_64f;
}

int cvSolve( const CvArr* A, const CvArr* b, CvArr* x, int method )
{
    CvMat* u = 0;
    CvMat* v = 0;
    CvMat* w = 0;

    uchar* buffer = 0;
    int local_alloc = 0;
    int result = 1;

    CV_FUNCNAME( "cvSolve" );

    __BEGIN__;

    CvMat sstub, *src = (CvMat*)A;
    CvMat dstub, *dst = (CvMat*)x;
    CvMat bstub, *src2 = (CvMat*)b;
    int type;

    if( !CV_IS_MAT( src ))
        CV_CALL( src = cvGetMat( src, &sstub ,NULL,0));

    if( !CV_IS_MAT( src2 ))
        CV_CALL( src2 = cvGetMat( src2, &bstub,NULL,0 ));

    if( !CV_IS_MAT( dst ))
        CV_CALL( dst = cvGetMat( dst, &dstub,NULL,0 ));

    if( method == CV_SVD || method == CV_SVD_SYM )
    {
        int n = MIN(src->rows,src->cols);

        if( method == CV_SVD_SYM && src->rows != src->cols )
            CV_ERROR( CV_StsBadSize, "CV_SVD_SYM method is used for non-square matrix" );

        CV_CALL( u = cvCreateMat( n, src->rows, src->type ));
        if( method != CV_SVD_SYM )
            CV_CALL( v = cvCreateMat( n, src->cols, src->type ));
        CV_CALL( w = cvCreateMat( n, 1, src->type ));
        CV_CALL( cvSVD( src, w, u, v, CV_SVD_U_T + CV_SVD_V_T ));
        CV_CALL( cvSVBkSb( w, u, v ? v : u, src2, dst, CV_SVD_U_T + CV_SVD_V_T ));
        EXIT;
    }
    else if( method != CV_LU )
        CV_ERROR( CV_StsBadArg, "Unknown inversion method" );

    type = CV_MAT_TYPE( src->type );

    if( !CV_ARE_TYPES_EQ( src, dst ) || !CV_ARE_TYPES_EQ( src, src2 ))
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    if( src->width != src->height )
        CV_ERROR( CV_StsBadSize, "The matrix must be square" );

    if( !CV_ARE_SIZES_EQ( src2, dst ) || src->width != src2->height )
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    if( type != CV_32FC1 && type != CV_64FC1 )
        CV_ERROR( CV_StsUnsupportedFormat, "" );

    // check case of a single equation and small matrix
    if( src->width <= 3 && src2->width == 1 )
    {
        #define bf(y) ((float*)(bdata + y*src2step))[0]
        #define bd(y) ((double*)(bdata + y*src2step))[0]

        uchar* srcdata = src->data.ptr;
        uchar* bdata = src2->data.ptr;
        uchar* dstdata = dst->data.ptr;
        int srcstep = src->step;
        int src2step = src2->step;
        int dststep = dst->step;

        if( src->width == 2 )
        {
            if( type == CV_32FC1 )
            {
                double d = det2(Sf);
                if( d != 0. )
                {
                    float t;
                    d = 1./d;
                    t = (float)((bf(0)*Sf(1,1) - bf(1)*Sf(0,1))*d);
                    Df(1,0) = (float)((bf(1)*Sf(0,0) - bf(0)*Sf(1,0))*d);
                    Df(0,0) = t;
                }
                else
                    result = 0;
            }
            else
            {
                double d = det2(Sd);
                if( d != 0. )
                {
                    double t;
                    d = 1./d;
                    t = (bd(0)*Sd(1,1) - bd(1)*Sd(0,1))*d;
                    Dd(1,0) = (bd(1)*Sd(0,0) - bd(0)*Sd(1,0))*d;
                    Dd(0,0) = t;
                }
                else
                    result = 0;
            }
        }
        else if( src->width == 3 )
        {
            if( type == CV_32FC1 )
            {
                double d = det3(Sf);
                if( d != 0. )
                {
                    float t[3];
                    d = 1./d;

                    t[0] = (float)(d*
                           (bf(0)*(Sf(1,1)*Sf(2,2) - Sf(1,2)*Sf(2,1)) -
                            Sf(0,1)*(bf(1)*Sf(2,2) - Sf(1,2)*bf(2)) +
                            Sf(0,2)*(bf(1)*Sf(2,1) - Sf(1,1)*bf(2))));

                    t[1] = (float)(d*
                           (Sf(0,0)*(bf(1)*Sf(2,2) - Sf(1,2)*bf(2)) -
                            bf(0)*(Sf(1,0)*Sf(2,2) - Sf(1,2)*Sf(2,0)) +
                            Sf(0,2)*(Sf(1,0)*bf(2) - bf(1)*Sf(2,0))));

                    t[2] = (float)(d*
                           (Sf(0,0)*(Sf(1,1)*bf(2) - bf(1)*Sf(2,1)) -
                            Sf(0,1)*(Sf(1,0)*bf(2) - bf(1)*Sf(2,0)) +
                            bf(0)*(Sf(1,0)*Sf(2,1) - Sf(1,1)*Sf(2,0))));

                    Df(0,0) = t[0];
                    Df(1,0) = t[1];
                    Df(2,0) = t[2];
                }
                else
                    result = 0;
            }
            else
            {
                double d = det3(Sd);
                if( d != 0. )
                {
                    double t[9];

                    d = 1./d;
                    
                    t[0] = ((Sd(1,1) * Sd(2,2) - Sd(1,2) * Sd(2,1))*bd(0) +
                            (Sd(0,2) * Sd(2,1) - Sd(0,1) * Sd(2,2))*bd(1) +
                            (Sd(0,1) * Sd(1,2) - Sd(0,2) * Sd(1,1))*bd(2))*d;

                    t[1] = ((Sd(1,2) * Sd(2,0) - Sd(1,0) * Sd(2,2))*bd(0) +
                            (Sd(0,0) * Sd(2,2) - Sd(0,2) * Sd(2,0))*bd(1) +
                            (Sd(0,2) * Sd(1,0) - Sd(0,0) * Sd(1,2))*bd(2))*d;

                    t[2] = ((Sd(1,0) * Sd(2,1) - Sd(1,1) * Sd(2,0))*bd(0) +
                            (Sd(0,1) * Sd(2,0) - Sd(0,0) * Sd(2,1))*bd(1) +
                            (Sd(0,0) * Sd(1,1) - Sd(0,1) * Sd(1,0))*bd(2))*d;

                    Dd(0,0) = t[0];
                    Dd(1,0) = t[1];
                    Dd(2,0) = t[2];
                }
                else
                    result = 0;
            }
        }
        else
        {
            assert( src->width == 1 );

            if( type == CV_32FC1 )
            {
                double d = Sf(0,0);
                if( d != 0. )
                    Df(0,0) = (float)(bf(0)/d);
                else
                    result = 0;
            }
            else
            {
                double d = Sd(0,0);
                if( d != 0. )
                    Dd(0,0) = (bd(0)/d);
                else
                    result = 0;
            }
        }
    }
    else
    {
        CvLUDecompFunc decomp_func;
        CvLUBackFunc back_func;
        CvSize size = cvGetMatSize( src );
        CvSize dstsize = cvGetMatSize( dst );
        int worktype = CV_64FC1;
        int buf_size = size.width*size.height*CV_ELEM_SIZE(worktype);
        double d = 0;
        CvMat tmat;

        if( !lu_inittab )
        {
            icvInitLUTable( &lu_decomp_tab, &lu_back_tab );
            lu_inittab = 1;
        }

        if( size.width <= CV_MAX_LOCAL_MAT_SIZE )
        {
            buffer = (uchar*)cvStackAlloc( buf_size );
            local_alloc = 1;
        }
        else
        {
            CV_CALL( buffer = (uchar*)cvAlloc( buf_size ));
        }

        CV_CALL( cvInitMatHeader( &tmat, size.height, size.width, worktype, buffer,CV_AUTOSTEP ));
        if( type == worktype )
        {
            CV_CALL( cvCopy( src, &tmat,NULL ));
        }
        else
            CV_CALL( cvConvert( src, &tmat ));

        if( src2->data.ptr != dst->data.ptr )
        {
            CV_CALL( cvCopy( src2, dst,NULL ));
        }

        decomp_func = (CvLUDecompFunc)(lu_decomp_tab.fn_2d[CV_MAT_DEPTH(type)-CV_32F]);
        back_func = (CvLUBackFunc)(lu_back_tab.fn_2d[CV_MAT_DEPTH(type)-CV_32F]);
        assert( decomp_func && back_func );

        IPPI_CALL( decomp_func( tmat.data.db, tmat.step, size,
                                dst->data.ptr, dst->step, dstsize, &d ));

        if( d != 0 )
        {
            IPPI_CALL( back_func( tmat.data.db, tmat.step, size,
                                  dst->data.ptr, dst->step, dstsize ));
        }
        else
            result = 0;
    }

    if( !result )
        CV_CALL( cvSetZero( dst ));

    __END__;

    if( buffer && !local_alloc )
        cvFree( &buffer );

    if( u || v || w )
    {
        cvReleaseMat( &u );
        cvReleaseMat( &v );
        cvReleaseMat( &w );
    }

    return result;
}

static void icvCheckHuge( CvMat* arr )
{
    if( (int64)arr->step*arr->rows > INT_MAX )
        arr->type &= ~CV_MAT_CONT_FLAG;
}

CvMat* cvInitMatHeader( CvMat* arr, int rows, int cols,int type, void* data, int step )
{
    CV_FUNCNAME( "cvInitMatHeader" );
    
    __BEGIN__;

    int mask, pix_size, min_step;

    if( !arr )
        CV_ERROR_FROM_CODE( CV_StsNullPtr );

    if( (unsigned)CV_MAT_DEPTH(type) > CV_DEPTH_MAX )
        CV_ERROR_FROM_CODE( CV_BadNumChannels );

    if( rows <= 0 || cols <= 0 )
        CV_ERROR( CV_StsBadSize, "Non-positive cols or rows" );
 
    type = CV_MAT_TYPE( type );
    arr->type = type | CV_MAT_MAGIC_VAL;
    arr->rows = rows;
    arr->cols = cols;
    arr->data.ptr = (uchar*)data;
    arr->refcount = 0;
    arr->hdr_refcount = 0;

    mask = (arr->rows <= 1) - 1;
    pix_size = CV_ELEM_SIZE(type);
    min_step = arr->cols*pix_size & mask;

    if( step != CV_AUTOSTEP && step != 0 )
    {
        if( step < min_step )
            CV_ERROR_FROM_CODE( CV_BadStep );
        arr->step = step & mask;
    }
    else
    {
        arr->step = min_step;
    }

    arr->type = CV_MAT_MAGIC_VAL | type |
                (arr->step == min_step ? CV_MAT_CONT_FLAG : 0);

    icvCheckHuge( arr );

    __END__;

    return arr;
}

CvPoint2D64f  cvPoint2D64f( double x, double y )
{
    CvPoint2D64f p;

    p.x = x;
    p.y = y;

    return p;
}
#define ICV_DEF_GEMM_SINGLE_MUL( flavor, arrtype, worktype )                \
static CvStatus CV_STDCALL                                                  \
icvGEMMSingleMul_##flavor( const arrtype* a_data, size_t a_step,            \
                         const arrtype* b_data, size_t b_step,              \
                         const arrtype* c_data, size_t c_step,              \
                         arrtype* d_data, size_t d_step,                    \
                         CvSize a_size, CvSize d_size,                      \
                         double alpha, double beta, int flags )             \
{                                                                           \
    int i, j, k, n = a_size.width, m = d_size.width, drows = d_size.height; \
    const arrtype *_a_data = a_data, *_b_data = b_data, *_c_data = c_data;  \
    arrtype* a_buf = 0;                                                     \
    size_t a_step0, a_step1, c_step0, c_step1, t_step;                      \
                                                                            \
    a_step /= sizeof(a_data[0]);                                            \
    b_step /= sizeof(b_data[0]);                                            \
    c_step /= sizeof(c_data[0]);                                            \
    d_step /= sizeof(d_data[0]);                                            \
    a_step0 = a_step;                                                       \
    a_step1 = 1;                                                            \
                                                                            \
    if( !c_data )                                                           \
        c_step0 = c_step1 = 0;                                              \
    else if( !(flags & CV_GEMM_C_T) )                                       \
        c_step0 = c_step, c_step1 = 1;                                      \
    else                                                                    \
        c_step0 = 1, c_step1 = c_step;                                      \
                                                                            \
    if( flags & CV_GEMM_A_T )                                               \
    {                                                                       \
        CV_SWAP( a_step0, a_step1, t_step );                                \
        n = a_size.height;                                                  \
        if( a_step > 1 && n > 1 )                                           \
            a_buf = (arrtype*)cvStackAlloc(n*sizeof(a_data[0]));            \
    }                                                                       \
                                                                            \
    if( n == 1 ) /* external product */                                     \
    {                                                                       \
        arrtype* b_buf = 0;                                                 \
                                                                            \
        if( a_step > 1 )                                                    \
        {                                                                   \
            a_buf = (arrtype*)cvStackAlloc(drows*sizeof(a_data[0]));        \
            for( k = 0; k < drows; k++ )                                    \
                a_buf[k] = a_data[a_step*k];                                \
            a_data = a_buf;                                                 \
        }                                                                   \
                                                                            \
        if( b_step > 1 )                                                    \
        {                                                                   \
            b_buf = (arrtype*)cvStackAlloc(d_size.width*sizeof(b_buf[0]) ); \
            for( j = 0; j < d_size.width; j++ )                             \
                b_buf[j] = b_data[j*b_step];                                \
            b_data = b_buf;                                                 \
        }                                                                   \
                                                                            \
        for( i = 0; i < drows; i++, _c_data += c_step0,                     \
                                    d_data += d_step )                      \
        {                                                                   \
            worktype al = worktype(a_data[i])*alpha;                        \
            c_data = _c_data;                                               \
            for( j = 0; j <= d_size.width - 2; j += 2, c_data += 2*c_step1 )\
            {                                                               \
                worktype s0 = al*b_data[j];                                 \
                worktype s1 = al*b_data[j+1];                               \
                if( !c_data )                                               \
                {                                                           \
                    d_data[j] = arrtype(s0);                                \
                    d_data[j+1] = arrtype(s1);                              \
                }                                                           \
                else                                                        \
                {                                                           \
                    d_data[j] = arrtype(s0 + c_data[0]*beta);               \
                    d_data[j+1] = arrtype(s1 + c_data[c_step1]*beta);       \
                }                                                           \
            }                                                               \
                                                                            \
            for( ; j < d_size.width; j++, c_data += c_step1 )               \
            {                                                               \
                worktype s0 = al*b_data[j];                                 \
                if( !c_data )                                               \
                    d_data[j] = arrtype(s0);                                \
                else                                                        \
                    d_data[j] = arrtype(s0 + c_data[0]*beta);               \
            }                                                               \
        }                                                                   \
    }                                                                       \
    else if( flags & CV_GEMM_B_T ) /* A * Bt */                             \
    {                                                                       \
        for( i = 0; i < drows; i++, _a_data += a_step0,                     \
                                    _c_data += c_step0,                     \
                                    d_data += d_step )                      \
        {                                                                   \
            a_data = _a_data;                                               \
            b_data = _b_data;                                               \
            c_data = _c_data;                                               \
                                                                            \
            if( a_buf )                                                     \
            {                                                               \
                for( k = 0; k < n; k++ )                                    \
                    a_buf[k] = a_data[a_step1*k];                           \
                a_data = a_buf;                                             \
            }                                                               \
                                                                            \
            for( j = 0; j < d_size.width; j++, b_data += b_step,            \
                                               c_data += c_step1 )          \
            {                                                               \
                worktype s0(0), s1(0), s2(0), s3(0);                        \
                                                                            \
                for( k = 0; k <= n - 4; k += 4 )                            \
                {                                                           \
                    s0 += worktype(a_data[k])*b_data[k];                    \
                    s1 += worktype(a_data[k+1])*b_data[k+1];                \
                    s2 += worktype(a_data[k+2])*b_data[k+2];                \
                    s3 += worktype(a_data[k+3])*b_data[k+3];                \
                }                                                           \
                                                                            \
                for( ; k < n; k++ )                                         \
                    s0 += worktype(a_data[k])*b_data[k];                    \
                s0 = (s0+s1+s2+s3)*alpha;                                   \
                                                                            \
                if( !c_data )                                               \
                    d_data[j] = arrtype(s0);                                \
                else                                                        \
                    d_data[j] = arrtype(s0 + c_data[0]*beta);               \
            }                                                               \
        }                                                                   \
    }                                                                       \
    else if( d_size.width*sizeof(d_data[0]) <= 1600 )                       \
    {                                                                       \
        for( i = 0; i < drows; i++, _a_data += a_step0,                     \
                                    _c_data += c_step0,                     \
                                    d_data += d_step )                      \
        {                                                                   \
            a_data = _a_data, c_data = _c_data;                             \
                                                                            \
            if( a_buf )                                                     \
            {                                                               \
                for( k = 0; k < n; k++ )                                    \
                    a_buf[k] = a_data[a_step1*k];                           \
                a_data = a_buf;                                             \
            }                                                               \
                                                                            \
            for( j = 0; j <= m - 4; j += 4, c_data += 4*c_step1 )           \
            {                                                               \
                const arrtype* b = _b_data + j;                             \
                worktype s0(0), s1(0), s2(0), s3(0);                        \
                                                                            \
                for( k = 0; k < n; k++, b += b_step )                       \
                {                                                           \
                    worktype a(a_data[k]);                                  \
                    s0 += a * b[0]; s1 += a * b[1];                         \
                    s2 += a * b[2]; s3 += a * b[3];                         \
                }                                                           \
                                                                            \
                if( !c_data )                                               \
                {                                                           \
                    d_data[j] = arrtype(s0*alpha);                          \
                    d_data[j+1] = arrtype(s1*alpha);                        \
                    d_data[j+2] = arrtype(s2*alpha);                        \
                    d_data[j+3] = arrtype(s3*alpha);                        \
                }                                                           \
                else                                                        \
                {                                                           \
                    s0 = s0*alpha; s1 = s1*alpha;                           \
                    s2 = s2*alpha; s3 = s3*alpha;                           \
                    d_data[j] = arrtype(s0 + c_data[0]*beta);               \
                    d_data[j+1] = arrtype(s1 + c_data[c_step1]*beta);       \
                    d_data[j+2] = arrtype(s2 + c_data[c_step1*2]*beta);     \
                    d_data[j+3] = arrtype(s3 + c_data[c_step1*3]*beta);     \
                }                                                           \
            }                                                               \
                                                                            \
            for( ; j < m; j++, c_data += c_step1 )                          \
            {                                                               \
                const arrtype* b = _b_data + j;                             \
                worktype s0(0);                                             \
                                                                            \
                for( k = 0; k < n; k++, b += b_step )                       \
                    s0 += worktype(a_data[k]) * b[0];                       \
                                                                            \
                s0 = s0*alpha;                                              \
                if( !c_data )                                               \
                    d_data[j] = arrtype(s0);                                \
                else                                                        \
                    d_data[j] = arrtype(s0 + c_data[0]*beta);               \
            }                                                               \
        }                                                                   \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        worktype* d_buf = (worktype*)cvStackAlloc(m*sizeof(d_buf[0]));      \
                                                                            \
        for( i = 0; i < drows; i++, _a_data += a_step0,                     \
                                            _c_data += c_step0,             \
                                            d_data += d_step )              \
        {                                                                   \
            a_data = _a_data;                                               \
            b_data = _b_data;                                               \
            c_data = _c_data;                                               \
                                                                            \
            if( a_buf )                                                     \
            {                                                               \
                for( k = 0; k < n; k++ )                                    \
                    a_buf[k] = _a_data[a_step1*k];                          \
                a_data = a_buf;                                             \
            }                                                               \
                                                                            \
            for( j = 0; j < m; j++ )                                        \
                d_buf[j] = worktype(0);                                     \
                                                                            \
            for( k = 0; k < n; k++, b_data += b_step )                      \
            {                                                               \
                worktype al(a_data[k]);                                     \
                                                                            \
                for( j = 0; j <= m - 4; j += 4 )                            \
                {                                                           \
                    worktype t0 = d_buf[j] + b_data[j]*al;                  \
                    worktype t1 = d_buf[j+1] + b_data[j+1]*al;              \
                    d_buf[j] = t0;                                          \
                    d_buf[j+1] = t1;                                        \
                    t0 = d_buf[j+2] + b_data[j+2]*al;                       \
                    t1 = d_buf[j+3] + b_data[j+3]*al;                       \
                    d_buf[j+2] = t0;                                        \
                    d_buf[j+3] = t1;                                        \
                }                                                           \
                                                                            \
                for( ; j < m; j++ )                                         \
                    d_buf[j] += b_data[j]*al;                               \
            }                                                               \
                                                                            \
            if( !c_data )                                                   \
                for( j = 0; j < m; j++ )                                    \
                    d_data[j] = arrtype(d_buf[j]*alpha);                    \
            else                                                            \
                for( j = 0; j < m; j++, c_data += c_step1 )                 \
                {                                                           \
                    worktype t = d_buf[j]*alpha;                            \
                    d_data[j] = arrtype(t + c_data[0]*beta);                \
                }                                                           \
        }                                                                   \
    }                                                                       \
    return CV_OK;                                                           \
}

#define ICV_DEF_GEMM_BLOCK_MUL( flavor, arrtype, worktype )         \
static CvStatus CV_STDCALL                                          \
icvGEMMBlockMul_##flavor( const arrtype* a_data, size_t a_step,     \
                        const arrtype* b_data, size_t b_step,       \
                        worktype* d_data, size_t d_step,            \
                        CvSize a_size, CvSize d_size, int flags )   \
{                                                                   \
    int i, j, k, n = a_size.width, m = d_size.width;                \
    const arrtype *_a_data = a_data, *_b_data = b_data;             \
    arrtype* a_buf = 0;                                             \
    size_t a_step0, a_step1, t_step;                                \
    int do_acc = flags & 16;                                        \
                                                                    \
    a_step /= sizeof(a_data[0]);                                    \
    b_step /= sizeof(b_data[0]);                                    \
    d_step /= sizeof(d_data[0]);                                    \
                                                                    \
    a_step0 = a_step;                                               \
    a_step1 = 1;                                                    \
                                                                    \
    if( flags & CV_GEMM_A_T )                                       \
    {                                                               \
        CV_SWAP( a_step0, a_step1, t_step );                        \
        n = a_size.height;                                          \
        a_buf = (arrtype*)cvStackAlloc(n*sizeof(a_data[0]));        \
    }                                                               \
                                                                    \
    if( flags & CV_GEMM_B_T )                                       \
    {                                                               \
        /* second operand is transposed */                          \
        for( i = 0; i < d_size.height; i++, _a_data += a_step0,     \
                                            d_data += d_step )      \
        {                                                           \
            a_data = _a_data; b_data = _b_data;                     \
                                                                    \
            if( a_buf )                                             \
            {                                                       \
                for( k = 0; k < n; k++ )                            \
                    a_buf[k] = a_data[a_step1*k];                   \
                a_data = a_buf;                                     \
            }                                                       \
                                                                    \
            for( j = 0; j < d_size.width; j++, b_data += b_step )   \
            {                                                       \
                worktype s0 = do_acc ? d_data[j]:worktype(0), s1(0);\
                for( k = 0; k <= n - 2; k += 2 )                    \
                {                                                   \
                    s0 += worktype(a_data[k])*b_data[k];            \
                    s1 += worktype(a_data[k+1])*b_data[k+1];        \
                }                                                   \
                                                                    \
                for( ; k < n; k++ )                                 \
                    s0 += worktype(a_data[k])*b_data[k];            \
                                                                    \
                d_data[j] = s0 + s1;                                \
            }                                                       \
        }                                                           \
    }                                                               \
    else                                                            \
    {                                                               \
        for( i = 0; i < d_size.height; i++, _a_data += a_step0,     \
                                            d_data += d_step )      \
        {                                                           \
            a_data = _a_data, b_data = _b_data;                     \
                                                                    \
            if( a_buf )                                             \
            {                                                       \
                for( k = 0; k < n; k++ )                            \
                    a_buf[k] = a_data[a_step1*k];                   \
                a_data = a_buf;                                     \
            }                                                       \
                                                                    \
            for( j = 0; j <= m - 4; j += 4 )                        \
            {                                                       \
                worktype s0, s1, s2, s3;                            \
                const arrtype* b = b_data + j;                      \
                                                                    \
                if( do_acc )                                        \
                {                                                   \
                    s0 = d_data[j]; s1 = d_data[j+1];               \
                    s2 = d_data[j+2]; s3 = d_data[j+3];             \
                }                                                   \
                else                                                \
                    s0 = s1 = s2 = s3 = worktype(0);                \
                                                                    \
                for( k = 0; k < n; k++, b += b_step )               \
                {                                                   \
                    worktype a(a_data[k]);                          \
                    s0 += a * b[0]; s1 += a * b[1];                 \
                    s2 += a * b[2]; s3 += a * b[3];                 \
                }                                                   \
                                                                    \
                d_data[j] = s0; d_data[j+1] = s1;                   \
                d_data[j+2] = s2; d_data[j+3] = s3;                 \
            }                                                       \
                                                                    \
            for( ; j < m; j++ )                                     \
            {                                                       \
                const arrtype* b = b_data + j;                      \
                worktype s0 = do_acc ? d_data[j] : worktype(0);     \
                                                                    \
                for( k = 0; k < n; k++, b += b_step )               \
                    s0 += worktype(a_data[k]) * b[0];               \
                                                                    \
                d_data[j] = s0;                                     \
            }                                                       \
        }                                                           \
    }                                                               \
                                                                    \
    return CV_OK;                                                   \
}

#define ICV_DEF_GEMM_STORE( flavor, arrtype, worktype )             \
static CvStatus CV_STDCALL                                          \
icvGEMMStore_##flavor( const arrtype* c_data, size_t c_step,        \
                       const worktype* d_buf, size_t d_buf_step,    \
                       arrtype* d_data, size_t d_step, CvSize d_size,\
                       double alpha, double beta, int flags )       \
{                                                                   \
    const arrtype* _c_data = c_data;                                \
    int j;                                                          \
    size_t c_step0, c_step1;                                        \
                                                                    \
    c_step /= sizeof(c_data[0]);                                    \
    d_buf_step /= sizeof(d_buf[0]);                                 \
    d_step /= sizeof(d_data[0]);                                    \
                                                                    \
    if( !c_data )                                                   \
        c_step0 = c_step1 = 0;                                      \
    else if( !(flags & CV_GEMM_C_T) )                               \
        c_step0 = c_step, c_step1 = 1;                              \
    else                                                            \
        c_step0 = 1, c_step1 = c_step;                              \
                                                                    \
    for( ; d_size.height--; _c_data += c_step0,                     \
                            d_buf += d_buf_step,                    \
                            d_data += d_step )                      \
    {                                                               \
        if( _c_data )                                               \
        {                                                           \
            c_data = _c_data;                                       \
            for( j = 0; j <= d_size.width - 4; j += 4, c_data += 4*c_step1 )\
            {                                                       \
                worktype t0 = alpha*d_buf[j];                       \
                worktype t1 = alpha*d_buf[j+1];                     \
                t0 += beta*worktype(c_data[0]);                     \
                t1 += beta*worktype(c_data[c_step1]);               \
                d_data[j] = arrtype(t0);                            \
                d_data[j+1] = arrtype(t1);                          \
                t0 = alpha*d_buf[j+2];                              \
                t1 = alpha*d_buf[j+3];                              \
                t0 += beta*worktype(c_data[c_step1*2]);             \
                t1 += beta*worktype(c_data[c_step1*3]);             \
                d_data[j+2] = arrtype(t0);                          \
                d_data[j+3] = arrtype(t1);                          \
            }                                                       \
            for( ; j < d_size.width; j++, c_data += c_step1 )       \
            {                                                       \
                worktype t0 = alpha*d_buf[j];                       \
                d_data[j] = arrtype(t0 + beta*c_data[0]);           \
            }                                                       \
        }                                                           \
        else                                                        \
        {                                                           \
            for( j = 0; j <= d_size.width - 4; j += 4 )             \
            {                                                       \
                worktype t0 = alpha*d_buf[j];                       \
                worktype t1 = alpha*d_buf[j+1];                     \
                d_data[j] = arrtype(t0);                            \
                d_data[j+1] = arrtype(t1);                          \
                t0 = alpha*d_buf[j+2];                              \
                t1 = alpha*d_buf[j+3];                              \
                d_data[j+2] = arrtype(t0);                          \
                d_data[j+3] = arrtype(t1);                          \
            }                                                       \
            for( ; j < d_size.width; j++ )                          \
                d_data[j] = arrtype(alpha*d_buf[j]);                \
        }                                                           \
    }                                                               \
    return CV_OK;                                                   \
}

ICV_DEF_GEMM_SINGLE_MUL( 32f_C1R, float, double)
ICV_DEF_GEMM_BLOCK_MUL( 32f_C1R, float, double)
ICV_DEF_GEMM_STORE( 32f_C1R, float, double)

ICV_DEF_GEMM_SINGLE_MUL( 64f_C1R, double, double)
ICV_DEF_GEMM_BLOCK_MUL( 64f_C1R, double, double)
ICV_DEF_GEMM_STORE( 64f_C1R, double, double)

ICV_DEF_GEMM_SINGLE_MUL( 32f_C2R, CvComplex32f, CvComplex64f)
ICV_DEF_GEMM_BLOCK_MUL( 32f_C2R, CvComplex32f, CvComplex64f)
ICV_DEF_GEMM_STORE( 32f_C2R, CvComplex32f, CvComplex64f)

ICV_DEF_GEMM_SINGLE_MUL( 64f_C2R, CvComplex64f, CvComplex64f)
ICV_DEF_GEMM_BLOCK_MUL( 64f_C2R, CvComplex64f, CvComplex64f)
ICV_DEF_GEMM_STORE( 64f_C2R, CvComplex64f, CvComplex64f)

IPCVAPI_C_EX( void, icvBLAS_GEMM_32f, "sgemm, mkl_sgemm", CV_PLUGINS2(CV_PLUGIN_MKL,CV_PLUGIN_MKL),
                        (const char *transa, const char *transb, int *n, int *m, int *k,
                         const void *alpha, const void *a, int *lda, const void *b, int *ldb,
                         const void *beta, void *c, int *ldc ))

IPCVAPI_C_EX( void, icvBLAS_GEMM_64f, "dgemm, mkl_dgemm", CV_PLUGINS2(CV_PLUGIN_MKL,CV_PLUGIN_MKL),
                        (const char *transa, const char *transb, int *n, int *m, int *k,
                         const void *alpha, const void *a, int *lda, const void *b, int *ldb,
                         const void *beta, void *c, int *ldc ))

IPCVAPI_C_EX( void, icvBLAS_GEMM_32fc, "cgemm, mkl_cgemm", CV_PLUGINS2(CV_PLUGIN_MKL,CV_PLUGIN_MKL),
                        (const char *transa, const char *transb, int *n, int *m, int *k,
                         const void *alpha, const void *a, int *lda, const void *b, int *ldb,
                         const void *beta, void *c, int *ldc ))

IPCVAPI_C_EX( void, icvBLAS_GEMM_64fc, "zgemm, mkl_zgemm", CV_PLUGINS2(CV_PLUGIN_MKL,CV_PLUGIN_MKL),
                        (const char *transa, const char *transb, int *n, int *m, int *k,
                         const void *alpha, const void *a, int *lda, const void *b, int *ldb,
                         const void *beta, void *c, int *ldc ))

static void icvInitGEMMTable( CvBigFuncTable* single_mul_tab,
                              CvBigFuncTable* block_mul_tab,
                              CvBigFuncTable* store_tab )
{
    single_mul_tab->fn_2d[CV_32FC1] = (void*)icvGEMMSingleMul_32f_C1R;
    single_mul_tab->fn_2d[CV_64FC1] = (void*)icvGEMMSingleMul_64f_C1R;
    single_mul_tab->fn_2d[CV_32FC2] = (void*)icvGEMMSingleMul_32f_C2R;
    single_mul_tab->fn_2d[CV_64FC2] = (void*)icvGEMMSingleMul_64f_C2R;
    block_mul_tab->fn_2d[CV_32FC1] = (void*)icvGEMMBlockMul_32f_C1R;
    block_mul_tab->fn_2d[CV_64FC1] = (void*)icvGEMMBlockMul_64f_C1R;
    block_mul_tab->fn_2d[CV_32FC2] = (void*)icvGEMMBlockMul_32f_C2R;
    block_mul_tab->fn_2d[CV_64FC2] = (void*)icvGEMMBlockMul_64f_C2R;
    store_tab->fn_2d[CV_32FC1] = (void*)icvGEMMStore_32f_C1R;
    store_tab->fn_2d[CV_64FC1] = (void*)icvGEMMStore_64f_C1R;
    store_tab->fn_2d[CV_32FC2] = (void*)icvGEMMStore_32f_C2R;
    store_tab->fn_2d[CV_64FC2] = (void*)icvGEMMStore_64f_C2R;
}

icvBLAS_GEMM_32f_t icvBLAS_GEMM_32f_p = 0;
icvBLAS_GEMM_64f_t icvBLAS_GEMM_64f_p = 0;
icvBLAS_GEMM_32fc_t icvBLAS_GEMM_32fc_p = 0;
icvBLAS_GEMM_64fc_t icvBLAS_GEMM_64fc_p = 0;

static void
icvGEMM_TransposeBlock( const uchar* src, int src_step,
                        uchar* dst, int dst_step,
                        CvSize size, int pix_size )
{
    int i, j;
    for( i = 0; i < size.width; i++, dst += dst_step, src += pix_size )
    {
        const uchar* _src = src;
        switch( pix_size )
        {
        case sizeof(int):
            for( j = 0; j < size.height; j++, _src += src_step )
                ((int*)dst)[j] = ((int*)_src)[0];
            break;
        case sizeof(int)*2:
            for( j = 0; j < size.height*2; j += 2, _src += src_step )
            {
                int t0 = ((int*)_src)[0];
                int t1 = ((int*)_src)[1];
                ((int*)dst)[j] = t0;
                ((int*)dst)[j+1] = t1;
            }
            break;
        case sizeof(int)*4:
            for( j = 0; j < size.height*4; j += 4, _src += src_step )
            {
                int t0 = ((int*)_src)[0];
                int t1 = ((int*)_src)[1];
                ((int*)dst)[j] = t0;
                ((int*)dst)[j+1] = t1;
                t0 = ((int*)_src)[2];
                t1 = ((int*)_src)[3];
                ((int*)dst)[j+2] = t0;
                ((int*)dst)[j+3] = t1;
            }
            break;
        default:
            assert(0);
            return;
        }
    }
}

static void
icvGEMM_CopyBlock( const uchar* src, int src_step,
                   uchar* dst, int dst_step,
                   CvSize size, int pix_size )
{
    int j;
    size.width = size.width * (pix_size / sizeof(int));

    for( ; size.height--; src += src_step, dst += dst_step )
    {
        for( j = 0; j <= size.width - 4; j += 4 )
        {
            int t0 = ((const int*)src)[j];
            int t1 = ((const int*)src)[j+1];
            ((int*)dst)[j] = t0;
            ((int*)dst)[j+1] = t1;
            t0 = ((const int*)src)[j+2];
            t1 = ((const int*)src)[j+3];
            ((int*)dst)[j+2] = t0;
            ((int*)dst)[j+3] = t1;
        }

        for( ; j < size.width; j++ )
            ((int*)dst)[j] = ((const int*)src)[j];
    }
}

void cvGEMM( const CvArr* Aarr, const CvArr* Barr, double alpha,const CvArr* Carr, double beta, CvArr* Darr, int flags )
{
    const int block_lin_size = 128;
    const int block_size = block_lin_size * block_lin_size;

    static CvBigFuncTable single_mul_tab, block_mul_tab, store_tab;
    static int inittab = 0;
    static double zero[] = {0,0,0,0};
    static float zerof[] = {0,0,0,0};
    
    uchar* buffer = 0;
    int local_alloc = 0;
    uchar* block_buffer = 0;

    CV_FUNCNAME( "cvGEMM" );

    __BEGIN__;

    CvMat *A = (CvMat*)Aarr;
    CvMat *B = (CvMat*)Barr;
    CvMat *C = (CvMat*)Carr;
    CvMat *D = (CvMat*)Darr;
    int len = 0;
    
    CvMat stub, stub1, stub2, stub3;
    CvSize a_size, d_size;
    int type;

    if( !CV_IS_MAT( A ))
    {
        int coi = 0;
        CV_CALL( A = cvGetMat( A, &stub1, &coi,0 ));

        if( coi != 0 )
            CV_ERROR( CV_BadCOI, "" );
    }

    if( !CV_IS_MAT( B ))
    {
        int coi = 0;
        CV_CALL( B = cvGetMat( B, &stub2, &coi,0 ));

        if( coi != 0 )
            CV_ERROR( CV_BadCOI, "" );
    }

    if( !CV_IS_MAT( D ))
    {
        int coi = 0;
        CV_CALL( D = cvGetMat( D, &stub, &coi,0 ));

        if( coi != 0 )
            CV_ERROR( CV_BadCOI, "" );
    }

    if( beta == 0 )
        C = 0;

    if( C )
    {
        if( !CV_IS_MAT( C ))
        {
            int coi = 0;
            CV_CALL( C = cvGetMat( C, &stub3, &coi,0 ));

            if( coi != 0 )
                CV_ERROR( CV_BadCOI, "" );
        }

        if( !CV_ARE_TYPES_EQ( C, D ))
            CV_ERROR( CV_StsUnmatchedFormats, "" );

        if( (flags&CV_GEMM_C_T) == 0 && (C->cols != D->cols || C->rows != D->rows) ||
            (flags&CV_GEMM_C_T) != 0 && (C->rows != D->cols || C->cols != D->rows))
            CV_ERROR( CV_StsUnmatchedSizes, "" );

        if( (flags & CV_GEMM_C_T) != 0 && C->data.ptr == D->data.ptr )
        {
            cvTranspose( C, D );
            C = D;
            flags &= ~CV_GEMM_C_T;
        }
    }
    else
    {
        C = &stub3;
        C->data.ptr = 0;
        C->step = 0;
        C->type = CV_MAT_CONT_FLAG;
    }

    type = CV_MAT_TYPE(A->type);
    if( !CV_ARE_TYPES_EQ( A, B ) || !CV_ARE_TYPES_EQ( A, D ) )
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    a_size.width = A->cols;
    a_size.height = A->rows;
    d_size.width = D->cols;
    d_size.height = D->rows;

    switch( flags & (CV_GEMM_A_T|CV_GEMM_B_T) )
    {
    case 0:
        len = B->rows;
        if( a_size.width != len ||
            B->cols != d_size.width ||
            a_size.height != d_size.height )
            CV_ERROR( CV_StsUnmatchedSizes, "" );
        break;
    case 1:
        len = B->rows;
        if( a_size.height != len ||
            B->cols != d_size.width ||
            a_size.width != d_size.height )
            CV_ERROR( CV_StsUnmatchedSizes, "" );
        break;
    case 2:
        len = B->cols;
        if( a_size.width != len ||
            B->rows != d_size.width ||
            a_size.height != d_size.height )
            CV_ERROR( CV_StsUnmatchedSizes, "" );
        break;
    case 3:
        len = B->cols;
        if( a_size.height != len ||
            B->rows != d_size.width ||
            a_size.width != d_size.height )
            CV_ERROR( CV_StsUnmatchedSizes, "" );
        break;
    }

    if( flags == 0 && 2 <= len && len <= 4 && (len == d_size.width || len == d_size.height) )
    {
        int i;
        if( type == CV_64F )
        {
            double* d = D->data.db;
            const double *a = A->data.db, *b = B->data.db, *c = C->data.db;
            size_t d_step = D->step/sizeof(d[0]),
                   a_step = A->step/sizeof(a[0]),
                   b_step = B->step/sizeof(b[0]),
                   c_step = C->step/sizeof(c[0]);

            if( !c )
                c = zero;

            switch( len )
            {
            case 2:
                if( len == d_size.width && b != d )
                {
                    for( i = 0; i < d_size.height; i++, d += d_step, a += a_step, c += c_step )
                    {
                        double t0 = a[0]*b[0] + a[1]*b[b_step];
                        double t1 = a[0]*b[1] + a[1]*b[b_step+1];
                        d[0] = t0*alpha + c[0]*beta;
                        d[1] = t1*alpha + c[1]*beta;
                    }
                }
                else if( a != d )
                {
                    int c_step0 = 1;
                    if( c == zero )
                    {
                        c_step0 = 0;
                        c_step = 1;
                    }

                    for( i = 0; i < d_size.width; i++, d++, b++, c += c_step0 )
                    {
                        double t0 = a[0]*b[0] + a[1]*b[b_step];
                        double t1 = a[a_step]*b[0] + a[a_step+1]*b[b_step];
                        d[0] = t0*alpha + c[0]*beta;
                        d[d_step] = t1*alpha + c[c_step]*beta;
                    }
                }
                else
                    break;
                EXIT;
            case 3:
                if( len == d_size.width && b != d )
                {
                    for( i = 0; i < d_size.height; i++, d += d_step, a += a_step, c += c_step )
                    {
                        double t0 = a[0]*b[0] + a[1]*b[b_step] + a[2]*b[b_step*2];
                        double t1 = a[0]*b[1] + a[1]*b[b_step+1] + a[2]*b[b_step*2+1];
                        double t2 = a[0]*b[2] + a[1]*b[b_step+2] + a[2]*b[b_step*2+2];
                        d[0] = t0*alpha + c[0]*beta;
                        d[1] = t1*alpha + c[1]*beta;
                        d[2] = t2*alpha + c[2]*beta;
                    }
                }
                else if( a != d )
                {
                    int c_step0 = 1;
                    if( c == zero )
                    {
                        c_step0 = 0;
                        c_step = 1;
                    }

                    for( i = 0; i < d_size.width; i++, d++, b++, c += c_step0 )
                    {
                        double t0 = a[0]*b[0] + a[1]*b[b_step] + a[2]*b[b_step*2];
                        double t1 = a[a_step]*b[0] + a[a_step+1]*b[b_step] + a[a_step+2]*b[b_step*2];
                        double t2 = a[a_step*2]*b[0] + a[a_step*2+1]*b[b_step] + a[a_step*2+2]*b[b_step*2];

                        d[0] = t0*alpha + c[0]*beta;
                        d[d_step] = t1*alpha + c[c_step]*beta;
                        d[d_step*2] = t2*alpha + c[c_step*2]*beta;
                    }
                }
                else
                    break;
                EXIT;
            case 4:
                if( len == d_size.width && b != d )
                {
                    for( i = 0; i < d_size.height; i++, d += d_step, a += a_step, c += c_step )
                    {
                        double t0 = a[0]*b[0] + a[1]*b[b_step] + a[2]*b[b_step*2] + a[3]*b[b_step*3];
                        double t1 = a[0]*b[1] + a[1]*b[b_step+1] + a[2]*b[b_step*2+1] + a[3]*b[b_step*3+1];
                        double t2 = a[0]*b[2] + a[1]*b[b_step+2] + a[2]*b[b_step*2+2] + a[3]*b[b_step*3+2];
                        double t3 = a[0]*b[3] + a[1]*b[b_step+3] + a[2]*b[b_step*2+3] + a[3]*b[b_step*3+3];
                        d[0] = t0*alpha + c[0]*beta;
                        d[1] = t1*alpha + c[1]*beta;
                        d[2] = t2*alpha + c[2]*beta;
                        d[3] = t3*alpha + c[3]*beta;
                    }
                }
                else if( d_size.width <= 16 && a != d )
                {
                    int c_step0 = 1;
                    if( c == zero )
                    {
                        c_step0 = 0;
                        c_step = 1;
                    }

                    for( i = 0; i < d_size.width; i++, d++, b++, c += c_step0 )
                    {
                        double t0 = a[0]*b[0] + a[1]*b[b_step] + a[2]*b[b_step*2] + a[3]*b[b_step*3];
                        double t1 = a[a_step]*b[0] + a[a_step+1]*b[b_step] +
                                    a[a_step+2]*b[b_step*2] + a[a_step+3]*b[b_step*3];
                        double t2 = a[a_step*2]*b[0] + a[a_step*2+1]*b[b_step] +
                                    a[a_step*2+2]*b[b_step*2] + a[a_step*2+3]*b[b_step*3];
                        double t3 = a[a_step*3]*b[0] + a[a_step*3+1]*b[b_step] +
                                    a[a_step*3+2]*b[b_step*2] + a[a_step*3+3]*b[b_step*3];
                        d[0] = t0*alpha + c[0]*beta;
                        d[d_step] = t1*alpha + c[c_step]*beta;
                        d[d_step*2] = t2*alpha + c[c_step*2]*beta;
                        d[d_step*3] = t3*alpha + c[c_step*3]*beta;
                    }
                }
                else
                    break;
                EXIT;
            }
        }

        if( type == CV_32F )
        {
            float* d = D->data.fl;
            const float *a = A->data.fl, *b = B->data.fl, *c = C->data.fl;
            size_t d_step = D->step/sizeof(d[0]),
                   a_step = A->step/sizeof(a[0]),
                   b_step = B->step/sizeof(b[0]),
                   c_step = C->step/sizeof(c[0]);

            if( !c )
                c = zerof;

            switch( len )
            {
            case 2:
                if( len == d_size.width && b != d )
                {
                    for( i = 0; i < d_size.height; i++, d += d_step, a += a_step, c += c_step )
                    {
                        float t0 = a[0]*b[0] + a[1]*b[b_step];
                        float t1 = a[0]*b[1] + a[1]*b[b_step+1];
                        d[0] = (float)(t0*alpha + c[0]*beta);
                        d[1] = (float)(t1*alpha + c[1]*beta);
                    }
                }
                else if( a != d )
                {
                    int c_step0 = 1;
                    if( c == zerof )
                    {
                        c_step0 = 0;
                        c_step = 1;
                    }

                    for( i = 0; i < d_size.width; i++, d++, b++, c += c_step0 )
                    {
                        float t0 = a[0]*b[0] + a[1]*b[b_step];
                        float t1 = a[a_step]*b[0] + a[a_step+1]*b[b_step];
                        d[0] = (float)(t0*alpha + c[0]*beta);
                        d[d_step] = (float)(t1*alpha + c[c_step]*beta);
                    }
                }
                else
                    break;
                EXIT;
            case 3:
                if( len == d_size.width && b != d )
                {
                    for( i = 0; i < d_size.height; i++, d += d_step, a += a_step, c += c_step )
                    {
                        float t0 = a[0]*b[0] + a[1]*b[b_step] + a[2]*b[b_step*2];
                        float t1 = a[0]*b[1] + a[1]*b[b_step+1] + a[2]*b[b_step*2+1];
                        float t2 = a[0]*b[2] + a[1]*b[b_step+2] + a[2]*b[b_step*2+2];
                        d[0] = (float)(t0*alpha + c[0]*beta);
                        d[1] = (float)(t1*alpha + c[1]*beta);
                        d[2] = (float)(t2*alpha + c[2]*beta);
                    }
                }
                else if( a != d )
                {
                    int c_step0 = 1;
                    if( c == zerof )
                    {
                        c_step0 = 0;
                        c_step = 1;
                    }

                    for( i = 0; i < d_size.width; i++, d++, b++, c += c_step0 )
                    {
                        float t0 = a[0]*b[0] + a[1]*b[b_step] + a[2]*b[b_step*2];
                        float t1 = a[a_step]*b[0] + a[a_step+1]*b[b_step] + a[a_step+2]*b[b_step*2];
                        float t2 = a[a_step*2]*b[0] + a[a_step*2+1]*b[b_step] + a[a_step*2+2]*b[b_step*2];

                        d[0] = (float)(t0*alpha + c[0]*beta);
                        d[d_step] = (float)(t1*alpha + c[c_step]*beta);
                        d[d_step*2] = (float)(t2*alpha + c[c_step*2]*beta);
                    }
                }
                else
                    break;
                EXIT;
            case 4:
                if( len == d_size.width && b != d )
                {
                    for( i = 0; i < d_size.height; i++, d += d_step, a += a_step, c += c_step )
                    {
                        float t0 = a[0]*b[0] + a[1]*b[b_step] + a[2]*b[b_step*2] + a[3]*b[b_step*3];
                        float t1 = a[0]*b[1] + a[1]*b[b_step+1] + a[2]*b[b_step*2+1] + a[3]*b[b_step*3+1];
                        float t2 = a[0]*b[2] + a[1]*b[b_step+2] + a[2]*b[b_step*2+2] + a[3]*b[b_step*3+2];
                        float t3 = a[0]*b[3] + a[1]*b[b_step+3] + a[2]*b[b_step*2+3] + a[3]*b[b_step*3+3];
                        d[0] = (float)(t0*alpha + c[0]*beta);
                        d[1] = (float)(t1*alpha + c[1]*beta);
                        d[2] = (float)(t2*alpha + c[2]*beta);
                        d[3] = (float)(t3*alpha + c[3]*beta);
                    }
                }
                else if( len <= 16 && a != d )
                {
                    int c_step0 = 1;
                    if( c == zerof )
                    {
                        c_step0 = 0;
                        c_step = 1;
                    }

                    for( i = 0; i < d_size.width; i++, d++, b++, c += c_step0 )
                    {
                        float t0 = a[0]*b[0] + a[1]*b[b_step] + a[2]*b[b_step*2] + a[3]*b[b_step*3];
                        float t1 = a[a_step]*b[0] + a[a_step+1]*b[b_step] +
                                   a[a_step+2]*b[b_step*2] + a[a_step+3]*b[b_step*3];
                        float t2 = a[a_step*2]*b[0] + a[a_step*2+1]*b[b_step] +
                                   a[a_step*2+2]*b[b_step*2] + a[a_step*2+3]*b[b_step*3];
                        float t3 = a[a_step*3]*b[0] + a[a_step*3+1]*b[b_step] +
                                   a[a_step*3+2]*b[b_step*2] + a[a_step*3+3]*b[b_step*3];
                        d[0] = (float)(t0*alpha + c[0]*beta);
                        d[d_step] = (float)(t1*alpha + c[c_step]*beta);
                        d[d_step*2] = (float)(t2*alpha + c[c_step*2]*beta);
                        d[d_step*3] = (float)(t3*alpha + c[c_step*3]*beta);
                    }
                }
                else
                    break;
                EXIT;
            }
        }
    }
 
    {
        int b_step = B->step;
        CvGEMMSingleMulFunc single_mul_func;
        CvMat tmat, *D0 = D;
        icvBLAS_GEMM_32f_t blas_func = 0;

        if( !inittab )
        {
            icvInitGEMMTable( &single_mul_tab, &block_mul_tab, &store_tab );
            inittab = 1;
        }

        single_mul_func = (CvGEMMSingleMulFunc)single_mul_tab.fn_2d[type];
        if( !single_mul_func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        if( D->data.ptr == A->data.ptr || D->data.ptr == B->data.ptr )
        {
            int buf_size = d_size.width*d_size.height*CV_ELEM_SIZE(type);
            if( d_size.width <= CV_MAX_LOCAL_MAT_SIZE )
            {
                buffer = (uchar*)cvStackAlloc( buf_size );
                local_alloc = 1;
            }
            else
                CV_CALL( buffer = (uchar*)cvAlloc( buf_size ));

            tmat = cvMat( d_size.height, d_size.width, type, buffer );
            D = &tmat;
        }

        if( (d_size.width == 1 || len == 1) && !(flags & CV_GEMM_B_T) && CV_IS_MAT_CONT(B->type) )
        {
            b_step = d_size.width == 1 ? 0 : CV_ELEM_SIZE(type);
            flags |= CV_GEMM_B_T;
        }

        if( (d_size.width | d_size.height | len) >= 16 && icvBLAS_GEMM_32f_p != 0 )
        {
            blas_func = type == CV_32FC1 ? (icvBLAS_GEMM_32f_t)icvBLAS_GEMM_32f_p :
                        type == CV_64FC1 ? (icvBLAS_GEMM_32f_t)icvBLAS_GEMM_64f_p :
                        type == CV_32FC2 ? (icvBLAS_GEMM_32f_t)icvBLAS_GEMM_32fc_p :
                        type == CV_64FC2 ? (icvBLAS_GEMM_32f_t)icvBLAS_GEMM_64fc_p : 0;
        }

        if( blas_func )
        {
            const char* transa = flags & CV_GEMM_A_T ? "t" : "n";
            const char* transb = flags & CV_GEMM_B_T ? "t" : "n";
            int lda, ldb, ldd;
            
            if( C->data.ptr )
            {
                if( C->data.ptr != D->data.ptr )
                {
                    if( !(flags & CV_GEMM_C_T) )
                        cvCopy( C, D,NULL );
                    else
                        cvTranspose( C, D );
                }
            }

            if( CV_MAT_DEPTH(type) == CV_32F )
            {
                CvComplex32f _alpha, _beta;
                
                lda = A->step/sizeof(float);
                ldb = b_step/sizeof(float);
                ldd = D->step/sizeof(float);
                _alpha.re = (float)alpha;
                _alpha.im = 0;
                _beta.re = C->data.ptr ? (float)beta : 0;
                _beta.im = 0;
                if( CV_MAT_CN(type) == 2 )
                    lda /= 2, ldb /= 2, ldd /= 2;

                blas_func( transb, transa, &d_size.width, &d_size.height, &len,
                       &_alpha, B->data.ptr, &ldb, A->data.ptr, &lda,
                       &_beta, D->data.ptr, &ldd );
            }
            else
            {
                CvComplex64f _alpha, _beta;
                
                lda = A->step/sizeof(double);
                ldb = b_step/sizeof(double);
                ldd = D->step/sizeof(double);
                _alpha.re = alpha;
                _alpha.im = 0;
                _beta.re = C->data.ptr ? beta : 0;
                _beta.im = 0;
                if( CV_MAT_CN(type) == 2 )
                    lda /= 2, ldb /= 2, ldd /= 2;

                blas_func( transb, transa, &d_size.width, &d_size.height, &len,
                       &_alpha, B->data.ptr, &ldb, A->data.ptr, &lda,
                       &_beta, D->data.ptr, &ldd );
            }
        }
        else if( d_size.height <= block_lin_size/2 || d_size.width <= block_lin_size/2 || len <= 10 ||
            d_size.width <= block_lin_size && d_size.height <= block_lin_size && len <= block_lin_size )
        {
            single_mul_func( A->data.ptr, A->step, B->data.ptr, b_step,
                             C->data.ptr, C->step, D->data.ptr, D->step,
                             a_size, d_size, alpha, beta, flags );
        }
        else
        {
            int is_a_t = flags & CV_GEMM_A_T;
            int is_b_t = flags & CV_GEMM_B_T;
            int elem_size = CV_ELEM_SIZE(type);
            int dk0_1, dk0_2;
            int a_buf_size = 0, b_buf_size, d_buf_size;
            uchar* a_buf = 0;
            uchar* b_buf = 0;
            uchar* d_buf = 0;
            int i, j, k, di = 0, dj = 0, dk = 0;
            int dm0, dn0, dk0;
            int a_step0, a_step1, b_step0, b_step1, c_step0, c_step1;
            int work_elem_size = elem_size << (CV_MAT_DEPTH(type) == CV_32F ? 1 : 0);
            CvGEMMBlockMulFunc block_mul_func = (CvGEMMBlockMulFunc)block_mul_tab.fn_2d[type];
            CvGEMMStoreFunc store_func = (CvGEMMStoreFunc)store_tab.fn_2d[type];

            assert( block_mul_func && store_func );

            if( !is_a_t )
                a_step0 = A->step, a_step1 = elem_size;
            else
                a_step0 = elem_size, a_step1 = A->step;

            if( !is_b_t )
                b_step0 = b_step, b_step1 = elem_size;
            else
                b_step0 = elem_size, b_step1 = b_step;

            if( !C->data.ptr )
            {
                c_step0 = c_step1 = 0;
                flags &= ~CV_GEMM_C_T;
            }
            else if( !(flags & CV_GEMM_C_T) )
                c_step0 = C->step, c_step1 = elem_size;
            else
                c_step0 = elem_size, c_step1 = C->step;

            dm0 = MIN( block_lin_size, d_size.height );
            dn0 = MIN( block_lin_size, d_size.width );
            dk0_1 = block_size / dm0;
            dk0_2 = block_size / dn0;
            dk0 = MAX( dk0_1, dk0_2 );
            dk0 = MIN( dk0, len );
            if( dk0*dm0 > block_size )
                dm0 = block_size / dk0;
            if( dk0*dn0 > block_size )
                dn0 = block_size / dk0;

            dk0_1 = (dn0+dn0/8+2) & -2;
            b_buf_size = (dk0+dk0/8+1)*dk0_1*elem_size;
            d_buf_size = (dk0+dk0/8+1)*dk0_1*work_elem_size;
        
            if( is_a_t )
            {
                a_buf_size = (dm0+dm0/8+1)*((dk0+dk0/8+2)&-2)*elem_size;
                flags &= ~CV_GEMM_A_T;
            }

            CV_CALL( block_buffer = (uchar*)cvAlloc(a_buf_size + b_buf_size + d_buf_size));
            d_buf = block_buffer;
            b_buf = d_buf + d_buf_size;

            if( is_a_t )
                a_buf = b_buf + b_buf_size;

            for( i = 0; i < d_size.height; i += di )
            {
                di = dm0;
                if( i + di >= d_size.height || 8*(i + di) + di > 8*d_size.height )
                    di = d_size.height - i;

                for( j = 0; j < d_size.width; j += dj )
                {
                    uchar* _d = D->data.ptr + i*D->step + j*elem_size;
                    const uchar* _c = C->data.ptr + i*c_step0 + j*c_step1;
                    int _d_step = D->step;
                    dj = dn0;

                    if( j + dj >= d_size.width || 8*(j + dj) + dj > 8*d_size.width )
                        dj = d_size.width - j;

                    flags &= 15;
                    if( dk0 < len )
                    {
                        _d = d_buf;
                        _d_step = dj*work_elem_size;
                    }

                    for( k = 0; k < len; k += dk )
                    {
                        const uchar* _a = A->data.ptr + i*a_step0 + k*a_step1;
                        int _a_step = A->step;
                        const uchar* _b = B->data.ptr + k*b_step0 + j*b_step1;
                        int _b_step = b_step;
                        CvSize a_bl_size;

                        dk = dk0;
                        if( k + dk >= len || 8*(k + dk) + dk > 8*len )
                            dk = len - k;

                        if( !is_a_t )
                            a_bl_size.width = dk, a_bl_size.height = di;
                        else
                            a_bl_size.width = di, a_bl_size.height = dk;

                        if( a_buf && is_a_t )
                        {
                            int t;
                            _a_step = dk*elem_size;
                            icvGEMM_TransposeBlock( _a, A->step, a_buf, _a_step, a_bl_size, elem_size );
                            CV_SWAP( a_bl_size.width, a_bl_size.height, t );
                            _a = a_buf;
                        }
                
                        if( dj < d_size.width )
                        {
                            CvSize b_size;
                            if( !is_b_t )
                                b_size.width = dj, b_size.height = dk;
                            else
                                b_size.width = dk, b_size.height = dj;

                            _b_step = b_size.width*elem_size;
                            icvGEMM_CopyBlock( _b, b_step, b_buf, _b_step, b_size, elem_size );
                            _b = b_buf;
                        }

                        if( dk0 < len )
                            block_mul_func( _a, _a_step, _b, _b_step, _d, _d_step,
                                            a_bl_size, cvSize(dj,di), flags );
                        else
                            single_mul_func( _a, _a_step, _b, _b_step, _c, C->step, _d, _d_step,
                                             a_bl_size, cvSize(dj,di), alpha, beta, flags );
                        flags |= 16;
                    }

                    if( dk0 < len )
                        store_func( _c, C->step, _d, _d_step, D->data.ptr + i*D->step + j*elem_size,
                                    D->step, cvSize(dj,di), alpha, beta, flags );
                }
            }
        }

        if( D0 != D )
            CV_CALL( cvCopy( D, D0,NULL));
    }

    __END__;

    if( buffer && !local_alloc )
        cvFree( &buffer );
    if( block_buffer )
        cvFree( &block_buffer );
}

CvRect cvRect( int x, int y, int width, int height )
{
    CvRect r;

    r.x = x;
    r.y = y;
    r.width = width;
    r.height = height;

    return r;
}

  // Functions for manipulating memory storage - list of memory blocks           *
//****************************************************************************************/

/* initializes allocated storage */
static void icvInitMemStorage( CvMemStorage* storage, int block_size )
{
    CV_FUNCNAME( "icvInitMemStorage " );
    
    __BEGIN__;

    if( !storage )
        CV_ERROR( CV_StsNullPtr, "" );

    if( block_size <= 0 )
        block_size = CV_STORAGE_BLOCK_SIZE;

    block_size = cvAlign( block_size, CV_STRUCT_ALIGN );
    assert( sizeof(CvMemBlock) % CV_STRUCT_ALIGN == 0 );

    memset( storage, 0, sizeof( *storage ));
    storage->signature = CV_STORAGE_MAGIC_VAL;
    storage->block_size = block_size;

    __END__;
}


/* creates root memory storage */
CvMemStorage* cvCreateMemStorage( int block_size )
{
    CvMemStorage *storage = 0;

    CV_FUNCNAME( "cvCreateMemStorage" );

    __BEGIN__;

    CV_CALL( storage = (CvMemStorage *)cvAlloc( sizeof( CvMemStorage )));
    CV_CALL( icvInitMemStorage( storage, block_size ));

    __END__;

    if( cvGetErrStatus() < 0 )
        cvFree( &storage );

    return storage;
}


/* creates child memory storage */
CvMemStorage * cvCreateChildMemStorage( CvMemStorage * parent )
{
    CvMemStorage *storage = 0;
    CV_FUNCNAME( "cvCreateChildMemStorage" );

    __BEGIN__;

    if( !parent )
        CV_ERROR( CV_StsNullPtr, "" );

    CV_CALL( storage = cvCreateMemStorage(parent->block_size));
    storage->parent = parent;

    __END__;

    if( cvGetErrStatus() < 0 )
        cvFree( &storage );

    return storage;
}


/* releases all blocks of the storage (or returns them to parent if any) */
static void icvDestroyMemStorage( CvMemStorage* storage )
{
    CV_FUNCNAME( "icvDestroyMemStorage" );

    __BEGIN__;

    int k = 0;

    CvMemBlock *block;
    CvMemBlock *dst_top = 0;

    if( !storage )
        CV_ERROR( CV_StsNullPtr, "" );

    if( storage->parent )
        dst_top = storage->parent->top;

    for( block = storage->bottom; block != 0; k++ )
    {
        CvMemBlock *temp = block;

        block = block->next;
        if( storage->parent )
        {
            if( dst_top )
            {
                temp->prev = dst_top;
                temp->next = dst_top->next;
                if( temp->next )
                    temp->next->prev = temp;
                dst_top = dst_top->next = temp;
            }
            else
            {
                dst_top = storage->parent->bottom = storage->parent->top = temp;
                temp->prev = temp->next = 0;
                storage->free_space = storage->block_size - sizeof( *temp );
            }
        }
        else
        {
            cvFree( &temp );
        }
    }

    storage->top = storage->bottom = 0;
    storage->free_space = 0;

    __END__;
}


/* releases memory storage */
void cvReleaseMemStorage( CvMemStorage** storage )
{
    CvMemStorage *st;
    CV_FUNCNAME( "cvReleaseMemStorage" );

    __BEGIN__;

    if( !storage )
        CV_ERROR( CV_StsNullPtr, "" );

    st = *storage;
    *storage = 0;

    if( st )
    {
        CV_CALL( icvDestroyMemStorage( st ));
        cvFree( &st );
    }

    __END__;
}


/* clears memory storage (returns blocks to the parent if any) */
void cvClearMemStorage( CvMemStorage * storage )
{
    CV_FUNCNAME( "cvClearMemStorage" );

    __BEGIN__;

    if( !storage )
        CV_ERROR( CV_StsNullPtr, "" );

    if( storage->parent )
    {
        icvDestroyMemStorage( storage );
    }
    else
    {
        storage->top = storage->bottom;
        storage->free_space = storage->bottom ? storage->block_size - sizeof(CvMemBlock) : 0;
    }

    __END__;
}


/* moves stack pointer to next block.
   If no blocks, allocate new one and link it to the storage */
static void icvGoNextMemBlock( CvMemStorage * storage )
{
    CV_FUNCNAME( "icvGoNextMemBlock" );
    
    __BEGIN__;
    
    if( !storage )
        CV_ERROR( CV_StsNullPtr, "" );

    if( !storage->top || !storage->top->next )
    {
        CvMemBlock *block;

        if( !(storage->parent) )
        {
            CV_CALL( block = (CvMemBlock *)cvAlloc( storage->block_size ));
        }
        else
        {
            CvMemStorage *parent = storage->parent;
            CvMemStoragePos parent_pos;

            cvSaveMemStoragePos( parent, &parent_pos );
            CV_CALL( icvGoNextMemBlock( parent ));

            block = parent->top;
            cvRestoreMemStoragePos( parent, &parent_pos );

            if( block == parent->top )  /* the single allocated block */
            {
                assert( parent->bottom == block );
                parent->top = parent->bottom = 0;
                parent->free_space = 0;
            }
            else
            {
                /* cut the block from the parent's list of blocks */
                parent->top->next = block->next;
                if( block->next )
                    block->next->prev = parent->top;
            }
        }

        /* link block */
        block->next = 0;
        block->prev = storage->top;

        if( storage->top )
            storage->top->next = block;
        else
            storage->top = storage->bottom = block;
    }

    if( storage->top->next )
        storage->top = storage->top->next;
    storage->free_space = storage->block_size - sizeof(CvMemBlock);
    assert( storage->free_space % CV_STRUCT_ALIGN == 0 );

    __END__;
}


/* remembers memory storage position */
void cvSaveMemStoragePos( const CvMemStorage * storage, CvMemStoragePos * pos )
{
    CV_FUNCNAME( "cvSaveMemStoragePos" );

    __BEGIN__;

    if( !storage || !pos )
        CV_ERROR( CV_StsNullPtr, "" );

    pos->top = storage->top;
    pos->free_space = storage->free_space;

    __END__;
}


/* restores memory storage position */
void cvRestoreMemStoragePos( CvMemStorage * storage, CvMemStoragePos * pos )
{
    CV_FUNCNAME( "cvRestoreMemStoragePos" );

    __BEGIN__;

    if( !storage || !pos )
        CV_ERROR( CV_StsNullPtr, "" );
    if( pos->free_space > storage->block_size )
        CV_ERROR( CV_StsBadSize, "" );

 
    storage->top = pos->top;
    storage->free_space = pos->free_space;

    if( !storage->top )
    {
        storage->top = storage->bottom;
        storage->free_space = storage->top ? storage->block_size - sizeof(CvMemBlock) : 0;
    }

    __END__;
}

int cvAlignLeft( int size, int align )
{
    return size & -align;
}

/* Allocates continuous buffer of the specified size in the storage */
void* cvMemStorageAlloc( CvMemStorage* storage, size_t size )
{
    char *ptr = 0;
    
    CV_FUNCNAME( "cvMemStorageAlloc" );

    __BEGIN__;

    if( !storage )
        CV_ERROR( CV_StsNullPtr, "NULL storage pointer" );

    if( size > INT_MAX )
        CV_ERROR( CV_StsOutOfRange, "Too large memory block is requested" );

    assert( storage->free_space % CV_STRUCT_ALIGN == 0 );

    if( (size_t)storage->free_space < size )
    {
        size_t max_free_space = cvAlignLeft(storage->block_size - sizeof(CvMemBlock), CV_STRUCT_ALIGN);
        if( max_free_space < size )
            CV_ERROR( CV_StsOutOfRange, "requested size is negative or too big" );

        CV_CALL( icvGoNextMemBlock( storage ));
    }

    ptr = ICV_FREE_PTR(storage);
    assert( (size_t)ptr % CV_STRUCT_ALIGN == 0 );
    storage->free_space = cvAlignLeft(storage->free_space - (int)size, CV_STRUCT_ALIGN );

    __END__;

    return ptr;
}

inline char* icvMed3( char* a, char* b, char* c, CvCmpFunc cmp_func, void* aux )
{
    return cmp_func(a, b, aux) < 0 ?
      (cmp_func(b, c, aux) < 0 ? b : cmp_func(a, c, aux) < 0 ? c : a)
     :(cmp_func(b, c, aux) > 0 ? b : cmp_func(a, c, aux) < 0 ? a : c);
}

void cvSeqSort( CvSeq* seq, CvCmpFunc cmp_func, void* aux )
{         
    int elem_size;
    int isort_thresh = 7;
    CvSeqReader left, right;
    int sp = 0;

    struct
    {
        CvSeqReaderPos lb;
        CvSeqReaderPos ub;
    }
    stack[48];

    CV_FUNCNAME( "cvSeqSort" );

    __BEGIN__;
    
    if( !CV_IS_SEQ(seq) )
        CV_ERROR( !seq ? CV_StsNullPtr : CV_StsBadArg, "Bad input sequence" );

    if( !cmp_func )
        CV_ERROR( CV_StsNullPtr, "Null compare function" );

    if( seq->total <= 1 )
        EXIT;

    elem_size = seq->elem_size;
    isort_thresh *= elem_size;

    cvStartReadSeq( seq, &left, 0 );
    right = left;
    CV_SAVE_READER_POS( left, stack[0].lb );
    CV_PREV_SEQ_ELEM( elem_size, right );
    CV_SAVE_READER_POS( right, stack[0].ub );

    while( sp >= 0 )
    {
        CV_RESTORE_READER_POS( left, stack[sp].lb );
        CV_RESTORE_READER_POS( right, stack[sp].ub );
        sp--;

        for(;;)
        {
            int i, n, m;
            CvSeqReader ptr, ptr2;

            if( left.block == right.block )
                n = (int)(right.ptr - left.ptr) + elem_size;
            else
            {
                n = cvGetSeqReaderPos( &right );
                n = (n - cvGetSeqReaderPos( &left ) + 1)*elem_size;
            }

            if( n <= isort_thresh )
            {
            insert_sort:
                ptr = ptr2 = left;
                CV_NEXT_SEQ_ELEM( elem_size, ptr );
                CV_NEXT_SEQ_ELEM( elem_size, right );
                while( ptr.ptr != right.ptr )
                {
                    ptr2.ptr = ptr.ptr;
                    if( ptr2.block != ptr.block )
                    {
                        ptr2.block = ptr.block;
                        ptr2.block_min = ptr.block_min;
                        ptr2.block_max = ptr.block_max;
                    }
                    while( ptr2.ptr != left.ptr )
                    {
                        char* cur = ptr2.ptr;
                        CV_PREV_SEQ_ELEM( elem_size, ptr2 );
                        if( cmp_func( ptr2.ptr, cur, aux ) <= 0 )
                            break;
                        CV_SWAP_ELEMS( ptr2.ptr, cur, elem_size );
                    }
                    CV_NEXT_SEQ_ELEM( elem_size, ptr );
                }
                break;
            }
            else
            {
                CvSeqReader left0, left1, right0, right1;
                CvSeqReader tmp0, tmp1;
                char *m1, *m2, *m3, *pivot;
                int swap_cnt = 0;
                int l, l0, l1, r, r0, r1;

                left0 = tmp0 = left;
                right0 = right1 = right;
                n /= elem_size;
                
                if( n > 40 )
                {
                    int d = n / 8;
                    char *p1, *p2, *p3;
                    p1 = tmp0.ptr;
                    cvSetSeqReaderPos( &tmp0, d, 1 );
                    p2 = tmp0.ptr;
                    cvSetSeqReaderPos( &tmp0, d, 1 );
                    p3 = tmp0.ptr;
                    m1 = icvMed3( p1, p2, p3, cmp_func, aux );
                    cvSetSeqReaderPos( &tmp0, (n/2) - d*3, 1 );
                    p1 = tmp0.ptr;
                    cvSetSeqReaderPos( &tmp0, d, 1 );
                    p2 = tmp0.ptr;
                    cvSetSeqReaderPos( &tmp0, d, 1 );
                    p3 = tmp0.ptr;
                    m2 = icvMed3( p1, p2, p3, cmp_func, aux );
                    cvSetSeqReaderPos( &tmp0, n - 1 - d*3 - n/2, 1 );
                    p1 = tmp0.ptr;
                    cvSetSeqReaderPos( &tmp0, d, 1 );
                    p2 = tmp0.ptr;
                    cvSetSeqReaderPos( &tmp0, d, 1 );
                    p3 = tmp0.ptr;
                    m3 = icvMed3( p1, p2, p3, cmp_func, aux );
                }
                else
                {
                    m1 = tmp0.ptr;
                    cvSetSeqReaderPos( &tmp0, n/2, 1 );
                    m2 = tmp0.ptr;
                    cvSetSeqReaderPos( &tmp0, n - 1 - n/2, 1 );
                    m3 = tmp0.ptr;
                }

                pivot = icvMed3( m1, m2, m3, cmp_func, aux );
                left = left0;
                if( pivot != left.ptr )
                {
                    CV_SWAP_ELEMS( pivot, left.ptr, elem_size );
                    pivot = left.ptr;
                }
                CV_NEXT_SEQ_ELEM( elem_size, left );
                left1 = left;

                for(;;)
                {
                    while( left.ptr != right.ptr && (r = cmp_func(left.ptr, pivot, aux)) <= 0 )
                    {
                        if( r == 0 )
                        {
                            if( left1.ptr != left.ptr )
                                CV_SWAP_ELEMS( left1.ptr, left.ptr, elem_size );
                            swap_cnt = 1;
                            CV_NEXT_SEQ_ELEM( elem_size, left1 );
                        }
                        CV_NEXT_SEQ_ELEM( elem_size, left );
                    }

                    while( left.ptr != right.ptr && (r = cmp_func(right.ptr,pivot, aux)) >= 0 )
                    {
                        if( r == 0 )
                        {
                            if( right1.ptr != right.ptr )
                                CV_SWAP_ELEMS( right1.ptr, right.ptr, elem_size );
                            swap_cnt = 1;
                            CV_PREV_SEQ_ELEM( elem_size, right1 );
                        }
                        CV_PREV_SEQ_ELEM( elem_size, right );
                    }

                    if( left.ptr == right.ptr )
                    {
                        r = cmp_func(left.ptr, pivot, aux);
                        if( r == 0 )
                        {
                            if( left1.ptr != left.ptr )
                                CV_SWAP_ELEMS( left1.ptr, left.ptr, elem_size );
                            swap_cnt = 1;
                            CV_NEXT_SEQ_ELEM( elem_size, left1 );
                        }
                        if( r <= 0 )
                        {
                            CV_NEXT_SEQ_ELEM( elem_size, left );
                        }
                        else
                        {
                            CV_PREV_SEQ_ELEM( elem_size, right );
                        }
                        break;
                    }

                    CV_SWAP_ELEMS( left.ptr, right.ptr, elem_size );
                    CV_NEXT_SEQ_ELEM( elem_size, left );
                    r = left.ptr == right.ptr;
                    CV_PREV_SEQ_ELEM( elem_size, right );
                    swap_cnt = 1;
                    if( r )
                        break;
                }

                if( swap_cnt == 0 )
                {
                    left = left0, right = right0;
                    goto insert_sort;
                }

                l = cvGetSeqReaderPos( &left );
                if( l == 0 )
                    l = seq->total;
                l0 = cvGetSeqReaderPos( &left0 );
                l1 = cvGetSeqReaderPos( &left1 );
                if( l1 == 0 )
                    l1 = seq->total;
                
                n = MIN( l - l1, l1 - l0 );
                if( n > 0 )
                {
                    tmp0 = left0;
                    tmp1 = left;
                    cvSetSeqReaderPos( &tmp1, 0-n, 1 );
                    for( i = 0; i < n; i++ )
                    {
                        CV_SWAP_ELEMS( tmp0.ptr, tmp1.ptr, elem_size );
                        CV_NEXT_SEQ_ELEM( elem_size, tmp0 );
                        CV_NEXT_SEQ_ELEM( elem_size, tmp1 );
                    }
                }

                r = cvGetSeqReaderPos( &right );
                r0 = cvGetSeqReaderPos( &right0 );
                r1 = cvGetSeqReaderPos( &right1 );
                m = MIN( r0 - r1, r1 - r );
                if( m > 0 )
                {
                    tmp0 = left;
                    tmp1 = right0;
                    cvSetSeqReaderPos( &tmp1, 1-m, 1 );
                    for( i = 0; i < m; i++ )
                    {
                        CV_SWAP_ELEMS( tmp0.ptr, tmp1.ptr, elem_size );
                        CV_NEXT_SEQ_ELEM( elem_size, tmp0 );
                        CV_NEXT_SEQ_ELEM( elem_size, tmp1 );
                    }
                }

                n = l - l1;
                m = r1 - r;
                if( n > 1 )
                {
                    if( m > 1 )
                    {
                        if( n > m )
                        {
                            sp++;
                            CV_SAVE_READER_POS( left0, stack[sp].lb );
                            cvSetSeqReaderPos( &left0, n - 1, 1 );
                            CV_SAVE_READER_POS( left0, stack[sp].ub );
                            left = right = right0;
                            cvSetSeqReaderPos( &left, 1 - m, 1 );
                        }
                        else
                        {
                            sp++;
                            CV_SAVE_READER_POS( right0, stack[sp].ub );
                            cvSetSeqReaderPos( &right0, 1 - m, 1 );
                            CV_SAVE_READER_POS( right0, stack[sp].lb );
                            left = right = left0;
                            cvSetSeqReaderPos( &right, n - 1, 1 );
                        }
                    }
                    else
                    {
                        left = right = left0;
                        cvSetSeqReaderPos( &right, n - 1, 1 );
                    }
                }
                else if( m > 1 )
                {
                    left = right = right0;
                    cvSetSeqReaderPos( &left, 1 - m, 1 );
                }
                else
                    break;
            }
        }
    }

    __END__;
}

/****************************************************************************************\
*                               Sequence implementation                                  *
\****************************************************************************************/

/* creates empty sequence */
CvSeq * cvCreateSeq( int seq_flags, int header_size, int elem_size, CvMemStorage * storage )
{
    CvSeq *seq = 0;

    CV_FUNCNAME( "cvCreateSeq" );

    __BEGIN__;

    if( !storage )
        CV_ERROR( CV_StsNullPtr, "" );
    if( header_size < (int)sizeof( CvSeq ) || elem_size <= 0 )
        CV_ERROR( CV_StsBadSize, "" );

    /* allocate sequence header */
    CV_CALL( seq = (CvSeq*)cvMemStorageAlloc( storage, header_size ));
    memset( seq, 0, header_size );

    seq->header_size = header_size;
    seq->flags = (seq_flags & ~CV_MAGIC_MASK) | CV_SEQ_MAGIC_VAL;
    {
        int elemtype = CV_MAT_TYPE(seq_flags);
        int typesize = CV_ELEM_SIZE(elemtype);

        if( elemtype != CV_SEQ_ELTYPE_GENERIC &&
            typesize != 0 && typesize != elem_size )
            CV_ERROR( CV_StsBadSize,
            "Specified element size doesn't match to the size of the specified element type "
            "(try to use 0 for element type)" );
    }
    seq->elem_size = elem_size;
    seq->storage = storage;

    CV_CALL( cvSetSeqBlockSize( seq, (1 << 10)/elem_size ));

    __END__;

    return seq;
}


/* adjusts <delta_elems> field of sequence. It determines how much the sequence
   grows if there are no free space inside the sequence buffers */
void cvSetSeqBlockSize( CvSeq *seq, int delta_elements )
{
    int elem_size;
    int useful_block_size;

    CV_FUNCNAME( "cvSetSeqBlockSize" );

    __BEGIN__;

    if( !seq || !seq->storage )
        CV_ERROR( CV_StsNullPtr, "" );
    if( delta_elements < 0 )
        CV_ERROR( CV_StsOutOfRange, "" );

    useful_block_size = cvAlignLeft(seq->storage->block_size - sizeof(CvMemBlock) -
                                    sizeof(CvSeqBlock), CV_STRUCT_ALIGN);
    elem_size = seq->elem_size;

    if( delta_elements == 0 )
    {
        delta_elements = (1 << 10) / elem_size;
        delta_elements = MAX( delta_elements, 1 );
    }
    if( delta_elements * elem_size > useful_block_size )
    {
        delta_elements = useful_block_size / elem_size;
        if( delta_elements == 0 )
            CV_ERROR( CV_StsOutOfRange, "Storage block size is too small "
                                        "to fit the sequence elements" );
    }

    seq->delta_elems = delta_elements;

    __END__;
}


/* finds sequence element by its index */
char* cvGetSeqElem( const CvSeq *seq, int index )
{
    CvSeqBlock *block;
    int count, total = seq->total;

    if( (unsigned)index >= (unsigned)total )
    {
        index += index < 0 ? total : 0;
        index -= index >= total ? total : 0;
        if( (unsigned)index >= (unsigned)total )
            return 0;
    }

    block = seq->first;
    if( index + index <= total )
    {
        while( index >= (count = block->count) )
        {
            block = block->next;
            index -= count;
        }
    }
    else
    {
        do
        {
            block = block->prev;
            total -= block->count;
        }
        while( index < total );
        index -= total;
    }

    return block->data + index * seq->elem_size;
}


/* calculates index of sequence element */
int cvSeqElemIdx( const CvSeq* seq, const void* _element, CvSeqBlock** _block )
{
    const char *element = (const char *)_element;
    int elem_size;
    int id = -1;
    CvSeqBlock *first_block;
    CvSeqBlock *block;

    CV_FUNCNAME( "cvSeqElemIdx" );

    __BEGIN__;

    if( !seq || !element )
        CV_ERROR( CV_StsNullPtr, "" );

    block = first_block = seq->first;
    elem_size = seq->elem_size;

    for( ;; )
    {
        if( (unsigned)(element - block->data) < (unsigned) (block->count * elem_size) )
        {
            if( _block )
                *_block = block;
            if( elem_size <= ICV_SHIFT_TAB_MAX && (id = icvPower2ShiftTab[elem_size - 1]) >= 0 )
                id = (int)((size_t)(element - block->data) >> id);
            else
                id = (int)((size_t)(element - block->data) / elem_size);
            id += block->start_index - seq->first->start_index;
            break;
        }
        block = block->next;
        if( block == first_block )
            break;
    }

    __END__;

    return id;
}


int cvSliceLength( CvSlice slice, const CvSeq* seq )
{
    int total = seq->total;
    int length = slice.end_index - slice.start_index;
    
    if( length != 0 )
    {
        if( slice.start_index < 0 )
            slice.start_index += total;
        if( slice.end_index <= 0 )
            slice.end_index += total;

        length = slice.end_index - slice.start_index;
    }

    if( length < 0 )
    {
        length += total;
        /*if( length < 0 )
            length += total;*/
    }
    else if( length > total )
        length = total;

    return length;
}


/* copies all the sequence elements into single continuous array */
void* cvCvtSeqToArray( const CvSeq *seq, void *array, CvSlice slice )
{
    CV_FUNCNAME( "cvCvtSeqToArray" );

    __BEGIN__;

    int elem_size, total;
    CvSeqReader reader;
    char *dst = (char*)array;

    if( !seq || !array )
        CV_ERROR( CV_StsNullPtr, "" );

    elem_size = seq->elem_size;
    total = cvSliceLength( slice, seq )*elem_size;
    
    if( total == 0 )
        EXIT;
    
    cvStartReadSeq( seq, &reader, 0 );
    CV_CALL( cvSetSeqReaderPos( &reader, slice.start_index, 0 ));

    do
    {
        int count = (int)(reader.block_max - reader.ptr);
        if( count > total )
            count = total;

        memcpy( dst, reader.ptr, count );
        dst += count;
        reader.block = reader.block->next;
        reader.ptr = reader.block->data;
        reader.block_max = reader.ptr + reader.block->count*elem_size;
        total -= count;
    }
    while( total > 0 );

    __END__;

    return array;
}


/* constructs sequence from array without copying any data.
   the resultant sequence can't grow above its initial size */
CvSeq* cvMakeSeqHeaderForArray( int seq_flags, int header_size, int elem_size,void *array, int total, CvSeq *seq, CvSeqBlock * block )
{
    CvSeq* result = 0;
    
    CV_FUNCNAME( "cvMakeSeqHeaderForArray" );

    __BEGIN__;

    if( elem_size <= 0 || header_size < (int)sizeof( CvSeq ) || total < 0 )
        CV_ERROR( CV_StsBadSize, "" );

    if( !seq || ((!array || !block) && total > 0) )
        CV_ERROR( CV_StsNullPtr, "" );

    memset( seq, 0, header_size );

    seq->header_size = header_size;
    seq->flags = (seq_flags & ~CV_MAGIC_MASK) | CV_SEQ_MAGIC_VAL;
    {
        int elemtype = CV_MAT_TYPE(seq_flags);
        int typesize = CV_ELEM_SIZE(elemtype);

        if( elemtype != CV_SEQ_ELTYPE_GENERIC &&
            typesize != 0 && typesize != elem_size )
            CV_ERROR( CV_StsBadSize,
            "Element size doesn't match to the size of predefined element type "
            "(try to use 0 for sequence element type)" );
    }
    seq->elem_size = elem_size;
    seq->total = total;
    seq->block_max = seq->ptr = (char *) array + total * elem_size;

    if( total > 0 )
    {
        seq->first = block;
        block->prev = block->next = block;
        block->start_index = 0;
        block->count = total;
        block->data = (char *) array;
    }

    result = seq;

    __END__;

    return result;
}


/* the function allocates space for at least one more sequence element.
   if there are free sequence blocks (seq->free_blocks != 0),
   they are reused, otherwise the space is allocated in the storage */
static void icvGrowSeq( CvSeq *seq, int in_front_of )
{
    CV_FUNCNAME( "icvGrowSeq" );

    __BEGIN__;

    CvSeqBlock *block;

    if( !seq )
        CV_ERROR( CV_StsNullPtr, "" );
    block = seq->free_blocks;

    if( !block )
    {
        int elem_size = seq->elem_size;
        int delta_elems = seq->delta_elems;
        CvMemStorage *storage = seq->storage;

        if( seq->total >= delta_elems*4 )
            cvSetSeqBlockSize( seq, delta_elems*2 );

        if( !storage )
            CV_ERROR( CV_StsNullPtr, "The sequence has NULL storage pointer" );

        /* if there is a free space just after last allocated block
           and it's big enough then enlarge the last block
           (this can happen only if the new block is added to the end of sequence */
        if( (unsigned)(ICV_FREE_PTR(storage) - seq->block_max) < CV_STRUCT_ALIGN &&
            storage->free_space >= seq->elem_size && !in_front_of )
        {
            int delta = storage->free_space / elem_size;

            delta = MIN( delta, delta_elems ) * elem_size;
            seq->block_max += delta;
            storage->free_space = cvAlignLeft((int)(((char*)storage->top + storage->block_size) -
                                              seq->block_max), CV_STRUCT_ALIGN );
            EXIT;
        }
        else
        {
            int delta = elem_size * delta_elems + ICV_ALIGNED_SEQ_BLOCK_SIZE;

            /* try to allocate <delta_elements> elements */
            if( storage->free_space < delta )
            {
                int small_block_size = MAX(1, delta_elems/3)*elem_size +
                                       ICV_ALIGNED_SEQ_BLOCK_SIZE;
                /* try to allocate smaller part */
                if( storage->free_space >= small_block_size + CV_STRUCT_ALIGN )
                {
                    delta = (storage->free_space - ICV_ALIGNED_SEQ_BLOCK_SIZE)/seq->elem_size;
                    delta = delta*seq->elem_size + ICV_ALIGNED_SEQ_BLOCK_SIZE;
                }
                else
                {
                    CV_CALL( icvGoNextMemBlock( storage ));
                    assert( storage->free_space >= delta );
                }
            }

            CV_CALL( block = (CvSeqBlock*)cvMemStorageAlloc( storage, delta ));
            block->data = (char*)cvAlignPtr( block + 1, CV_STRUCT_ALIGN );
            block->count = delta - ICV_ALIGNED_SEQ_BLOCK_SIZE;
            block->prev = block->next = 0;
        }
    }
    else
    {
        seq->free_blocks = block->next;
    }

    if( !(seq->first) )
    {
        seq->first = block;
        block->prev = block->next = block;
    }
    else
    {
        block->prev = seq->first->prev;
        block->next = seq->first;
        block->prev->next = block->next->prev = block;
    }

    /* for free blocks the <count> field means total number of bytes in the block.
       And for used blocks it means a current number of sequence
       elements in the block */
    assert( block->count % seq->elem_size == 0 && block->count > 0 );

    if( !in_front_of )
    {
        seq->ptr = block->data;
        seq->block_max = block->data + block->count;
        block->start_index = block == block->prev ? 0 :
            block->prev->start_index + block->prev->count;
    }
    else
    {
        int delta = block->count / seq->elem_size;
        block->data += block->count;

        if( block != block->prev )
        {
            assert( seq->first->start_index == 0 );
            seq->first = block;
        }
        else
        {
            seq->block_max = seq->ptr = block->data;
        }

        block->start_index = 0;

        for( ;; )
        {
            block->start_index += delta;
            block = block->next;
            if( block == seq->first )
                break;
        }
    }

    block->count = 0;

    __END__;
}

/* recycles a sequence block for the further use */
static void icvFreeSeqBlock( CvSeq *seq, int in_front_of )
{
    /*CV_FUNCNAME( "icvFreeSeqBlock" );*/

    __BEGIN__;

    CvSeqBlock *block = seq->first;

    assert( (in_front_of ? block : block->prev)->count == 0 );

    if( block == block->prev )  /* single block case */
    {
        block->count = (int)(seq->block_max - block->data) + block->start_index * seq->elem_size;
        block->data = seq->block_max - block->count;
        seq->first = 0;
        seq->ptr = seq->block_max = 0;
        seq->total = 0;
    }
    else
    {
        if( !in_front_of )
        {
            block = block->prev;
            assert( seq->ptr == block->data );

            block->count = (int)(seq->block_max - seq->ptr);
            seq->block_max = seq->ptr = block->prev->data +
                block->prev->count * seq->elem_size;
        }
        else
        {
            int delta = block->start_index;

            block->count = delta * seq->elem_size;
            block->data -= block->count;

            /* update start indices of sequence blocks */
            for( ;; )
            {
                block->start_index -= delta;
                block = block->next;
                if( block == seq->first )
                    break;
            }

            seq->first = block->next;
        }

        block->prev->next = block->next;
        block->next->prev = block->prev;
    }

    assert( block->count > 0 && block->count % seq->elem_size == 0 );
    block->next = seq->free_blocks;
    seq->free_blocks = block;

    __END__;
}


CvSlice  cvSlice( int start, int end )
{
    CvSlice slice;
    slice.start_index = start;
    slice.end_index = end;

    return slice;
}

#define  ICV_DEF_RESIZE_BILINEAR_FUNC( flavor, arrtype, worktype, alpha_field,  \
                                       mul_one_macro, descale_macro )           \
static CvStatus CV_STDCALL                                                      \
icvResize_Bilinear_##flavor##_CnR( const arrtype* src, int srcstep, CvSize ssize,\
                                   arrtype* dst, int dststep, CvSize dsize,     \
                                   int cn, int xmax,                            \
                                   const CvResizeAlpha* xofs,                   \
                                   const CvResizeAlpha* yofs,                   \
                                   worktype* buf0, worktype* buf1 )             \
{                                                                               \
    int prev_sy0 = -1, prev_sy1 = -1;                                           \
    int k, dx, dy;                                                              \
                                                                                \
    srcstep /= sizeof(src[0]);                                                  \
    dststep /= sizeof(dst[0]);                                                  \
    dsize.width *= cn;                                                          \
    xmax *= cn;                                                                 \
                                                                                \
    for( dy = 0; dy < dsize.height; dy++, dst += dststep )                      \
    {                                                                           \
        worktype fy = yofs[dy].alpha_field, *swap_t;                            \
        int sy0 = yofs[dy].idx, sy1 = sy0 + (fy > 0 && sy0 < ssize.height-1);   \
                                                                                \
        if( sy0 == prev_sy0 && sy1 == prev_sy1 )                                \
            k = 2;                                                              \
        else if( sy0 == prev_sy1 )                                              \
        {                                                                       \
            CV_SWAP( buf0, buf1, swap_t );                                      \
            k = 1;                                                              \
        }                                                                       \
        else                                                                    \
            k = 0;                                                              \
                                                                                \
        for( ; k < 2; k++ )                                                     \
        {                                                                       \
            worktype* _buf = k == 0 ? buf0 : buf1;                              \
            const arrtype* _src;                                                \
            int sy = k == 0 ? sy0 : sy1;                                        \
            if( k == 1 && sy1 == sy0 )                                          \
            {                                                                   \
                memcpy( buf1, buf0, dsize.width*sizeof(buf0[0]) );              \
                continue;                                                       \
            }                                                                   \
                                                                                \
            _src = src + sy*srcstep;                                            \
            for( dx = 0; dx < xmax; dx++ )                                      \
            {                                                                   \
                int sx = xofs[dx].idx;                                          \
                worktype fx = xofs[dx].alpha_field;                             \
                worktype t = _src[sx];                                          \
                _buf[dx] = mul_one_macro(t) + fx*(_src[sx+cn] - t);             \
            }                                                                   \
                                                                                \
            for( ; dx < dsize.width; dx++ )                                     \
                _buf[dx] = mul_one_macro(_src[xofs[dx].idx]);                   \
        }                                                                       \
                                                                                \
        prev_sy0 = sy0;                                                         \
        prev_sy1 = sy1;                                                         \
                                                                                \
        if( sy0 == sy1 )                                                        \
            for( dx = 0; dx < dsize.width; dx++ )                               \
                dst[dx] = (arrtype)descale_macro( mul_one_macro(buf0[dx]));     \
        else                                                                    \
            for( dx = 0; dx < dsize.width; dx++ )                               \
                dst[dx] = (arrtype)descale_macro( mul_one_macro(buf0[dx]) +     \
                                                  fy*(buf1[dx] - buf0[dx]));    \
    }                                                                           \
                                                                                \
    return CV_OK;                                                               \
}





#define  ICV_DEF_RESIZE_AREA_FAST_FUNC( flavor, arrtype, worktype, cast_macro ) \
static CvStatus CV_STDCALL                                                      \
icvResize_AreaFast_##flavor##_CnR( const arrtype* src, int srcstep, CvSize ssize,\
                              arrtype* dst, int dststep, CvSize dsize, int cn,  \
                              const int* ofs, const int* xofs )                 \
{                                                                               \
    int dy, dx, k = 0;                                                          \
    int scale_x = ssize.width/dsize.width;                                      \
    int scale_y = ssize.height/dsize.height;                                    \
    int area = scale_x*scale_y;                                                 \
    float scale = 1.f/(scale_x*scale_y);                                        \
                                                                                \
    srcstep /= sizeof(src[0]);                                                  \
    dststep /= sizeof(dst[0]);                                                  \
    dsize.width *= cn;                                                          \
                                                                                \
    for( dy = 0; dy < dsize.height; dy++, dst += dststep )                      \
        for( dx = 0; dx < dsize.width; dx++ )                                   \
        {                                                                       \
            const arrtype* _src = src + dy*scale_y*srcstep + xofs[dx];          \
            worktype sum = 0;                                                   \
                                                                                \
            for( k = 0; k <= area - 4; k += 4 )                                 \
                sum += _src[ofs[k]] + _src[ofs[k+1]] +                          \
                       _src[ofs[k+2]] + _src[ofs[k+3]];                         \
                                                                                \
            for( ; k < area; k++ )                                              \
                sum += _src[ofs[k]];                                            \
                                                                                \
            dst[dx] = (arrtype)cast_macro( sum*scale );                         \
        }                                                                       \
                                                                                \
    return CV_OK;                                                               \
}


#define  ICV_DEF_RESIZE_AREA_FUNC( flavor, arrtype, load_macro, cast_macro )    \
static CvStatus CV_STDCALL                                                      \
icvResize_Area_##flavor##_CnR( const arrtype* src, int srcstep, CvSize ssize,   \
                               arrtype* dst, int dststep, CvSize dsize,         \
                               int cn, const CvDecimateAlpha* xofs,             \
                               int xofs_count, float* buf, float* sum )         \
{                                                                               \
    int k, sy, dx, cur_dy = 0;                                                  \
    float scale_y = (float)ssize.height/dsize.height;                           \
                                                                                \
    srcstep /= sizeof(src[0]);                                                  \
    dststep /= sizeof(dst[0]);                                                  \
    dsize.width *= cn;                                                          \
                                                                                \
    for( sy = 0; sy < ssize.height; sy++, src += srcstep )                      \
    {                                                                           \
        if( cn == 1 )                                                           \
            for( k = 0; k < xofs_count; k++ )                                   \
            {                                                                   \
                int dxn = xofs[k].di;                                           \
                float alpha = xofs[k].alpha;                                    \
                buf[dxn] = buf[dxn] + load_macro(src[xofs[k].si])*alpha;        \
            }                                                                   \
        else if( cn == 2 )                                                      \
            for( k = 0; k < xofs_count; k++ )                                   \
            {                                                                   \
                int sxn = xofs[k].si;                                           \
                int dxn = xofs[k].di;                                           \
                float alpha = xofs[k].alpha;                                    \
                float t0 = buf[dxn] + load_macro(src[sxn])*alpha;               \
                float t1 = buf[dxn+1] + load_macro(src[sxn+1])*alpha;           \
                buf[dxn] = t0; buf[dxn+1] = t1;                                 \
            }                                                                   \
        else if( cn == 3 )                                                      \
            for( k = 0; k < xofs_count; k++ )                                   \
            {                                                                   \
                int sxn = xofs[k].si;                                           \
                int dxn = xofs[k].di;                                           \
                float alpha = xofs[k].alpha;                                    \
                float t0 = buf[dxn] + load_macro(src[sxn])*alpha;               \
                float t1 = buf[dxn+1] + load_macro(src[sxn+1])*alpha;           \
                float t2 = buf[dxn+2] + load_macro(src[sxn+2])*alpha;           \
                buf[dxn] = t0; buf[dxn+1] = t1; buf[dxn+2] = t2;                \
            }                                                                   \
        else                                                                    \
            for( k = 0; k < xofs_count; k++ )                                   \
            {                                                                   \
                int sxn = xofs[k].si;                                           \
                int dxn = xofs[k].di;                                           \
                float alpha = xofs[k].alpha;                                    \
                float t0 = buf[dxn] + load_macro(src[sxn])*alpha;               \
                float t1 = buf[dxn+1] + load_macro(src[sxn+1])*alpha;           \
                buf[dxn] = t0; buf[dxn+1] = t1;                                 \
                t0 = buf[dxn+2] + load_macro(src[sxn+2])*alpha;                 \
                t1 = buf[dxn+3] + load_macro(src[sxn+3])*alpha;                 \
                buf[dxn+2] = t0; buf[dxn+3] = t1;                               \
            }                                                                   \
                                                                                \
        if( (cur_dy + 1)*scale_y <= sy + 1 || sy == ssize.height - 1 )          \
        {                                                                       \
            float beta = sy + 1 - (cur_dy+1)*scale_y, beta1;                    \
            beta = MAX( beta, 0 );                                              \
            beta1 = 1 - beta;                                                   \
            if( fabs(beta) < 1e-3 )                                             \
                for( dx = 0; dx < dsize.width; dx++ )                           \
                {                                                               \
                    dst[dx] = (arrtype)cast_macro(sum[dx] + buf[dx]);           \
                    sum[dx] = buf[dx] = 0;                                      \
                }                                                               \
            else                                                                \
                for( dx = 0; dx < dsize.width; dx++ )                           \
                {                                                               \
                    dst[dx] = (arrtype)cast_macro(sum[dx] + buf[dx]*beta1);     \
                    sum[dx] = buf[dx]*beta;                                     \
                    buf[dx] = 0;                                                \
                }                                                               \
            dst += dststep;                                                     \
            cur_dy++;                                                           \
        }                                                                       \
        else                                                                    \
            for( dx = 0; dx < dsize.width; dx += 2 )                            \
            {                                                                   \
                float t0 = sum[dx] + buf[dx];                                   \
                float t1 = sum[dx+1] + buf[dx+1];                               \
                sum[dx] = t0; sum[dx+1] = t1;                                   \
                buf[dx] = buf[dx+1] = 0;                                        \
            }                                                                   \
    }                                                                           \
                                                                                \
    return CV_OK;                                                               \
}

static CvStatus CV_STDCALL
icvResize_NN_8u_C1R( const uchar* src, int srcstep, CvSize ssize,
                     uchar* dst, int dststep, CvSize dsize, int pix_size )
{
    int* x_ofs = (int*)cvStackAlloc( dsize.width * sizeof(x_ofs[0]) );
    int pix_size4 = pix_size / sizeof(int);
    int x, y, t;

    for( x = 0; x < dsize.width; x++ )
    {
        t = (ssize.width*x*2 + MIN(ssize.width, dsize.width) - 1)/(dsize.width*2);
        t -= t >= ssize.width;
        x_ofs[x] = t*pix_size;
    }

    for( y = 0; y < dsize.height; y++, dst += dststep )
    {
        const uchar* tsrc;
        t = (ssize.height*y*2 + MIN(ssize.height, dsize.height) - 1)/(dsize.height*2);
        t -= t >= ssize.height;
        tsrc = src + srcstep*t;

        switch( pix_size )
        {
        case 1:
            for( x = 0; x <= dsize.width - 2; x += 2 )
            {
                uchar t0 = tsrc[x_ofs[x]];
                uchar t1 = tsrc[x_ofs[x+1]];

                dst[x] = t0;
                dst[x+1] = t1;
            }

            for( ; x < dsize.width; x++ )
                dst[x] = tsrc[x_ofs[x]];
            break;
        case 2:
            for( x = 0; x < dsize.width; x++ )
                *(ushort*)(dst + x*2) = *(ushort*)(tsrc + x_ofs[x]);
            break;
        case 3:
            for( x = 0; x < dsize.width; x++ )
            {
                const uchar* _tsrc = tsrc + x_ofs[x];
                dst[x*3] = _tsrc[0]; dst[x*3+1] = _tsrc[1]; dst[x*3+2] = _tsrc[2];
            }
            break;
        case 4:
            for( x = 0; x < dsize.width; x++ )
                *(int*)(dst + x*4) = *(int*)(tsrc + x_ofs[x]);
            break;
        case 6:
            for( x = 0; x < dsize.width; x++ )
            {
                const ushort* _tsrc = (const ushort*)(tsrc + x_ofs[x]);
                ushort* _tdst = (ushort*)(dst + x*6);
                _tdst[0] = _tsrc[0]; _tdst[1] = _tsrc[1]; _tdst[2] = _tsrc[2];
            }
            break;
        default:
            for( x = 0; x < dsize.width; x++ )
                CV_MEMCPY_INT( dst + x*pix_size, tsrc + x_ofs[x], pix_size4 );
        }
    }

    return CV_OK;
}


#define  ICV_DEF_RESIZE_BICUBIC_FUNC( flavor, arrtype, worktype, load_macro,    \
                                      cast_macro1, cast_macro2 )                \
static CvStatus CV_STDCALL                                                      \
icvResize_Bicubic_##flavor##_CnR( const arrtype* src, int srcstep, CvSize ssize,\
                                  arrtype* dst, int dststep, CvSize dsize,      \
                                  int cn, int xmin, int xmax,                   \
                                  const CvResizeAlpha* xofs, float** buf )      \
{                                                                               \
    float scale_y = (float)ssize.height/dsize.height;                           \
    int dx, dy, sx, sy, sy2, ify;                                               \
    int prev_sy2 = -2;                                                          \
                                                                                \
    xmin *= cn; xmax *= cn;                                                     \
    dsize.width *= cn;                                                          \
    ssize.width *= cn;                                                          \
    srcstep /= sizeof(src[0]);                                                  \
    dststep /= sizeof(dst[0]);                                                  \
                                                                                \
    for( dy = 0; dy < dsize.height; dy++, dst += dststep )                      \
    {                                                                           \
        float w0, w1, w2, w3;                                                   \
        float fy, x, sum;                                                       \
        float *row, *row0, *row1, *row2, *row3;                                 \
        int k1, k = 4;                                                          \
                                                                                \
        fy = dy*scale_y;                                                        \
        sy = floor(fy);                                                       \
        fy -= sy;                                                               \
        ify = cvRound(fy*ICV_CUBIC_TAB_SIZE);                                   \
        sy2 = sy + 2;                                                           \
                                                                                \
        if( sy2 > prev_sy2 )                                                    \
        {                                                                       \
            int delta = prev_sy2 - sy + 2;                                      \
            for( k = 0; k < delta; k++ )                                        \
                CV_SWAP( buf[k], buf[k+4-delta], row );                         \
        }                                                                       \
                                                                                \
        for( sy += k - 1; k < 4; k++, sy++ )                                    \
        {                                                                       \
            const arrtype* _src = src + sy*srcstep;                             \
                                                                                \
            row = buf[k];                                                       \
            if( sy < 0 )                                                        \
                continue;                                                       \
            if( sy >= ssize.height )                                            \
            {                                                                   \
                assert( k > 0 );                                                \
                memcpy( row, buf[k-1], dsize.width*sizeof(row[0]) );            \
                continue;                                                       \
            }                                                                   \
                                                                                \
            for( dx = 0; dx < xmin; dx++ )                                      \
            {                                                                   \
                int ifx = xofs[dx].ialpha, sx0 = xofs[dx].idx;                  \
                sx = sx0 + cn*2;                                                \
                while( sx >= ssize.width )                                      \
                    sx -= cn;                                                   \
                x = load_macro(_src[sx]);                                       \
                sum = x*icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE - ifx)*2 + 1];       \
                if( (unsigned)(sx = sx0 + cn) < (unsigned)ssize.width )         \
                    x = load_macro(_src[sx]);                                   \
                sum += x*icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE - ifx)*2];          \
                if( (unsigned)(sx = sx0) < (unsigned)ssize.width )              \
                    x = load_macro(_src[sx]);                                   \
                sum += x*icvCubicCoeffs[ifx*2];                                 \
                if( (unsigned)(sx = sx0 - cn) < (unsigned)ssize.width )         \
                    x = load_macro(_src[sx]);                                   \
                row[dx] = sum + x*icvCubicCoeffs[ifx*2 + 1];                    \
            }                                                                   \
                                                                                \
            for( ; dx < xmax; dx++ )                                            \
            {                                                                   \
                int ifx = xofs[dx].ialpha;                                      \
                int sx0 = xofs[dx].idx;                                         \
                row[dx] = _src[sx0 - cn]*icvCubicCoeffs[ifx*2 + 1] +            \
                    _src[sx0]*icvCubicCoeffs[ifx*2] +                           \
                    _src[sx0 + cn]*icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE-ifx)*2] + \
                    _src[sx0 + cn*2]*icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE-ifx)*2+1];\
            }                                                                   \
                                                                                \
            for( ; dx < dsize.width; dx++ )                                     \
            {                                                                   \
                int ifx = xofs[dx].ialpha, sx0 = xofs[dx].idx;                  \
                x = load_macro(_src[sx0 - cn]);                                 \
                sum = x*icvCubicCoeffs[ifx*2 + 1];                              \
                if( (unsigned)(sx = sx0) < (unsigned)ssize.width )              \
                    x = load_macro(_src[sx]);                                   \
                sum += x*icvCubicCoeffs[ifx*2];                                 \
                if( (unsigned)(sx = sx0 + cn) < (unsigned)ssize.width )         \
                    x = load_macro(_src[sx]);                                   \
                sum += x*icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE - ifx)*2];          \
                if( (unsigned)(sx = sx0 + cn*2) < (unsigned)ssize.width )       \
                    x = load_macro(_src[sx]);                                   \
                row[dx] = sum + x*icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE-ifx)*2+1]; \
            }                                                                   \
                                                                                \
            if( sy == 0 )                                                       \
                for( k1 = 0; k1 < k; k1++ )                                     \
                    memcpy( buf[k1], row, dsize.width*sizeof(row[0]));          \
        }                                                                       \
                                                                                \
        prev_sy2 = sy2;                                                         \
                                                                                \
        row0 = buf[0]; row1 = buf[1];                                           \
        row2 = buf[2]; row3 = buf[3];                                           \
                                                                                \
        w0 = icvCubicCoeffs[ify*2+1];                                           \
        w1 = icvCubicCoeffs[ify*2];                                             \
        w2 = icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE - ify)*2];                      \
        w3 = icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE - ify)*2 + 1];                  \
                                                                                \
        for( dx = 0; dx < dsize.width; dx++ )                                   \
        {                                                                       \
            worktype val = cast_macro1( row0[dx]*w0 + row1[dx]*w1 +             \
                                        row2[dx]*w2 + row3[dx]*w3 );            \
            dst[dx] = cast_macro2(val);                                         \
        }                                                                       \
    }                                                                           \
                                                                                \
    return CV_OK;                                                               \
}


ICV_DEF_RESIZE_BILINEAR_FUNC( 8u, uchar, int, ialpha,ICV_WARP_MUL_ONE_8U, ICV_WARP_DESCALE_8U )
ICV_DEF_RESIZE_BILINEAR_FUNC( 16u, ushort, float, alpha, CV_NOP, cvRound )
ICV_DEF_RESIZE_BILINEAR_FUNC( 32f, float, float, alpha, CV_NOP, CV_NOP )

ICV_DEF_RESIZE_BICUBIC_FUNC( 8u, uchar, int, CV_8TO32F, cvRound, CV_CAST_8U )
ICV_DEF_RESIZE_BICUBIC_FUNC( 16u, ushort, int, CV_NOP, cvRound, CV_CAST_16U )
ICV_DEF_RESIZE_BICUBIC_FUNC( 32f, float, float, CV_NOP, CV_NOP, CV_NOP )

ICV_DEF_RESIZE_AREA_FAST_FUNC( 8u, uchar, int, cvRound )
ICV_DEF_RESIZE_AREA_FAST_FUNC( 16u, ushort, int, cvRound )
ICV_DEF_RESIZE_AREA_FAST_FUNC( 32f, float, float, CV_NOP )

ICV_DEF_RESIZE_AREA_FUNC( 8u, uchar, CV_8TO32F, cvRound )
ICV_DEF_RESIZE_AREA_FUNC( 16u, ushort, CV_NOP, cvRound )
ICV_DEF_RESIZE_AREA_FUNC( 32f, float, CV_NOP, CV_NOP )

static void icvInitResizeTab( CvFuncTable* bilin_tab,
                              CvFuncTable* bicube_tab,
                              CvFuncTable* areafast_tab,
                              CvFuncTable* area_tab )
{
    bilin_tab->fn_2d[CV_8U] = (void*)icvResize_Bilinear_8u_CnR;
    bilin_tab->fn_2d[CV_16U] = (void*)icvResize_Bilinear_16u_CnR;
    bilin_tab->fn_2d[CV_32F] = (void*)icvResize_Bilinear_32f_CnR;

    bicube_tab->fn_2d[CV_8U] = (void*)icvResize_Bicubic_8u_CnR;
    bicube_tab->fn_2d[CV_16U] = (void*)icvResize_Bicubic_16u_CnR;
    bicube_tab->fn_2d[CV_32F] = (void*)icvResize_Bicubic_32f_CnR;

    areafast_tab->fn_2d[CV_8U] = (void*)icvResize_AreaFast_8u_CnR;
    areafast_tab->fn_2d[CV_16U] = (void*)icvResize_AreaFast_16u_CnR;
    areafast_tab->fn_2d[CV_32F] = (void*)icvResize_AreaFast_32f_CnR;

    area_tab->fn_2d[CV_8U] = (void*)icvResize_Area_8u_CnR;
    area_tab->fn_2d[CV_16U] = (void*)icvResize_Area_16u_CnR;
    area_tab->fn_2d[CV_32F] = (void*)icvResize_Area_32f_CnR;
}

icvResize_8u_C1R_t icvResize_8u_C1R_p = 0;
icvResize_8u_C3R_t icvResize_8u_C3R_p = 0;
icvResize_8u_C4R_t icvResize_8u_C4R_p = 0;
icvResize_16u_C1R_t icvResize_16u_C1R_p = 0;
icvResize_16u_C3R_t icvResize_16u_C3R_p = 0;
icvResize_16u_C4R_t icvResize_16u_C4R_p = 0;
icvResize_32f_C1R_t icvResize_32f_C1R_p = 0;
icvResize_32f_C3R_t icvResize_32f_C3R_p = 0;
icvResize_32f_C4R_t icvResize_32f_C4R_p = 0;

typedef CvStatus (CV_STDCALL * CvResizeIPPFunc)
( const void* src, CvSize srcsize, int srcstep, CvRect srcroi,
  void* dst, int dststep, CvSize dstroi,
  double xfactor, double yfactor, int interpolation );

void cvResize( const CvArr* srcarr, CvArr* dstarr, int method )
{
    static CvFuncTable bilin_tab, bicube_tab, areafast_tab, area_tab;
    static int inittab = 0;
    void* temp_buf = 0;

    CV_FUNCNAME( "cvResize" );

    __BEGIN__;
    
    CvMat srcstub, *src = (CvMat*)srcarr;
    CvMat dststub, *dst = (CvMat*)dstarr;
    CvSize ssize, dsize;
    float scale_x, scale_y;
    int k, sx, sy, dx, dy;
    int type, depth, cn;
    
    CV_CALL( src = cvGetMat( srcarr, &srcstub,NULL,0 ));
    CV_CALL( dst = cvGetMat( dstarr, &dststub,NULL,0 ));
    
    if( CV_ARE_SIZES_EQ( src, dst ))
        CV_CALL( cvCopy( src, dst,NULL ));

    if( !CV_ARE_TYPES_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    if( !inittab )
    {
        icvInitResizeTab( &bilin_tab, &bicube_tab, &areafast_tab, &area_tab );
        inittab = 1;
    }

    ssize = cvGetMatSize( src );
    dsize = cvGetMatSize( dst );
    type = CV_MAT_TYPE(src->type);
    depth = CV_MAT_DEPTH(type);
    cn = CV_MAT_CN(type);
    scale_x = (float)ssize.width/dsize.width;
    scale_y = (float)ssize.height/dsize.height;

    if( method == CV_INTER_CUBIC &&
        (MIN(ssize.width, dsize.width) <= 4 ||
        MIN(ssize.height, dsize.height) <= 4) )
        method = CV_INTER_LINEAR;

    if( icvResize_8u_C1R_p &&
        MIN(ssize.width, dsize.width) > 4 &&
        MIN(ssize.height, dsize.height) > 4 )
    {
        CvResizeIPPFunc ipp_func =
            type == CV_8UC1 ? icvResize_8u_C1R_p :
            type == CV_8UC3 ? icvResize_8u_C3R_p :
            type == CV_8UC4 ? icvResize_8u_C4R_p :
            type == CV_16UC1 ? icvResize_16u_C1R_p :
            type == CV_16UC3 ? icvResize_16u_C3R_p :
            type == CV_16UC4 ? icvResize_16u_C4R_p :
            type == CV_32FC1 ? icvResize_32f_C1R_p :
            type == CV_32FC3 ? icvResize_32f_C3R_p :
            type == CV_32FC4 ? icvResize_32f_C4R_p : 0;
        if( ipp_func && (CV_INTER_NN < method && method < CV_INTER_AREA))
        {
            int srcstep = src->step ? src->step : CV_STUB_STEP;
            int dststep = dst->step ? dst->step : CV_STUB_STEP;
            IPPI_CALL( ipp_func( src->data.ptr, ssize, srcstep,
                                 cvRect(0,0,ssize.width,ssize.height),
                                 dst->data.ptr, dststep, dsize,
                                 (double)dsize.width/ssize.width,
                                 (double)dsize.height/ssize.height, 1 << method ));
            EXIT;
        }
    }

    if( method == CV_INTER_NN )
    {
        IPPI_CALL( icvResize_NN_8u_C1R( src->data.ptr, src->step, ssize,
                                        dst->data.ptr, dst->step, dsize,
                                        CV_ELEM_SIZE(src->type)));
    }
    else if( method == CV_INTER_LINEAR || method == CV_INTER_AREA )
    {
        if( method == CV_INTER_AREA &&
            ssize.width >= dsize.width && ssize.height >= dsize.height )
        {
            // "area" method for (scale_x > 1 & scale_y > 1)
            int iscale_x = cvRound(scale_x);
            int iscale_y = cvRound(scale_y);

            if( fabs(scale_x - iscale_x) < DBL_EPSILON &&
                fabs(scale_y - iscale_y) < DBL_EPSILON )
            {
                int area = iscale_x*iscale_y;
                int srcstep = src->step / CV_ELEM_SIZE(depth);
                int* ofs = (int*)cvStackAlloc( (area + dsize.width*cn)*sizeof(int) );
                int* xofs = ofs + area;
                CvResizeAreaFastFunc func = (CvResizeAreaFastFunc)areafast_tab.fn_2d[depth];

                if( !func )
                    CV_ERROR( CV_StsUnsupportedFormat, "" );
                
                for( sy = 0, k = 0; sy < iscale_y; sy++ )
                    for( sx = 0; sx < iscale_x; sx++ )
                        ofs[k++] = sy*srcstep + sx*cn;

                for( dx = 0; dx < dsize.width; dx++ )
                {
                    sx = dx*iscale_x*cn;
                    for( k = 0; k < cn; k++ )
                        xofs[dx*cn + k] = sx + k;
                }

                IPPI_CALL( func( src->data.ptr, src->step, ssize, dst->data.ptr,
                                 dst->step, dsize, cn, ofs, xofs ));
            }
            else
            {
                int buf_len = dsize.width*cn + 4, buf_size, xofs_count = 0;
                float scale = 1.f/(scale_x*scale_y);
                float *buf, *sum;
                CvDecimateAlpha* xofs;
                CvResizeAreaFunc func = (CvResizeAreaFunc)area_tab.fn_2d[depth];

                if( !func || cn > 4 )
                    CV_ERROR( CV_StsUnsupportedFormat, "" );

                buf_size = buf_len*2*sizeof(float) + ssize.width*2*sizeof(CvDecimateAlpha);
                if( buf_size < CV_MAX_LOCAL_SIZE )
                    buf = (float*)cvStackAlloc(buf_size);
                else
                    CV_CALL( temp_buf = buf = (float*)cvAlloc(buf_size));
                sum = buf + buf_len;
                xofs = (CvDecimateAlpha*)(sum + buf_len);

                for( dx = 0, k = 0; dx < dsize.width; dx++ )
                {
                    float fsx1 = dx*scale_x, fsx2 = fsx1 + scale_x;
                    int sx1 = ceil(fsx1), sx2 = floor(fsx2);

                    assert( (unsigned)sx1 < (unsigned)ssize.width );

                    if( sx1 > fsx1 )
                    {
                        assert( k < ssize.width*2 );            
                        xofs[k].di = dx*cn;
                        xofs[k].si = (sx1-1)*cn;
                        xofs[k++].alpha = (sx1 - fsx1)*scale;
                    }

                    for( sx = sx1; sx < sx2; sx++ )
                    {
                        assert( k < ssize.width*2 );
                        xofs[k].di = dx*cn;
                        xofs[k].si = sx*cn;
                        xofs[k++].alpha = scale;
                    }

                    if( fsx2 - sx2 > 1e-3 )
                    {
                        assert( k < ssize.width*2 );
                        assert((unsigned)sx2 < (unsigned)ssize.width );
                        xofs[k].di = dx*cn;
                        xofs[k].si = sx2*cn;
                        xofs[k++].alpha = (fsx2 - sx2)*scale;
                    }
                }

                xofs_count = k;
                memset( sum, 0, buf_len*sizeof(float) );
                memset( buf, 0, buf_len*sizeof(float) );

                IPPI_CALL( func( src->data.ptr, src->step, ssize, dst->data.ptr,
                                 dst->step, dsize, cn, xofs, xofs_count, buf, sum ));
            }
        }
        else // true "area" method for the cases (scale_x > 1 & scale_y < 1) and
             // (scale_x < 1 & scale_y > 1) is not implemented.
             // instead, it is emulated via some variant of bilinear interpolation.
        {
            float inv_scale_x = (float)dsize.width/ssize.width;
            float inv_scale_y = (float)dsize.height/ssize.height;
            int xmax = dsize.width, width = dsize.width*cn, buf_size;
            float *buf0, *buf1;
            CvResizeAlpha *xofs, *yofs;
            int area_mode = method == CV_INTER_AREA;
            float fx, fy;
            CvResizeBilinearFunc func = (CvResizeBilinearFunc)bilin_tab.fn_2d[depth];

            if( !func )
                CV_ERROR( CV_StsUnsupportedFormat, "" );

            buf_size = width*2*sizeof(float) + (width + dsize.height)*sizeof(CvResizeAlpha);
            if( buf_size < CV_MAX_LOCAL_SIZE )
                buf0 = (float*)cvStackAlloc(buf_size);
            else
                CV_CALL( temp_buf = buf0 = (float*)cvAlloc(buf_size));
            buf1 = buf0 + width;
            xofs = (CvResizeAlpha*)(buf1 + width);
            yofs = xofs + width;

            for( dx = 0; dx < dsize.width; dx++ )
            {
                if( !area_mode )
                {
                    fx = (float)((dx+0.5)*scale_x - 0.5);
                    sx = floor(fx);
                    fx -= sx;
                }
                else
                {
                    sx = floor(dx*scale_x);
                    fx = (dx+1) - (sx+1)*inv_scale_x;
                    fx = fx <= 0 ? 0.f : fx - floor(fx);
                }

                if( sx < 0 )
                    fx = 0, sx = 0;

                if( sx >= ssize.width-1 )
                {
                    fx = 0, sx = ssize.width-1;
                    if( xmax >= dsize.width )
                        xmax = dx;
                }

                if( depth != CV_8U )
                    for( k = 0, sx *= cn; k < cn; k++ )
                        xofs[dx*cn + k].idx = sx + k, xofs[dx*cn + k].alpha = fx;
                else
                    for( k = 0, sx *= cn; k < cn; k++ )
                        xofs[dx*cn + k].idx = sx + k,
                        xofs[dx*cn + k].ialpha = CV_FLT_TO_FIX(fx, ICV_WARP_SHIFT);
            }

            for( dy = 0; dy < dsize.height; dy++ )
            {
                if( !area_mode )
                {
                    fy = (float)((dy+0.5)*scale_y - 0.5);
                    sy = floor(fy);
                    fy -= sy;
                    if( sy < 0 )
                        sy = 0, fy = 0;
                }
                else
                {
                    sy = floor(dy*scale_y);
                    fy = (dy+1) - (sy+1)*inv_scale_y;
                    fy = fy <= 0 ? 0.f : fy - floor(fy);
                }

                yofs[dy].idx = sy;
                if( depth != CV_8U )
                    yofs[dy].alpha = fy;
                else
                    yofs[dy].ialpha = CV_FLT_TO_FIX(fy, ICV_WARP_SHIFT);
            }

            IPPI_CALL( func( src->data.ptr, src->step, ssize, dst->data.ptr,
                             dst->step, dsize, cn, xmax, xofs, yofs, buf0, buf1 ));
        }
    }
    else if( method == CV_INTER_CUBIC )
    {
        int width = dsize.width*cn, buf_size;
        int xmin = dsize.width, xmax = -1;
        CvResizeAlpha* xofs;
        float* buf[4];
        CvResizeBicubicFunc func = (CvResizeBicubicFunc)bicube_tab.fn_2d[depth];

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );
        
        buf_size = width*(4*sizeof(float) + sizeof(xofs[0]));
        if( buf_size < CV_MAX_LOCAL_SIZE )
            buf[0] = (float*)cvStackAlloc(buf_size);
        else
            CV_CALL( temp_buf = buf[0] = (float*)cvAlloc(buf_size));

        for( k = 1; k < 4; k++ )
            buf[k] = buf[k-1] + width;
        xofs = (CvResizeAlpha*)(buf[3] + width);

        icvInitCubicCoeffTab();

        for( dx = 0; dx < dsize.width; dx++ )
        {
            float fx = dx*scale_x;
            sx = floor(fx);
            fx -= sx;
            int ifx = cvRound(fx*ICV_CUBIC_TAB_SIZE);
            if( sx-1 >= 0 && xmin > dx )
                xmin = dx;
            if( sx+2 < ssize.width )
                xmax = dx + 1;
        
            // at least one of 4 points should be within the image - to
            // be able to set other points to the same value. see the loops
            // for( dx = 0; dx < xmin; dx++ ) ... and for( ; dx < width; dx++ ) ...
            if( sx < -2 )
                sx = -2;
            else if( sx > ssize.width )
                sx = ssize.width;

            for( k = 0; k < cn; k++ )
            {
                xofs[dx*cn + k].idx = sx*cn + k;
                xofs[dx*cn + k].ialpha = ifx;
            }
        }
    
        IPPI_CALL( func( src->data.ptr, src->step, ssize, dst->data.ptr,
                         dst->step, dsize, cn, xmin, xmax, xofs, buf ));
    }
    else
        CV_ERROR( CV_StsBadFlag, "Unknown/unsupported interpolation method" );

    __END__;

    cvFree( &temp_buf );
}

CvSize cvGetSize( const CvArr* arr )
{
    CvSize size = { 0, 0 };

    CV_FUNCNAME( "cvGetSize" );

    __BEGIN__;

    if( CV_IS_MAT_HDR( arr ))
    {
        CvMat *mat = (CvMat*)arr;

        size.width = mat->cols;
        size.height = mat->rows;
    }
    else if( CV_IS_IMAGE_HDR( arr ))
    {
        IplImage* img = (IplImage*)arr;

        if( img->roi )
        {
            size.width = img->roi->width;
            size.height = img->roi->height;
        }
        else
        {
            size.width = img->width;
            size.height = img->height;
        }
    }
    else
    {
        CV_ERROR( CV_StsBadArg, "Array should be CvMat or IplImage" );
    }

    __END__;

    return size;
}

CvMat* cvCreateMatHeader( int rows, int cols, int type )
{
    CvMat* arr = 0;
    
    CV_FUNCNAME( "cvCreateMatHeader" );

    __BEGIN__;

    int min_step;
    type = CV_MAT_TYPE(type);

    if( rows <= 0 || cols <= 0 )
        CV_ERROR( CV_StsBadSize, "Non-positive width or height" );

    min_step = CV_ELEM_SIZE(type)*cols;
    if( min_step <= 0 )
        CV_ERROR( CV_StsUnsupportedFormat, "Invalid matrix type" );

    CV_CALL( arr = (CvMat*)cvAlloc( sizeof(*arr)));

    arr->step = rows == 1 ? 0 : cvAlign(min_step, CV_DEFAULT_MAT_ROW_ALIGN);
    arr->type = CV_MAT_MAGIC_VAL | type |
                (arr->step == 0 || arr->step == min_step ? CV_MAT_CONT_FLAG : 0);
    arr->rows = rows;
    arr->cols = cols;
    arr->data.ptr = 0;
    arr->refcount = 0;
    arr->hdr_refcount = 1;

    icvCheckHuge( arr );

    __END__;

    if( cvGetErrStatus() < 0 )
        cvReleaseMat( &arr );

    return arr;
}

static CvMatND*
cvGetMatND( const CvArr* arr, CvMatND* matnd, int* coi )
{
    CvMatND* result = 0;
    
    CV_FUNCNAME( "cvGetMatND" );

    __BEGIN__;

    if( coi )
        *coi = 0;

    if( !matnd || !arr )
        CV_ERROR( CV_StsNullPtr, "NULL array pointer is passed" );

    if( CV_IS_MATND_HDR(arr))
    {
        if( !((CvMatND*)arr)->data.ptr )
            CV_ERROR( CV_StsNullPtr, "The matrix has NULL data pointer" );
        
        result = (CvMatND*)arr;
    }
    else
    {
        CvMat stub, *mat = (CvMat*)arr;
        
        if( CV_IS_IMAGE_HDR( mat ))
            CV_CALL( mat = cvGetMat( mat, &stub, coi,0 ));

        if( !CV_IS_MAT_HDR( mat ))
            CV_ERROR( CV_StsBadArg, "Unrecognized or unsupported array type" );
        
        if( !mat->data.ptr )
            CV_ERROR( CV_StsNullPtr, "Input array has NULL data pointer" );

        matnd->data.ptr = mat->data.ptr;
        matnd->refcount = 0;
        matnd->hdr_refcount = 0;
        matnd->type = mat->type;
        matnd->dims = 2;
        matnd->dim[0].size = mat->rows;
        matnd->dim[0].step = mat->step;
        matnd->dim[1].size = mat->cols;
        matnd->dim[1].step = CV_ELEM_SIZE(mat->type);
        result = matnd;
    }

    __END__;

    return result;
}

int cvInitNArrayIterator( int count, CvArr** arrs,const CvArr* mask, CvMatND* stubs,CvNArrayIterator* iterator, int flags )
{
    int dims = -1;

    CV_FUNCNAME( "cvInitArrayOp" );
    
    __BEGIN__;

    int i, j, size, dim0 = -1;
    int64 step;
    CvMatND* hdr0 = 0;

    if( count < 1 || count > CV_MAX_ARR )
        CV_ERROR( CV_StsOutOfRange, "Incorrect number of arrays" );

    if( !arrs || !stubs )
        CV_ERROR( CV_StsNullPtr, "Some of required array pointers is NULL" );

    if( !iterator )
        CV_ERROR( CV_StsNullPtr, "Iterator pointer is NULL" );

    for( i = 0; i <= count; i++ )
    {
        const CvArr* arr = i < count ? arrs[i] : mask;
        CvMatND* hdr;
        
        if( !arr )
        {
            if( i < count )
                CV_ERROR( CV_StsNullPtr, "Some of required array pointers is NULL" );
            break;
        }

        if( CV_IS_MATND( arr ))
            hdr = (CvMatND*)arr;
        else
        {
            int coi = 0;
            CV_CALL( hdr = cvGetMatND( arr, stubs + i, &coi ));
            if( coi != 0 )
                CV_ERROR( CV_BadCOI, "COI set is not allowed here" );
        }

        iterator->hdr[i] = hdr;

        if( i > 0 )
        {
            if( hdr->dims != hdr0->dims )
                CV_ERROR( CV_StsUnmatchedSizes,
                          "Number of dimensions is the same for all arrays" );
            
            if( i < count )
            {
                switch( flags & (CV_NO_DEPTH_CHECK|CV_NO_CN_CHECK))
                {
                case 0:
                    if( !CV_ARE_TYPES_EQ( hdr, hdr0 ))
                        CV_ERROR( CV_StsUnmatchedFormats,
                                  "Data type is not the same for all arrays" );
                    break;
                case CV_NO_DEPTH_CHECK:
                    if( !CV_ARE_CNS_EQ( hdr, hdr0 ))
                        CV_ERROR( CV_StsUnmatchedFormats,
                                  "Number of channels is not the same for all arrays" );
                    break;
                case CV_NO_CN_CHECK:
                    if( !CV_ARE_CNS_EQ( hdr, hdr0 ))
                        CV_ERROR( CV_StsUnmatchedFormats,
                                  "Depth is not the same for all arrays" );
                    break;
                }
            }
            else
            {
                if( !CV_IS_MASK_ARR( hdr ))
                    CV_ERROR( CV_StsBadMask, "Mask should have 8uC1 or 8sC1 data type" );
            }

            if( !(flags & CV_NO_SIZE_CHECK) )
            {
                for( j = 0; j < hdr->dims; j++ )
                    if( hdr->dim[j].size != hdr0->dim[j].size )
                        CV_ERROR( CV_StsUnmatchedSizes,
                                  "Dimension sizes are the same for all arrays" );
            }
        }
        else
            hdr0 = hdr;

        step = CV_ELEM_SIZE(hdr->type);
        for( j = hdr->dims - 1; j > dim0; j-- )
        {
            if( step != hdr->dim[j].step )
                break;
            step *= hdr->dim[j].size;
        }

        if( j == dim0 && step > INT_MAX )
            j++;

        if( j > dim0 )
            dim0 = j;

        iterator->hdr[i] = (CvMatND*)hdr;
        iterator->ptr[i] = (uchar*)hdr->data.ptr;
    }

    size = 1;
    for( j = hdr0->dims - 1; j > dim0; j-- )
        size *= hdr0->dim[j].size;

    dims = dim0 + 1;
    iterator->dims = dims;
    iterator->count = count;
    iterator->size = cvSize(size,1);

    for( i = 0; i < dims; i++ )
        iterator->stack[i] = hdr0->dim[i].size;

    __END__;

    return dims;
}

int  cvNextNArraySlice( CvNArrayIterator* iterator )
{
    assert( iterator != 0 );
    int i, dims, size = 0;

    for( dims = iterator->dims; dims > 0; dims-- )
    {
        for( i = 0; i < iterator->count; i++ )
            iterator->ptr[i] += iterator->hdr[i]->dim[dims-1].step;

        if( --iterator->stack[dims-1] > 0 )
            break;

        size = iterator->hdr[0]->dim[dims-1].size;

        for( i = 0; i < iterator->count; i++ )
            iterator->ptr[i] -= (size_t)size*iterator->hdr[i]->dim[dims-1].step;

        iterator->stack[dims-1] = size;
    }

    return dims > 0;
}

icvSetByte_8u_C1R_t icvSetByte_8u_C1R_p = 0;

CvStatus CV_STDCALL icvSetZero_8u_C1R( uchar* dst, int dststep, CvSize size )
{
    if( size.width + size.height > 256 && icvSetByte_8u_C1R_p )
        return icvSetByte_8u_C1R_p( 0, dst, dststep, size );

    for( ; size.height--; dst += dststep )
        memset( dst, 0, size.width );

    return CV_OK;
}

IPCVAPI_IMPL( CvStatus, icvCopy_8u_C1R, ( const uchar* src, int srcstep,
                                          uchar* dst, int dststep, CvSize size ),
                                          (src, srcstep, dst, dststep, size) )
{
    for( ; size.height--; src += srcstep, dst += dststep )
        memcpy( dst, src, size.width );

    return  CV_OK;
}



void cvClearSet( CvSet* set )
{
    CV_FUNCNAME( "cvClearSet" );

    __BEGIN__;

    CV_CALL( cvClearSeq( (CvSeq*)set ));
    set->free_elems = 0;
    set->active_count = 0;

    __END__;
}

CvMat* cvGetMat( const CvArr* array, CvMat* mat,int* pCOI, int allowND )
{
    CvMat* result = 0;
    CvMat* src = (CvMat*)array;
    int coi = 0;
    
    CV_FUNCNAME( "cvGetMat" );

    __BEGIN__;

    if( !mat || !src )
        CV_ERROR( CV_StsNullPtr, "NULL array pointer is passed" );

    if( CV_IS_MAT_HDR(src))
    {
        if( !src->data.ptr )
            CV_ERROR( CV_StsNullPtr, "The matrix has NULL data pointer" );
        
        result = (CvMat*)src;
    }
    else if( CV_IS_IMAGE_HDR(src) )
    {
        const IplImage* img = (const IplImage*)src;
        int depth, order;

        if( img->imageData == 0 )
            CV_ERROR( CV_StsNullPtr, "The image has NULL data pointer" );

        depth = icvIplToCvDepth( img->depth );
        if( depth < 0 )
            CV_ERROR_FROM_CODE( CV_BadDepth );

        order = img->dataOrder & (img->nChannels > 1 ? -1 : 0);

        if( img->roi )
        {
            if( order == IPL_DATA_ORDER_PLANE )
            {
                int type = depth;

                if( img->roi->coi == 0 )
                    CV_ERROR( CV_StsBadFlag,
                    "Images with planar data layout should be used with COI selected" );

                CV_CALL( cvInitMatHeader( mat, img->roi->height,
                                   img->roi->width, type,
                                   img->imageData + (img->roi->coi-1)*img->imageSize +
                                   img->roi->yOffset*img->widthStep +
                                   img->roi->xOffset*CV_ELEM_SIZE(type),
                                   img->widthStep ));
            }
            else /* pixel order */
            {
                int type = CV_MAKETYPE( depth, img->nChannels );
                coi = img->roi->coi;

                if( img->nChannels > CV_CN_MAX )
                    CV_ERROR( CV_BadNumChannels,
                        "The image is interleaved and has over CV_CN_MAX channels" );

                CV_CALL( cvInitMatHeader( mat, img->roi->height, img->roi->width,
                                          type, img->imageData +
                                          img->roi->yOffset*img->widthStep +
                                          img->roi->xOffset*CV_ELEM_SIZE(type),
                                          img->widthStep ));
            }
        }
        else
        {
            int type = CV_MAKETYPE( depth, img->nChannels );

            if( order != IPL_DATA_ORDER_PIXEL )
                CV_ERROR( CV_StsBadFlag, "Pixel order should be used with coi == 0" );

            CV_CALL( cvInitMatHeader( mat, img->height, img->width, type,
                                      img->imageData, img->widthStep ));
        }

        result = mat;
    }
    else if( allowND && CV_IS_MATND_HDR(src) )
    {
        CvMatND* matnd = (CvMatND*)src;
        int i;
        int size1 = matnd->dim[0].size, size2 = 1;
        
        if( !src->data.ptr )
            CV_ERROR( CV_StsNullPtr, "Input array has NULL data pointer" );

        if( !CV_IS_MAT_CONT( matnd->type ))
            CV_ERROR( CV_StsBadArg, "Only continuous nD arrays are supported here" );

        if( matnd->dims > 2 )
            for( i = 1; i < matnd->dims; i++ )
                size2 *= matnd->dim[i].size;
        else
            size2 = matnd->dims == 1 ? 1 : matnd->dim[1].size;

        mat->refcount = 0;
        mat->hdr_refcount = 0;
        mat->data.ptr = matnd->data.ptr;
        mat->rows = size1;
        mat->cols = size2;
        mat->type = CV_MAT_TYPE(matnd->type) | CV_MAT_MAGIC_VAL | CV_MAT_CONT_FLAG;
        mat->step = size2*CV_ELEM_SIZE(matnd->type);
        mat->step &= size1 > 1 ? -1 : 0;

        icvCheckHuge( mat );
        result = mat;
    }
    else
    {
        CV_ERROR( CV_StsBadFlag, "Unrecognized or unsupported array type" );
    }

    __END__;

    if( pCOI )
        *pCOI = coi;

    return result;
}

 CvSize  cvGetMatSize( const CvMat* mat )
{
    CvSize size = { mat->width, mat->height };
    return size;
}

 /****************************************************************************************\
*                                     Matrix transpose                                   *
\****************************************************************************************/

/////////////////// macros for inplace transposition of square matrix ////////////////////

#define ICV_DEF_TRANSP_INP_CASE_C1( \
    arrtype, len )                  \
{                                   \
    arrtype* arr1 = arr;            \
    step /= sizeof(arr[0]);         \
                                    \
    while( --len )                  \
    {                               \
        arr += step, arr1++;        \
        arrtype* arr2 = arr;        \
        arrtype* arr3 = arr1;       \
                                    \
        do                          \
        {                           \
            arrtype t0 = arr2[0];   \
            arrtype t1 = arr3[0];   \
            arr2[0] = t1;           \
            arr3[0] = t0;           \
                                    \
            arr2++;                 \
            arr3 += step;           \
        }                           \
        while( arr2 != arr3  );     \
    }                               \
}


#define ICV_DEF_TRANSP_INP_CASE_C3( \
    arrtype, len )                  \
{                                   \
    arrtype* arr1 = arr;            \
    int y;                          \
    step /= sizeof(arr[0]);         \
                                    \
    for( y = 1; y < len; y++ )      \
    {                               \
        arr += step, arr1 += 3;     \
        arrtype* arr2 = arr;        \
        arrtype* arr3 = arr1;       \
                                    \
        for( ; arr2!=arr3; arr2+=3, \
                        arr3+=step )\
        {                           \
            arrtype t0 = arr2[0];   \
            arrtype t1 = arr3[0];   \
            arr2[0] = t1;           \
            arr3[0] = t0;           \
            t0 = arr2[1];           \
            t1 = arr3[1];           \
            arr2[1] = t1;           \
            arr3[1] = t0;           \
            t0 = arr2[2];           \
            t1 = arr3[2];           \
            arr2[2] = t1;           \
            arr3[2] = t0;           \
        }                           \
    }                               \
}


#define ICV_DEF_TRANSP_INP_CASE_C4( \
    arrtype, len )                  \
{                                   \
    arrtype* arr1 = arr;            \
    int y;                          \
    step /= sizeof(arr[0]);         \
                                    \
    for( y = 1; y < len; y++ )      \
    {                               \
        arr += step, arr1 += 4;     \
        arrtype* arr2 = arr;        \
        arrtype* arr3 = arr1;       \
                                    \
        for( ; arr2!=arr3; arr2+=4, \
                        arr3+=step )\
        {                           \
            arrtype t0 = arr2[0];   \
            arrtype t1 = arr3[0];   \
            arr2[0] = t1;           \
            arr3[0] = t0;           \
            t0 = arr2[1];           \
            t1 = arr3[1];           \
            arr2[1] = t1;           \
            arr3[1] = t0;           \
            t0 = arr2[2];           \
            t1 = arr3[2];           \
            arr2[2] = t1;           \
            arr3[2] = t0;           \
            t0 = arr2[3];           \
            t1 = arr3[3];           \
            arr2[3] = t1;           \
            arr3[3] = t0;           \
        }                           \
    }                               \
}


//////////////// macros for non-inplace transposition of rectangular matrix //////////////

#define ICV_DEF_TRANSP_CASE_C1( arrtype )       \
{                                               \
    int x, y;                                   \
    srcstep /= sizeof(src[0]);                  \
    dststep /= sizeof(dst[0]);                  \
                                                \
    for( y = 0; y <= size.height - 2; y += 2,   \
                src += 2*srcstep, dst += 2 )    \
    {                                           \
        const arrtype* src1 = src + srcstep;    \
        arrtype* dst1 = dst;                    \
                                                \
        for( x = 0; x <= size.width - 2;        \
                x += 2, dst1 += dststep )       \
        {                                       \
            arrtype t0 = src[x];                \
            arrtype t1 = src1[x];               \
            dst1[0] = t0;                       \
            dst1[1] = t1;                       \
            dst1 += dststep;                    \
                                                \
            t0 = src[x + 1];                    \
            t1 = src1[x + 1];                   \
            dst1[0] = t0;                       \
            dst1[1] = t1;                       \
        }                                       \
                                                \
        if( x < size.width )                    \
        {                                       \
            arrtype t0 = src[x];                \
            arrtype t1 = src1[x];               \
            dst1[0] = t0;                       \
            dst1[1] = t1;                       \
        }                                       \
    }                                           \
                                                \
    if( y < size.height )                       \
    {                                           \
        arrtype* dst1 = dst;                    \
        for( x = 0; x <= size.width - 2;        \
                x += 2, dst1 += 2*dststep )     \
        {                                       \
            arrtype t0 = src[x];                \
            arrtype t1 = src[x + 1];            \
            dst1[0] = t0;                       \
            dst1[dststep] = t1;                 \
        }                                       \
                                                \
        if( x < size.width )                    \
        {                                       \
            arrtype t0 = src[x];                \
            dst1[0] = t0;                       \
        }                                       \
    }                                           \
}


#define ICV_DEF_TRANSP_CASE_C3( arrtype )       \
{                                               \
    size.width *= 3;                            \
    srcstep /= sizeof(src[0]);                  \
    dststep /= sizeof(dst[0]);                  \
                                                \
    for( ; size.height--; src+=srcstep, dst+=3 )\
    {                                           \
        int x;                                  \
        arrtype* dst1 = dst;                    \
                                                \
        for( x = 0; x < size.width; x += 3,     \
                            dst1 += dststep )   \
        {                                       \
            arrtype t0 = src[x];                \
            arrtype t1 = src[x + 1];            \
            arrtype t2 = src[x + 2];            \
                                                \
            dst1[0] = t0;                       \
            dst1[1] = t1;                       \
            dst1[2] = t2;                       \
        }                                       \
    }                                           \
}


#define ICV_DEF_TRANSP_CASE_C4( arrtype )       \
{                                               \
    size.width *= 4;                            \
    srcstep /= sizeof(src[0]);                  \
    dststep /= sizeof(dst[0]);                  \
                                                \
    for( ; size.height--; src+=srcstep, dst+=4 )\
    {                                           \
        int x;                                  \
        arrtype* dst1 = dst;                    \
                                                \
        for( x = 0; x < size.width; x += 4,     \
                            dst1 += dststep )   \
        {                                       \
            arrtype t0 = src[x];                \
            arrtype t1 = src[x + 1];            \
                                                \
            dst1[0] = t0;                       \
            dst1[1] = t1;                       \
                                                \
            t0 = src[x + 2];                    \
            t1 = src[x + 3];                    \
                                                \
            dst1[2] = t0;                       \
            dst1[3] = t1;                       \
        }                                       \
    }                                           \
}


#define ICV_DEF_TRANSP_INP_FUNC( flavor, arrtype, cn )      \
static CvStatus CV_STDCALL                                  \
icvTranspose_##flavor( arrtype* arr, int step, CvSize size )\
{                                                           \
    assert( size.width == size.height );                    \
                                                            \
    ICV_DEF_TRANSP_INP_CASE_C##cn( arrtype, size.width )    \
    return CV_OK;                                           \
}


#define ICV_DEF_TRANSP_FUNC( flavor, arrtype, cn )          \
static CvStatus CV_STDCALL                                  \
icvTranspose_##flavor( const arrtype* src, int srcstep,     \
                    arrtype* dst, int dststep, CvSize size )\
{                                                           \
    ICV_DEF_TRANSP_CASE_C##cn( arrtype )                    \
    return CV_OK;                                           \
}


ICV_DEF_TRANSP_INP_FUNC( 8u_C1IR, uchar, 1 )
ICV_DEF_TRANSP_INP_FUNC( 8u_C2IR, ushort, 1 )
ICV_DEF_TRANSP_INP_FUNC( 8u_C3IR, uchar, 3 )
ICV_DEF_TRANSP_INP_FUNC( 16u_C2IR, int, 1 )
ICV_DEF_TRANSP_INP_FUNC( 16u_C3IR, ushort, 3 )
ICV_DEF_TRANSP_INP_FUNC( 32s_C2IR, int64, 1 )
ICV_DEF_TRANSP_INP_FUNC( 32s_C3IR, int, 3 )
ICV_DEF_TRANSP_INP_FUNC( 64s_C2IR, int, 4 )
ICV_DEF_TRANSP_INP_FUNC( 64s_C3IR, int64, 3 )
ICV_DEF_TRANSP_INP_FUNC( 64s_C4IR, int64, 4 )


ICV_DEF_TRANSP_FUNC( 8u_C1R, uchar, 1 )
ICV_DEF_TRANSP_FUNC( 8u_C2R, ushort, 1 )
ICV_DEF_TRANSP_FUNC( 8u_C3R, uchar, 3 )
ICV_DEF_TRANSP_FUNC( 16u_C2R, int, 1 )
ICV_DEF_TRANSP_FUNC( 16u_C3R, ushort, 3 )
ICV_DEF_TRANSP_FUNC( 32s_C2R, int64, 1 )
ICV_DEF_TRANSP_FUNC( 32s_C3R, int, 3 )
ICV_DEF_TRANSP_FUNC( 64s_C2R, int, 4 )
ICV_DEF_TRANSP_FUNC( 64s_C3R, int64, 3 )
ICV_DEF_TRANSP_FUNC( 64s_C4R, int64, 4 )

CV_DEF_INIT_PIXSIZE_TAB_2D( Transpose, R )
CV_DEF_INIT_PIXSIZE_TAB_2D( Transpose, IR )

void cvTranspose( const CvArr* srcarr, CvArr* dstarr )
{
    static CvBtFuncTable tab, inp_tab;
    static int inittab = 0;
    
    CV_FUNCNAME( "cvTranspose" );

    __BEGIN__;

    CvMat sstub, *src = (CvMat*)srcarr;
    CvMat dstub, *dst = (CvMat*)dstarr;
    CvSize size;
    int type, pix_size;

    if( !inittab )
    {
        icvInitTransposeIRTable( &inp_tab );
        icvInitTransposeRTable( &tab );
        inittab = 1;
    }

    if( !CV_IS_MAT( src ))
    {
        int coi = 0;
        CV_CALL( src = cvGetMat( src, &sstub, &coi,0 ));
        if( coi != 0 )
            CV_ERROR( CV_BadCOI, "coi is not supported" );
    }

    type = CV_MAT_TYPE( src->type );
    pix_size = CV_ELEM_SIZE(type);
    size = cvGetMatSize( src );

    if( dstarr == srcarr )
    {
        dst = src; 
    }
    else
    {
        if( !CV_IS_MAT( dst ))
        {
            int coi = 0;
            CV_CALL( dst = cvGetMat( dst, &dstub, &coi,0 ));

            if( coi != 0 )
            CV_ERROR( CV_BadCOI, "coi is not supported" );
        }

        if( !CV_ARE_TYPES_EQ( src, dst ))
            CV_ERROR( CV_StsUnmatchedFormats, "" );

        if( size.width != dst->height || size.height != dst->width )
            CV_ERROR( CV_StsUnmatchedSizes, "" );
    }

    if( src->data.ptr == dst->data.ptr )
    {
        if( size.width == size.height )
        {
            CvFunc2D_1A func = (CvFunc2D_1A)(inp_tab.fn_2d[pix_size]);

            if( !func )
                CV_ERROR( CV_StsUnsupportedFormat, "" );

            IPPI_CALL( func( src->data.ptr, src->step, size ));
        }
        else
        {
            if( size.width != 1 && size.height != 1 )
                CV_ERROR( CV_StsBadSize,
                    "Rectangular matrix can not be transposed inplace" );
            
            if( !CV_IS_MAT_CONT( src->type & dst->type ))
                CV_ERROR( CV_StsBadFlag, "In case of inplace column/row transposition "
                                       "both source and destination must be continuous" );

            if( dst == src )
            {
                int t;
                CV_SWAP( dst->width, dst->height, t );
                dst->step = dst->height == 1 ? 0 : pix_size;
            }
        }
    }
    else
    {
        CvFunc2D_2A func = (CvFunc2D_2A)(tab.fn_2d[pix_size]);

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        IPPI_CALL( func( src->data.ptr, src->step,
                         dst->data.ptr, dst->step, size ));
    }

    __END__;
}

#define MAX_ITERS  30

static  void
icvMatrAXPY_64f( int m, int n, const double* x, int dx,
                 const double* a, double* y, int dy )
{
    int i, j;

    for( i = 0; i < m; i++, x += dx, y += dy )
    {
        double s = a[i];

        for( j = 0; j <= n - 4; j += 4 )
        {
            double t0 = y[j]   + s*x[j];
            double t1 = y[j+1] + s*x[j+1];
            y[j]   = t0;
            y[j+1] = t1;
            t0 = y[j+2] + s*x[j+2];
            t1 = y[j+3] + s*x[j+3];
            y[j+2] = t0;
            y[j+3] = t1;
        }

        for( ; j < n; j++ ) y[j] += s*x[j];
    }
}


/* y[1:m,-1] = h*y[1:m,0:n]*x[0:1,0:n]'*x[-1]  (this is used for U&V reconstruction)
   y[1:m,0:n] += h*y[1:m,0:n]*x[0:1,0:n]'*x[0:1,0:n] */
static void
icvMatrAXPY3_64f( int m, int n, const double* x, int l, double* y, double h )
{
    int i, j;

    for( i = 1; i < m; i++ )
    {
        double s = 0;

        y += l;

        for( j = 0; j <= n - 4; j += 4 )
            s += x[j]*y[j] + x[j+1]*y[j+1] + x[j+2]*y[j+2] + x[j+3]*y[j+3];

        for( ; j < n; j++ )  s += x[j]*y[j];

        s *= h;
        y[-1] = s*x[-1];

        for( j = 0; j <= n - 4; j += 4 )
        {
            double t0 = y[j]   + s*x[j];
            double t1 = y[j+1] + s*x[j+1];
            y[j]   = t0;
            y[j+1] = t1;
            t0 = y[j+2] + s*x[j+2];
            t1 = y[j+3] + s*x[j+3];
            y[j+2] = t0;
            y[j+3] = t1;
        }

        for( ; j < n; j++ ) y[j] += s*x[j];
    }
}

#define icvGivens_32f( n, x, y, c, s ) \
{                                      \
    int _i;                            \
    float* _x = (x);                   \
    float* _y = (y);                   \
                                       \
    for( _i = 0; _i < n; _i++ )        \
    {                                  \
        double t0 = _x[_i];            \
        double t1 = _y[_i];            \
        _x[_i] = (float)(t0*c + t1*s); \
        _y[_i] = (float)(-t0*s + t1*c);\
    }                                  \
}

static  void
icvMatrAXPY_32f( int m, int n, const float* x, int dx,
                 const float* a, float* y, int dy )
{
    int i, j;

    for( i = 0; i < m; i++, x += dx, y += dy )
    {
        double s = a[i];

        for( j = 0; j <= n - 4; j += 4 )
        {
            double t0 = y[j]   + s*x[j];
            double t1 = y[j+1] + s*x[j+1];
            y[j]   = (float)t0;
            y[j+1] = (float)t1;
            t0 = y[j+2] + s*x[j+2];
            t1 = y[j+3] + s*x[j+3];
            y[j+2] = (float)t0;
            y[j+3] = (float)t1;
        }

        for( ; j < n; j++ )
            y[j] = (float)(y[j] + s*x[j]);
    }
}

static double
pythag( double a, double b )
{
    a = fabs( a );
    b = fabs( b );
    if( a > b )
    {
        b /= a;
        a *= sqrt( 1. + b * b );
    }
    else if( b != 0 )
    {
        a /= b;
        a = b * sqrt( 1. + a * a );
    }

    return a;
}

static void icvSVD_64f( double* a, int lda, int m, int n, double* w, double* uT, int lduT, int nu, double* vT, int ldvT, double* buffer )
{
    double* e;
    double* temp;
    double *w1, *e1;
    double *hv;
    double ku0 = 0, kv0 = 0;
    double anorm = 0;
    double *a1, *u0 = uT, *v0 = vT;
    double scale, h;
    int i, j, k, l;
    int nm, m1, n1;
    int nv = n;
    int iters = 0;
    double* hv0 = (double*)cvStackAlloc( (m+2)*sizeof(hv0[0])) + 1; 

    e = buffer;
    w1 = w;
    e1 = e + 1;
    nm = n;
    
    temp = buffer + nm;

    memset( w, 0, nm * sizeof( w[0] ));
    memset( e, 0, nm * sizeof( e[0] ));

    m1 = m;
    n1 = n;

    /* transform a to bi-diagonal form */
    for( ;; )
    {
        int update_u;
        int update_v;
        
        if( m1 == 0 )
            break;

        scale = h = 0;
        update_u = uT && m1 > m - nu;
        hv = update_u ? uT : hv0;

        for( j = 0, a1 = a; j < m1; j++, a1 += lda )
        {
            double t = a1[0];
            scale += fabs( hv[j] = t );
        }

        if( scale != 0 )
        {
            double f = 1./scale, g, s = 0;

            for( j = 0; j < m1; j++ )
            {
                double t = (hv[j] *= f);
                s += t * t;
            }

            g = sqrt( s );
            f = hv[0];
            if( f >= 0 )
                g = -g;
            hv[0] = f - g;
            h = 1. / (f * g - s);

            memset( temp, 0, n1 * sizeof( temp[0] ));

            /* calc temp[0:n-i] = a[i:m,i:n]'*hv[0:m-i] */
            icvMatrAXPY_64f( m1, n1 - 1, a + 1, lda, hv, temp + 1, 0 );
            for( k = 1; k < n1; k++ ) temp[k] *= h;

            /* modify a: a[i:m,i:n] = a[i:m,i:n] + hv[0:m-i]*temp[0:n-i]' */
            icvMatrAXPY_64f( m1, n1 - 1, temp + 1, 0, hv, a + 1, lda );
            *w1 = g*scale;
        }
        w1++;

        /* store -2/(hv'*hv) */
        if( update_u )
        {
            if( m1 == m )
                ku0 = h;
            else
                hv[-1] = h;
        }

        a++;
        n1--;
        if( vT )
            vT += ldvT + 1;

        if( n1 == 0 )
            break;

        scale = h = 0;
        update_v = vT && n1 > n - nv;

        hv = update_v ? vT : hv0;

        for( j = 0; j < n1; j++ )
        {
            double t = a[j];
            scale += fabs( hv[j] = t );
        }

        if( scale != 0 )
        {
            double f = 1./scale, g, s = 0;

            for( j = 0; j < n1; j++ )
            {
                double t = (hv[j] *= f);
                s += t * t;
            }

            g = sqrt( s );
            f = hv[0];
            if( f >= 0 )
                g = -g;
            hv[0] = f - g;
            h = 1. / (f * g - s);
            hv[-1] = 0.;

            /* update a[i:m:i+1:n] = a[i:m,i+1:n] + (a[i:m,i+1:n]*hv[0:m-i])*... */
            icvMatrAXPY3_64f( m1, n1, hv, lda, a, h );

            *e1 = g*scale;
        }
        e1++;

        /* store -2/(hv'*hv) */
        if( update_v )
        {
            if( n1 == n )
                kv0 = h;
            else
                hv[-1] = h;
        }

        a += lda;
        m1--;
        if( uT )
            uT += lduT + 1;
    }

    m1 -= m1 != 0;
    n1 -= n1 != 0;

    /* accumulate left transformations */
    if( uT )
    {
        m1 = m - m1;
        uT = u0 + m1 * lduT;
        for( i = m1; i < nu; i++, uT += lduT )
        {
            memset( uT + m1, 0, (m - m1) * sizeof( uT[0] ));
            uT[i] = 1.;
        }

        for( i = m1 - 1; i >= 0; i-- )
        {
            double s;
            int lh = nu - i;

            l = m - i;

            hv = u0 + (lduT + 1) * i;
            h = i == 0 ? ku0 : hv[-1];

            assert( h <= 0 );

            if( h != 0 )
            {
                uT = hv;
                icvMatrAXPY3_64f( lh, l-1, hv+1, lduT, uT+1, h );

                s = hv[0] * h;
                for( k = 0; k < l; k++ ) hv[k] *= s;
                hv[0] += 1;
            }
            else
            {
                for( j = 1; j < l; j++ )
                    hv[j] = 0;
                for( j = 1; j < lh; j++ )
                    hv[j * lduT] = 0;
                hv[0] = 1;
            }
        }
        uT = u0;
    }

    /* accumulate right transformations */
    if( vT )
    {
        n1 = n - n1;
        vT = v0 + n1 * ldvT;
        for( i = n1; i < nv; i++, vT += ldvT )
        {
            memset( vT + n1, 0, (n - n1) * sizeof( vT[0] ));
            vT[i] = 1.;
        }

        for( i = n1 - 1; i >= 0; i-- )
        {
            double s;
            int lh = nv - i;

            l = n - i;
            hv = v0 + (ldvT + 1) * i;
            h = i == 0 ? kv0 : hv[-1];

            assert( h <= 0 );

            if( h != 0 )
            {
                vT = hv;
                icvMatrAXPY3_64f( lh, l-1, hv+1, ldvT, vT+1, h );

                s = hv[0] * h;
                for( k = 0; k < l; k++ ) hv[k] *= s;
                hv[0] += 1;
            }
            else
            {
                for( j = 1; j < l; j++ )
                    hv[j] = 0;
                for( j = 1; j < lh; j++ )
                    hv[j * ldvT] = 0;
                hv[0] = 1;
            }
        }
        vT = v0;
    }

    for( i = 0; i < nm; i++ )
    {
        double tnorm = fabs( w[i] );
        tnorm += fabs( e[i] );

        if( anorm < tnorm )
            anorm = tnorm;
    }

    anorm *= DBL_EPSILON;

    /* diagonalization of the bidiagonal form */
    for( k = nm - 1; k >= 0; k-- )
    {
        double z = 0;
        iters = 0;

        for( ;; )               /* do iterations */
        {
            double c, s, f, g, x, y;
            int flag = 0;

            /* test for splitting */
            for( l = k; l >= 0; l-- )
            {
                if( fabs(e[l]) <= anorm )
                {
                    flag = 1;
                    break;
                }
                assert( l > 0 );
                if( fabs(w[l - 1]) <= anorm )
                    break;
            }

            if( !flag )
            {
                c = 0;
                s = 1;

                for( i = l; i <= k; i++ )
                {
                    f = s * e[i];

                    e[i] *= c;

                    if( anorm + fabs( f ) == anorm )
                        break;

                    g = w[i];
                    h = pythag( f, g );
                    w[i] = h;
                    c = g / h;
                    s = -f / h;

                    if( uT )
                        icvGivens_64f( m, uT + lduT * (l - 1), uT + lduT * i, c, s );
                }
            }

            z = w[k];
            if( l == k || iters++ == MAX_ITERS )
                break;

            /* shift from bottom 2x2 minor */
            x = w[l];
            y = w[k - 1];
            g = e[k - 1];
            h = e[k];
            f = 0.5 * (((g + z) / h) * ((g - z) / y) + y / h - h / y);
            g = pythag( f, 1 );
            if( f < 0 )
                g = -g;
            f = x - (z / x) * z + (h / x) * (y / (f + g) - h);
            /* next QR transformation */
            c = s = 1;

            for( i = l + 1; i <= k; i++ )
            {
                g = e[i];
                y = w[i];
                h = s * g;
                g *= c;
                z = pythag( f, h );
                e[i - 1] = z;
                c = f / z;
                s = h / z;
                f = x * c + g * s;
                g = -x * s + g * c;
                h = y * s;
                y *= c;

                if( vT )
                    icvGivens_64f( n, vT + ldvT * (i - 1), vT + ldvT * i, c, s );

                z = pythag( f, h );
                w[i - 1] = z;

                /* rotation can be arbitrary if z == 0 */
                if( z != 0 )
                {
                    c = f / z;
                    s = h / z;
                }
                f = c * g + s * y;
                x = -s * g + c * y;

                if( uT )
                    icvGivens_64f( m, uT + lduT * (i - 1), uT + lduT * i, c, s );
            }

            e[l] = 0;
            e[k] = f;
            w[k] = x;
        }                       /* end of iteration loop */

        if( iters > MAX_ITERS )
            break;

        if( z < 0 )
        {
            w[k] = -z;
            if( vT )
            {
                for( j = 0; j < n; j++ )
                    vT[j + k * ldvT] = -vT[j + k * ldvT];
            }
        }
    }                           /* end of diagonalization loop */

    /* sort singular values and corresponding values */
    for( i = 0; i < nm; i++ )
    {
        k = i;
        for( j = i + 1; j < nm; j++ )
            if( w[k] < w[j] )
                k = j;

        if( k != i )
        {
            double t;
            CV_SWAP( w[i], w[k], t );

            if( vT )
                for( j = 0; j < n; j++ )
                    CV_SWAP( vT[j + ldvT*k], vT[j + ldvT*i], t );

            if( uT )
                for( j = 0; j < m; j++ )
                    CV_SWAP( uT[j + lduT*k], uT[j + lduT*i], t );
        }
    }
}

static void
icvMatrAXPY3_32f( int m, int n, const float* x, int l, float* y, double h )
{
    int i, j;

    for( i = 1; i < m; i++ )
    {
        double s = 0;
        y += l;

        for( j = 0; j <= n - 4; j += 4 )
            s += x[j]*y[j] + x[j+1]*y[j+1] + x[j+2]*y[j+2] + x[j+3]*y[j+3];

        for( ; j < n; j++ )  s += x[j]*y[j];

        s *= h;
        y[-1] = (float)(s*x[-1]);

        for( j = 0; j <= n - 4; j += 4 )
        {
            double t0 = y[j]   + s*x[j];
            double t1 = y[j+1] + s*x[j+1];
            y[j]   = (float)t0;
            y[j+1] = (float)t1;
            t0 = y[j+2] + s*x[j+2];
            t1 = y[j+3] + s*x[j+3];
            y[j+2] = (float)t0;
            y[j+3] = (float)t1;
        }

        for( ; j < n; j++ ) y[j] = (float)(y[j] + s*x[j]);
    }
}

static void icvSVD_32f( float* a, int lda, int m, int n, float* w, float* uT, int lduT, int nu, float* vT, int ldvT, float* buffer )
{
    float* e;
    float* temp;
    float *w1, *e1;
    float *hv;
    double ku0 = 0, kv0 = 0;
    double anorm = 0;
    float *a1, *u0 = uT, *v0 = vT;
    double scale, h;
    int i, j, k, l;
    int nm, m1, n1;
    int nv = n;
    int iters = 0;
    float* hv0 = (float*)cvStackAlloc( (m+2)*sizeof(hv0[0])) + 1;

    e = buffer;

    w1 = w;
    e1 = e + 1;
    nm = n;
    
    temp = buffer + nm;

    memset( w, 0, nm * sizeof( w[0] ));
    memset( e, 0, nm * sizeof( e[0] ));

    m1 = m;
    n1 = n;

    /* transform a to bi-diagonal form */
    for( ;; )
    {
        int update_u;
        int update_v;
        
        if( m1 == 0 )
            break;

        scale = h = 0;

        update_u = uT && m1 > m - nu;
        hv = update_u ? uT : hv0;

        for( j = 0, a1 = a; j < m1; j++, a1 += lda )
        {
            double t = a1[0];
            scale += fabs( hv[j] = (float)t );
        }

        if( scale != 0 )
        {
            double f = 1./scale, g, s = 0;

            for( j = 0; j < m1; j++ )
            {
                double t = (hv[j] = (float)(hv[j]*f));
                s += t * t;
            }

            g = sqrt( s );
            f = hv[0];
            if( f >= 0 )
                g = -g;
            hv[0] = (float)(f - g);
            h = 1. / (f * g - s);

            memset( temp, 0, n1 * sizeof( temp[0] ));

            /* calc temp[0:n-i] = a[i:m,i:n]'*hv[0:m-i] */
            icvMatrAXPY_32f( m1, n1 - 1, a + 1, lda, hv, temp + 1, 0 );

            for( k = 1; k < n1; k++ ) temp[k] = (float)(temp[k]*h);

            /* modify a: a[i:m,i:n] = a[i:m,i:n] + hv[0:m-i]*temp[0:n-i]' */
            icvMatrAXPY_32f( m1, n1 - 1, temp + 1, 0, hv, a + 1, lda );
            *w1 = (float)(g*scale);
        }
        w1++;
        
        /* store -2/(hv'*hv) */
        if( update_u )
        {
            if( m1 == m )
                ku0 = h;
            else
                hv[-1] = (float)h;
        }

        a++;
        n1--;
        if( vT )
            vT += ldvT + 1;

        if( n1 == 0 )
            break;

        scale = h = 0;
        update_v = vT && n1 > n - nv;
        hv = update_v ? vT : hv0;

        for( j = 0; j < n1; j++ )
        {
            double t = a[j];
            scale += fabs( hv[j] = (float)t );
        }

        if( scale != 0 )
        {
            double f = 1./scale, g, s = 0;

            for( j = 0; j < n1; j++ )
            {
                double t = (hv[j] = (float)(hv[j]*f));
                s += t * t;
            }

            g = sqrt( s );
            f = hv[0];
            if( f >= 0 )
                g = -g;
            hv[0] = (float)(f - g);
            h = 1. / (f * g - s);
            hv[-1] = 0.f;

            /* update a[i:m:i+1:n] = a[i:m,i+1:n] + (a[i:m,i+1:n]*hv[0:m-i])*... */
            icvMatrAXPY3_32f( m1, n1, hv, lda, a, h );

            *e1 = (float)(g*scale);
        }
        e1++;

        /* store -2/(hv'*hv) */
        if( update_v )
        {
            if( n1 == n )
                kv0 = h;
            else
                hv[-1] = (float)h;
        }

        a += lda;
        m1--;
        if( uT )
            uT += lduT + 1;
    }

    m1 -= m1 != 0;
    n1 -= n1 != 0;

    /* accumulate left transformations */
    if( uT )
    {
        m1 = m - m1;
        uT = u0 + m1 * lduT;
        for( i = m1; i < nu; i++, uT += lduT )
        {
            memset( uT + m1, 0, (m - m1) * sizeof( uT[0] ));
            uT[i] = 1.;
        }

        for( i = m1 - 1; i >= 0; i-- )
        {
            double s;
            int lh = nu - i;

            l = m - i;

            hv = u0 + (lduT + 1) * i;
            h = i == 0 ? ku0 : hv[-1];

            assert( h <= 0 );

            if( h != 0 )
            {
                uT = hv;
                icvMatrAXPY3_32f( lh, l-1, hv+1, lduT, uT+1, h );

                s = hv[0] * h;
                for( k = 0; k < l; k++ ) hv[k] = (float)(hv[k]*s);
                hv[0] += 1;
            }
            else
            {
                for( j = 1; j < l; j++ )
                    hv[j] = 0;
                for( j = 1; j < lh; j++ )
                    hv[j * lduT] = 0;
                hv[0] = 1;
            }
        }
        uT = u0;
    }

    /* accumulate right transformations */
    if( vT )
    {
        n1 = n - n1;
        vT = v0 + n1 * ldvT;
        for( i = n1; i < nv; i++, vT += ldvT )
        {
            memset( vT + n1, 0, (n - n1) * sizeof( vT[0] ));
            vT[i] = 1.;
        }

        for( i = n1 - 1; i >= 0; i-- )
        {
            double s;
            int lh = nv - i;

            l = n - i;
            hv = v0 + (ldvT + 1) * i;
            h = i == 0 ? kv0 : hv[-1];

            assert( h <= 0 );

            if( h != 0 )
            {
                vT = hv;
                icvMatrAXPY3_32f( lh, l-1, hv+1, ldvT, vT+1, h );

                s = hv[0] * h;
                for( k = 0; k < l; k++ ) hv[k] = (float)(hv[k]*s);
                hv[0] += 1;
            }
            else
            {
                for( j = 1; j < l; j++ )
                    hv[j] = 0;
                for( j = 1; j < lh; j++ )
                    hv[j * ldvT] = 0;
                hv[0] = 1;
            }
        }
        vT = v0;
    }

    for( i = 0; i < nm; i++ )
    {
        double tnorm = fabs( w[i] );
        tnorm += fabs( e[i] );

        if( anorm < tnorm )
            anorm = tnorm;
    }

    anorm *= FLT_EPSILON;

    /* diagonalization of the bidiagonal form */
    for( k = nm - 1; k >= 0; k-- )
    {
        double z = 0;
        iters = 0;

        for( ;; )               /* do iterations */
        {
            double c, s, f, g, x, y;
            int flag = 0;

            /* test for splitting */
            for( l = k; l >= 0; l-- )
            {
                if( fabs( e[l] ) <= anorm )
                {
                    flag = 1;
                    break;
                }
                assert( l > 0 );
                if( fabs( w[l - 1] ) <= anorm )
                    break;
            }

            if( !flag )
            {
                c = 0;
                s = 1;

                for( i = l; i <= k; i++ )
                {
                    f = s * e[i];
                    e[i] = (float)(e[i]*c);

                    if( anorm + fabs( f ) == anorm )
                        break;

                    g = w[i];
                    h = pythag( f, g );
                    w[i] = (float)h;
                    c = g / h;
                    s = -f / h;

                    if( uT )
                        icvGivens_32f( m, uT + lduT * (l - 1), uT + lduT * i, c, s );
                }
            }

            z = w[k];
            if( l == k || iters++ == MAX_ITERS )
                break;

            /* shift from bottom 2x2 minor */
            x = w[l];
            y = w[k - 1];
            g = e[k - 1];
            h = e[k];
            f = 0.5 * (((g + z) / h) * ((g - z) / y) + y / h - h / y);
            g = pythag( f, 1 );
            if( f < 0 )
                g = -g;
            f = x - (z / x) * z + (h / x) * (y / (f + g) - h);
            /* next QR transformation */
            c = s = 1;

            for( i = l + 1; i <= k; i++ )
            {
                g = e[i];
                y = w[i];
                h = s * g;
                g *= c;
                z = pythag( f, h );
                e[i - 1] = (float)z;
                c = f / z;
                s = h / z;
                f = x * c + g * s;
                g = -x * s + g * c;
                h = y * s;
                y *= c;

                if( vT )
                    icvGivens_32f( n, vT + ldvT * (i - 1), vT + ldvT * i, c, s );

                z = pythag( f, h );
                w[i - 1] = (float)z;

                /* rotation can be arbitrary if z == 0 */
                if( z != 0 )
                {
                    c = f / z;
                    s = h / z;
                }
                f = c * g + s * y;
                x = -s * g + c * y;

                if( uT )
                    icvGivens_32f( m, uT + lduT * (i - 1), uT + lduT * i, c, s );
            }

            e[l] = 0;
            e[k] = (float)f;
            w[k] = (float)x;
        }                       /* end of iteration loop */

        if( iters > MAX_ITERS )
            break;

        if( z < 0 )
        {
            w[k] = (float)(-z);
            if( vT )
            {
                for( j = 0; j < n; j++ )
                    vT[j + k * ldvT] = -vT[j + k * ldvT];
            }
        }
    }                           /* end of diagonalization loop */

    /* sort singular values and corresponding vectors */
    for( i = 0; i < nm; i++ )
    {
        k = i;
        for( j = i + 1; j < nm; j++ )
            if( w[k] < w[j] )
                k = j;

        if( k != i )
        {
            float t;
            CV_SWAP( w[i], w[k], t );

            if( vT )
                for( j = 0; j < n; j++ )
                    CV_SWAP( vT[j + ldvT*k], vT[j + ldvT*i], t );

            if( uT )
                for( j = 0; j < m; j++ )
                    CV_SWAP( uT[j + lduT*k], uT[j + lduT*i], t );
        }
    }
}

CvMat* cvGetRows( const CvArr* arr, CvMat* submat,int start_row, int end_row, int delta_row )
{
    CvMat* res = 0;
    
    CV_FUNCNAME( "cvGetRows" );

    __BEGIN__;

    CvMat stub, *mat = (CvMat*)arr;

    if( !CV_IS_MAT( mat ))
        CV_CALL( mat = cvGetMat( mat, &stub,NULL,0 ));

    if( !submat )
        CV_ERROR( CV_StsNullPtr, "" );

    if( (unsigned)start_row >= (unsigned)mat->rows ||
        (unsigned)end_row > (unsigned)mat->rows || delta_row <= 0 )
        CV_ERROR( CV_StsOutOfRange, "" );

    {
    /*
    int* refcount = mat->refcount;

    if( refcount )
        ++*refcount;

    cvDecRefData( submat );
    */
    if( delta_row == 1 )
    {
        submat->rows = end_row - start_row;
        submat->step = mat->step & (submat->rows > 1 ? -1 : 0);
    }
    else
    {
        submat->rows = (end_row - start_row + delta_row - 1)/delta_row;
        submat->step = mat->step * delta_row;
    }

    submat->cols = mat->cols;
    submat->step &= submat->rows > 1 ? -1 : 0;
    submat->data.ptr = mat->data.ptr + (size_t)start_row*mat->step;
    submat->type = (mat->type | (submat->step == 0 ? CV_MAT_CONT_FLAG : 0)) &
                   (delta_row != 1 ? ~CV_MAT_CONT_FLAG : -1);
    submat->refcount = 0;
    submat->hdr_refcount = 0;
    res = submat;
    }
    
    __END__;

    return res;
}

static CvStatus CV_STDCALL
icvSet_8u_C1R( uchar* dst, int dst_step, CvSize size,
               const void* scalar, int pix_size )
{
    int copy_len = 12*pix_size;
    uchar* dst_limit = dst + size.width;
    
    if( size.height-- )
    {
        while( dst + copy_len <= dst_limit )
        {
            memcpy( dst, scalar, copy_len );
            dst += copy_len;
        }

        memcpy( dst, scalar, dst_limit - dst );
    }

    if( size.height )
    {
        dst = dst_limit - size.width + dst_step;

        for( ; size.height--; dst += dst_step )
            memcpy( dst, dst - dst_step, size.width );
    }

    return CV_OK;
}

#define ICV_DEF_COPY_MASK_C1_CASE( type )   \
    for( i = 0; i <= size.width-2; i += 2 ) \
    {                                       \
        if( mask[i] )                       \
            dst[i] = src[i];                \
        if( mask[i+1] )                     \
            dst[i+1] = src[i+1];            \
    }                                       \
                                            \
    for( ; i < size.width; i++ )            \
    {                                       \
        if( mask[i] )                       \
            dst[i] = src[i];                \
    }

#define ICV_DEF_COPY_MASK_C3_CASE( type )   \
    for( i = 0; i < size.width; i++ )       \
        if( mask[i] )                       \
        {                                   \
            type t0 = src[i*3];             \
            type t1 = src[i*3+1];           \
            type t2 = src[i*3+2];           \
                                            \
            dst[i*3] = t0;                  \
            dst[i*3+1] = t1;                \
            dst[i*3+2] = t2;                \
        }



#define ICV_DEF_COPY_MASK_C4_CASE( type )   \
    for( i = 0; i < size.width; i++ )       \
        if( mask[i] )                       \
        {                                   \
            type t0 = src[i*4];             \
            type t1 = src[i*4+1];           \
            dst[i*4] = t0;                  \
            dst[i*4+1] = t1;                \
                                            \
            t0 = src[i*4+2];                \
            t1 = src[i*4+3];                \
            dst[i*4+2] = t0;                \
            dst[i*4+3] = t1;                \
        }


#define ICV_DEF_COPY_MASK_2D( name, type, cn )              \
IPCVAPI_IMPL( CvStatus,                                     \
name,( const type* src, int srcstep, type* dst, int dststep,\
       CvSize size, const uchar* mask, int maskstep ),      \
       (src, srcstep, dst, dststep, size, mask, maskstep))  \
{                                                           \
    srcstep /= sizeof(src[0]); dststep /= sizeof(dst[0]);   \
    for( ; size.height--; src += srcstep,                   \
            dst += dststep, mask += maskstep )              \
    {                                                       \
        int i;                                              \
        ICV_DEF_COPY_MASK_C##cn##_CASE( type )              \
    }                                                       \
                                                            \
    return  CV_OK;                                          \
}


#define ICV_DEF_SET_MASK_C1_CASE( type )    \
    for( i = 0; i <= size.width-2; i += 2 ) \
    {                                       \
        if( mask[i] )                       \
            dst[i] = s0;                    \
        if( mask[i+1] )                     \
            dst[i+1] = s0;                  \
    }                                       \
                                            \
    for( ; i < size.width; i++ )            \
    {                                       \
        if( mask[i] )                       \
            dst[i] = s0;                    \
    }


#define ICV_DEF_SET_MASK_C3_CASE( type )    \
    for( i = 0; i < size.width; i++ )       \
        if( mask[i] )                       \
        {                                   \
            dst[i*3] = s0;                  \
            dst[i*3+1] = s1;                \
            dst[i*3+2] = s2;                \
        }

#define ICV_DEF_SET_MASK_C4_CASE( type )    \
    for( i = 0; i < size.width; i++ )       \
        if( mask[i] )                       \
        {                                   \
            dst[i*4] = s0;                  \
            dst[i*4+1] = s1;                \
            dst[i*4+2] = s2;                \
            dst[i*4+3] = s3;                \
        }

#define ICV_DEF_SET_MASK_2D( name, type, cn )       \
IPCVAPI_IMPL( CvStatus,                             \
name,( type* dst, int dststep,                      \
       const uchar* mask, int maskstep,             \
       CvSize size, const type* scalar ),           \
       (dst, dststep, mask, maskstep, size, scalar))\
{                                                   \
    CV_UN_ENTRY_C##cn( type );                      \
    dststep /= sizeof(dst[0]);                      \
                                                    \
    for( ; size.height--; mask += maskstep,         \
                          dst += dststep )          \
    {                                               \
        int i;                                      \
        ICV_DEF_SET_MASK_C##cn##_CASE( type )       \
    }                                               \
                                                    \
    return CV_OK;                                   \
}


ICV_DEF_SET_MASK_2D( icvSet_8u_C1MR, uchar, 1 )
ICV_DEF_SET_MASK_2D( icvSet_16s_C1MR, ushort, 1 )
ICV_DEF_SET_MASK_2D( icvSet_8u_C3MR, uchar, 3 )
ICV_DEF_SET_MASK_2D( icvSet_8u_C4MR, int, 1 )
ICV_DEF_SET_MASK_2D( icvSet_16s_C3MR, ushort, 3 )
ICV_DEF_SET_MASK_2D( icvSet_16s_C4MR, int64, 1 )
ICV_DEF_SET_MASK_2D( icvSet_32f_C3MR, int, 3 )
ICV_DEF_SET_MASK_2D( icvSet_32f_C4MR, int, 4 )
ICV_DEF_SET_MASK_2D( icvSet_64s_C3MR, int64, 3 )
ICV_DEF_SET_MASK_2D( icvSet_64s_C4MR, int64, 4 )

ICV_DEF_COPY_MASK_2D( icvCopy_8u_C1MR, uchar, 1 )
ICV_DEF_COPY_MASK_2D( icvCopy_16s_C1MR, ushort, 1 )
ICV_DEF_COPY_MASK_2D( icvCopy_8u_C3MR, uchar, 3 )
ICV_DEF_COPY_MASK_2D( icvCopy_8u_C4MR, int, 1 )
ICV_DEF_COPY_MASK_2D( icvCopy_16s_C3MR, ushort, 3 )
ICV_DEF_COPY_MASK_2D( icvCopy_16s_C4MR, int64, 1 )
ICV_DEF_COPY_MASK_2D( icvCopy_32f_C3MR, int, 3 )
ICV_DEF_COPY_MASK_2D( icvCopy_32f_C4MR, int, 4 )
ICV_DEF_COPY_MASK_2D( icvCopy_64s_C3MR, int64, 3 )
ICV_DEF_COPY_MASK_2D( icvCopy_64s_C4MR, int64, 4 )

#define CV_DEF_INIT_COPYSET_TAB_2D( FUNCNAME, FLAG )                \
static void icvInit##FUNCNAME##FLAG##Table( CvBtFuncTable* table )  \
{                                                                   \
    table->fn_2d[1]  = (void*)icv##FUNCNAME##_8u_C1##FLAG;          \
    table->fn_2d[2]  = (void*)icv##FUNCNAME##_16s_C1##FLAG;         \
    table->fn_2d[3]  = (void*)icv##FUNCNAME##_8u_C3##FLAG;          \
    table->fn_2d[4]  = (void*)icv##FUNCNAME##_8u_C4##FLAG;          \
    table->fn_2d[6]  = (void*)icv##FUNCNAME##_16s_C3##FLAG;         \
    table->fn_2d[8]  = (void*)icv##FUNCNAME##_16s_C4##FLAG;         \
    table->fn_2d[12] = (void*)icv##FUNCNAME##_32f_C3##FLAG;         \
    table->fn_2d[16] = (void*)icv##FUNCNAME##_32f_C4##FLAG;         \
    table->fn_2d[24] = (void*)icv##FUNCNAME##_64s_C3##FLAG;         \
    table->fn_2d[32] = (void*)icv##FUNCNAME##_64s_C4##FLAG;         \
}

CV_DEF_INIT_COPYSET_TAB_2D( Set, MR )
CV_DEF_INIT_COPYSET_TAB_2D( Copy, MR )

void cvSet( void* arr, CvScalar value, const void* maskarr )
{
    static CvBtFuncTable setm_tab;
    static int inittab = 0;
    
    CV_FUNCNAME( "cvSet" );

    __BEGIN__;

    CvMat stub, *mat = (CvMat*)arr;
    int pix_size, type;
    double buf[12];
    int mat_step;
    CvSize size;

    if( !value.val[0] && !value.val[1] &&
        !value.val[2] && !value.val[3] && !maskarr )
    {
        cvZero( arr );
        EXIT;
    }

    if( !CV_IS_MAT(mat))
    {
        if( CV_IS_MATND(mat))
        {
            CvMatND nstub;
            CvNArrayIterator iterator;
            int pix_size1;
            
            CV_CALL( cvInitNArrayIterator( 1, &arr, maskarr, &nstub, &iterator,0 ));

            type = CV_MAT_TYPE(iterator.hdr[0]->type);
            pix_size1 = CV_ELEM_SIZE1(type);
            pix_size = pix_size1*CV_MAT_CN(type);

            CV_CALL( cvScalarToRawData( &value, buf, type, maskarr == 0 ));

            if( !maskarr )
            {
                iterator.size.width *= pix_size;
                do
                {
                    icvSet_8u_C1R( iterator.ptr[0], CV_STUB_STEP,
                                   iterator.size, buf, pix_size1 );
                }
                while( cvNextNArraySlice( &iterator ));
            }
            else
            {
                CvFunc2D_2A1P func = (CvFunc2D_2A1P)(setm_tab.fn_2d[pix_size]);
                if( !func )
                    CV_ERROR( CV_StsUnsupportedFormat, "" );

                do
                {
                    func( iterator.ptr[0], CV_STUB_STEP,
                          iterator.ptr[1], CV_STUB_STEP,
                          iterator.size, buf );
                }
                while( cvNextNArraySlice( &iterator ));
            }
            EXIT;
        }    
        else
        {
            int coi = 0;
            CV_CALL( mat = cvGetMat( mat, &stub, &coi,0 ));

            if( coi != 0 )
                CV_ERROR( CV_BadCOI, "" );
        }
    }

    type = CV_MAT_TYPE( mat->type );
    pix_size = CV_ELEM_SIZE(type);
    size = cvGetMatSize( mat );
    mat_step = mat->step;

    if( !maskarr )
    {
        if( CV_IS_MAT_CONT( mat->type ))
        {
            size.width *= size.height;
        
            if( size.width <= (int)(CV_MAX_INLINE_MAT_OP_SIZE*sizeof(double)))
            {
                if( type == CV_32FC1 )
                {
                    float* dstdata = (float*)(mat->data.ptr);
                    float val = (float)value.val[0];

                    do
                    {
                        dstdata[size.width-1] = val;
                    }
                    while( --size.width );

                    EXIT;
                }

                if( type == CV_64FC1 )
                {
                    double* dstdata = (double*)(mat->data.ptr);
                    double val = value.val[0];

                    do
                    {
                        dstdata[size.width-1] = val;
                    }
                    while( --size.width );

                    EXIT;
                }
            }

            mat_step = CV_STUB_STEP;
            size.height = 1;
        }
        
        size.width *= pix_size;
        CV_CALL( cvScalarToRawData( &value, buf, type, 1 ));

        IPPI_CALL( icvSet_8u_C1R( mat->data.ptr, mat_step, size, buf,
                                  CV_ELEM_SIZE1(type)));
    }
    else
    {
        CvFunc2D_2A1P func;
        CvMat maskstub, *mask = (CvMat*)maskarr;
        int mask_step;

        CV_CALL( mask = cvGetMat( mask, &maskstub,NULL,0 ));

        if( !CV_IS_MASK_ARR( mask ))
            CV_ERROR( CV_StsBadMask, "" );

        if( !inittab )
        {
            icvInitSetMRTable( &setm_tab );
            inittab = 1;
        }

        if( !CV_ARE_SIZES_EQ( mat, mask ))
            CV_ERROR( CV_StsUnmatchedSizes, "" );

        mask_step = mask->step;

        if( CV_IS_MAT_CONT( mat->type & mask->type ))
        {
            size.width *= size.height;
            mat_step = mask_step = CV_STUB_STEP;
            size.height = 1;
        }

        func = (CvFunc2D_2A1P)(setm_tab.fn_2d[pix_size]);
        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        CV_CALL( cvScalarToRawData( &value, buf, type, 0 ));

        IPPI_CALL( func( mat->data.ptr, mat_step, mask->data.ptr,
                         mask_step, size, buf ));
    }

    __END__;
}

CvSparseNode* cvInitSparseMatIterator( const CvSparseMat* mat, CvSparseMatIterator* iterator )
{
    CvSparseNode* node = 0;
    
    CV_FUNCNAME( "cvInitSparseMatIterator" );

    __BEGIN__;

    int idx;

    if( !CV_IS_SPARSE_MAT( mat ))
        CV_ERROR( CV_StsBadArg, "Invalid sparse matrix header" );

    if( !iterator )
        CV_ERROR( CV_StsNullPtr, "NULL iterator pointer" );

    iterator->mat = (CvSparseMat*)mat;
    iterator->node = 0;

    for( idx = 0; idx < mat->hashsize; idx++ )
        if( mat->hashtable[idx] )
        {
            node = iterator->node = (CvSparseNode*)mat->hashtable[idx];
            break;
        }

    iterator->curidx = idx;

    __END__;

    return node;
}

CvSparseNode* cvGetNextSparseNode( CvSparseMatIterator* mat_iterator )
{
    if( mat_iterator->node->next )
        return mat_iterator->node = mat_iterator->node->next;
    else
    {
        int idx;
        for( idx = ++mat_iterator->curidx; idx < mat_iterator->mat->hashsize; idx++ )
        {
            CvSparseNode* node = (CvSparseNode*)mat_iterator->mat->hashtable[idx];
            if( node )
            {
                mat_iterator->curidx = idx;
                return mat_iterator->node = node;
            }
        }
        return NULL;
    }
}

CvSetElem* cvSetNew( CvSet* set_header )
{
    CvSetElem* elem = set_header->free_elems;
    if( elem )
    {
        set_header->free_elems = elem->next_free;
        elem->flags = elem->flags & CV_SET_ELEM_IDX_MASK;
        set_header->active_count++;
    }
    else
        cvSetAdd( set_header, NULL, (CvSetElem**)&elem );
    return elem;
}

CvCopyMaskFunc icvGetCopyMaskFunc( int elem_size )
{
    static CvBtFuncTable copym_tab;
    static int inittab = 0;

    if( !inittab )
    {
        icvInitCopyMRTable( &copym_tab );
        inittab = 1;
    }
    return (CvCopyMaskFunc)copym_tab.fn_2d[elem_size];
}

//////////////////////////////////////////////////////////////////////////

#define  ICV_DEF_PX2PL2PX_ENTRY_C2( arrtype_ptr, ptr )  \
    arrtype_ptr plane0 = ptr[0];                        \
    arrtype_ptr plane1 = ptr[1];

#define  ICV_DEF_PX2PL2PX_ENTRY_C3( arrtype_ptr, ptr )  \
    arrtype_ptr plane0 = ptr[0];                        \
    arrtype_ptr plane1 = ptr[1];                        \
    arrtype_ptr plane2 = ptr[2];

#define  ICV_DEF_PX2PL2PX_ENTRY_C4( arrtype_ptr, ptr )  \
    arrtype_ptr plane0 = ptr[0];                        \
    arrtype_ptr plane1 = ptr[1];                        \
    arrtype_ptr plane2 = ptr[2];                        \
    arrtype_ptr plane3 = ptr[3];


#define  ICV_DEF_PX2PL_C2( arrtype, len )           \
{                                                   \
    int j;                                          \
                                                    \
    for( j = 0; j < (len); j++, (src) += 2 )        \
    {                                               \
        arrtype t0 = (src)[0];                      \
        arrtype t1 = (src)[1];                      \
                                                    \
        plane0[j] = t0;                             \
        plane1[j] = t1;                             \
    }                                               \
    plane0 += dststep;                              \
    plane1 += dststep;                              \
}


#define  ICV_DEF_PX2PL_C3( arrtype, len )           \
{                                                   \
    int j;                                          \
                                                    \
    for( j = 0; j < (len); j++, (src) += 3 )        \
    {                                               \
        arrtype t0 = (src)[0];                      \
        arrtype t1 = (src)[1];                      \
        arrtype t2 = (src)[2];                      \
                                                    \
        plane0[j] = t0;                             \
        plane1[j] = t1;                             \
        plane2[j] = t2;                             \
    }                                               \
    plane0 += dststep;                              \
    plane1 += dststep;                              \
    plane2 += dststep;                              \
}


#define  ICV_DEF_PX2PL_C4( arrtype, len )           \
{                                                   \
    int j;                                          \
                                                    \
    for( j = 0; j < (len); j++, (src) += 4 )        \
    {                                               \
        arrtype t0 = (src)[0];                      \
        arrtype t1 = (src)[1];                      \
                                                    \
        plane0[j] = t0;                             \
        plane1[j] = t1;                             \
                                                    \
        t0 = (src)[2];                              \
        t1 = (src)[3];                              \
                                                    \
        plane2[j] = t0;                             \
        plane3[j] = t1;                             \
    }                                               \
    plane0 += dststep;                              \
    plane1 += dststep;                              \
    plane2 += dststep;                              \
    plane3 += dststep;                              \
}


#define  ICV_DEF_PX2PL_COI( arrtype, len, cn )      \
{                                                   \
    int j;                                          \
                                                    \
    for( j = 0; j <= (len) - 4; j += 4, (src) += 4*(cn))\
    {                                               \
        arrtype t0 = (src)[0];                      \
        arrtype t1 = (src)[(cn)];                   \
                                                    \
        (dst)[j] = t0;                              \
        (dst)[j+1] = t1;                            \
                                                    \
        t0 = (src)[(cn)*2];                         \
        t1 = (src)[(cn)*3];                         \
                                                    \
        (dst)[j+2] = t0;                            \
        (dst)[j+3] = t1;                            \
    }                                               \
                                                    \
    for( ; j < (len); j++, (src) += (cn))           \
    {                                               \
        (dst)[j] = (src)[0];                        \
    }                                               \
}


#define  ICV_DEF_COPY_PX2PL_FUNC_2D( arrtype, flavor,   \
                                     cn, entry_macro )  \
IPCVAPI_IMPL( CvStatus, icvCopy_##flavor##_C##cn##P##cn##R,\
( const arrtype* src, int srcstep,                      \
  arrtype** dst, int dststep, CvSize size ),            \
  (src, srcstep, dst, dststep, size))                   \
{                                                       \
    entry_macro(arrtype*, dst);                         \
    srcstep /= sizeof(src[0]);                          \
    dststep /= sizeof(dst[0][0]);                       \
                                                        \
    for( ; size.height--; src += srcstep )              \
    {                                                   \
        ICV_DEF_PX2PL_C##cn( arrtype, size.width );     \
        src -= size.width*(cn);                         \
    }                                                   \
                                                        \
    return CV_OK;                                       \
}


#define  ICV_DEF_COPY_PX2PL_FUNC_2D_COI( arrtype, flavor )\
IPCVAPI_IMPL( CvStatus, icvCopy_##flavor##_CnC1CR,      \
( const arrtype* src, int srcstep, arrtype* dst, int dststep,\
  CvSize size, int cn, int coi ),                       \
  (src, srcstep, dst, dststep, size, cn, coi))          \
{                                                       \
    src += coi - 1;                                     \
    srcstep /= sizeof(src[0]);                          \
    dststep /= sizeof(dst[0]);                          \
                                                        \
    for( ; size.height--; src += srcstep, dst += dststep )\
    {                                                   \
        ICV_DEF_PX2PL_COI( arrtype, size.width, cn );   \
        src -= size.width*(cn);                         \
    }                                                   \
                                                        \
    return CV_OK;                                       \
}


ICV_DEF_COPY_PX2PL_FUNC_2D( uchar, 8u, 2, ICV_DEF_PX2PL2PX_ENTRY_C2 )
ICV_DEF_COPY_PX2PL_FUNC_2D( uchar, 8u, 3, ICV_DEF_PX2PL2PX_ENTRY_C3 )
ICV_DEF_COPY_PX2PL_FUNC_2D( uchar, 8u, 4, ICV_DEF_PX2PL2PX_ENTRY_C4 )
ICV_DEF_COPY_PX2PL_FUNC_2D( ushort, 16s, 2, ICV_DEF_PX2PL2PX_ENTRY_C2 )
ICV_DEF_COPY_PX2PL_FUNC_2D( ushort, 16s, 3, ICV_DEF_PX2PL2PX_ENTRY_C3 )
ICV_DEF_COPY_PX2PL_FUNC_2D( ushort, 16s, 4, ICV_DEF_PX2PL2PX_ENTRY_C4 )
ICV_DEF_COPY_PX2PL_FUNC_2D( int, 32f, 2, ICV_DEF_PX2PL2PX_ENTRY_C2 )
ICV_DEF_COPY_PX2PL_FUNC_2D( int, 32f, 3, ICV_DEF_PX2PL2PX_ENTRY_C3 )
ICV_DEF_COPY_PX2PL_FUNC_2D( int, 32f, 4, ICV_DEF_PX2PL2PX_ENTRY_C4 )
ICV_DEF_COPY_PX2PL_FUNC_2D( int64, 64f, 2, ICV_DEF_PX2PL2PX_ENTRY_C2 )
ICV_DEF_COPY_PX2PL_FUNC_2D( int64, 64f, 3, ICV_DEF_PX2PL2PX_ENTRY_C3 )
ICV_DEF_COPY_PX2PL_FUNC_2D( int64, 64f, 4, ICV_DEF_PX2PL2PX_ENTRY_C4 )


ICV_DEF_COPY_PX2PL_FUNC_2D_COI( uchar, 8u )
ICV_DEF_COPY_PX2PL_FUNC_2D_COI( ushort, 16s )
ICV_DEF_COPY_PX2PL_FUNC_2D_COI( int, 32f )
ICV_DEF_COPY_PX2PL_FUNC_2D_COI( int64, 64f )


/****************************************************************************************\
*                            Merging/inserting array channels                            *
\****************************************************************************************/


#define  ICV_DEF_PL2PX_C2( arrtype, len )   \
{                                           \
    int j;                                  \
                                            \
    for( j = 0; j < (len); j++, (dst) += 2 )\
    {                                       \
        arrtype t0 = plane0[j];             \
        arrtype t1 = plane1[j];             \
                                            \
        dst[0] = t0;                        \
        dst[1] = t1;                        \
    }                                       \
    plane0 += srcstep;                      \
    plane1 += srcstep;                      \
}


#define  ICV_DEF_PL2PX_C3( arrtype, len )   \
{                                           \
    int j;                                  \
                                            \
    for( j = 0; j < (len); j++, (dst) += 3 )\
    {                                       \
        arrtype t0 = plane0[j];             \
        arrtype t1 = plane1[j];             \
        arrtype t2 = plane2[j];             \
                                            \
        dst[0] = t0;                        \
        dst[1] = t1;                        \
        dst[2] = t2;                        \
    }                                       \
    plane0 += srcstep;                      \
    plane1 += srcstep;                      \
    plane2 += srcstep;                      \
}


#define  ICV_DEF_PL2PX_C4( arrtype, len )   \
{                                           \
    int j;                                  \
                                            \
    for( j = 0; j < (len); j++, (dst) += 4 )\
    {                                       \
        arrtype t0 = plane0[j];             \
        arrtype t1 = plane1[j];             \
                                            \
        dst[0] = t0;                        \
        dst[1] = t1;                        \
                                            \
        t0 = plane2[j];                     \
        t1 = plane3[j];                     \
                                            \
        dst[2] = t0;                        \
        dst[3] = t1;                        \
    }                                       \
    plane0 += srcstep;                      \
    plane1 += srcstep;                      \
    plane2 += srcstep;                      \
    plane3 += srcstep;                      \
}


#define  ICV_DEF_PL2PX_COI( arrtype, len, cn )          \
{                                                       \
    int j;                                              \
                                                        \
    for( j = 0; j <= (len) - 4; j += 4, (dst) += 4*(cn))\
    {                                                   \
        arrtype t0 = (src)[j];                          \
        arrtype t1 = (src)[j+1];                        \
                                                        \
        (dst)[0] = t0;                                  \
        (dst)[(cn)] = t1;                               \
                                                        \
        t0 = (src)[j+2];                                \
        t1 = (src)[j+3];                                \
                                                        \
        (dst)[(cn)*2] = t0;                             \
        (dst)[(cn)*3] = t1;                             \
    }                                                   \
                                                        \
    for( ; j < (len); j++, (dst) += (cn))               \
    {                                                   \
        (dst)[0] = (src)[j];                            \
    }                                                   \
}


#define  ICV_DEF_COPY_PL2PX_FUNC_2D( arrtype, flavor, cn, entry_macro ) \
IPCVAPI_IMPL( CvStatus, icvCopy_##flavor##_P##cn##C##cn##R, \
( const arrtype** src, int srcstep,                         \
  arrtype* dst, int dststep, CvSize size ),                 \
  (src, srcstep, dst, dststep, size))                       \
{                                                           \
    entry_macro(const arrtype*, src);                       \
    srcstep /= sizeof(src[0][0]);                           \
    dststep /= sizeof(dst[0]);                              \
                                                            \
    for( ; size.height--; dst += dststep )                  \
    {                                                       \
        ICV_DEF_PL2PX_C##cn( arrtype, size.width );         \
        dst -= size.width*(cn);                             \
    }                                                       \
                                                            \
    return CV_OK;                                           \
}


#define  ICV_DEF_COPY_PL2PX_FUNC_2D_COI( arrtype, flavor )  \
IPCVAPI_IMPL( CvStatus, icvCopy_##flavor##_C1CnCR,          \
( const arrtype* src, int srcstep,                          \
  arrtype* dst, int dststep,                                \
  CvSize size, int cn, int coi ),                           \
  (src, srcstep, dst, dststep, size, cn, coi))              \
{                                                           \
    dst += coi - 1;                                         \
    srcstep /= sizeof(src[0]); dststep /= sizeof(dst[0]);   \
                                                            \
    for( ; size.height--; src += srcstep, dst += dststep )  \
    {                                                       \
        ICV_DEF_PL2PX_COI( arrtype, size.width, cn );       \
        dst -= size.width*(cn);                             \
    }                                                       \
                                                            \
    return CV_OK;                                           \
}


ICV_DEF_COPY_PL2PX_FUNC_2D( uchar, 8u, 2, ICV_DEF_PX2PL2PX_ENTRY_C2 )
ICV_DEF_COPY_PL2PX_FUNC_2D( uchar, 8u, 3, ICV_DEF_PX2PL2PX_ENTRY_C3 )
ICV_DEF_COPY_PL2PX_FUNC_2D( uchar, 8u, 4, ICV_DEF_PX2PL2PX_ENTRY_C4 )
ICV_DEF_COPY_PL2PX_FUNC_2D( ushort, 16s, 2, ICV_DEF_PX2PL2PX_ENTRY_C2 )
ICV_DEF_COPY_PL2PX_FUNC_2D( ushort, 16s, 3, ICV_DEF_PX2PL2PX_ENTRY_C3 )
ICV_DEF_COPY_PL2PX_FUNC_2D( ushort, 16s, 4, ICV_DEF_PX2PL2PX_ENTRY_C4 )
ICV_DEF_COPY_PL2PX_FUNC_2D( int, 32f, 2, ICV_DEF_PX2PL2PX_ENTRY_C2 )
ICV_DEF_COPY_PL2PX_FUNC_2D( int, 32f, 3, ICV_DEF_PX2PL2PX_ENTRY_C3 )
ICV_DEF_COPY_PL2PX_FUNC_2D( int, 32f, 4, ICV_DEF_PX2PL2PX_ENTRY_C4 )
ICV_DEF_COPY_PL2PX_FUNC_2D( int64, 64f, 2, ICV_DEF_PX2PL2PX_ENTRY_C2 )
ICV_DEF_COPY_PL2PX_FUNC_2D( int64, 64f, 3, ICV_DEF_PX2PL2PX_ENTRY_C3 )
ICV_DEF_COPY_PL2PX_FUNC_2D( int64, 64f, 4, ICV_DEF_PX2PL2PX_ENTRY_C4 )

ICV_DEF_COPY_PL2PX_FUNC_2D_COI( uchar, 8u )
ICV_DEF_COPY_PL2PX_FUNC_2D_COI( ushort, 16s )
ICV_DEF_COPY_PL2PX_FUNC_2D_COI( int, 32f )
ICV_DEF_COPY_PL2PX_FUNC_2D_COI( int64, 64f )






#define  ICV_DEF_PXPLPX_TAB( name, FROM, TO )                           \
static void                                                             \
name( CvBigFuncTable* tab )                                             \
{                                                                       \
    tab->fn_2d[CV_8UC2] = (void*)icvCopy##_8u_##FROM##2##TO##2R;        \
    tab->fn_2d[CV_8UC3] = (void*)icvCopy##_8u_##FROM##3##TO##3R;        \
    tab->fn_2d[CV_8UC4] = (void*)icvCopy##_8u_##FROM##4##TO##4R;        \
                                                                        \
    tab->fn_2d[CV_8SC2] = (void*)icvCopy##_8u_##FROM##2##TO##2R;        \
    tab->fn_2d[CV_8SC3] = (void*)icvCopy##_8u_##FROM##3##TO##3R;        \
    tab->fn_2d[CV_8SC4] = (void*)icvCopy##_8u_##FROM##4##TO##4R;        \
                                                                        \
    tab->fn_2d[CV_16UC2] = (void*)icvCopy##_16s_##FROM##2##TO##2R;      \
    tab->fn_2d[CV_16UC3] = (void*)icvCopy##_16s_##FROM##3##TO##3R;      \
    tab->fn_2d[CV_16UC4] = (void*)icvCopy##_16s_##FROM##4##TO##4R;      \
                                                                        \
    tab->fn_2d[CV_16SC2] = (void*)icvCopy##_16s_##FROM##2##TO##2R;      \
    tab->fn_2d[CV_16SC3] = (void*)icvCopy##_16s_##FROM##3##TO##3R;      \
    tab->fn_2d[CV_16SC4] = (void*)icvCopy##_16s_##FROM##4##TO##4R;      \
                                                                        \
    tab->fn_2d[CV_32SC2] = (void*)icvCopy##_32f_##FROM##2##TO##2R;      \
    tab->fn_2d[CV_32SC3] = (void*)icvCopy##_32f_##FROM##3##TO##3R;      \
    tab->fn_2d[CV_32SC4] = (void*)icvCopy##_32f_##FROM##4##TO##4R;      \
                                                                        \
    tab->fn_2d[CV_32FC2] = (void*)icvCopy##_32f_##FROM##2##TO##2R;      \
    tab->fn_2d[CV_32FC3] = (void*)icvCopy##_32f_##FROM##3##TO##3R;      \
    tab->fn_2d[CV_32FC4] = (void*)icvCopy##_32f_##FROM##4##TO##4R;      \
                                                                        \
    tab->fn_2d[CV_64FC2] = (void*)icvCopy##_64f_##FROM##2##TO##2R;      \
    tab->fn_2d[CV_64FC3] = (void*)icvCopy##_64f_##FROM##3##TO##3R;      \
    tab->fn_2d[CV_64FC4] = (void*)icvCopy##_64f_##FROM##4##TO##4R;      \
}

#define  ICV_DEF_PXPLCOI_TAB( name, FROM, TO )                          \
static void                                                             \
name( CvFuncTable* tab )                                                \
{                                                                       \
    tab->fn_2d[CV_8U] = (void*)icvCopy##_8u_##FROM##TO##CR;             \
    tab->fn_2d[CV_8S] = (void*)icvCopy##_8u_##FROM##TO##CR;             \
    tab->fn_2d[CV_16U] = (void*)icvCopy##_16s_##FROM##TO##CR;           \
    tab->fn_2d[CV_16S] = (void*)icvCopy##_16s_##FROM##TO##CR;           \
    tab->fn_2d[CV_32S] = (void*)icvCopy##_32f_##FROM##TO##CR;           \
    tab->fn_2d[CV_32F] = (void*)icvCopy##_32f_##FROM##TO##CR;           \
    tab->fn_2d[CV_64F] = (void*)icvCopy##_64f_##FROM##TO##CR;           \
}

ICV_DEF_PXPLPX_TAB( icvInitSplitRTable, C, P )
ICV_DEF_PXPLCOI_TAB( icvInitSplitRCoiTable, Cn, C1 )
ICV_DEF_PXPLPX_TAB( icvInitCvtPlaneToPixRTable, P, C )
ICV_DEF_PXPLCOI_TAB( icvInitCvtPlaneToPixRCoiTable, C1, Cn )



void cvSplit( const void* srcarr, void* dstarr0, void* dstarr1, void* dstarr2, void* dstarr3 )
{
    static CvBigFuncTable  pxpl_tab;
    static CvFuncTable  pxplcoi_tab;
    static int inittab = 0;

    CV_FUNCNAME( "cvSplit" );

    __BEGIN__;

    CvMat stub[5], *dst[4], *src = (CvMat*)srcarr;
    CvSize size;
    void* dstptr[4] = { 0, 0, 0, 0 };
    int type, cn, coi = 0;
    int i, nzplanes = 0, nzidx = -1;
    int cont_flag;
    int src_step, dst_step = 0;

    if( !inittab )
    {
        icvInitSplitRTable( &pxpl_tab );
        icvInitSplitRCoiTable( &pxplcoi_tab );
        inittab = 1;
    }

    dst[0] = (CvMat*)dstarr0;
    dst[1] = (CvMat*)dstarr1;
    dst[2] = (CvMat*)dstarr2;
    dst[3] = (CvMat*)dstarr3;

    CV_CALL( src = cvGetMat( src, stub + 4, &coi,0 ));

    //if( coi != 0 )
    //    CV_ERROR( CV_BadCOI, "" );

    type = CV_MAT_TYPE( src->type );
    cn = CV_MAT_CN( type );

    cont_flag = src->type;

    if( cn == 1 )
        CV_ERROR( CV_BadNumChannels, "" );

    for( i = 0; i < 4; i++ )
    {
        if( dst[i] )
        {
            nzplanes++;
            nzidx = i;
            CV_CALL( dst[i] = cvGetMat( dst[i], stub + i,NULL,0 ));
            if( CV_MAT_CN( dst[i]->type ) != 1 )
                CV_ERROR( CV_BadNumChannels, "" );
            if( !CV_ARE_DEPTHS_EQ( dst[i], src ))
                CV_ERROR( CV_StsUnmatchedFormats, "" );
            if( !CV_ARE_SIZES_EQ( dst[i], src ))
                CV_ERROR( CV_StsUnmatchedSizes, "" );
            if( nzplanes > i && i > 0 && dst[i]->step != dst[i-1]->step )
                CV_ERROR( CV_BadStep, "" );
            dst_step = dst[i]->step;
            dstptr[nzplanes-1] = dst[i]->data.ptr;

            cont_flag &= dst[i]->type;
        }
    }

    src_step = src->step;
    size = cvGetMatSize( src );

    if( CV_IS_MAT_CONT( cont_flag ))
    {
        size.width *= size.height;
        src_step = dst_step = CV_STUB_STEP;

        size.height = 1;
    }

    if( nzplanes == cn )
    {
        CvSplitFunc func = (CvSplitFunc)pxpl_tab.fn_2d[type];

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        IPPI_CALL( func( src->data.ptr, src_step, dstptr, dst_step, size ));
    }
    else if( nzplanes == 1 )
    {
        CvExtractPlaneFunc func = (CvExtractPlaneFunc)pxplcoi_tab.fn_2d[CV_MAT_DEPTH(type)];

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        IPPI_CALL( func( src->data.ptr, src_step,
                         dst[nzidx]->data.ptr, dst_step,
                         size, cn, nzidx + 1 ));
    }
    else
    {
        CV_ERROR( CV_StsBadArg,
            "Either all output planes or only one output plane should be non zero" );
    }

    __END__;
}

void cvMerge( const void* srcarr0, const void* srcarr1, const void* srcarr2,const void* srcarr3, void* dstarr )
{
    static CvBigFuncTable plpx_tab;
    static CvFuncTable plpxcoi_tab;
    static int inittab = 0;

    CV_FUNCNAME( "cvMerge" );

    __BEGIN__;

    int src_step = 0, dst_step;
    CvMat stub[5], *src[4], *dst = (CvMat*)dstarr;
    CvSize size;
    const void* srcptr[4] = { 0, 0, 0, 0 };
    int type, cn, coi = 0;
    int i, nzplanes = 0, nzidx = -1;
    int cont_flag;

    if( !inittab )
    {
        icvInitCvtPlaneToPixRTable( &plpx_tab );
        icvInitCvtPlaneToPixRCoiTable( &plpxcoi_tab );
        inittab = 1;
    }

    src[0] = (CvMat*)srcarr0;
    src[1] = (CvMat*)srcarr1;
    src[2] = (CvMat*)srcarr2;
    src[3] = (CvMat*)srcarr3;

    CV_CALL( dst = cvGetMat( dst, stub + 4, &coi,0 ));

    type = CV_MAT_TYPE( dst->type );
    cn = CV_MAT_CN( type );

    cont_flag = dst->type;

    if( cn == 1 )
        CV_ERROR( CV_BadNumChannels, "" );

    for( i = 0; i < 4; i++ )
    {
        if( src[i] )
        {
            nzplanes++;
            nzidx = i;
            CV_CALL( src[i] = cvGetMat( src[i], stub + i,NULL,0 ));
            if( CV_MAT_CN( src[i]->type ) != 1 )
                CV_ERROR( CV_BadNumChannels, "" );
            if( !CV_ARE_DEPTHS_EQ( src[i], dst ))
                CV_ERROR( CV_StsUnmatchedFormats, "" );
            if( !CV_ARE_SIZES_EQ( src[i], dst ))
                CV_ERROR( CV_StsUnmatchedSizes, "" );
            if( nzplanes > i && i > 0 && src[i]->step != src[i-1]->step )
                CV_ERROR( CV_BadStep, "" );
            src_step = src[i]->step;
            srcptr[nzplanes-1] = (const void*)(src[i]->data.ptr);

            cont_flag &= src[i]->type;
        }
    }

    size = cvGetMatSize( dst );
    dst_step = dst->step;

    if( CV_IS_MAT_CONT( cont_flag ))
    {
        size.width *= size.height;
        src_step = dst_step = CV_STUB_STEP;
        size.height = 1;
    }

    if( nzplanes == cn )
    {
        CvMergeFunc func = (CvMergeFunc)plpx_tab.fn_2d[type];

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        IPPI_CALL( func( srcptr, src_step, dst->data.ptr, dst_step, size ));
    }
    else if( nzplanes == 1 )
    {
        CvInsertPlaneFunc func = (CvInsertPlaneFunc)plpxcoi_tab.fn_2d[CV_MAT_DEPTH(type)];

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        IPPI_CALL( func( src[nzidx]->data.ptr, src_step,
                         dst->data.ptr, dst_step,
                         size, cn, nzidx + 1 ));
    }
    else
    {
        CV_ERROR( CV_StsBadArg,
            "Either all input planes or only one input plane should be non zero" );
    }

    __END__;
}

int  cvRound( double value )
{
  int returnvalue=(int)(value+0.5);
  return returnvalue;
}

static void icvSVBkSb_64f( int m, int n, const double* w, const double* uT, int lduT, const double* vT, int ldvT, const double* b, int ldb, int nb, double* x, int ldx, double* buffer )
{
    double threshold = 0;
    int i, j, nm = MIN( m, n );

    if( !b )
        nb = m;

    for( i = 0; i < n; i++ )
        memset( x + i*ldx, 0, nb*sizeof(x[0]));

    for( i = 0; i < nm; i++ )
        threshold += w[i];
    threshold *= 2*DBL_EPSILON;

    /* vT * inv(w) * uT * b */
    for( i = 0; i < nm; i++, uT += lduT, vT += ldvT )
    {
        double wi = w[i];

        if( wi > threshold )
        {
            wi = 1./wi;

            if( nb == 1 )
            {
                double s = 0;
                if( b )
                {
                    if( ldb == 1 )
                    {
                        for( j = 0; j <= m - 4; j += 4 )
                            s += uT[j]*b[j] + uT[j+1]*b[j+1] + uT[j+2]*b[j+2] + uT[j+3]*b[j+3];
                        for( ; j < m; j++ )
                            s += uT[j]*b[j];
                    }
                    else
                    {
                        for( j = 0; j < m; j++ )
                            s += uT[j]*b[j*ldb];
                    }
                }
                else
                    s = uT[0];
                s *= wi;
                if( ldx == 1 )
                {
                    for( j = 0; j <= n - 4; j += 4 )
                    {
                        double t0 = x[j] + s*vT[j];
                        double t1 = x[j+1] + s*vT[j+1];
                        x[j] = t0;
                        x[j+1] = t1;
                        t0 = x[j+2] + s*vT[j+2];
                        t1 = x[j+3] + s*vT[j+3];
                        x[j+2] = t0;
                        x[j+3] = t1;
                    }

                    for( ; j < n; j++ )
                        x[j] += s*vT[j];
                }
                else
                {
                    for( j = 0; j < n; j++ )
                        x[j*ldx] += s*vT[j];
                }
            }
            else
            {
                if( b )
                {
                    memset( buffer, 0, nb*sizeof(buffer[0]));
                    icvMatrAXPY_64f( m, nb, b, ldb, uT, buffer, 0 );
                    for( j = 0; j < nb; j++ )
                        buffer[j] *= wi;
                }
                else
                {
                    for( j = 0; j < nb; j++ )
                        buffer[j] = uT[j]*wi;
                }
                icvMatrAXPY_64f( n, nb, buffer, 0, vT, x, ldx );
            }
        }
    }
}

static void icvSVBkSb_32f( int m, int n, const float* w, const float* uT, int lduT, const float* vT, int ldvT, const float* b, int ldb, int nb, float* x, int ldx, float* buffer )
{
    float threshold = 0.f;
    int i, j, nm = MIN( m, n );

    if( !b )
        nb = m;

    for( i = 0; i < n; i++ )
        memset( x + i*ldx, 0, nb*sizeof(x[0]));

    for( i = 0; i < nm; i++ )
        threshold += w[i];
    threshold *= 2*FLT_EPSILON;

    /* vT * inv(w) * uT * b */
    for( i = 0; i < nm; i++, uT += lduT, vT += ldvT )
    {
        double wi = w[i];
        
        if( wi > threshold )
        {
            wi = 1./wi;

            if( nb == 1 )
            {
                double s = 0;
                if( b )
                {
                    if( ldb == 1 )
                    {
                        for( j = 0; j <= m - 4; j += 4 )
                            s += uT[j]*b[j] + uT[j+1]*b[j+1] + uT[j+2]*b[j+2] + uT[j+3]*b[j+3];
                        for( ; j < m; j++ )
                            s += uT[j]*b[j];
                    }
                    else
                    {
                        for( j = 0; j < m; j++ )
                            s += uT[j]*b[j*ldb];
                    }
                }
                else
                    s = uT[0];
                s *= wi;

                if( ldx == 1 )
                {
                    for( j = 0; j <= n - 4; j += 4 )
                    {
                        double t0 = x[j] + s*vT[j];
                        double t1 = x[j+1] + s*vT[j+1];
                        x[j] = (float)t0;
                        x[j+1] = (float)t1;
                        t0 = x[j+2] + s*vT[j+2];
                        t1 = x[j+3] + s*vT[j+3];
                        x[j+2] = (float)t0;
                        x[j+3] = (float)t1;
                    }

                    for( ; j < n; j++ )
                        x[j] = (float)(x[j] + s*vT[j]);
                }
                else
                {
                    for( j = 0; j < n; j++ )
                        x[j*ldx] = (float)(x[j*ldx] + s*vT[j]);
                }
            }
            else
            {
                if( b )
                {
                    memset( buffer, 0, nb*sizeof(buffer[0]));
                    icvMatrAXPY_32f( m, nb, b, ldb, uT, buffer, 0 );
                    for( j = 0; j < nb; j++ )
                        buffer[j] = (float)(buffer[j]*wi);
                }
                else
                {
                    for( j = 0; j < nb; j++ )
                        buffer[j] = (float)(uT[j]*wi);
                }
                icvMatrAXPY_32f( n, nb, buffer, 0, vT, x, ldx );
            }
        }
    }
}

void cvSVBkSb( const CvArr* warr, const CvArr* uarr,const CvArr* varr, const CvArr* barr,CvArr* xarr, int flags )
{
    uchar* buffer = 0;
    int local_alloc = 0;

    CV_FUNCNAME( "cvSVBkSb" );

    __BEGIN__;

    CvMat wstub, *w = (CvMat*)warr;
    CvMat bstub, *b = (CvMat*)barr;
    CvMat xstub, *x = (CvMat*)xarr;
    CvMat ustub, ustub2, *u = (CvMat*)uarr;
    CvMat vstub, vstub2, *v = (CvMat*)varr;
    uchar* tw = 0;
    int type;
    int temp_u = 0, temp_v = 0;
    int u_buf_offset = 0, v_buf_offset = 0, w_buf_offset = 0, t_buf_offset = 0;
    int buf_size = 0, pix_size;
    int m, n, nm;
    int u_rows, u_cols;
    int v_rows, v_cols;

    if( !CV_IS_MAT( w ))
        CV_CALL( w = cvGetMat( w, &wstub,NULL,0 ));

    if( !CV_IS_MAT( u ))
        CV_CALL( u = cvGetMat( u, &ustub ,NULL,0));

    if( !CV_IS_MAT( v ))
        CV_CALL( v = cvGetMat( v, &vstub,NULL,0 ));

    if( !CV_IS_MAT( x ))
        CV_CALL( x = cvGetMat( x, &xstub,NULL,0 ));

    if( !CV_ARE_TYPES_EQ( w, u ) || !CV_ARE_TYPES_EQ( w, v ) || !CV_ARE_TYPES_EQ( w, x ))
        CV_ERROR( CV_StsUnmatchedFormats, "All matrices must have the same type" );

    type = CV_MAT_TYPE( w->type );
    pix_size = CV_ELEM_SIZE(type);

    if( !(flags & CV_SVD_U_T) )
    {
        temp_u = 1;
        u_buf_offset = buf_size;
        buf_size += u->cols*u->rows*pix_size;
        u_rows = u->rows;
        u_cols = u->cols;
    }
    else
    {
        u_rows = u->cols;
        u_cols = u->rows;
    }

    if( !(flags & CV_SVD_V_T) )
    {
        temp_v = 1;
        v_buf_offset = buf_size;
        buf_size += v->cols*v->rows*pix_size;
        v_rows = v->rows;
        v_cols = v->cols;
    }
    else
    {
        v_rows = v->cols;
        v_cols = v->rows;
    }

    m = u_rows;
    n = v_rows;
    nm = MIN(n,m);

    if( (u_rows != u_cols && v_rows != v_cols) || x->rows != v_rows )
        CV_ERROR( CV_StsBadSize, "V or U matrix must be square" );

    if( (w->rows == 1 || w->cols == 1) && w->rows + w->cols - 1 == nm )
    {
        if( CV_IS_MAT_CONT(w->type) )
            tw = w->data.ptr;
        else
        {
            w_buf_offset = buf_size;
            buf_size += nm*pix_size;
        }
    }
    else
    {
        if( w->cols != v_cols || w->rows != u_cols )
            CV_ERROR( CV_StsBadSize, "W must be 1d array of MIN(m,n) elements or "
                                    "matrix which size matches to U and V" );
        w_buf_offset = buf_size;
        buf_size += nm*pix_size;
    }

    if( b )
    {
        if( !CV_IS_MAT( b ))
            CV_CALL( b = cvGetMat( b, &bstub,NULL,0 ));
        if( !CV_ARE_TYPES_EQ( w, b ))
            CV_ERROR( CV_StsUnmatchedFormats, "All matrices must have the same type" );
        if( b->cols != x->cols || b->rows != m )
            CV_ERROR( CV_StsUnmatchedSizes, "b matrix must have (m x x->cols) size" );
    }
    else
    {
        b = &bstub;
        memset( b, 0, sizeof(*b));
    }

    t_buf_offset = buf_size;
    buf_size += (MAX(m,n) + b->cols)*pix_size;

    if( buf_size <= CV_MAX_LOCAL_SIZE )
    {
        buffer = (uchar*)cvStackAlloc( buf_size );
        local_alloc = 1;
    }
    else
        CV_CALL( buffer = (uchar*)cvAlloc( buf_size ));

    if( temp_u )
    {
        cvInitMatHeader( &ustub2, u_cols, u_rows, type, buffer + u_buf_offset,CV_AUTOSTEP );
        cvT( u, &ustub2 );
        u = &ustub2;
    }

    if( temp_v )
    {
        cvInitMatHeader( &vstub2, v_cols, v_rows, type, buffer + v_buf_offset,CV_AUTOSTEP );
        cvT( v, &vstub2 );
        v = &vstub2;
    }

    if( !tw )
    {
        int i, shift = w->cols > 1 ? pix_size : 0;
        tw = buffer + w_buf_offset;
        for( i = 0; i < nm; i++ )
            memcpy( tw + i*pix_size, w->data.ptr + i*(w->step + shift), pix_size );
    }

    if( type == CV_32FC1 )
    {
        icvSVBkSb_32f( m, n, (float*)tw, u->data.fl, u->step/sizeof(float),
                       v->data.fl, v->step/sizeof(float),
                       b->data.fl, b->step/sizeof(float), b->cols,
                       x->data.fl, x->step/sizeof(float),
                       (float*)(buffer + t_buf_offset) );
    }
    else if( type == CV_64FC1 )
    {
        icvSVBkSb_64f( m, n, (double*)tw, u->data.db, u->step/sizeof(double),
                       v->data.db, v->step/sizeof(double),
                       b->data.db, b->step/sizeof(double), b->cols,
                       x->data.db, x->step/sizeof(double),
                       (double*)(buffer + t_buf_offset) );
    }
    else
    {
        CV_ERROR( CV_StsUnsupportedFormat, "" );
    }

    __END__;

    if( buffer && !local_alloc )
        cvFree( &buffer );
}

void cvStartReadSeq( const CvSeq *seq, CvSeqReader * reader, int reverse )
{
    CvSeqBlock *first_block;
    CvSeqBlock *last_block;

    CV_FUNCNAME( "cvStartReadSeq" );

    if( reader )
    {
        reader->seq = 0;
        reader->block = 0;
        reader->ptr = reader->block_max = reader->block_min = 0;
    }

    __BEGIN__;

    if( !seq || !reader )
        CV_ERROR( CV_StsNullPtr, "" );

    reader->header_size = sizeof( CvSeqReader );
    reader->seq = (CvSeq*)seq;

    first_block = seq->first;

    if( first_block )
    {
        last_block = first_block->prev;
        reader->ptr = first_block->data;
        reader->prev_elem = CV_GET_LAST_ELEM( seq, last_block );
        reader->delta_index = seq->first->start_index;

        if( reverse )
        {
            char *temp = reader->ptr;

            reader->ptr = reader->prev_elem;
            reader->prev_elem = temp;

            reader->block = last_block;
        }
        else
        {
            reader->block = first_block;
        }

        reader->block_min = reader->block->data;
        reader->block_max = reader->block_min + reader->block->count * seq->elem_size;
    }
    else
    {
        reader->delta_index = 0;
        reader->block = 0;

        reader->ptr = reader->prev_elem = reader->block_min = reader->block_max = 0;
    }

    __END__;
}

void cvChangeSeqBlock( void* _reader, int direction )
{
    CV_FUNCNAME( "cvChangeSeqBlock" );

    __BEGIN__;

    CvSeqReader* reader = (CvSeqReader*)_reader;
    
    if( !reader )
        CV_ERROR( CV_StsNullPtr, "" );

    if( direction > 0 )
    {
        reader->block = reader->block->next;
        reader->ptr = reader->block->data;
    }
    else
    {
        reader->block = reader->block->prev;
        reader->ptr = CV_GET_LAST_ELEM( reader->seq, reader->block );
    }
    reader->block_min = reader->block->data;
    reader->block_max = reader->block_min + reader->block->count * reader->seq->elem_size;

    __END__;
}

int cvGetSeqReaderPos( CvSeqReader* reader )
{
    int elem_size;
    int index = -1;

    CV_FUNCNAME( "cvGetSeqReaderPos" );

    __BEGIN__;

    if( !reader || !reader->ptr )
        CV_ERROR( CV_StsNullPtr, "" );

    elem_size = reader->seq->elem_size;
    if( elem_size <= ICV_SHIFT_TAB_MAX && (index = icvPower2ShiftTab[elem_size - 1]) >= 0 )
        index = (int)((reader->ptr - reader->block_min) >> index);
    else
        index = (int)((reader->ptr - reader->block_min) / elem_size);

    index += reader->block->start_index - reader->delta_index;

    __END__;

    return index;
}

void cvSetSeqReaderPos( CvSeqReader* reader, int index, int is_relative )
{
    CV_FUNCNAME( "cvSetSeqReaderPos" );

    __BEGIN__;

    CvSeqBlock *block;
    int elem_size, count, total;

    if( !reader || !reader->seq )
        CV_ERROR( CV_StsNullPtr, "" );

    total = reader->seq->total;
    elem_size = reader->seq->elem_size;

    if( !is_relative )
    {
        if( index < 0 )
        {
            if( index < -total )
                CV_ERROR( CV_StsOutOfRange, "" );
            index += total;
        }
        else if( index >= total )
        {
            index -= total;
            if( index >= total )
                CV_ERROR( CV_StsOutOfRange, "" );
        }

        block = reader->seq->first;
        if( index >= (count = block->count) )
        {
            if( index + index <= total )
            {
                do
                {
                    block = block->next;
                    index -= count;
                }
                while( index >= (count = block->count) );
            }
            else
            {
                do
                {
                    block = block->prev;
                    total -= block->count;
                }
                while( index < total );
                index -= total;
            }
        }
        reader->ptr = block->data + index * elem_size;
        if( reader->block != block )
        {
            reader->block = block;
            reader->block_min = block->data;
            reader->block_max = block->data + block->count * elem_size;
        }
    }
    else
    {
        char* ptr = reader->ptr;
        index *= elem_size;
        block = reader->block;

        if( index > 0 )
        {
            while( ptr + index >= reader->block_max )
            {
                int delta = (int)(reader->block_max - ptr);
                index -= delta;
                reader->block = block = block->next;
                reader->block_min = ptr = block->data;
                reader->block_max = block->data + block->count*elem_size;
            }
            reader->ptr = ptr + index;
        }
        else
        {
            while( ptr + index < reader->block_min )
            {
                int delta = (int)(ptr - reader->block_min);
                index += delta;
                reader->block = block = block->prev;
                reader->block_min = block->data;
                reader->block_max = ptr = block->data + block->count*elem_size;
            }
            reader->ptr = ptr + index;
        }
    }

    __END__;
}

void icvInitCubicCoeffTab()
{
    static int inittab = 0;
    if( !inittab )
    {
#if 0
        // classical Mitchell-Netravali filter
        const double B = 1./3;
        const double C = 1./3;
        const double p0 = (6 - 2*B)/6.;
        const double p2 = (-18 + 12*B + 6*C)/6.;
        const double p3 = (12 - 9*B - 6*C)/6.;
        const double q0 = (8*B + 24*C)/6.;
        const double q1 = (-12*B - 48*C)/6.;
        const double q2 = (6*B + 30*C)/6.;
        const double q3 = (-B - 6*C)/6.;

        #define ICV_CUBIC_1(x)  (((x)*p3 + p2)*(x)*(x) + p0)
        #define ICV_CUBIC_2(x)  ((((x)*q3 + q2)*(x) + q1)*(x) + q0)
#else
        // alternative "sharp" filter
        const double A = -0.75;
        #define ICV_CUBIC_1(x)  (((A + 2)*(x) - (A + 3))*(x)*(x) + 1)
        #define ICV_CUBIC_2(x)  (((A*(x) - 5*A)*(x) + 8*A)*(x) - 4*A)
#endif
        for( int i = 0; i <= ICV_CUBIC_TAB_SIZE; i++ )
        {
            float x = (float)i/ICV_CUBIC_TAB_SIZE;
            icvCubicCoeffs[i*2] = (float)ICV_CUBIC_1(x);
            x += 1.f;
            icvCubicCoeffs[i*2+1] = (float)ICV_CUBIC_2(x);
        }

        inittab = 1;
    }
}

void cvClearSeq( CvSeq *seq )
{
    CV_FUNCNAME( "cvClearSeq" );

    __BEGIN__;

    if( !seq )
        CV_ERROR( CV_StsNullPtr, "" );
    cvSeqPopMulti( seq, 0, seq->total,0 );

    __END__;
}

void cvScalarToRawData( const CvScalar* scalar, void* data, int type, int extend_to_12 )
{
    CV_FUNCNAME( "cvScalarToRawData" );

    type = CV_MAT_TYPE(type);
    
    __BEGIN__;

    int cn = CV_MAT_CN( type );
    int depth = type & CV_MAT_DEPTH_MASK;

    assert( scalar && data );
    if( (unsigned)(cn - 1) >= 4 )
        CV_ERROR( CV_StsOutOfRange, "The number of channels must be 1, 2, 3 or 4" );

    switch( depth )
    {
    case CV_8UC1:
        while( cn-- )
        {
            int t = cvRound( scalar->val[cn] );
            ((uchar*)data)[cn] = CV_CAST_8U(t);
        }
        break;
    case CV_8SC1:
        while( cn-- )
        {
            int t = cvRound( scalar->val[cn] );
            ((char*)data)[cn] = CV_CAST_8S(t);
        }
        break;
    case CV_16UC1:
        while( cn-- )
        {
            int t = cvRound( scalar->val[cn] );
            ((ushort*)data)[cn] = CV_CAST_16U(t);
        }
        break;
    case CV_16SC1:
        while( cn-- )
        {
            int t = cvRound( scalar->val[cn] );
            ((short*)data)[cn] = CV_CAST_16S(t);
        }
        break;
    case CV_32SC1:
        while( cn-- )
            ((int*)data)[cn] = cvRound( scalar->val[cn] );
        break;
    case CV_32FC1:
        while( cn-- )
            ((float*)data)[cn] = (float)(scalar->val[cn]);
        break;
    case CV_64FC1:
        while( cn-- )
            ((double*)data)[cn] = (double)(scalar->val[cn]);
        break;
    default:
        assert(0);
        CV_ERROR_FROM_CODE( CV_BadDepth );
    }

    if( extend_to_12 )
    {
        int pix_size = CV_ELEM_SIZE(type);
        int offset = CV_ELEM_SIZE1(depth)*12;

        do
        {
            offset -= pix_size;
            CV_MEMCPY_AUTO( (char*)data + offset, data, pix_size );
        }
        while( offset > pix_size );
    }

    __END__;
}

int cvSetAdd( CvSet* set, CvSetElem* element, CvSetElem** inserted_element )
{
    int id = -1;

    CV_FUNCNAME( "cvSetAdd" );

    __BEGIN__;

    CvSetElem *free_elem;

    if( !set )
        CV_ERROR( CV_StsNullPtr, "" );

    if( !(set->free_elems) )
    {
        int count = set->total;
        int elem_size = set->elem_size;
        char *ptr;
        CV_CALL( icvGrowSeq( (CvSeq *) set, 0 ));

        set->free_elems = (CvSetElem*) (ptr = set->ptr);
        for( ; ptr + elem_size <= set->block_max; ptr += elem_size, count++ )
        {
            ((CvSetElem*)ptr)->flags = count | CV_SET_ELEM_FREE_FLAG;
            ((CvSetElem*)ptr)->next_free = (CvSetElem*)(ptr + elem_size);
        }
        assert( count <= CV_SET_ELEM_IDX_MASK+1 );
        ((CvSetElem*)(ptr - elem_size))->next_free = 0;
        set->first->prev->count += count - set->total;
        set->total = count;
        set->ptr = set->block_max;
    }

    free_elem = set->free_elems;
    set->free_elems = free_elem->next_free;

    id = free_elem->flags & CV_SET_ELEM_IDX_MASK;
    if( element )
        CV_MEMCPY_INT( free_elem, element, (size_t)set->elem_size/sizeof(int) );

    free_elem->flags = id;
    set->active_count++;

    if( inserted_element )
        *inserted_element = free_elem;

    __END__;

    return id;
}

void cvSeqPopMulti( CvSeq *seq, void *_elements, int count, int front )
{
    char *elements = (char *) _elements;

    CV_FUNCNAME( "cvSeqPopMulti" );

    __BEGIN__;

    if( !seq )
        CV_ERROR( CV_StsNullPtr, "NULL sequence pointer" );
    if( count < 0 )
        CV_ERROR( CV_StsBadSize, "number of removed elements is negative" );

    count = MIN( count, seq->total );

    if( !front )
    {
        if( elements )
            elements += count * seq->elem_size;

        while( count > 0 )
        {
            int delta = seq->first->prev->count;

            delta = MIN( delta, count );
            assert( delta > 0 );

            seq->first->prev->count -= delta;
            seq->total -= delta;
            count -= delta;
            delta *= seq->elem_size;
            seq->ptr -= delta;

            if( elements )
            {
                elements -= delta;
                memcpy( elements, seq->ptr, delta );
            }

            if( seq->first->prev->count == 0 )
                icvFreeSeqBlock( seq, 0 );
        }
    }
    else
    {
        while( count > 0 )
        {
            int delta = seq->first->count;

            delta = MIN( delta, count );
            assert( delta > 0 );

            seq->first->count -= delta;
            seq->total -= delta;
            count -= delta;
            seq->first->start_index += delta;
            delta *= seq->elem_size;

            if( elements )
            {
                memcpy( elements, seq->first->data, delta );
                elements += delta;
            }

            seq->first->data += delta;
            if( seq->first->count == 0 )
                icvFreeSeqBlock( seq, 1 );
        }
    }

    __END__;
}

void* cvClone( const void* struct_ptr )
{
    void* struct_copy = 0;

    CV_FUNCNAME("cvClone" );

    __BEGIN__;

    CvTypeInfo* info;

    if( !struct_ptr )
        CV_ERROR( CV_StsNullPtr, "NULL structure pointer" );

    CV_CALL( info = cvTypeOf( struct_ptr ));
    if( !info )
        CV_ERROR( CV_StsError, "Unknown object type" );
    if( !info->clone )
        CV_ERROR( CV_StsError, "clone function pointer is NULL" );

    CV_CALL( struct_copy = info->clone( struct_ptr ));

    __END__;

    return struct_copy;
}

CvTypeInfo* cvTypeOf( const void* struct_ptr )
{
    CvTypeInfo* info = 0;

    for( info = 0; info != 0; info = info->next )
        if( info->is_instance( struct_ptr ))
            break;

    return info;
}

#define CV_IMPL_BGRX2BGR( flavor, arrtype )                             \
static CvStatus CV_STDCALL                                              \
icvBGRx2BGR_##flavor##_CnC3R( const arrtype* src, int srcstep,          \
                              arrtype* dst, int dststep,                \
                              CvSize size, int src_cn, int blue_idx )   \
{                                                                       \
    int i;                                                              \
                                                                        \
    srcstep /= sizeof(src[0]);                                          \
    dststep /= sizeof(dst[0]);                                          \
    srcstep -= size.width*src_cn;                                       \
    size.width *= 3;                                                    \
                                                                        \
    for( ; size.height--; src += srcstep, dst += dststep )              \
    {                                                                   \
        for( i = 0; i < size.width; i += 3, src += src_cn )             \
        {                                                               \
            arrtype t0=src[blue_idx], t1=src[1], t2=src[blue_idx^2];    \
            dst[i] = t0;                                                \
            dst[i+1] = t1;                                              \
            dst[i+2] = t2;                                              \
        }                                                               \
    }                                                                   \
                                                                        \
    return CV_OK;                                                       \
}

#define CV_IMPL_BGR2BGRX( flavor, arrtype )                             \
static CvStatus CV_STDCALL                                              \
icvBGR2BGRx_##flavor##_C3C4R( const arrtype* src, int srcstep,          \
                              arrtype* dst, int dststep,                \
                              CvSize size, int blue_idx )               \
{                                                                       \
    int i;                                                              \
                                                                        \
    srcstep /= sizeof(src[0]);                                          \
    dststep /= sizeof(dst[0]);                                          \
    srcstep -= size.width*3;                                            \
    size.width *= 4;                                                    \
                                                                        \
    for( ; size.height--; src += srcstep, dst += dststep )              \
    {                                                                   \
        for( i = 0; i < size.width; i += 4, src += 3 )                  \
        {                                                               \
            arrtype t0=src[blue_idx], t1=src[1], t2=src[blue_idx^2];    \
            dst[i] = t0;                                                \
            dst[i+1] = t1;                                              \
            dst[i+2] = t2;                                              \
            dst[i+3] = 0;                                               \
        }                                                               \
    }                                                                   \
                                                                        \
    return CV_OK;                                                       \
}


#define CV_IMPL_BGRA2RGBA( flavor, arrtype )                            \
static CvStatus CV_STDCALL                                              \
icvBGRA2RGBA_##flavor##_C4R( const arrtype* src, int srcstep,           \
                             arrtype* dst, int dststep, CvSize size )   \
{                                                                       \
    int i;                                                              \
                                                                        \
    srcstep /= sizeof(src[0]);                                          \
    dststep /= sizeof(dst[0]);                                          \
    size.width *= 4;                                                    \
                                                                        \
    for( ; size.height--; src += srcstep, dst += dststep )              \
    {                                                                   \
        for( i = 0; i < size.width; i += 4 )                            \
        {                                                               \
            arrtype t0 = src[2], t1 = src[1], t2 = src[0], t3 = src[3]; \
            dst[i] = t0;                                                \
            dst[i+1] = t1;                                              \
            dst[i+2] = t2;                                              \
            dst[i+3] = t3;                                              \
        }                                                               \
    }                                                                   \
                                                                        \
    return CV_OK;                                                       \
}


CV_IMPL_BGRX2BGR( 8u, uchar )
CV_IMPL_BGRX2BGR( 16u, ushort )
CV_IMPL_BGRX2BGR( 32f, int )
CV_IMPL_BGR2BGRX( 8u, uchar )
CV_IMPL_BGR2BGRX( 16u, ushort )
CV_IMPL_BGR2BGRX( 32f, int )
CV_IMPL_BGRA2RGBA( 8u, uchar )
CV_IMPL_BGRA2RGBA( 16u, ushort )
CV_IMPL_BGRA2RGBA( 32f, int )

static CvStatus CV_STDCALL
icvBGR5x52BGRx_8u_C2CnR( const uchar* src, int srcstep,
                         uchar* dst, int dststep,
                         CvSize size, int dst_cn,
                         int blue_idx, int green_bits )
{
    int i;
    assert( green_bits == 5 || green_bits == 6 );
    dststep -= size.width*dst_cn;

    for( ; size.height--; src += srcstep, dst += dststep )
    {
        if( green_bits == 6 )
            for( i = 0; i < size.width; i++, dst += dst_cn )
            {
                unsigned t = ((const ushort*)src)[i];
                dst[blue_idx] = (uchar)(t << 3);
                dst[1] = (uchar)((t >> 3) & ~3);
                dst[blue_idx ^ 2] = (uchar)((t >> 8) & ~7);
                if( dst_cn == 4 )
                    dst[3] = 0;
            }
        else
            for( i = 0; i < size.width; i++, dst += dst_cn )
            {
                unsigned t = ((const ushort*)src)[i];
                dst[blue_idx] = (uchar)(t << 3);
                dst[1] = (uchar)((t >> 2) & ~7);
                dst[blue_idx ^ 2] = (uchar)((t >> 7) & ~7);
                if( dst_cn == 4 )
                    dst[3] = 0;
            }
    }

    return CV_OK;
}


static CvStatus CV_STDCALL
icvBGRx2BGR5x5_8u_CnC2R( const uchar* src, int srcstep,
                         uchar* dst, int dststep,
                         CvSize size, int src_cn,
                         int blue_idx, int green_bits )
{
    int i;
    srcstep -= size.width*src_cn;

    for( ; size.height--; src += srcstep, dst += dststep )
    {
        if( green_bits == 6 )
            for( i = 0; i < size.width; i++, src += src_cn )
            {
                int t = (src[blue_idx] >> 3)|((src[1]&~3) << 3)|((src[blue_idx^2]&~7) << 8);
                ((ushort*)dst)[i] = (ushort)t;
            }
        else
            for( i = 0; i < size.width; i++, src += src_cn )
            {
                int t = (src[blue_idx] >> 3)|((src[1]&~7) << 2)|((src[blue_idx^2]&~7) << 7);
                ((ushort*)dst)[i] = (ushort)t;
            }
    }

    return CV_OK;
}

static CvStatus CV_STDCALL
icvBGRx2Gray_8u_CnC1R( const uchar* src, int srcstep,
                       uchar* dst, int dststep, CvSize size,
                       int src_cn, int blue_idx )
{
    int i;
    srcstep -= size.width*src_cn;

    if( size.width*size.height >= 1024 )
    {
        int* tab = (int*)cvStackAlloc( 256*3*sizeof(tab[0]) );
        int r = 0, g = 0, b = (1 << (csc_shift-1));
    
        for( i = 0; i < 256; i++ )
        {
            tab[i] = b;
            tab[i+256] = g;
            tab[i+512] = r;
            g += cscGg;
            if( !blue_idx )
                b += cscGb, r += cscGr;
            else
                b += cscGr, r += cscGb;
        }

        for( ; size.height--; src += srcstep, dst += dststep )
        {
            for( i = 0; i < size.width; i++, src += src_cn )
            {
                int t0 = tab[src[0]] + tab[src[1] + 256] + tab[src[2] + 512];
                dst[i] = (uchar)(t0 >> csc_shift);
            }
        }
    }
    else
    {
        for( ; size.height--; src += srcstep, dst += dststep )
        {
            for( i = 0; i < size.width; i++, src += src_cn )
            {
                int t0 = src[blue_idx]*cscGb + src[1]*cscGg + src[blue_idx^2]*cscGr;
                dst[i] = (uchar)CV_DESCALE(t0, csc_shift);
            }
        }
    }
    return CV_OK;
}

static CvStatus CV_STDCALL
icvBGRx2Gray_16u_CnC1R( const ushort* src, int srcstep,
                        ushort* dst, int dststep, CvSize size,
                        int src_cn, int blue_idx )
{
    int i;
    int cb = cscGb, cr = cscGr;
    srcstep /= sizeof(src[0]);
    dststep /= sizeof(dst[0]);
    srcstep -= size.width*src_cn;

    if( blue_idx )
        cb = cscGr, cr = cscGb;

    for( ; size.height--; src += srcstep, dst += dststep )
        for( i = 0; i < size.width; i++, src += src_cn )
            dst[i] = (ushort)CV_DESCALE((unsigned)(src[0]*cb +
                    src[1]*cscGg + src[2]*cr), csc_shift);

    return CV_OK;
}


static CvStatus CV_STDCALL
icvBGRx2Gray_32f_CnC1R( const float* src, int srcstep,
                        float* dst, int dststep, CvSize size,
                        int src_cn, int blue_idx )
{
    int i;
    float cb = cscGb_32f, cr = cscGr_32f;
    if( blue_idx )
        cb = cscGr_32f, cr = cscGb_32f;

    srcstep /= sizeof(src[0]);
    dststep /= sizeof(dst[0]);
    srcstep -= size.width*src_cn;
    for( ; size.height--; src += srcstep, dst += dststep )
        for( i = 0; i < size.width; i++, src += src_cn )
            dst[i] = src[0]*cb + src[1]*cscGg_32f + src[2]*cr;

    return CV_OK;
}

static CvStatus CV_STDCALL
icvBGR5x52Gray_8u_C2C1R( const uchar* src, int srcstep,
                         uchar* dst, int dststep,
                         CvSize size, int green_bits )
{
    int i;
    assert( green_bits == 5 || green_bits == 6 );

    for( ; size.height--; src += srcstep, dst += dststep )
    {
        if( green_bits == 6 )
            for( i = 0; i < size.width; i++ )
            {
                int t = ((ushort*)src)[i];
                t = ((t << 3) & 0xf8)*cscGb + ((t >> 3) & 0xfc)*cscGg +
                    ((t >> 8) & 0xf8)*cscGr;
                dst[i] = (uchar)CV_DESCALE(t,csc_shift);
            }
        else
            for( i = 0; i < size.width; i++ )
            {
                int t = ((ushort*)src)[i];
                t = ((t << 3) & 0xf8)*cscGb + ((t >> 2) & 0xf8)*cscGg +
                    ((t >> 7) & 0xf8)*cscGr;
                dst[i] = (uchar)CV_DESCALE(t,csc_shift);
            }
    }

    return CV_OK;
}


static CvStatus CV_STDCALL
icvGray2BGR5x5_8u_C1C2R( const uchar* src, int srcstep,
                         uchar* dst, int dststep,
                         CvSize size, int green_bits )
{
    int i;
    assert( green_bits == 5 || green_bits == 6 );

    for( ; size.height--; src += srcstep, dst += dststep )
    {
        if( green_bits == 6 )
            for( i = 0; i < size.width; i++ )
            {
                int t = src[i];
                ((ushort*)dst)[i] = (ushort)((t >> 3)|((t & ~3) << 3)|((t & ~7) << 8));
            }
        else
            for( i = 0; i < size.width; i++ )
            {
                int t = src[i] >> 3;
                ((ushort*)dst)[i] = (ushort)(t|(t << 5)|(t << 10));
            }
    }

    return CV_OK;
}

#define CV_IMPL_GRAY2BGRX( flavor, arrtype )                    \
static CvStatus CV_STDCALL                                      \
icvGray2BGRx_##flavor##_C1CnR( const arrtype* src, int srcstep, \
                       arrtype* dst, int dststep, CvSize size,  \
                       int dst_cn )                             \
{                                                               \
    int i;                                                      \
    srcstep /= sizeof(src[0]);                                  \
    dststep /= sizeof(src[0]);                                  \
    dststep -= size.width*dst_cn;                               \
                                                                \
    for( ; size.height--; src += srcstep, dst += dststep )      \
    {                                                           \
        if( dst_cn == 3 )                                       \
            for( i = 0; i < size.width; i++, dst += 3 )         \
                dst[0] = dst[1] = dst[2] = src[i];              \
        else                                                    \
            for( i = 0; i < size.width; i++, dst += 4 )         \
            {                                                   \
                dst[0] = dst[1] = dst[2] = src[i];              \
                dst[3] = 0;                                     \
            }                                                   \
    }                                                           \
                                                                \
    return CV_OK;                                               \
}


CV_IMPL_GRAY2BGRX( 8u, uchar )
CV_IMPL_GRAY2BGRX( 16u, ushort )
CV_IMPL_GRAY2BGRX( 32f, float )

#define CV_IMPL_BGRx2YCrCb( flavor, arrtype, worktype, scale_macro, cast_macro,     \
                            YUV_YB, YUV_YG, YUV_YR, YUV_CR, YUV_CB, YUV_Cx_BIAS )   \
static CvStatus CV_STDCALL                                                  \
icvBGRx2YCrCb_##flavor##_CnC3R( const arrtype* src, int srcstep,            \
    arrtype* dst, int dststep, CvSize size, int src_cn, int blue_idx )      \
{                                                                           \
    int i;                                                                  \
    srcstep /= sizeof(src[0]);                                              \
    dststep /= sizeof(src[0]);                                              \
    srcstep -= size.width*src_cn;                                           \
    size.width *= 3;                                                        \
                                                                            \
    for( ; size.height--; src += srcstep, dst += dststep )                  \
    {                                                                       \
        for( i = 0; i < size.width; i += 3, src += src_cn )                 \
        {                                                                   \
            worktype b = src[blue_idx], r = src[2^blue_idx], y;             \
            y = scale_macro(b*YUV_YB + src[1]*YUV_YG + r*YUV_YR);           \
            r = scale_macro((r - y)*YUV_CR) + YUV_Cx_BIAS;                  \
            b = scale_macro((b - y)*YUV_CB) + YUV_Cx_BIAS;                  \
            dst[i] = cast_macro(y);                                         \
            dst[i+1] = cast_macro(r);                                       \
            dst[i+2] = cast_macro(b);                                       \
        }                                                                   \
    }                                                                       \
                                                                            \
    return CV_OK;                                                           \
}


CV_IMPL_BGRx2YCrCb( 8u, uchar, int, yuv_descale, CV_CAST_8U,
                    yuvYb, yuvYg, yuvYr, yuvCr, yuvCb, 128 )

CV_IMPL_BGRx2YCrCb( 16u, ushort, int, yuv_descale, CV_CAST_16U,
                    yuvYb, yuvYg, yuvYr, yuvCr, yuvCb, 32768 )

CV_IMPL_BGRx2YCrCb( 32f, float, float, CV_NOP, CV_NOP,
                    yuvYb_32f, yuvYg_32f, yuvYr_32f, yuvCr_32f, yuvCb_32f, 0.5f )

					//////////////////////////////////

icvRGB2XYZ_8u_C3R_t  icvRGB2XYZ_8u_C3R_p = 0;

static CvStatus CV_STDCALL
icvBGRx2ABC_IPP_8u_CnC3R( const uchar* src, int srcstep,
    uchar* dst, int dststep, CvSize size, int src_cn,
    int blue_idx, CvColorCvtFunc0 ipp_func )
{
    int block_size = MIN(1 << 14, size.width);
    uchar* buffer;
    int i, di, k;
    int do_copy = src_cn > 3 || blue_idx != 2 || src == dst;
    CvStatus status = CV_OK;

    if( !do_copy )
        return ipp_func( src, srcstep, dst, dststep, size );

    srcstep /= sizeof(src[0]);
    dststep /= sizeof(dst[0]);

    buffer = (uchar*)cvStackAlloc( block_size*3*sizeof(buffer[0]) );
    srcstep -= size.width*src_cn;

    for( ; size.height--; src += srcstep, dst += dststep )
    {
        for( i = 0; i < size.width; i += block_size )
        {
            uchar* dst1 = dst + i*3;
            di = MIN(block_size, size.width - i);

            for( k = 0; k < di*3; k += 3, src += src_cn )
            {
                uchar b = src[blue_idx];
                uchar g = src[1];
                uchar r = src[blue_idx^2];
                buffer[k] = r;
                buffer[k+1] = g;
                buffer[k+2] = b;
            }

            status = ipp_func( buffer, CV_STUB_STEP,
                               dst1, CV_STUB_STEP, cvSize(di,1) );
            if( status < 0 )
                return status;
        }
    }

    return CV_OK;
}



#define CV_IMPL_BGRx2ABC_IPP( flavor, arrtype )                         \
static CvStatus CV_STDCALL                                              \
icvBGRx2ABC_IPP_##flavor##_CnC3R( const arrtype* src, int srcstep,      \
    arrtype* dst, int dststep, CvSize size, int src_cn,                 \
    int blue_idx, CvColorCvtFunc0 ipp_func )                            \
{                                                                       \
    int block_size = MIN(1 << 14, size.width);                          \
    arrtype* buffer;                                                    \
    int i, di, k;                                                       \
    int do_copy = src_cn > 3 || blue_idx != 2 || src == dst;            \
    CvStatus status = CV_OK;                                            \
                                                                        \
    if( !do_copy )                                                      \
        return ipp_func( src, srcstep, dst, dststep, size );            \
                                                                        \
    srcstep /= sizeof(src[0]);                                          \
    dststep /= sizeof(dst[0]);                                          \
                                                                        \
    buffer = (arrtype*)cvStackAlloc( block_size*3*sizeof(buffer[0]) );  \
    srcstep -= size.width*src_cn;                                       \
                                                                        \
    for( ; size.height--; src += srcstep, dst += dststep )              \
    {                                                                   \
        for( i = 0; i < size.width; i += block_size )                   \
        {                                                               \
            arrtype* dst1 = dst + i*3;                                  \
            di = MIN(block_size, size.width - i);                       \
                                                                        \
            for( k = 0; k < di*3; k += 3, src += src_cn )               \
            {                                                           \
                arrtype b = src[blue_idx];                              \
                arrtype g = src[1];                                     \
                arrtype r = src[blue_idx^2];                            \
                buffer[k] = r;                                          \
                buffer[k+1] = g;                                        \
                buffer[k+2] = b;                                        \
            }                                                           \
                                                                        \
            status = ipp_func( buffer, CV_STUB_STEP,                    \
                               dst1, CV_STUB_STEP, cvSize(di,1) );      \
            if( status < 0 )                                            \
                return status;                                          \
        }                                                               \
    }                                                                   \
                                                                        \
    return CV_OK;                                                       \
}

CV_IMPL_BGRx2ABC_IPP( 16u, ushort )
CV_IMPL_BGRx2ABC_IPP( 32f, float )

#define CV_IMPL_BGRx2XYZ( flavor, arrtype, worktype,                        \
                          scale_macro, cast_macro, suffix )                 \
static CvStatus CV_STDCALL                                                  \
icvBGRx2XYZ_##flavor##_CnC3R( const arrtype* src, int srcstep,              \
                              arrtype* dst, int dststep, CvSize size,       \
                              int src_cn, int blue_idx )                    \
{                                                                           \
    int i;                                                                  \
    worktype t, matrix[] =                                                  \
    {                                                                       \
        xyzXb##suffix, xyzXg##suffix, xyzXr##suffix,                        \
        xyzYb##suffix, xyzYg##suffix, xyzYr##suffix,                        \
        xyzZb##suffix, xyzZg##suffix, xyzZr##suffix                         \
    };                                                                      \
                                                                            \
    if( icvRGB2XYZ_##flavor##_C3R_p )                                       \
        return icvBGRx2ABC_IPP_##flavor##_CnC3R( src, srcstep,              \
            dst, dststep, size, src_cn, blue_idx,                           \
            icvRGB2XYZ_##flavor##_C3R_p );                                  \
                                                                            \
    srcstep /= sizeof(src[0]);                                              \
    dststep /= sizeof(dst[0]);                                              \
    srcstep -= size.width*src_cn;                                           \
    size.width *= 3;                                                        \
                                                                            \
    if( blue_idx )                                                          \
    {                                                                       \
        CV_SWAP( matrix[0], matrix[2], t );                                 \
        CV_SWAP( matrix[3], matrix[5], t );                                 \
        CV_SWAP( matrix[6], matrix[8], t );                                 \
    }                                                                       \
                                                                            \
    for( ; size.height--; src += srcstep, dst += dststep )                  \
    {                                                                       \
        for( i = 0; i < size.width; i += 3, src += src_cn )                 \
        {                                                                   \
            worktype x = scale_macro(src[0]*matrix[0] +                     \
                    src[1]*matrix[1] + src[2]*matrix[2]);                   \
            worktype y = scale_macro(src[0]*matrix[3] +                     \
                    src[1]*matrix[4] + src[2]*matrix[5]);                   \
            worktype z = scale_macro(src[0]*matrix[6] +                     \
                    src[1]*matrix[7] + src[2]*matrix[8]);                   \
                                                                            \
            dst[i] = (arrtype)(x);                                          \
            dst[i+1] = (arrtype)(y);                                        \
            dst[i+2] = cast_macro(z); /*sum of weights for z > 1*/          \
        }                                                                   \
    }                                                                       \
                                                                            \
    return CV_OK;                                                           \
}


CV_IMPL_BGRx2XYZ( 8u, uchar, int, xyz_descale, CV_CAST_8U, _32s )
CV_IMPL_BGRx2XYZ( 16u, ushort, int, xyz_descale, CV_CAST_16U, _32s )
CV_IMPL_BGRx2XYZ( 32f, float, float, CV_NOP, CV_NOP, _32f )

static const uchar icvHue255To180[] =
{
      0,   1,   1,   2,   3,   4,   4,   5,   6,   6,   7,   8,   8,   9,  10,  11,
     11,  12,  13,  13,  14,  15,  16,  16,  17,  18,  18,  19,  20,  20,  21,  22,
     23,  23,  24,  25,  25,  26,  27,  28,  28,  29,  30,  30,  31,  32,  32,  33,
     34,  35,  35,  36,  37,  37,  38,  39,  40,  40,  41,  42,  42,  43,  44,  44,
     45,  46,  47,  47,  48,  49,  49,  50,  51,  52,  52,  53,  54,  54,  55,  56,
     56,  57,  58,  59,  59,  60,  61,  61,  62,  63,  64,  64,  65,  66,  66,  67,
     68,  68,  69,  70,  71,  71,  72,  73,  73,  74,  75,  76,  76,  77,  78,  78,
     79,  80,  80,  81,  82,  83,  83,  84,  85,  85,  86,  87,  88,  88,  89,  90,
     90,  91,  92,  92,  93,  94,  95,  95,  96,  97,  97,  98,  99, 100, 100, 101,
    102, 102, 103, 104, 104, 105, 106, 107, 107, 108, 109, 109, 110, 111, 112, 112,
    113, 114, 114, 115, 116, 116, 117, 118, 119, 119, 120, 121, 121, 122, 123, 124,
    124, 125, 126, 126, 127, 128, 128, 129, 130, 131, 131, 132, 133, 133, 134, 135,
    136, 136, 137, 138, 138, 139, 140, 140, 141, 142, 143, 143, 144, 145, 145, 146,
    147, 148, 148, 149, 150, 150, 151, 152, 152, 153, 154, 155, 155, 156, 157, 157,
    158, 159, 160, 160, 161, 162, 162, 163, 164, 164, 165, 166, 167, 167, 168, 169,
    169, 170, 171, 172, 172, 173, 174, 174, 175, 176, 176, 177, 178, 179, 179, 180
};

static const uchar icvHue180To255[] =
{
      0,   1,   3,   4,   6,   7,   9,  10,  11,  13,  14,  16,  17,  18,  20,  21,
     23,  24,  26,  27,  28,  30,  31,  33,  34,  35,  37,  38,  40,  41,  43,  44,
     45,  47,  48,  50,  51,  52,  54,  55,  57,  58,  60,  61,  62,  64,  65,  67,
     68,  69,  71,  72,  74,  75,  77,  78,  79,  81,  82,  84,  85,  86,  88,  89,
     91,  92,  94,  95,  96,  98,  99, 101, 102, 103, 105, 106, 108, 109, 111, 112,
    113, 115, 116, 118, 119, 120, 122, 123, 125, 126, 128, 129, 130, 132, 133, 135,
    136, 137, 139, 140, 142, 143, 145, 146, 147, 149, 150, 152, 153, 154, 156, 157,
    159, 160, 162, 163, 164, 166, 167, 169, 170, 171, 173, 174, 176, 177, 179, 180,
    181, 183, 184, 186, 187, 188, 190, 191, 193, 194, 196, 197, 198, 200, 201, 203,
    204, 205, 207, 208, 210, 211, 213, 214, 215, 217, 218, 220, 221, 222, 224, 225,
    227, 228, 230, 231, 232, 234, 235, 237, 238, 239, 241, 242, 244, 245, 247, 248,
    249, 251, 252, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
};

static CvStatus CV_STDCALL
icvBGRx2HSV_8u_CnC3R( const uchar* src, int srcstep, uchar* dst, int dststep,
                      CvSize size, int src_cn, int blue_idx )
{
    const int hsv_shift = 12;

    static const int div_table[] = {
        0, 1044480, 522240, 348160, 261120, 208896, 174080, 149211,
        130560, 116053, 104448, 94953, 87040, 80345, 74606, 69632,
        65280, 61440, 58027, 54973, 52224, 49737, 47476, 45412,
        43520, 41779, 40172, 38684, 37303, 36017, 34816, 33693,
        32640, 31651, 30720, 29842, 29013, 28229, 27486, 26782,
        26112, 25475, 24869, 24290, 23738, 23211, 22706, 22223,
        21760, 21316, 20890, 20480, 20086, 19707, 19342, 18991,
        18651, 18324, 18008, 17703, 17408, 17123, 16846, 16579,
        16320, 16069, 15825, 15589, 15360, 15137, 14921, 14711,
        14507, 14308, 14115, 13926, 13743, 13565, 13391, 13221,
        13056, 12895, 12738, 12584, 12434, 12288, 12145, 12006,
        11869, 11736, 11605, 11478, 11353, 11231, 11111, 10995,
        10880, 10768, 10658, 10550, 10445, 10341, 10240, 10141,
        10043, 9947, 9854, 9761, 9671, 9582, 9495, 9410,
        9326, 9243, 9162, 9082, 9004, 8927, 8852, 8777,
        8704, 8632, 8561, 8492, 8423, 8356, 8290, 8224,
        8160, 8097, 8034, 7973, 7913, 7853, 7795, 7737,
        7680, 7624, 7569, 7514, 7461, 7408, 7355, 7304,
        7253, 7203, 7154, 7105, 7057, 7010, 6963, 6917,
        6872, 6827, 6782, 6739, 6695, 6653, 6611, 6569,
        6528, 6487, 6447, 6408, 6369, 6330, 6292, 6254,
        6217, 6180, 6144, 6108, 6073, 6037, 6003, 5968,
        5935, 5901, 5868, 5835, 5803, 5771, 5739, 5708,
        5677, 5646, 5615, 5585, 5556, 5526, 5497, 5468,
        5440, 5412, 5384, 5356, 5329, 5302, 5275, 5249,
        5222, 5196, 5171, 5145, 5120, 5095, 5070, 5046,
        5022, 4998, 4974, 4950, 4927, 4904, 4881, 4858,
        4836, 4813, 4791, 4769, 4748, 4726, 4705, 4684,
        4663, 4642, 4622, 4601, 4581, 4561, 4541, 4522,
        4502, 4483, 4464, 4445, 4426, 4407, 4389, 4370,
        4352, 4334, 4316, 4298, 4281, 4263, 4246, 4229,
        4212, 4195, 4178, 4161, 4145, 4128, 4112, 4096
    };

    int i;
    if( icvRGB2HSV_8u_C3R_p )
    {
        CvStatus status = icvBGRx2ABC_IPP_8u_CnC3R( src, srcstep, dst, dststep, size,
                                                src_cn, blue_idx, icvRGB2HSV_8u_C3R_p );
        if( status >= 0 )
        {
            size.width *= 3;
            for( ; size.height--; dst += dststep )
            {
                for( i = 0; i <= size.width - 12; i += 12 )
                {
                    uchar t0 = icvHue255To180[dst[i]], t1 = icvHue255To180[dst[i+3]];
                    dst[i] = t0; dst[i+3] = t1;
                    t0 = icvHue255To180[dst[i+6]]; t1 = icvHue255To180[dst[i+9]];
                    dst[i+6] = t0; dst[i+9] = t1;
                }
                for( ; i < size.width; i += 3 )
                    dst[i] = icvHue255To180[dst[i]];
            }
        }
        return status;
    }

    srcstep -= size.width*src_cn;
    size.width *= 3;

    for( ; size.height--; src += srcstep, dst += dststep )
    {
        for( i = 0; i < size.width; i += 3, src += src_cn )
        {
            int b = (src)[blue_idx], g = (src)[1], r = (src)[2^blue_idx];
            int h, s, v = b;
            int vmin = b, diff;
            int vr, vg;

            CV_CALC_MAX_8U( v, g );
            CV_CALC_MAX_8U( v, r );
            CV_CALC_MIN_8U( vmin, g );
            CV_CALC_MIN_8U( vmin, r );

            diff = v - vmin;
            vr = v == r ? -1 : 0;
            vg = v == g ? -1 : 0;

            s = diff * div_table[v] >> hsv_shift;
            h = (vr & (g - b)) +
                (~vr & ((vg & (b - r + 2 * diff)) + ((~vg) & (r - g + 4 * diff))));
            h = ((h * div_table[diff] * 15 + (1 << (hsv_shift + 6))) >> (7 + hsv_shift))\
                + (h < 0 ? 30*6 : 0);

            dst[i] = (uchar)h;
            dst[i+1] = (uchar)s;
            dst[i+2] = (uchar)v;
        }
    }

    return CV_OK;
}


#define  labXr_32f  0.433953f /* = xyzXr_32f / 0.950456 */
#define  labXg_32f  0.376219f /* = xyzXg_32f / 0.950456 */
#define  labXb_32f  0.189828f /* = xyzXb_32f / 0.950456 */

#define  labYr_32f  0.212671f /* = xyzYr_32f */
#define  labYg_32f  0.715160f /* = xyzYg_32f */ 
#define  labYb_32f  0.072169f /* = xyzYb_32f */ 

#define  labZr_32f  0.017758f /* = xyzZr_32f / 1.088754 */
#define  labZg_32f  0.109477f /* = xyzZg_32f / 1.088754 */
#define  labZb_32f  0.872766f /* = xyzZb_32f / 1.088754 */

#define  labRx_32f  3.0799327f  /* = xyzRx_32f * 0.950456 */
#define  labRy_32f  (-1.53715f) /* = xyzRy_32f */
#define  labRz_32f  (-0.542782f)/* = xyzRz_32f * 1.088754 */

#define  labGx_32f  (-0.921235f)/* = xyzGx_32f * 0.950456 */
#define  labGy_32f  1.875991f   /* = xyzGy_32f */ 
#define  labGz_32f  0.04524426f /* = xyzGz_32f * 1.088754 */

#define  labBx_32f  0.0528909755f /* = xyzBx_32f * 0.950456 */
#define  labBy_32f  (-0.204043f)  /* = xyzBy_32f */
#define  labBz_32f  1.15115158f   /* = xyzBz_32f * 1.088754 */

#define  labT_32f   0.008856f

#define labT   fix(labT_32f*255,lab_shift)

#undef lab_shift
#define lab_shift 10
#define labXr  fix(labXr_32f,lab_shift)
#define labXg  fix(labXg_32f,lab_shift)
#define labXb  fix(labXb_32f,lab_shift)
                            
#define labYr  fix(labYr_32f,lab_shift)
#define labYg  fix(labYg_32f,lab_shift)
#define labYb  fix(labYb_32f,lab_shift)
                            
#define labZr  fix(labZr_32f,lab_shift)
#define labZg  fix(labZg_32f,lab_shift)
#define labZb  fix(labZb_32f,lab_shift)

#define labSmallScale_32f  7.787f
#define labSmallShift_32f  0.13793103448275862f  /* 16/116 */
#define labLScale_32f      116.f
#define labLShift_32f      16.f
#define labLScale2_32f     903.3f

#define labSmallScale fix(31.27 /* labSmallScale_32f*(1<<lab_shift)/255 */,lab_shift)
#define labSmallShift fix(141.24138 /* labSmallScale_32f*(1<<lab) */,lab_shift)
#define labLScale fix(295.8 /* labLScale_32f*255/100 */,lab_shift)
#define labLShift fix(41779.2 /* labLShift_32f*1024*255/100 */,lab_shift)
#define labLScale2 fix(labLScale2_32f*0.01,lab_shift)

/* 1024*(([0..511]./255)**(1./3)) */
static ushort icvLabCubeRootTab[] = {
       0,  161,  203,  232,  256,  276,  293,  308,  322,  335,  347,  359,  369,  379,  389,  398,
     406,  415,  423,  430,  438,  445,  452,  459,  465,  472,  478,  484,  490,  496,  501,  507,
     512,  517,  523,  528,  533,  538,  542,  547,  552,  556,  561,  565,  570,  574,  578,  582,
     586,  590,  594,  598,  602,  606,  610,  614,  617,  621,  625,  628,  632,  635,  639,  642,
     645,  649,  652,  655,  659,  662,  665,  668,  671,  674,  677,  680,  684,  686,  689,  692,
     695,  698,  701,  704,  707,  710,  712,  715,  718,  720,  723,  726,  728,  731,  734,  736,
     739,  741,  744,  747,  749,  752,  754,  756,  759,  761,  764,  766,  769,  771,  773,  776,
     778,  780,  782,  785,  787,  789,  792,  794,  796,  798,  800,  803,  805,  807,  809,  811,
     813,  815,  818,  820,  822,  824,  826,  828,  830,  832,  834,  836,  838,  840,  842,  844,
     846,  848,  850,  852,  854,  856,  857,  859,  861,  863,  865,  867,  869,  871,  872,  874,
     876,  878,  880,  882,  883,  885,  887,  889,  891,  892,  894,  896,  898,  899,  901,  903,
     904,  906,  908,  910,  911,  913,  915,  916,  918,  920,  921,  923,  925,  926,  928,  929,
     931,  933,  934,  936,  938,  939,  941,  942,  944,  945,  947,  949,  950,  952,  953,  955,
     956,  958,  959,  961,  962,  964,  965,  967,  968,  970,  971,  973,  974,  976,  977,  979,
     980,  982,  983,  985,  986,  987,  989,  990,  992,  993,  995,  996,  997,  999, 1000, 1002,
    1003, 1004, 1006, 1007, 1009, 1010, 1011, 1013, 1014, 1015, 1017, 1018, 1019, 1021, 1022, 1024,
    1025, 1026, 1028, 1029, 1030, 1031, 1033, 1034, 1035, 1037, 1038, 1039, 1041, 1042, 1043, 1044,
    1046, 1047, 1048, 1050, 1051, 1052, 1053, 1055, 1056, 1057, 1058, 1060, 1061, 1062, 1063, 1065,
    1066, 1067, 1068, 1070, 1071, 1072, 1073, 1074, 1076, 1077, 1078, 1079, 1081, 1082, 1083, 1084,
    1085, 1086, 1088, 1089, 1090, 1091, 1092, 1094, 1095, 1096, 1097, 1098, 1099, 1101, 1102, 1103,
    1104, 1105, 1106, 1107, 1109, 1110, 1111, 1112, 1113, 1114, 1115, 1117, 1118, 1119, 1120, 1121,
    1122, 1123, 1124, 1125, 1127, 1128, 1129, 1130, 1131, 1132, 1133, 1134, 1135, 1136, 1138, 1139,
    1140, 1141, 1142, 1143, 1144, 1145, 1146, 1147, 1148, 1149, 1150, 1151, 1152, 1154, 1155, 1156,
    1157, 1158, 1159, 1160, 1161, 1162, 1163, 1164, 1165, 1166, 1167, 1168, 1169, 1170, 1171, 1172,
    1173, 1174, 1175, 1176, 1177, 1178, 1179, 1180, 1181, 1182, 1183, 1184, 1185, 1186, 1187, 1188,
    1189, 1190, 1191, 1192, 1193, 1194, 1195, 1196, 1197, 1198, 1199, 1200, 1201, 1202, 1203, 1204,
    1205, 1206, 1207, 1208, 1209, 1210, 1211, 1212, 1213, 1214, 1215, 1215, 1216, 1217, 1218, 1219,
    1220, 1221, 1222, 1223, 1224, 1225, 1226, 1227, 1228, 1229, 1230, 1230, 1231, 1232, 1233, 1234,
    1235, 1236, 1237, 1238, 1239, 1240, 1241, 1242, 1242, 1243, 1244, 1245, 1246, 1247, 1248, 1249,
    1250, 1251, 1251, 1252, 1253, 1254, 1255, 1256, 1257, 1258, 1259, 1259, 1260, 1261, 1262, 1263,
    1264, 1265, 1266, 1266, 1267, 1268, 1269, 1270, 1271, 1272, 1273, 1273, 1274, 1275, 1276, 1277,
    1278, 1279, 1279, 1280, 1281, 1282, 1283, 1284, 1285, 1285, 1286, 1287, 1288, 1289, 1290, 1291
};

static CvStatus CV_STDCALL
icvBGRx2Lab_8u_CnC3R( const uchar* src, int srcstep, uchar* dst, int dststep,
                      CvSize size, int src_cn, int blue_idx )
{
    int i;

    /*if( icvBGR2Lab_8u_C3R_p )
        return icvBGRx2ABC_IPP_8u_CnC3R( src, srcstep, dst, dststep, size,
                                         src_cn, blue_idx^2, icvBGR2Lab_8u_C3R_p );*/

    srcstep -= size.width*src_cn;
    size.width *= 3;

    for( ; size.height--; src += srcstep, dst += dststep )
    {
        for( i = 0; i < size.width; i += 3, src += src_cn )
        {
            int b = src[blue_idx], g = src[1], r = src[2^blue_idx];
            int x, y, z, f;
            int L, a;

            x = b*labXb + g*labXg + r*labXr;
            y = b*labYb + g*labYg + r*labYr;
            z = b*labZb + g*labZg + r*labZr;

            f = x > labT;
            x = CV_DESCALE( x, lab_shift );

            if( f )
                assert( (unsigned)x < 512 ), x = icvLabCubeRootTab[x];
            else
                x = CV_DESCALE(x*labSmallScale + labSmallShift,lab_shift);

            f = z > labT;
            z = CV_DESCALE( z, lab_shift );

            if( f )
                assert( (unsigned)z < 512 ), z = icvLabCubeRootTab[z];
            else
                z = CV_DESCALE(z*labSmallScale + labSmallShift,lab_shift);

            f = y > labT;
            y = CV_DESCALE( y, lab_shift );

            if( f )
            {
                assert( (unsigned)y < 512 ), y = icvLabCubeRootTab[y];
                L = CV_DESCALE(y*labLScale - labLShift, 2*lab_shift );
            }
            else
            {
                L = CV_DESCALE(y*labLScale2,lab_shift);
                y = CV_DESCALE(y*labSmallScale + labSmallShift,lab_shift);
            }

            a = CV_DESCALE( 500*(x - y), lab_shift ) + 128;
            b = CV_DESCALE( 200*(y - z), lab_shift ) + 128;

            dst[i] = CV_CAST_8U(L);
            dst[i+1] = CV_CAST_8U(a);
            dst[i+2] = CV_CAST_8U(b);
        }
    }

    return CV_OK;
}

#define luvUn_32f  0.19793943f 
#define luvVn_32f  0.46831096f 
#define luvYmin_32f  0.05882353f /* 15/255 */

static CvStatus CV_STDCALL
icvBGRx2Luv_32f_CnC3R( const float* src, int srcstep, float* dst, int dststep,
                       CvSize size, int src_cn, int blue_idx )
{
    int i;

    /*if( icvRGB2Luv_32f_C3R_p )
        return icvBGRx2ABC_IPP_32f_CnC3R( src, srcstep, dst, dststep, size,
                                          src_cn, blue_idx, icvRGB2Luv_32f_C3R_p );*/

    srcstep /= sizeof(src[0]);
    dststep /= sizeof(dst[0]);
    srcstep -= size.width*src_cn;
    size.width *= 3;

    for( ; size.height--; src += srcstep, dst += dststep )
    {
        for( i = 0; i < size.width; i += 3, src += src_cn )
        {
            float b = src[blue_idx], g = src[1], r = src[2^blue_idx];
            float x, y, z;
            float L, u, v, t;

            x = b*xyzXb_32f + g*xyzXg_32f + r*xyzXr_32f;
            y = b*xyzYb_32f + g*xyzYg_32f + r*xyzYr_32f;
            z = b*xyzZb_32f + g*xyzZg_32f + r*xyzZr_32f;

            if( !x && !y && !z )
                L = u = v = 0.f;
            else
            {
                if( y > labT_32f )
                    L = labLScale_32f * cvCbrt(y) - labLShift_32f;
                else
                    L = labLScale2_32f * y;

                t = 1.f / (x + 15 * y + 3 * z);            
                u = 4.0f * x * t;
                v = 9.0f * y * t;

                u = 13*L*(u - luvUn_32f);
                v = 13*L*(v - luvVn_32f);
            }

            dst[i] = L;
            dst[i+1] = u;
            dst[i+2] = v;
        }
    }

    return CV_OK;
}

static CvStatus CV_STDCALL
icvBGRx2ABC_8u_CnC3R( const uchar* src, int srcstep, uchar* dst, int dststep,
                      CvSize size, int src_cn, int blue_idx, CvColorCvtFunc2 cvtfunc_32f,
                      int prescale, const float* post_coeffs )
{
    int block_size = MIN(1 << 8, size.width);
    float* buffer = (float*)cvStackAlloc( block_size*3*sizeof(buffer[0]) );
    int i, di, k;
    CvStatus status = CV_OK;

    srcstep -= size.width*src_cn;

    for( ; size.height--; src += srcstep, dst += dststep )
    {
        for( i = 0; i < size.width; i += block_size )
        {
            uchar* dst1 = dst + i*3;
            di = MIN(block_size, size.width - i);

            if( prescale )
            {
                for( k = 0; k < di*3; k += 3, src += src_cn )
                {
                    float b = CV_8TO32F(src[0])*0.0039215686274509803f;
                    float g = CV_8TO32F(src[1])*0.0039215686274509803f;
                    float r = CV_8TO32F(src[2])*0.0039215686274509803f;

                    buffer[k] = b;
                    buffer[k+1] = g;
                    buffer[k+2] = r;
                }
            }
            else
            {
                for( k = 0; k < di*3; k += 3, src += src_cn )
                {
                    float b = CV_8TO32F(src[0]);
                    float g = CV_8TO32F(src[1]);
                    float r = CV_8TO32F(src[2]);

                    buffer[k] = b;
                    buffer[k+1] = g;
                    buffer[k+2] = r;
                }
            }
            
            status = cvtfunc_32f( buffer, 0, buffer, 0, cvSize(di,1), 3, blue_idx );
            if( status < 0 )
                return status;

            for( k = 0; k < di*3; k += 3 )
            {
                int a = cvRound( buffer[k]*post_coeffs[0] + post_coeffs[1] );
                int b = cvRound( buffer[k+1]*post_coeffs[2] + post_coeffs[3] );
                int c = cvRound( buffer[k+2]*post_coeffs[4] + post_coeffs[5] );
                dst1[k] = CV_CAST_8U(a);
                dst1[k+1] = CV_CAST_8U(b);
                dst1[k+2] = CV_CAST_8U(c);
            }
        }
    }

    return CV_OK;
}

static CvStatus CV_STDCALL
icvBGRx2Luv_8u_CnC3R( const uchar* src, int srcstep, uchar* dst, int dststep,
                      CvSize size, int src_cn, int blue_idx )
{
    // L: [0..100] -> [0..255]
    // u: [-134..220] -> [0..255]
    // v: [-140..122] -> [0..255]
    //static const float post_coeffs[] = { 2.55f, 0.f, 1.f, 83.f, 1.f, 140.f };
    static const float post_coeffs[] = { 2.55f, 0.f, 0.72033898305084743f, 96.525423728813564f,
                                         0.99609375f, 139.453125f };

    if( icvRGB2Luv_8u_C3R_p )
        return icvBGRx2ABC_IPP_8u_CnC3R( src, srcstep, dst, dststep, size,
                                         src_cn, blue_idx, icvRGB2Luv_8u_C3R_p );

    return icvBGRx2ABC_8u_CnC3R( src, srcstep, dst, dststep, size, src_cn, blue_idx,
                                 (CvColorCvtFunc2)icvBGRx2Luv_32f_CnC3R, 1, post_coeffs );
}

static CvStatus CV_STDCALL
icvBGRx2HLS_32f_CnC3R( const float* src, int srcstep, float* dst, int dststep,
                       CvSize size, int src_cn, int blue_idx )
{
    int i;

    if( icvRGB2HLS_32f_C3R_p )
    {
        CvStatus status = icvBGRx2ABC_IPP_32f_CnC3R( src, srcstep, dst, dststep, size,
                                                     src_cn, blue_idx, icvRGB2HLS_32f_C3R_p );
        if( status >= 0 )
        {
            size.width *= 3;
            dststep /= sizeof(dst[0]);

            for( ; size.height--; dst += dststep )
            {
                for( i = 0; i <= size.width - 12; i += 12 )
                {
                    float t0 = dst[i]*360.f, t1 = dst[i+3]*360.f;
                    dst[i] = t0; dst[i+3] = t1;
                    t0 = dst[i+6]*360.f; t1 = dst[i+9]*360.f;
                    dst[i+6] = t0; dst[i+9] = t1;
                }
                for( ; i < size.width; i += 3 )
                    dst[i] = dst[i]*360.f;
            }
        }
        return status;
    }

    srcstep /= sizeof(src[0]);
    dststep /= sizeof(dst[0]);
    srcstep -= size.width*src_cn;
    size.width *= 3;

    for( ; size.height--; src += srcstep, dst += dststep )
    {
        for( i = 0; i < size.width; i += 3, src += src_cn )
        {
            float b = src[blue_idx], g = src[1], r = src[2^blue_idx];
            float h = 0.f, s = 0.f, l;
            float vmin, vmax, diff;

            vmax = vmin = r;
            if( vmax < g ) vmax = g;
            if( vmax < b ) vmax = b;
            if( vmin > g ) vmin = g;
            if( vmin > b ) vmin = b;

            diff = vmax - vmin;
            l = (vmax + vmin)*0.5f;

            if( diff > FLT_EPSILON )
            {
                s = l < 0.5f ? diff/(vmax + vmin) : diff/(2 - vmax - vmin);
                diff = 60.f/diff;

                if( vmax == r )
                    h = (g - b)*diff;
                else if( vmax == g )
                    h = (b - r)*diff + 120.f;
                else
                    h = (r - g)*diff + 240.f;

                if( h < 0.f ) h += 360.f;
            }

            dst[i] = h;
            dst[i+1] = l;
            dst[i+2] = s;
        }
    }

    return CV_OK;
}


static CvStatus CV_STDCALL
icvBGRx2HLS_8u_CnC3R( const uchar* src, int srcstep, uchar* dst, int dststep,
                      CvSize size, int src_cn, int blue_idx )
{
    static const float post_coeffs[] = { 0.5f, 0.f, 255.f, 0.f, 255.f, 0.f };

    if( icvRGB2HLS_8u_C3R_p )
    {
        CvStatus status = icvBGRx2ABC_IPP_8u_CnC3R( src, srcstep, dst, dststep, size,
                                                    src_cn, blue_idx, icvRGB2HLS_8u_C3R_p );
        if( status >= 0 )
        {
            size.width *= 3;
            for( ; size.height--; dst += dststep )
            {
                int i;
                for( i = 0; i <= size.width - 12; i += 12 )
                {
                    uchar t0 = icvHue255To180[dst[i]], t1 = icvHue255To180[dst[i+3]];
                    dst[i] = t0; dst[i+3] = t1;
                    t0 = icvHue255To180[dst[i+6]]; t1 = icvHue255To180[dst[i+9]];
                    dst[i+6] = t0; dst[i+9] = t1;
                }
                for( ; i < size.width; i += 3 )
                    dst[i] = icvHue255To180[dst[i]];
            }
        }
        return status;
    }

    return icvBGRx2ABC_8u_CnC3R( src, srcstep, dst, dststep, size, src_cn, blue_idx,
                                 (CvColorCvtFunc2)icvBGRx2HLS_32f_CnC3R, 1, post_coeffs );
}

static CvStatus CV_STDCALL
icvBGRx2HSV_32f_CnC3R( const float* src, int srcstep,
                       float* dst, int dststep,
                       CvSize size, int src_cn, int blue_idx )
{
    int i;
    srcstep /= sizeof(src[0]);
    dststep /= sizeof(dst[0]);
    srcstep -= size.width*src_cn;
    size.width *= 3;

    for( ; size.height--; src += srcstep, dst += dststep )
    {
        for( i = 0; i < size.width; i += 3, src += src_cn )
        {
            float b = src[blue_idx], g = src[1], r = src[2^blue_idx];
            float h, s, v;

            float vmin, diff;

            v = vmin = r;
            if( v < g ) v = g;
            if( v < b ) v = b;
            if( vmin > g ) vmin = g;
            if( vmin > b ) vmin = b;

            diff = v - vmin;
            s = diff/(float)(fabs(v) + FLT_EPSILON);
            diff = (float)(60./(diff + FLT_EPSILON));
            if( v == r )
                h = (g - b)*diff;
            else if( v == g )
                h = (b - r)*diff + 120.f;
            else
                h = (r - g)*diff + 240.f;

            if( h < 0 ) h += 360.f;

            dst[i] = h;
            dst[i+1] = s;
            dst[i+2] = v;
        }
    }

    return CV_OK;
}

static CvStatus CV_STDCALL
icvBGRx2Lab_32f_CnC3R( const float* src, int srcstep, float* dst, int dststep,
                       CvSize size, int src_cn, int blue_idx )
{
    int i;
    srcstep /= sizeof(src[0]);
    dststep /= sizeof(dst[0]);
    srcstep -= size.width*src_cn;
    size.width *= 3;

    for( ; size.height--; src += srcstep, dst += dststep )
    {
        for( i = 0; i < size.width; i += 3, src += src_cn )
        {
            float b = src[blue_idx], g = src[1], r = src[2^blue_idx];
            float x, y, z;
            float L, a;

            x = b*labXb_32f + g*labXg_32f + r*labXr_32f;
            y = b*labYb_32f + g*labYg_32f + r*labYr_32f;
            z = b*labZb_32f + g*labZg_32f + r*labZr_32f;

            if( x > labT_32f )
                x = cvCbrt(x);
            else
                x = x*labSmallScale_32f + labSmallShift_32f;

            if( z > labT_32f )
                z = cvCbrt(z);
            else
                z = z*labSmallScale_32f + labSmallShift_32f;

            if( y > labT_32f )
            {
                y = cvCbrt(y);
                L = y*labLScale_32f - labLShift_32f;
            }
            else
            {
                L = y*labLScale2_32f;
                y = y*labSmallScale_32f + labSmallShift_32f;
            }

            a = 500.f*(x - y);
            b = 200.f*(y - z);

            dst[i] = L;
            dst[i+1] = a;
            dst[i+2] = b;
        }
    }

    return CV_OK;
}

#define CV_IMPL_YCrCb2BGRx( flavor, arrtype, worktype, prescale_macro,      \
    scale_macro, cast_macro, YUV_BCb, YUV_GCr, YUV_GCb, YUV_RCr, YUV_Cx_BIAS)\
static CvStatus CV_STDCALL                                                  \
icvYCrCb2BGRx_##flavor##_C3CnR( const arrtype* src, int srcstep,            \
                                arrtype* dst, int dststep, CvSize size,     \
                                int dst_cn, int blue_idx )                  \
{                                                                           \
    int i;                                                                  \
    srcstep /= sizeof(src[0]);                                              \
    dststep /= sizeof(src[0]);                                              \
    dststep -= size.width*dst_cn;                                           \
    size.width *= 3;                                                        \
                                                                            \
    for( ; size.height--; src += srcstep, dst += dststep )                  \
    {                                                                       \
        for( i = 0; i < size.width; i += 3, dst += dst_cn )                 \
        {                                                                   \
            worktype Y = prescale_macro(src[i]),                            \
                     Cr = src[i+1] - YUV_Cx_BIAS,                           \
                     Cb = src[i+2] - YUV_Cx_BIAS;                           \
            worktype b, g, r;                                               \
            b = scale_macro( Y + YUV_BCb*Cb );                              \
            g = scale_macro( Y + YUV_GCr*Cr + YUV_GCb*Cb );                 \
            r = scale_macro( Y + YUV_RCr*Cr );                              \
                                                                            \
            dst[blue_idx] = cast_macro(b);                                  \
            dst[1] = cast_macro(g);                                         \
            dst[blue_idx^2] = cast_macro(r);                                \
            if( dst_cn == 4 )                                               \
                dst[3] = 0;                                                 \
        }                                                                   \
    }                                                                       \
                                                                            \
    return CV_OK;                                                           \
}


CV_IMPL_YCrCb2BGRx( 8u, uchar, int, yuv_prescale, yuv_descale, CV_CAST_8U,
                    yuvBCb, yuvGCr, yuvGCb, yuvRCr, 128 )

CV_IMPL_YCrCb2BGRx( 16u, ushort, int, yuv_prescale, yuv_descale, CV_CAST_16U,
                    yuvBCb, yuvGCr, yuvGCb, yuvRCr, 32768 )

CV_IMPL_YCrCb2BGRx( 32f, float, float, CV_NOP, CV_NOP, CV_NOP,
                    yuvBCb_32f, yuvGCr_32f, yuvGCb_32f, yuvRCr_32f, 0.5f )


#define CV_IMPL_ABC2BGRx_IPP( flavor, arrtype )                         \
static CvStatus CV_STDCALL                                              \
icvABC2BGRx_IPP_##flavor##_C3CnR( const arrtype* src, int srcstep,      \
    arrtype* dst, int dststep, CvSize size, int dst_cn,                 \
    int blue_idx, CvColorCvtFunc0 ipp_func )                            \
{                                                                       \
    int block_size = MIN(1 << 10, size.width);                          \
    arrtype* buffer;                                                    \
    int i, di, k;                                                       \
    int do_copy = dst_cn > 3 || blue_idx != 2 || src == dst;            \
    CvStatus status = CV_OK;                                            \
                                                                        \
    if( !do_copy )                                                      \
        return ipp_func( src, srcstep, dst, dststep, size );            \
                                                                        \
    srcstep /= sizeof(src[0]);                                          \
    dststep /= sizeof(dst[0]);                                          \
                                                                        \
    buffer = (arrtype*)cvStackAlloc( block_size*3*sizeof(buffer[0]) );  \
    dststep -= size.width*dst_cn;                                       \
                                                                        \
    for( ; size.height--; src += srcstep, dst += dststep )              \
    {                                                                   \
        for( i = 0; i < size.width; i += block_size )                   \
        {                                                               \
            const arrtype* src1 = src + i*3;                            \
            di = MIN(block_size, size.width - i);                       \
                                                                        \
            status = ipp_func( src1, CV_STUB_STEP,                      \
                               buffer, CV_STUB_STEP, cvSize(di,1) );    \
            if( status < 0 )                                            \
                return status;                                          \
                                                                        \
            for( k = 0; k < di*3; k += 3, dst += dst_cn )               \
            {                                                           \
                arrtype r = buffer[k];                                  \
                arrtype g = buffer[k+1];                                \
                arrtype b = buffer[k+2];                                \
                dst[blue_idx] = b;                                      \
                dst[1] = g;                                             \
                dst[blue_idx^2] = r;                                    \
                if( dst_cn == 4 )                                       \
                    dst[3] = 0;                                         \
            }                                                           \
        }                                                               \
    }                                                                   \
                                                                        \
    return CV_OK;                                                       \
}

CV_IMPL_ABC2BGRx_IPP( 8u, uchar )
CV_IMPL_ABC2BGRx_IPP( 16u, ushort )
CV_IMPL_ABC2BGRx_IPP( 32f, float )



#define CV_IMPL_XYZ2BGRx( flavor, arrtype, worktype, scale_macro,           \
                          cast_macro, suffix )                              \
static CvStatus CV_STDCALL                                                  \
icvXYZ2BGRx_##flavor##_C3CnR( const arrtype* src, int srcstep,              \
                              arrtype* dst, int dststep, CvSize size,       \
                              int dst_cn, int blue_idx )                    \
{                                                                           \
    int i;                                                                  \
    worktype t, matrix[] =                                                  \
    {                                                                       \
        xyzBx##suffix, xyzBy##suffix, xyzBz##suffix,                        \
        xyzGx##suffix, xyzGy##suffix, xyzGz##suffix,                        \
        xyzRx##suffix, xyzRy##suffix, xyzRz##suffix                         \
    };                                                                      \
                                                                            \
    if( icvXYZ2RGB_##flavor##_C3R_p )                                       \
        return icvABC2BGRx_IPP_##flavor##_C3CnR( src, srcstep,              \
            dst, dststep, size, dst_cn, blue_idx,                           \
            icvXYZ2RGB_##flavor##_C3R_p );                                  \
                                                                            \
    srcstep /= sizeof(src[0]);                                              \
    dststep /= sizeof(dst[0]);                                              \
    dststep -= size.width*dst_cn;                                           \
    size.width *= 3;                                                        \
                                                                            \
    if( blue_idx )                                                          \
    {                                                                       \
        CV_SWAP( matrix[0], matrix[6], t );                                 \
        CV_SWAP( matrix[1], matrix[7], t );                                 \
        CV_SWAP( matrix[2], matrix[8], t );                                 \
    }                                                                       \
                                                                            \
    for( ; size.height--; src += srcstep, dst += dststep )                  \
    {                                                                       \
        for( i = 0; i < size.width; i += 3, dst += dst_cn )                 \
        {                                                                   \
            worktype b = scale_macro(src[i]*matrix[0] +                     \
                    src[i+1]*matrix[1] + src[i+2]*matrix[2]);               \
            worktype g = scale_macro(src[i]*matrix[3] +                     \
                    src[i+1]*matrix[4] + src[i+2]*matrix[5]);               \
            worktype r = scale_macro(src[i]*matrix[6] +                     \
                    src[i+1]*matrix[7] + src[i+2]*matrix[8]);               \
                                                                            \
            dst[0] = cast_macro(b);                                         \
            dst[1] = cast_macro(g);                                         \
            dst[2] = cast_macro(r);                                         \
                                                                            \
            if( dst_cn == 4 )                                               \
                dst[3] = 0;                                                 \
        }                                                                   \
    }                                                                       \
                                                                            \
    return CV_OK;                                                           \
}

CV_IMPL_XYZ2BGRx( 8u, uchar, int, xyz_descale, CV_CAST_8U, _32s )
CV_IMPL_XYZ2BGRx( 16u, ushort, int, xyz_descale, CV_CAST_16U, _32s )
CV_IMPL_XYZ2BGRx( 32f, float, float, CV_NOP, CV_NOP, _32f )

static CvStatus CV_STDCALL
icvHSV2BGRx_32f_C3CnR( const float* src, int srcstep, float* dst,
                       int dststep, CvSize size, int dst_cn, int blue_idx )
{
    int i;
    srcstep /= sizeof(src[0]);
    dststep /= sizeof(dst[0]);
    dststep -= size.width*dst_cn;
    size.width *= 3;

    for( ; size.height--; src += srcstep, dst += dststep )
    {
        for( i = 0; i < size.width; i += 3, dst += dst_cn )
        {
            float h = src[i], s = src[i+1], v = src[i+2];
            float b, g, r;

            if( s == 0 )
                b = g = r = v;
            else
            {
                static const int sector_data[][3]=
                    {{1,3,0}, {1,0,2}, {3,0,1}, {0,2,1}, {0,1,3}, {2,1,0}};
                float tab[4];
                int sector;
                h *= 0.016666666666666666f; // h /= 60;
                if( h < 0 )
                    do h += 6; while( h < 0 );
                else if( h >= 6 )
                    do h -= 6; while( h >= 6 );
                sector = floor(h);
                h -= sector;

                tab[0] = v;
                tab[1] = v*(1.f - s);
                tab[2] = v*(1.f - s*h);
                tab[3] = v*(1.f - s*(1.f - h));
                
                b = tab[sector_data[sector][0]];
                g = tab[sector_data[sector][1]];
                r = tab[sector_data[sector][2]];
            }

            dst[blue_idx] = b;
            dst[1] = g;
            dst[blue_idx^2] = r;
            if( dst_cn == 4 )
                dst[3] = 0;
        }
    }

    return CV_OK;
}

static CvStatus CV_STDCALL
icvABC2BGRx_8u_C3CnR( const uchar* src, int srcstep, uchar* dst, int dststep,
                      CvSize size, int dst_cn, int blue_idx, CvColorCvtFunc2 cvtfunc_32f,
                     const float* pre_coeffs, int postscale )
{
    int block_size = MIN(1 << 8, size.width);
    float* buffer = (float*)cvStackAlloc( block_size*3*sizeof(buffer[0]) );
    int i, di, k;
    CvStatus status = CV_OK;

    dststep -= size.width*dst_cn;

    for( ; size.height--; src += srcstep, dst += dststep )
    {
        for( i = 0; i < size.width; i += block_size )
        {
            const uchar* src1 = src + i*3;
            di = MIN(block_size, size.width - i);
            
            for( k = 0; k < di*3; k += 3 )
            {
                float a = CV_8TO32F(src1[k])*pre_coeffs[0] + pre_coeffs[1];
                float b = CV_8TO32F(src1[k+1])*pre_coeffs[2] + pre_coeffs[3];
                float c = CV_8TO32F(src1[k+2])*pre_coeffs[4] + pre_coeffs[5];
                buffer[k] = a;
                buffer[k+1] = b;
                buffer[k+2] = c;
            }

            status = cvtfunc_32f( buffer, 0, buffer, 0, cvSize(di,1), 3, blue_idx );
            if( status < 0 )
                return status;
            
            if( postscale )
            {
                for( k = 0; k < di*3; k += 3, dst += dst_cn )
                {
                    int b = cvRound(buffer[k]*255.);
                    int g = cvRound(buffer[k+1]*255.);
                    int r = cvRound(buffer[k+2]*255.);

                    dst[0] = CV_CAST_8U(b);
                    dst[1] = CV_CAST_8U(g);
                    dst[2] = CV_CAST_8U(r);
                    if( dst_cn == 4 )
                        dst[3] = 0;
                }
            }
            else
            {
                for( k = 0; k < di*3; k += 3, dst += dst_cn )
                {
                    int b = cvRound(buffer[k]);
                    int g = cvRound(buffer[k+1]);
                    int r = cvRound(buffer[k+2]);

                    dst[0] = CV_CAST_8U(b);
                    dst[1] = CV_CAST_8U(g);
                    dst[2] = CV_CAST_8U(r);
                    if( dst_cn == 4 )
                        dst[3] = 0;
                }
            }
        }
    }

    return CV_OK;
}

static CvStatus CV_STDCALL
icvHSV2BGRx_8u_C3CnR( const uchar* src, int srcstep, uchar* dst, int dststep,
                      CvSize size, int dst_cn, int blue_idx )
{
    static const float pre_coeffs[] = { 2.f, 0.f, 0.0039215686274509803f, 0.f, 1.f, 0.f };

    if( icvHSV2RGB_8u_C3R_p )
    {
        int block_size = MIN(1 << 14, size.width);
        uchar* buffer;
        int i, di, k;
        CvStatus status = CV_OK;

        buffer = (uchar*)cvStackAlloc( block_size*3*sizeof(buffer[0]) );
        dststep -= size.width*dst_cn;

        for( ; size.height--; src += srcstep, dst += dststep )
        {
            for( i = 0; i < size.width; i += block_size )
            {
                const uchar* src1 = src + i*3;
                di = MIN(block_size, size.width - i);
                for( k = 0; k < di*3; k += 3 )
                {
                    uchar h = icvHue180To255[src1[k]];
                    uchar s = src1[k+1];
                    uchar v = src1[k+2];
                    buffer[k] = h;
                    buffer[k+1] = s;
                    buffer[k+2] = v;
                }

                status = icvHSV2RGB_8u_C3R_p( buffer, di*3,
                                buffer, di*3, cvSize(di,1) );
                if( status < 0 )
                    return status;

                for( k = 0; k < di*3; k += 3, dst += dst_cn )
                {
                    uchar r = buffer[k];
                    uchar g = buffer[k+1];
                    uchar b = buffer[k+2];
                    dst[blue_idx] = b;
                    dst[1] = g;
                    dst[blue_idx^2] = r;
                    if( dst_cn == 4 )
                        dst[3] = 0;
                }
            }
        }

        return CV_OK;
    }

    return icvABC2BGRx_8u_C3CnR( src, srcstep, dst, dststep, size, dst_cn, blue_idx,
                                 (CvColorCvtFunc2)icvHSV2BGRx_32f_C3CnR, pre_coeffs, 0 );
}

static CvStatus CV_STDCALL
icvHLS2BGRx_32f_C3CnR( const float* src, int srcstep, float* dst, int dststep,
                       CvSize size, int dst_cn, int blue_idx )
{
    int i;
    srcstep /= sizeof(src[0]);
    dststep /= sizeof(dst[0]);

    if( icvHLS2RGB_32f_C3R_p )
    {
        int block_size = MIN(1 << 10, size.width);
        float* buffer;
        int di, k;
        CvStatus status = CV_OK;

        buffer = (float*)cvStackAlloc( block_size*3*sizeof(buffer[0]) );
        dststep -= size.width*dst_cn;

        for( ; size.height--; src += srcstep, dst += dststep )
        {
            for( i = 0; i < size.width; i += block_size )
            {
                const float* src1 = src + i*3;
                di = MIN(block_size, size.width - i);
                for( k = 0; k < di*3; k += 3 )
                {
                    float h = src1[k]*0.0027777777777777779f; // /360.
                    float s = src1[k+1], v = src1[k+2];
                    buffer[k] = h; buffer[k+1] = s; buffer[k+2] = v;
                }

                status = icvHLS2RGB_32f_C3R_p( buffer, di*3*sizeof(dst[0]),
                                buffer, di*3*sizeof(dst[0]), cvSize(di,1) );
                if( status < 0 )
                    return status;

                for( k = 0; k < di*3; k += 3, dst += dst_cn )
                {
                    float r = buffer[k], g = buffer[k+1], b = buffer[k+2];
                    dst[blue_idx] = b; dst[1] = g; dst[blue_idx^2] = r;
                    if( dst_cn == 4 )
                        dst[3] = 0;
                }
            }
        }

        return CV_OK;
    }
    
    dststep -= size.width*dst_cn;
    size.width *= 3;

    for( ; size.height--; src += srcstep, dst += dststep )
    {
        for( i = 0; i < size.width; i += 3, dst += dst_cn )
        {
            float h = src[i], l = src[i+1], s = src[i+2];
            float b, g, r;

            if( s == 0 )
                b = g = r = l;
            else
            {
                static const int sector_data[][3]=
                    {{1,3,0}, {1,0,2}, {3,0,1}, {0,2,1}, {0,1,3}, {2,1,0}};
                float tab[4];
                int sector;
                
                float p2 = l <= 0.5f ? l*(1 + s) : l + s - l*s;
                float p1 = 2*l - p2;

                h *= 0.016666666666666666f; // h /= 60;
                if( h < 0 )
                    do h += 6; while( h < 0 );
                else if( h >= 6 )
                    do h -= 6; while( h >= 6 );

                assert( 0 <= h && h < 6 );
                sector = floor(h);
                h -= sector;

                tab[0] = p2;
                tab[1] = p1;
                tab[2] = p1 + (p2 - p1)*(1-h);
                tab[3] = p1 + (p2 - p1)*h;

                b = tab[sector_data[sector][0]];
                g = tab[sector_data[sector][1]];
                r = tab[sector_data[sector][2]];
            }

            dst[blue_idx] = b;
            dst[1] = g;
            dst[blue_idx^2] = r;
            if( dst_cn == 4 )
                dst[3] = 0;
        }
    }

    return CV_OK;
}

static CvStatus CV_STDCALL
icvHLS2BGRx_8u_C3CnR( const uchar* src, int srcstep, uchar* dst, int dststep,
                      CvSize size, int dst_cn, int blue_idx )
{
    static const float pre_coeffs[] = { 2.f, 0.f, 0.0039215686274509803f, 0.f,
                                        0.0039215686274509803f, 0.f };

    if( icvHLS2RGB_8u_C3R_p )
    {
        int block_size = MIN(1 << 14, size.width);
        uchar* buffer;
        int i, di, k;
        CvStatus status = CV_OK;

        buffer = (uchar*)cvStackAlloc( block_size*3*sizeof(buffer[0]) );
        dststep -= size.width*dst_cn;

        for( ; size.height--; src += srcstep, dst += dststep )
        {
            for( i = 0; i < size.width; i += block_size )
            {
                const uchar* src1 = src + i*3;
                di = MIN(block_size, size.width - i);
                for( k = 0; k < di*3; k += 3 )
                {
                    uchar h = icvHue180To255[src1[k]];
                    uchar l = src1[k+1];
                    uchar s = src1[k+2];
                    buffer[k] = h;
                    buffer[k+1] = l;
                    buffer[k+2] = s;
                }

                status = icvHLS2RGB_8u_C3R_p( buffer, di*3,
                                buffer, di*3, cvSize(di,1) );
                if( status < 0 )
                    return status;

                for( k = 0; k < di*3; k += 3, dst += dst_cn )
                {
                    uchar r = buffer[k];
                    uchar g = buffer[k+1];
                    uchar b = buffer[k+2];
                    dst[blue_idx] = b;
                    dst[1] = g;
                    dst[blue_idx^2] = r;
                    if( dst_cn == 4 )
                        dst[3] = 0;
                }
            }
        }

        return CV_OK;
    }

    return icvABC2BGRx_8u_C3CnR( src, srcstep, dst, dststep, size, dst_cn, blue_idx,
                                 (CvColorCvtFunc2)icvHLS2BGRx_32f_C3CnR, pre_coeffs, 1 );
}

static CvStatus CV_STDCALL
icvLab2BGRx_32f_C3CnR( const float* src, int srcstep, float* dst, int dststep,
                       CvSize size, int dst_cn, int blue_idx )
{
    int i;
    srcstep /= sizeof(src[0]);
    dststep /= sizeof(dst[0]);
    dststep -= size.width*dst_cn;
    size.width *= 3;

    for( ; size.height--; src += srcstep, dst += dststep )
    {
        for( i = 0; i < size.width; i += 3, dst += dst_cn )
        {
            float L = src[i], a = src[i+1], b = src[i+2];
            float x, y, z;
            float g, r;

            L = (L + labLShift_32f)*(1.f/labLScale_32f);
            x = (L + a*0.002f);
            z = (L - b*0.005f);
            y = L*L*L;
            x = x*x*x;
            z = z*z*z;

            b = x*labBx_32f + y*labBy_32f + z*labBz_32f;
            g = x*labGx_32f + y*labGy_32f + z*labGz_32f;
            r = x*labRx_32f + y*labRy_32f + z*labRz_32f;

            dst[blue_idx] = b;
            dst[1] = g;
            dst[blue_idx^2] = r;
            if( dst_cn == 4 )
                dst[3] = 0;
        }
    }

    return CV_OK;
}

static CvStatus CV_STDCALL
icvLab2BGRx_8u_C3CnR( const uchar* src, int srcstep, uchar* dst, int dststep,
                      CvSize size, int dst_cn, int blue_idx )
{
    // L: [0..255] -> [0..100]
    // a: [0..255] -> [-128..127]
    // b: [0..255] -> [-128..127]
    static const float pre_coeffs[] = { 0.39215686274509809f, 0.f, 1.f, -128.f, 1.f, -128.f };

    if( icvLab2BGR_8u_C3R_p )
        return icvABC2BGRx_IPP_8u_C3CnR( src, srcstep, dst, dststep, size,
                                         dst_cn, blue_idx^2, icvLab2BGR_8u_C3R_p );

    return icvABC2BGRx_8u_C3CnR( src, srcstep, dst, dststep, size, dst_cn, blue_idx,
                                 (CvColorCvtFunc2)icvLab2BGRx_32f_C3CnR, pre_coeffs, 1 );
}

static CvStatus CV_STDCALL
icvLuv2BGRx_32f_C3CnR( const float* src, int srcstep, float* dst, int dststep,
                       CvSize size, int dst_cn, int blue_idx )
{
    int i;

    /*if( icvLuv2RGB_32f_C3R_p )
        return icvABC2BGRx_IPP_32f_C3CnR( src, srcstep, dst, dststep, size,
                                          dst_cn, blue_idx, icvLuv2RGB_32f_C3R_p );*/

    srcstep /= sizeof(src[0]);
    dststep /= sizeof(dst[0]);
    dststep -= size.width*dst_cn;
    size.width *= 3;

    for( ; size.height--; src += srcstep, dst += dststep )
    {
        for( i = 0; i < size.width; i += 3, dst += dst_cn )
        {
            float L = src[i], u = src[i+1], v = src[i+2];
            float x, y, z, t, u1, v1, b, g, r;

            if( L >= 8 ) 
            {
                t = (L + labLShift_32f) * (1.f/labLScale_32f);
                y = t*t*t;
            }
            else
            {
                y = L * (1.f/labLScale2_32f);
                L = MAX( L, 0.001f );
            }

            t = 1.f/(13.f * L);
            u1 = u*t + luvUn_32f;
            v1 = v*t + luvVn_32f;
            x = 2.25f * u1 * y / v1 ;
            z = (12 - 3 * u1 - 20 * v1) * y / (4 * v1);                
                       
            b = xyzBx_32f*x + xyzBy_32f*y + xyzBz_32f*z;
            g = xyzGx_32f*x + xyzGy_32f*y + xyzGz_32f*z;
            r = xyzRx_32f*x + xyzRy_32f*y + xyzRz_32f*z;

            dst[blue_idx] = b;
            dst[1] = g;
            dst[blue_idx^2] = r;
            if( dst_cn == 4 )
                dst[3] = 0.f;
        }
    }

    return CV_OK;
}

static CvStatus CV_STDCALL
icvLuv2BGRx_8u_C3CnR( const uchar* src, int srcstep, uchar* dst, int dststep,
                      CvSize size, int dst_cn, int blue_idx )
{
    // L: [0..255] -> [0..100]
    // u: [0..255] -> [-134..220]
    // v: [0..255] -> [-140..122]
    static const float pre_coeffs[] = { 0.39215686274509809f, 0.f, 1.388235294117647f, -134.f,
                                        1.003921568627451f, -140.f };

    if( icvLuv2RGB_8u_C3R_p )
        return icvABC2BGRx_IPP_8u_C3CnR( src, srcstep, dst, dststep, size,
                                         dst_cn, blue_idx, icvLuv2RGB_8u_C3R_p );

    return icvABC2BGRx_8u_C3CnR( src, srcstep, dst, dststep, size, dst_cn, blue_idx,
                                 (CvColorCvtFunc2)icvLuv2BGRx_32f_C3CnR, pre_coeffs, 1 );
}

static CvStatus CV_STDCALL
icvBayer2BGR_8u_C1C3R( const uchar* bayer0, int bayer_step,
                       uchar *dst0, int dst_step,
                       CvSize size, int code )
{
    int blue = code == CV_BayerBG2BGR || code == CV_BayerGB2BGR ? -1 : 1;
    int start_with_green = code == CV_BayerGB2BGR || code == CV_BayerGR2BGR;

    memset( dst0, 0, size.width*3*sizeof(dst0[0]) );
    memset( dst0 + (size.height - 1)*dst_step, 0, size.width*3*sizeof(dst0[0]) );
    dst0 += dst_step + 3 + 1;
    size.height -= 2;
    size.width -= 2;

    for( ; size.height-- > 0; bayer0 += bayer_step, dst0 += dst_step )
    {
        int t0, t1;
        const uchar* bayer = bayer0;
        uchar* dst = dst0;
        const uchar* bayer_end = bayer + size.width;

        dst[-4] = dst[-3] = dst[-2] = dst[size.width*3-1] =
            dst[size.width*3] = dst[size.width*3+1] = 0;

        if( size.width <= 0 )
            continue;

        if( start_with_green )
        {
            t0 = (bayer[1] + bayer[bayer_step*2+1] + 1) >> 1;
            t1 = (bayer[bayer_step] + bayer[bayer_step+2] + 1) >> 1;
            dst[-blue] = (uchar)t0;
            dst[0] = bayer[bayer_step+1];
            dst[blue] = (uchar)t1;
            bayer++;
            dst += 3;
        }

        if( blue > 0 )
        {
            for( ; bayer <= bayer_end - 2; bayer += 2, dst += 6 )
            {
                t0 = (bayer[0] + bayer[2] + bayer[bayer_step*2] +
                      bayer[bayer_step*2+2] + 2) >> 2;
                t1 = (bayer[1] + bayer[bayer_step] +
                      bayer[bayer_step+2] + bayer[bayer_step*2+1]+2) >> 2;
                dst[-1] = (uchar)t0;
                dst[0] = (uchar)t1;
                dst[1] = bayer[bayer_step+1];

                t0 = (bayer[2] + bayer[bayer_step*2+2] + 1) >> 1;
                t1 = (bayer[bayer_step+1] + bayer[bayer_step+3] + 1) >> 1;
                dst[2] = (uchar)t0;
                dst[3] = bayer[bayer_step+2];
                dst[4] = (uchar)t1;
            }
        }
        else
        {
            for( ; bayer <= bayer_end - 2; bayer += 2, dst += 6 )
            {
                t0 = (bayer[0] + bayer[2] + bayer[bayer_step*2] +
                      bayer[bayer_step*2+2] + 2) >> 2;
                t1 = (bayer[1] + bayer[bayer_step] +
                      bayer[bayer_step+2] + bayer[bayer_step*2+1]+2) >> 2;
                dst[1] = (uchar)t0;
                dst[0] = (uchar)t1;
                dst[-1] = bayer[bayer_step+1];

                t0 = (bayer[2] + bayer[bayer_step*2+2] + 1) >> 1;
                t1 = (bayer[bayer_step+1] + bayer[bayer_step+3] + 1) >> 1;
                dst[4] = (uchar)t0;
                dst[3] = bayer[bayer_step+2];
                dst[2] = (uchar)t1;
            }
        }

        if( bayer < bayer_end )
        {
            t0 = (bayer[0] + bayer[2] + bayer[bayer_step*2] +
                  bayer[bayer_step*2+2] + 2) >> 2;
            t1 = (bayer[1] + bayer[bayer_step] +
                  bayer[bayer_step+2] + bayer[bayer_step*2+1]+2) >> 2;
            dst[-blue] = (uchar)t0;
            dst[0] = (uchar)t1;
            dst[blue] = bayer[bayer_step+1];
            bayer++;
            dst += 3;
        }

        blue = -blue;
        start_with_green = !start_with_green;
    }

    return CV_OK;
}



void cvCvtColor( const CvArr* srcarr, CvArr* dstarr, int code )
{
    CV_FUNCNAME( "cvCvtColor" );

    __BEGIN__;
    
    CvMat srcstub, *src = (CvMat*)srcarr;
    CvMat dststub, *dst = (CvMat*)dstarr;
    CvSize size;
    int src_step, dst_step;
    int src_cn, dst_cn, depth;
    CvColorCvtFunc0 func0 = 0;
    CvColorCvtFunc1 func1 = 0;
    CvColorCvtFunc2 func2 = 0;
    CvColorCvtFunc3 func3 = 0;
    int param[] = { 0, 0, 0, 0 };
    
    CV_CALL( src = cvGetMat( srcarr, &srcstub,NULL,0 ));
    CV_CALL( dst = cvGetMat( dstarr, &dststub ,NULL,0 ));
    
    if( !CV_ARE_SIZES_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    if( !CV_ARE_DEPTHS_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    depth = CV_MAT_DEPTH(src->type);
    if( depth != CV_8U && depth != CV_16U && depth != CV_32F )
        CV_ERROR( CV_StsUnsupportedFormat, "" );

    src_cn = CV_MAT_CN( src->type );
    dst_cn = CV_MAT_CN( dst->type );
    size = cvGetMatSize( src );
    src_step = src->step;
    dst_step = dst->step;

    if( CV_IS_MAT_CONT(src->type & dst->type) &&
        code != CV_BayerBG2BGR && code != CV_BayerGB2BGR &&
        code != CV_BayerRG2BGR && code != CV_BayerGR2BGR ) 
    {
        size.width *= size.height;
        size.height = 1;
        src_step = dst_step = CV_STUB_STEP;
    }

    switch( code )
    {
    case CV_BGR2BGRA:
    case CV_RGB2BGRA:
        if( src_cn != 3 || dst_cn != 4 )
            CV_ERROR( CV_BadNumChannels,
            "Incorrect number of channels for this conversion code" );

        func1 = depth == CV_8U ? (CvColorCvtFunc1)icvBGR2BGRx_8u_C3C4R :
                depth == CV_16U ? (CvColorCvtFunc1)icvBGR2BGRx_16u_C3C4R :
                depth == CV_32F ? (CvColorCvtFunc1)icvBGR2BGRx_32f_C3C4R : 0;
        param[0] = code == CV_BGR2BGRA ? 0 : 2; // blue_idx
        break;

    case CV_BGRA2BGR:
    case CV_RGBA2BGR:
    case CV_RGB2BGR:
        if( (src_cn != 3 && src_cn != 4) || dst_cn != 3 )
            CV_ERROR( CV_BadNumChannels,
            "Incorrect number of channels for this conversion code" );

        func2 = depth == CV_8U ? (CvColorCvtFunc2)icvBGRx2BGR_8u_CnC3R :
                depth == CV_16U ? (CvColorCvtFunc2)icvBGRx2BGR_16u_CnC3R :
                depth == CV_32F ? (CvColorCvtFunc2)icvBGRx2BGR_32f_CnC3R : 0;
        param[0] = src_cn;
        param[1] = code == CV_BGRA2BGR ? 0 : 2; // blue_idx
        break;

    case CV_BGRA2RGBA:
        if( src_cn != 4 || dst_cn != 4 )
            CV_ERROR( CV_BadNumChannels,
            "Incorrect number of channels for this conversion code" );

        func0 = depth == CV_8U ? (CvColorCvtFunc0)icvBGRA2RGBA_8u_C4R :
                depth == CV_16U ? (CvColorCvtFunc0)icvBGRA2RGBA_16u_C4R :
                depth == CV_32F ? (CvColorCvtFunc0)icvBGRA2RGBA_32f_C4R : 0;
        break;

    case CV_BGR2BGR565:
    case CV_BGR2BGR555:
    case CV_RGB2BGR565:
    case CV_RGB2BGR555:
    case CV_BGRA2BGR565:
    case CV_BGRA2BGR555:
    case CV_RGBA2BGR565:
    case CV_RGBA2BGR555:
        if( (src_cn != 3 && src_cn != 4) || dst_cn != 2 )
            CV_ERROR( CV_BadNumChannels,
            "Incorrect number of channels for this conversion code" );

        if( depth != CV_8U )
            CV_ERROR( CV_BadDepth,
            "Conversion to/from 16-bit packed RGB format "
            "is only possible for 8-bit images (8-bit grayscale, 888 BGR/RGB or 8888 BGRA/RGBA)" );

        func3 = (CvColorCvtFunc3)icvBGRx2BGR5x5_8u_CnC2R;
        param[0] = src_cn;
        param[1] = code == CV_BGR2BGR565 || code == CV_BGR2BGR555 ||
                   code == CV_BGRA2BGR565 || code == CV_BGRA2BGR555 ? 0 : 2; // blue_idx
        param[2] = code == CV_BGR2BGR565 || code == CV_RGB2BGR565 ||
                   code == CV_BGRA2BGR565 || code == CV_RGBA2BGR565 ? 6 : 5; // green_bits
        break;

    case CV_BGR5652BGR:
    case CV_BGR5552BGR:
    case CV_BGR5652RGB:
    case CV_BGR5552RGB:
    case CV_BGR5652BGRA:
    case CV_BGR5552BGRA:
    case CV_BGR5652RGBA:
    case CV_BGR5552RGBA:
        if( src_cn != 2 || (dst_cn != 3 && dst_cn != 4))
            CV_ERROR( CV_BadNumChannels,
            "Incorrect number of channels for this conversion code" );

        if( depth != CV_8U )
            CV_ERROR( CV_BadDepth,
            "Conversion to/from 16-bit packed BGR format "
            "is only possible for 8-bit images (8-bit grayscale, 888 BGR/BGR or 8888 BGRA/BGRA)" );

        func3 = (CvColorCvtFunc3)icvBGR5x52BGRx_8u_C2CnR;
        param[0] = dst_cn;
        param[1] = code == CV_BGR5652BGR || code == CV_BGR5552BGR ||
                   code == CV_BGR5652BGRA || code == CV_BGR5552BGRA ? 0 : 2; // blue_idx
        param[2] = code == CV_BGR5652BGR || code == CV_BGR5652RGB ||
                   code == CV_BGR5652BGRA || code == CV_BGR5652RGBA ? 6 : 5; // green_bits
        break;

    case CV_BGR2GRAY:
    case CV_BGRA2GRAY:
    case CV_RGB2GRAY:
    case CV_RGBA2GRAY:
        if( (src_cn != 3 && src_cn != 4) || dst_cn != 1 )
            CV_ERROR( CV_BadNumChannels,
            "Incorrect number of channels for this conversion code" );

        func2 = depth == CV_8U ? (CvColorCvtFunc2)icvBGRx2Gray_8u_CnC1R :
                depth == CV_16U ? (CvColorCvtFunc2)icvBGRx2Gray_16u_CnC1R :
                depth == CV_32F ? (CvColorCvtFunc2)icvBGRx2Gray_32f_CnC1R : 0;
        
        param[0] = src_cn;
        param[1] = code == CV_BGR2GRAY || code == CV_BGRA2GRAY ? 0 : 2;
        break;

    case CV_BGR5652GRAY:
    case CV_BGR5552GRAY:
        if( src_cn != 2 || dst_cn != 1 )
            CV_ERROR( CV_BadNumChannels,
            "Incorrect number of channels for this conversion code" );

        if( depth != CV_8U )
            CV_ERROR( CV_BadDepth,
            "Conversion to/from 16-bit packed BGR format "
            "is only possible for 8-bit images (888 BGR/BGR or 8888 BGRA/BGRA)" );

        func2 = (CvColorCvtFunc2)icvBGR5x52Gray_8u_C2C1R;
        
        param[0] = code == CV_BGR5652GRAY ? 6 : 5; // green_bits
        break;

    case CV_GRAY2BGR:
    case CV_GRAY2BGRA:
        if( src_cn != 1 || (dst_cn != 3 && dst_cn != 4))
            CV_ERROR( CV_BadNumChannels,
            "Incorrect number of channels for this conversion code" );

        func1 = depth == CV_8U ? (CvColorCvtFunc1)icvGray2BGRx_8u_C1CnR :
                depth == CV_16U ? (CvColorCvtFunc1)icvGray2BGRx_16u_C1CnR :
                depth == CV_32F ? (CvColorCvtFunc1)icvGray2BGRx_32f_C1CnR : 0;
        
        param[0] = dst_cn;
        break;

    case CV_GRAY2BGR565:
    case CV_GRAY2BGR555:
        if( src_cn != 1 || dst_cn != 2 )
            CV_ERROR( CV_BadNumChannels,
            "Incorrect number of channels for this conversion code" );

        if( depth != CV_8U )
            CV_ERROR( CV_BadDepth,
            "Conversion to/from 16-bit packed BGR format "
            "is only possible for 8-bit images (888 BGR/BGR or 8888 BGRA/BGRA)" );

        func2 = (CvColorCvtFunc2)icvGray2BGR5x5_8u_C1C2R;
        param[0] = code == CV_GRAY2BGR565 ? 6 : 5; // green_bits
        break;

    case CV_BGR2YCrCb:
    case CV_RGB2YCrCb:
    case CV_BGR2XYZ:
    case CV_RGB2XYZ:
    case CV_BGR2HSV:
    case CV_RGB2HSV:
    case CV_BGR2Lab:
    case CV_RGB2Lab:
    case CV_BGR2Luv:
    case CV_RGB2Luv:
    case CV_BGR2HLS:
    case CV_RGB2HLS:
        if( (src_cn != 3 && src_cn != 4) || dst_cn != 3 )
            CV_ERROR( CV_BadNumChannels,
            "Incorrect number of channels for this conversion code" );

        if( depth == CV_8U )
            func2 = code == CV_BGR2YCrCb || code == CV_RGB2YCrCb ? (CvColorCvtFunc2)icvBGRx2YCrCb_8u_CnC3R :
                    code == CV_BGR2XYZ || code == CV_RGB2XYZ ? (CvColorCvtFunc2)icvBGRx2XYZ_8u_CnC3R :
                    code == CV_BGR2HSV || code == CV_RGB2HSV ? (CvColorCvtFunc2)icvBGRx2HSV_8u_CnC3R :
                    code == CV_BGR2Lab || code == CV_RGB2Lab ? (CvColorCvtFunc2)icvBGRx2Lab_8u_CnC3R :
                    code == CV_BGR2Luv || code == CV_RGB2Luv ? (CvColorCvtFunc2)icvBGRx2Luv_8u_CnC3R :
                    code == CV_BGR2HLS || code == CV_RGB2HLS ? (CvColorCvtFunc2)icvBGRx2HLS_8u_CnC3R : 0;
        else if( depth == CV_16U )
            func2 = code == CV_BGR2YCrCb || code == CV_RGB2YCrCb ? (CvColorCvtFunc2)icvBGRx2YCrCb_16u_CnC3R :
                    code == CV_BGR2XYZ || code == CV_RGB2XYZ ? (CvColorCvtFunc2)icvBGRx2XYZ_16u_CnC3R : 0;
        else if( depth == CV_32F )
            func2 = code == CV_BGR2YCrCb || code == CV_RGB2YCrCb ? (CvColorCvtFunc2)icvBGRx2YCrCb_32f_CnC3R :
                    code == CV_BGR2XYZ || code == CV_RGB2XYZ ? (CvColorCvtFunc2)icvBGRx2XYZ_32f_CnC3R :
                    code == CV_BGR2HSV || code == CV_RGB2HSV ? (CvColorCvtFunc2)icvBGRx2HSV_32f_CnC3R :
                    code == CV_BGR2Lab || code == CV_RGB2Lab ? (CvColorCvtFunc2)icvBGRx2Lab_32f_CnC3R :
                    code == CV_BGR2Luv || code == CV_RGB2Luv ? (CvColorCvtFunc2)icvBGRx2Luv_32f_CnC3R :
                    code == CV_BGR2HLS || code == CV_RGB2HLS ? (CvColorCvtFunc2)icvBGRx2HLS_32f_CnC3R : 0;
        
        param[0] = src_cn;
        param[1] = code == CV_BGR2XYZ || code == CV_BGR2YCrCb || code == CV_BGR2HSV ||
                   code == CV_BGR2Lab || code == CV_BGR2Luv || code == CV_BGR2HLS ? 0 : 2;
        break;

    case CV_YCrCb2BGR:
    case CV_YCrCb2RGB:
    case CV_XYZ2BGR:
    case CV_XYZ2RGB:
    case CV_HSV2BGR:
    case CV_HSV2RGB:
    case CV_Lab2BGR:
    case CV_Lab2RGB:
    case CV_Luv2BGR:
    case CV_Luv2RGB:
    case CV_HLS2BGR:
    case CV_HLS2RGB:
        if( src_cn != 3 || (dst_cn != 3 && dst_cn != 4) )
            CV_ERROR( CV_BadNumChannels,
            "Incorrect number of channels for this conversion code" );

        if( depth == CV_8U )
            func2 = code == CV_YCrCb2BGR || code == CV_YCrCb2RGB ? (CvColorCvtFunc2)icvYCrCb2BGRx_8u_C3CnR :
                    code == CV_XYZ2BGR || code == CV_XYZ2RGB ? (CvColorCvtFunc2)icvXYZ2BGRx_8u_C3CnR :
                    code == CV_HSV2BGR || code == CV_HSV2RGB ? (CvColorCvtFunc2)icvHSV2BGRx_8u_C3CnR :
                    code == CV_HLS2BGR || code == CV_HLS2RGB ? (CvColorCvtFunc2)icvHLS2BGRx_8u_C3CnR :
                    code == CV_Lab2BGR || code == CV_Lab2RGB ? (CvColorCvtFunc2)icvLab2BGRx_8u_C3CnR :
                    code == CV_Luv2BGR || code == CV_Luv2RGB ? (CvColorCvtFunc2)icvLuv2BGRx_8u_C3CnR : 0;
        else if( depth == CV_16U )
            func2 = code == CV_YCrCb2BGR || code == CV_YCrCb2RGB ? (CvColorCvtFunc2)icvYCrCb2BGRx_16u_C3CnR :
                    code == CV_XYZ2BGR || code == CV_XYZ2RGB ? (CvColorCvtFunc2)icvXYZ2BGRx_16u_C3CnR : 0;
        else if( depth == CV_32F )
            func2 = code == CV_YCrCb2BGR || code == CV_YCrCb2RGB ? (CvColorCvtFunc2)icvYCrCb2BGRx_32f_C3CnR :
                    code == CV_XYZ2BGR || code == CV_XYZ2RGB ? (CvColorCvtFunc2)icvXYZ2BGRx_32f_C3CnR :
                    code == CV_HSV2BGR || code == CV_HSV2RGB ? (CvColorCvtFunc2)icvHSV2BGRx_32f_C3CnR :
                    code == CV_HLS2BGR || code == CV_HLS2RGB ? (CvColorCvtFunc2)icvHLS2BGRx_32f_C3CnR :
                    code == CV_Lab2BGR || code == CV_Lab2RGB ? (CvColorCvtFunc2)icvLab2BGRx_32f_C3CnR :
                    code == CV_Luv2BGR || code == CV_Luv2RGB ? (CvColorCvtFunc2)icvLuv2BGRx_32f_C3CnR : 0;
        
        param[0] = dst_cn;
        param[1] = code == CV_XYZ2BGR || code == CV_YCrCb2BGR || code == CV_HSV2BGR ||
                   code == CV_Lab2BGR || code == CV_Luv2BGR || code == CV_HLS2BGR ? 0 : 2;
        break;

    case CV_BayerBG2BGR:
    case CV_BayerGB2BGR:
    case CV_BayerRG2BGR:
    case CV_BayerGR2BGR:
        if( src_cn != 1 || dst_cn != 3 )
            CV_ERROR( CV_BadNumChannels,
            "Incorrect number of channels for this conversion code" );
        
        if( depth != CV_8U )
            CV_ERROR( CV_BadDepth,
            "Bayer pattern can be converted only to 8-bit 3-channel BGR/RGB image" );

        func1 = (CvColorCvtFunc1)icvBayer2BGR_8u_C1C3R;
        param[0] = code; // conversion code
        break;
    default:
        CV_ERROR( CV_StsBadFlag, "Unknown/unsupported color conversion code" );
    }

    if( func0 )
    {
        IPPI_CALL( func0( src->data.ptr, src_step, dst->data.ptr, dst_step, size ));
    }
    else if( func1 )
    {
        IPPI_CALL( func1( src->data.ptr, src_step,
            dst->data.ptr, dst_step, size, param[0] ));
    }
    else if( func2 )
    {
        IPPI_CALL( func2( src->data.ptr, src_step,
            dst->data.ptr, dst_step, size, param[0], param[1] ));
    }
    else if( func3 )
    {
        IPPI_CALL( func3( src->data.ptr, src_step,
            dst->data.ptr, dst_step, size, param[0], param[1], param[2] ));
    }
    else
        CV_ERROR( CV_StsUnsupportedFormat, "The image format is not supported" );

    __END__;
}

float  cvCbrt( float value )
{
    float fr;
    Cv32suf v, m;
    int ix, s;
    int ex, shx;

    v.f = value;
    ix = v.i & 0x7fffffff;
    s = v.i & 0x80000000;
    ex = (ix >> 23) - 127;
    shx = ex % 3;
    shx -= shx >= 0 ? 3 : 0;
    ex = (ex - shx) / 3; /* exponent of cube root */
    v.i = (ix & ((1<<23)-1)) | ((shx + 127)<<23);
    fr = v.f;

    /* 0.125 <= fr < 1.0 */
    /* Use quartic rational polynomial with error < 2^(-24) */
    fr = (float)(((((45.2548339756803022511987494 * fr +
    192.2798368355061050458134625) * fr +
    119.1654824285581628956914143) * fr +
    13.43250139086239872172837314) * fr +
    0.1636161226585754240958355063)/
    ((((14.80884093219134573786480845 * fr +
    151.9714051044435648658557668) * fr +
    168.5254414101568283957668343) * fr +
    33.9905941350215598754191872) * fr +
    1.0));

    /* fr *= 2^ex * sign */
    m.f = value;
    v.f = fr;
    v.i = (v.i + (ex << 23) + s) & (m.i*2 != 0 ? -1 : 0);
    return v.f;
}

static IplROI* icvCreateROI( int coi, int xOffset, int yOffset, int width, int height )
{
    IplROI *roi = 0;

    CV_FUNCNAME( "icvCreateROI" );

    __BEGIN__;

    if( !CvIPL.createROI )
    {
        CV_CALL( roi = (IplROI*)cvAlloc( sizeof(*roi)));

        roi->coi = coi;
        roi->xOffset = xOffset;
        roi->yOffset = yOffset;
        roi->width = width;
        roi->height = height;
    }
    else
    {
        roi = CvIPL.createROI( coi, xOffset, yOffset, width, height );
    }

    __END__;

    return roi;
}

IplImage* cvCloneImage( const IplImage* src )
{
    IplImage* dst = 0;
    CV_FUNCNAME( "cvCloneImage" );

    __BEGIN__;

    if( !CV_IS_IMAGE_HDR( src ))
        CV_ERROR( CV_StsBadArg, "Bad image header" );

    if( !CvIPL.cloneImage )
    {
        CV_CALL( dst = (IplImage*)cvAlloc( sizeof(*dst)));

        memcpy( dst, src, sizeof(*src));
        dst->imageData = dst->imageDataOrigin = 0;
        dst->roi = 0;

        if( src->roi )
        {
            dst->roi = icvCreateROI( src->roi->coi, src->roi->xOffset,
                          src->roi->yOffset, src->roi->width, src->roi->height );
        }

        if( src->imageData )
        {
            int size = src->imageSize;
            cvCreateData( dst );
            memcpy( dst->imageData, src->imageData, size );
        }
    }
    else
    {
        dst = CvIPL.cloneImage( src );
    }

    __END__;

    return dst;
}

#define ICV_DEF_BIN_ARI_OP_CASE( __op__, worktype, cast_macro, len )\
{                                                                   \
    int i;                                                          \
                                                                    \
    for( i = 0; i <= (len) - 4; i += 4 )                            \
    {                                                               \
        worktype t0 = __op__((src1)[i], (src2)[i]);                 \
        worktype t1 = __op__((src1)[i+1], (src2)[i+1]);             \
                                                                    \
        (dst)[i] = cast_macro( t0 );                                \
        (dst)[i+1] = cast_macro( t1 );                              \
                                                                    \
        t0 = __op__((src1)[i+2],(src2)[i+2]);                       \
        t1 = __op__((src1)[i+3],(src2)[i+3]);                       \
                                                                    \
        (dst)[i+2] = cast_macro( t0 );                              \
        (dst)[i+3] = cast_macro( t1 );                              \
    }                                                               \
                                                                    \
    for( ; i < (len); i++ )                                         \
    {                                                               \
        worktype t0 = __op__((src1)[i],(src2)[i]);                  \
        (dst)[i] = cast_macro( t0 );                                \
    }                                                               \
}

#define ICV_DEF_BIN_ARI_OP_2D( __op__, name, type, worktype, cast_macro )   \
IPCVAPI_IMPL( CvStatus, name,                                               \
    ( const type* src1, int step1, const type* src2, int step2,             \
      type* dst, int step, CvSize size ),                                   \
      (src1, step1, src2, step2, dst, step, size) )                         \
{                                                                           \
    step1/=sizeof(src1[0]); step2/=sizeof(src2[0]); step/=sizeof(dst[0]);   \
                                                                            \
    if( size.width == 1 )                                                   \
    {                                                                       \
        for( ; size.height--; src1 += step1, src2 += step2, dst += step )   \
        {                                                                   \
            worktype t0 = __op__((src1)[0],(src2)[0]);                      \
            (dst)[0] = cast_macro( t0 );                                    \
        }                                                                   \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        for( ; size.height--; src1 += step1, src2 += step2, dst += step )   \
        {                                                                   \
            ICV_DEF_BIN_ARI_OP_CASE( __op__, worktype,                      \
                                     cast_macro, size.width );              \
        }                                                                   \
    }                                                                       \
                                                                            \
    return CV_OK;                                                           \
}


#define ICV_DEF_BIN_ARI_OP_2D_SFS(__op__, name, type, worktype, cast_macro) \
IPCVAPI_IMPL( CvStatus, name,                                               \
    ( const type* src1, int step1, const type* src2, int step2,             \
      type* dst, int step, CvSize size, int /*scalefactor*/ ),              \
      (src1, step1, src2, step2, dst, step, size, 0) )                      \
{                                                                           \
    step1/=sizeof(src1[0]); step2/=sizeof(src2[0]); step/=sizeof(dst[0]);   \
                                                                            \
    if( size.width == 1 )                                                   \
    {                                                                       \
        for( ; size.height--; src1 += step1, src2 += step2, dst += step )   \
        {                                                                   \
            worktype t0 = __op__((src1)[0],(src2)[0]);                      \
            (dst)[0] = cast_macro( t0 );                                    \
        }                                                                   \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        for( ; size.height--; src1 += step1, src2 += step2, dst += step )   \
        {                                                                   \
            ICV_DEF_BIN_ARI_OP_CASE( __op__, worktype,                      \
                                     cast_macro, size.width );              \
        }                                                                   \
    }                                                                       \
                                                                            \
    return CV_OK;                                                           \
}


#define ICV_DEF_UN_ARI_OP_CASE( __op__, worktype, cast_macro,               \
                                src, scalar, dst, len )                     \
{                                                                           \
    int i;                                                                  \
                                                                            \
    for( ; ((len) -= 12) >= 0; (dst) += 12, (src) += 12 )                   \
    {                                                                       \
        worktype t0 = __op__((scalar)[0], (src)[0]);                        \
        worktype t1 = __op__((scalar)[1], (src)[1]);                        \
                                                                            \
        (dst)[0] = cast_macro( t0 );                                        \
        (dst)[1] = cast_macro( t1 );                                        \
                                                                            \
        t0 = __op__((scalar)[2], (src)[2]);                                 \
        t1 = __op__((scalar)[3], (src)[3]);                                 \
                                                                            \
        (dst)[2] = cast_macro( t0 );                                        \
        (dst)[3] = cast_macro( t1 );                                        \
                                                                            \
        t0 = __op__((scalar)[4], (src)[4]);                                 \
        t1 = __op__((scalar)[5], (src)[5]);                                 \
                                                                            \
        (dst)[4] = cast_macro( t0 );                                        \
        (dst)[5] = cast_macro( t1 );                                        \
                                                                            \
        t0 = __op__((scalar)[6], (src)[6]);                                 \
        t1 = __op__((scalar)[7], (src)[7]);                                 \
                                                                            \
        (dst)[6] = cast_macro( t0 );                                        \
        (dst)[7] = cast_macro( t1 );                                        \
                                                                            \
        t0 = __op__((scalar)[8], (src)[8]);                                 \
        t1 = __op__((scalar)[9], (src)[9]);                                 \
                                                                            \
        (dst)[8] = cast_macro( t0 );                                        \
        (dst)[9] = cast_macro( t1 );                                        \
                                                                            \
        t0 = __op__((scalar)[10], (src)[10]);                               \
        t1 = __op__((scalar)[11], (src)[11]);                               \
                                                                            \
        (dst)[10] = cast_macro( t0 );                                       \
        (dst)[11] = cast_macro( t1 );                                       \
    }                                                                       \
                                                                            \
    for( (len) += 12, i = 0; i < (len); i++ )                               \
    {                                                                       \
        worktype t0 = __op__((scalar)[i],(src)[i]);                         \
        (dst)[i] = cast_macro( t0 );                                        \
    }                                                                       \
}


#define ICV_DEF_UN_ARI_OP_2D( __op__, name, type, worktype, cast_macro )    \
static CvStatus CV_STDCALL name                                             \
    ( const type* src, int step1, type* dst, int step,                      \
      CvSize size, const worktype* scalar )                                 \
{                                                                           \
    step1 /= sizeof(src[0]); step /= sizeof(dst[0]);                        \
                                                                            \
    if( size.width == 1 )                                                   \
    {                                                                       \
        for( ; size.height--; src += step1, dst += step )                   \
        {                                                                   \
            worktype t0 = __op__(*(scalar),*(src));                         \
            *(dst) = cast_macro( t0 );                                      \
        }                                                                   \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        for( ; size.height--; src += step1, dst += step )                   \
        {                                                                   \
            const type *tsrc = src;                                         \
            type *tdst = dst;                                               \
            int width = size.width;                                         \
                                                                            \
            ICV_DEF_UN_ARI_OP_CASE( __op__, worktype, cast_macro,           \
                                    tsrc, scalar, tdst, width );            \
        }                                                                   \
    }                                                                       \
                                                                            \
    return CV_OK;                                                           \
}


#define ICV_DEF_BIN_ARI_ALL( __op__, name, cast_8u )                                \
ICV_DEF_BIN_ARI_OP_2D_SFS( __op__, icv##name##_8u_C1R, uchar, int, cast_8u )        \
ICV_DEF_BIN_ARI_OP_2D_SFS( __op__, icv##name##_16u_C1R, ushort, int, CV_CAST_16U )  \
ICV_DEF_BIN_ARI_OP_2D_SFS( __op__, icv##name##_16s_C1R, short, int, CV_CAST_16S )   \
ICV_DEF_BIN_ARI_OP_2D( __op__, icv##name##_32s_C1R, int, int, CV_CAST_32S )         \
ICV_DEF_BIN_ARI_OP_2D( __op__, icv##name##_32f_C1R, float, float, CV_CAST_32F )     \
ICV_DEF_BIN_ARI_OP_2D( __op__, icv##name##_64f_C1R, double, double, CV_CAST_64F )

#define ICV_DEF_UN_ARI_ALL( __op__, name )                                          \
ICV_DEF_UN_ARI_OP_2D( __op__, icv##name##_8u_C1R, uchar, int, CV_CAST_8U )          \
ICV_DEF_UN_ARI_OP_2D( __op__, icv##name##_16u_C1R, ushort, int, CV_CAST_16U )       \
ICV_DEF_UN_ARI_OP_2D( __op__, icv##name##_16s_C1R, short, int, CV_CAST_16S )        \
ICV_DEF_UN_ARI_OP_2D( __op__, icv##name##_32s_C1R, int, int, CV_CAST_32S )          \
ICV_DEF_UN_ARI_OP_2D( __op__, icv##name##_32f_C1R, float, float, CV_CAST_32F )      \
ICV_DEF_UN_ARI_OP_2D( __op__, icv##name##_64f_C1R, double, double, CV_CAST_64F )




ICV_DEF_BIN_ARI_ALL( CV_ADD, Add, CV_FAST_CAST_8U )
ICV_DEF_BIN_ARI_ALL( CV_SUB_R, Sub, CV_FAST_CAST_8U )

ICV_DEF_UN_ARI_ALL( CV_ADD, AddC )
ICV_DEF_UN_ARI_ALL( CV_SUB, SubRC )

#define ICV_DEF_INIT_ARITHM_FUNC_TAB( FUNCNAME, FLAG )          \
static  void  icvInit##FUNCNAME##FLAG##Table( CvFuncTable* tab )\
{                                                               \
    tab->fn_2d[CV_8U] = (void*)icv##FUNCNAME##_8u_##FLAG;       \
    tab->fn_2d[CV_8S] = 0;                                      \
    tab->fn_2d[CV_16U] = (void*)icv##FUNCNAME##_16u_##FLAG;     \
    tab->fn_2d[CV_16S] = (void*)icv##FUNCNAME##_16s_##FLAG;     \
    tab->fn_2d[CV_32S] = (void*)icv##FUNCNAME##_32s_##FLAG;     \
    tab->fn_2d[CV_32F] = (void*)icv##FUNCNAME##_32f_##FLAG;     \
    tab->fn_2d[CV_64F] = (void*)icv##FUNCNAME##_64f_##FLAG;     \
}

ICV_DEF_INIT_ARITHM_FUNC_TAB( Sub, C1R )
ICV_DEF_INIT_ARITHM_FUNC_TAB( SubRC, C1R )
ICV_DEF_INIT_ARITHM_FUNC_TAB( Add, C1R )
ICV_DEF_INIT_ARITHM_FUNC_TAB( AddC, C1R )


void cvSub( const void* srcarr1, const void* srcarr2,void* dstarr, const void* maskarr )
{
    static CvFuncTable sub_tab;
    static int inittab = 0;
    int local_alloc = 1;
    uchar* buffer = 0;

    CV_FUNCNAME( "cvSub" );

    __BEGIN__;

    const CvArr* tmp;
    int y, dy, type, depth, cn, cont_flag = 0;
    int src1_step, src2_step, dst_step, tdst_step, mask_step;
    CvMat srcstub1, srcstub2, *src1, *src2;
    CvMat dststub,  *dst = (CvMat*)dstarr;
    CvMat maskstub, *mask = (CvMat*)maskarr;
    CvMat dstbuf, *tdst;
    CvFunc2D_3A func;
    CvFunc2D_3A1I func_sfs;
    CvCopyMaskFunc copym_func;
    CvSize size, tsize;

    CV_SWAP( srcarr1, srcarr2, tmp ); // to comply with IPP
    src1 = (CvMat*)srcarr1;
    src2 = (CvMat*)srcarr2;

    if( !CV_IS_MAT(src1) || !CV_IS_MAT(src2) || !CV_IS_MAT(dst))
    {
        if( CV_IS_MATND(src1) || CV_IS_MATND(src2) || CV_IS_MATND(dst))
        {
            CvArr* arrs[] = { src1, src2, dst };
            CvMatND stubs[3];
            CvNArrayIterator iterator;

            if( maskarr )
                CV_ERROR( CV_StsBadMask,
                "This operation on multi-dimensional arrays does not support mask" );

            CV_CALL( cvInitNArrayIterator( 3, arrs, 0, stubs, &iterator,0 ));

            type = iterator.hdr[0]->type;
            iterator.size.width *= CV_MAT_CN(type);

            if( !inittab )
            {
                icvInitSubC1RTable( &sub_tab );
                inittab = 1;
            }

            depth = CV_MAT_DEPTH(type);
            if( depth <= CV_16S )
            {
                func_sfs = (CvFunc2D_3A1I)(sub_tab.fn_2d[depth]);
                if( !func_sfs )
                    CV_ERROR( CV_StsUnsupportedFormat, "" );

                do
                {
                    IPPI_CALL( func_sfs( iterator.ptr[0], CV_STUB_STEP,
                                         iterator.ptr[1], CV_STUB_STEP,
                                         iterator.ptr[2], CV_STUB_STEP,
                                         iterator.size, 0 ));
                }
                while( cvNextNArraySlice( &iterator ));
            }
            else
            {
                func = (CvFunc2D_3A)(sub_tab.fn_2d[depth]);
                if( !func )
                    CV_ERROR( CV_StsUnsupportedFormat, "" );

                do
                {
                    IPPI_CALL( func( iterator.ptr[0], CV_STUB_STEP,
                                     iterator.ptr[1], CV_STUB_STEP,
                                     iterator.ptr[2], CV_STUB_STEP,
                                     iterator.size ));
                }
                while( cvNextNArraySlice( &iterator ));
            }
            EXIT;
        }
        else
        {
            int coi1 = 0, coi2 = 0, coi3 = 0;
            
            CV_CALL( src1 = cvGetMat( src1, &srcstub1, &coi1,0 ));
            CV_CALL( src2 = cvGetMat( src2, &srcstub2, &coi2,0 ));
            CV_CALL( dst = cvGetMat( dst, &dststub, &coi3,0 ));
            if( coi1 + coi2 + coi3 != 0 )
                CV_ERROR( CV_BadCOI, "" );
        }
    }

    if( !CV_ARE_TYPES_EQ( src1, src2 ) || !CV_ARE_TYPES_EQ( src1, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedFormats );

    if( !CV_ARE_SIZES_EQ( src1, src2 ) || !CV_ARE_SIZES_EQ( src1, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedSizes );

    type = CV_MAT_TYPE(src1->type);
    size = cvGetMatSize( src1 );
    depth = CV_MAT_DEPTH(type);
    cn = CV_MAT_CN(type);

    if( !mask )
    {
        if( CV_IS_MAT_CONT( src1->type & src2->type & dst->type ))
        {
            int len = size.width*size.height*cn;

            if( len <= CV_MAX_INLINE_MAT_OP_SIZE*CV_MAX_INLINE_MAT_OP_SIZE )
            {
                if( depth == CV_32F )
                {
                    const float* src1data = (const float*)(src1->data.ptr);
                    const float* src2data = (const float*)(src2->data.ptr);
                    float* dstdata = (float*)(dst->data.ptr);

                    do
                    {
                        dstdata[len-1] = (float)(src2data[len-1] - src1data[len-1]);
                    }
                    while( --len );

                    EXIT;
                }

                if( depth == CV_64F )
                {
                    const double* src1data = (const double*)(src1->data.ptr);
                    const double* src2data = (const double*)(src2->data.ptr);
                    double* dstdata = (double*)(dst->data.ptr);

                    do
                    {
                        dstdata[len-1] = src2data[len-1] - src1data[len-1];
                    }
                    while( --len );

                    EXIT;
                }
            }
            cont_flag = 1;
        }

        dy = size.height;
        copym_func = 0;
        tdst = dst;
    }
    else
    {
        int buf_size, elem_size;
        
        if( !CV_IS_MAT(mask) )
            CV_CALL( mask = cvGetMat( mask, &maskstub,NULL,0 ));

        if( !CV_IS_MASK_ARR(mask))
            CV_ERROR( CV_StsBadMask, "" );

        if( !CV_ARE_SIZES_EQ( mask, dst ))
            CV_ERROR( CV_StsUnmatchedSizes, "" );

        cont_flag = CV_IS_MAT_CONT( src1->type & src2->type & dst->type & mask->type );
        elem_size = CV_ELEM_SIZE(type);

        dy = CV_MAX_LOCAL_SIZE/(elem_size*size.height);
        dy = MAX(dy,1);
        dy = MIN(dy,size.height);
        dstbuf = cvMat( dy, size.width, type,NULL );
        if( !cont_flag )
            dstbuf.step = cvAlign( dstbuf.step, 8 );
        buf_size = dstbuf.step ? dstbuf.step*dy : size.width*elem_size;
        if( buf_size > CV_MAX_LOCAL_SIZE )
        {
            CV_CALL( buffer = (uchar*)cvAlloc( buf_size ));
            local_alloc = 0;
        }
        else
            buffer = (uchar*)cvStackAlloc( buf_size );
        dstbuf.data.ptr = buffer;
        tdst = &dstbuf;
        
        copym_func = icvGetCopyMaskFunc( elem_size );
    }

    if( !inittab )
    {
        icvInitSubC1RTable( &sub_tab );
        inittab = 1;
    }

    if( depth <= CV_16S )
    {
        func = 0;
        func_sfs = (CvFunc2D_3A1I)(sub_tab.fn_2d[depth]);
        if( !func_sfs )
            CV_ERROR( CV_StsUnsupportedFormat, "" );
    }
    else
    {
        func_sfs = 0;
        func = (CvFunc2D_3A)(sub_tab.fn_2d[depth]);
        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );
    }

    src1_step = src1->step;
    src2_step = src2->step;
    dst_step = dst->step;
    tdst_step = tdst->step;
    mask_step = mask ? mask->step : 0;

    for( y = 0; y < size.height; y += dy )
    {
        tsize.width = size.width;
        tsize.height = dy;
        if( y + dy > size.height )
            tsize.height = size.height - y;
        if( cont_flag || tsize.height == 1 )
        {
            tsize.width *= tsize.height;
            tsize.height = 1;
            src1_step = src2_step = tdst_step = dst_step = mask_step = CV_STUB_STEP;
        }

        IPPI_CALL( depth <= CV_16S ?
            func_sfs( src1->data.ptr + y*src1->step, src1_step,
                      src2->data.ptr + y*src2->step, src2_step,
                      tdst->data.ptr, tdst_step,
                      cvSize( tsize.width*cn, tsize.height ), 0 ) :
            func( src1->data.ptr + y*src1->step, src1_step,
                  src2->data.ptr + y*src2->step, src2_step,
                  tdst->data.ptr, tdst_step,
                  cvSize( tsize.width*cn, tsize.height )));

        if( mask )
        {
            IPPI_CALL( copym_func( tdst->data.ptr, tdst_step, dst->data.ptr + y*dst->step,
                                   dst_step, tsize, mask->data.ptr + y*mask->step, mask_step ));
        }
    }

    __END__;

    if( !local_alloc )
        cvFree( &buffer );
}

char* cvSeqPush( CvSeq *seq, void *element )
{
    char *ptr = 0;
    size_t elem_size;

    CV_FUNCNAME( "cvSeqPush" );

    __BEGIN__;

    if( !seq )
        CV_ERROR( CV_StsNullPtr, "" );

    elem_size = seq->elem_size;
    ptr = seq->ptr;

    if( ptr >= seq->block_max )
    {
        CV_CALL( icvGrowSeq( seq, 0 ));

        ptr = seq->ptr;
        assert( ptr + elem_size <= seq->block_max /*&& ptr == seq->block_min */  );
    }

    if( element )
        CV_MEMCPY_AUTO( ptr, element, elem_size );
    seq->first->prev->count++;
    seq->total++;
    seq->ptr = ptr + elem_size;

    __END__;

    return ptr;
}

void cvSeqPopFront( CvSeq *seq, void *element )
{
    int elem_size;
    CvSeqBlock *block;

    CV_FUNCNAME( "cvSeqPopFront" );

    __BEGIN__;

    if( !seq )
        CV_ERROR( CV_StsNullPtr, "" );
    if( seq->total <= 0 )
        CV_ERROR( CV_StsBadSize, "" );

    elem_size = seq->elem_size;
    block = seq->first;

    if( element )
        CV_MEMCPY_AUTO( element, block->data, elem_size );
    block->data += elem_size;
    block->start_index++;
    seq->total--;

    if( --(block->count) == 0 )
    {
        icvFreeSeqBlock( seq, 1 );
    }

    __END__;
}

double cvInvert( const CvArr* srcarr, CvArr* dstarr, int method )
{
    CvMat* u = 0;
    CvMat* v = 0;
    CvMat* w = 0;

    uchar* buffer = 0;
    int local_alloc = 0;
    double result = 0;
    
    CV_FUNCNAME( "cvInvert" );

    __BEGIN__;

    CvMat sstub, *src = (CvMat*)srcarr;
    CvMat dstub, *dst = (CvMat*)dstarr;
    int type;

    if( !CV_IS_MAT( src ))
        CV_CALL( src = cvGetMat( src, &sstub,NULL,0 ));

    if( !CV_IS_MAT( dst ))
        CV_CALL( dst = cvGetMat( dst, &dstub,NULL,0 ));

    type = CV_MAT_TYPE( src->type );

    if( method == CV_SVD || method == CV_SVD_SYM )
    {
        int n = MIN(src->rows,src->cols);
        if( method == CV_SVD_SYM && src->rows != src->cols )
            CV_ERROR( CV_StsBadSize, "CV_SVD_SYM method is used for non-square matrix" );

        CV_CALL( u = cvCreateMat( n, src->rows, src->type ));
        if( method != CV_SVD_SYM )
            CV_CALL( v = cvCreateMat( n, src->cols, src->type ));
        CV_CALL( w = cvCreateMat( n, 1, src->type ));
        CV_CALL( cvSVD( src, w, u, v, CV_SVD_U_T + CV_SVD_V_T ));

        if( type == CV_32FC1 )
            result = w->data.fl[0] >= FLT_EPSILON ?
                     w->data.fl[w->rows-1]/w->data.fl[0] : 0;
        else
            result = w->data.db[0] >= FLT_EPSILON ?
                     w->data.db[w->rows-1]/w->data.db[0] : 0;

        CV_CALL( cvSVBkSb( w, u, v ? v : u, 0, dst, CV_SVD_U_T + CV_SVD_V_T ));
        EXIT;
    }
    else if( method != CV_LU )
        CV_ERROR( CV_StsBadArg, "Unknown inversion method" );

    if( !CV_ARE_TYPES_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    if( src->width != src->height )
        CV_ERROR( CV_StsBadSize, "The matrix must be square" );

    if( !CV_ARE_SIZES_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    if( type != CV_32FC1 && type != CV_64FC1 )
        CV_ERROR( CV_StsUnsupportedFormat, "" );

    if( src->width <= 3 )
    {
        uchar* srcdata = src->data.ptr;
        uchar* dstdata = dst->data.ptr;
        int srcstep = src->step;
        int dststep = dst->step;

        if( src->width == 2 )
        {
            if( type == CV_32FC1 )
            {
                double d = det2(Sf);
                if( d != 0. )
                {
                    double t0, t1;
                    result = d;
                    d = 1./d;
                    t0 = Sf(0,0)*d;
                    t1 = Sf(1,1)*d;
                    Df(1,1) = (float)t0;
                    Df(0,0) = (float)t1;
                    t0 = -Sf(0,1)*d;
                    t1 = -Sf(1,0)*d;
                    Df(0,1) = (float)t0;
                    Df(1,0) = (float)t1;
                }
            }
            else
            {
                double d = det2(Sd);
                if( d != 0. )
                {
                    double t0, t1;
                    result = d;
                    d = 1./d;
                    t0 = Sd(0,0)*d;
                    t1 = Sd(1,1)*d;
                    Dd(1,1) = t0;
                    Dd(0,0) = t1;
                    t0 = -Sd(0,1)*d;
                    t1 = -Sd(1,0)*d;
                    Dd(0,1) = t0;
                    Dd(1,0) = t1;
                }
            }
        }
        else if( src->width == 3 )
        {
            if( type == CV_32FC1 )
            {
                double d = det3(Sf);
                if( d != 0. )
                {
                    float t[9];
                    result = d;
                    d = 1./d;

                    t[0] = (float)((Sf(1,1) * Sf(2,2) - Sf(1,2) * Sf(2,1)) * d);
                    t[1] = (float)((Sf(0,2) * Sf(2,1) - Sf(0,1) * Sf(2,2)) * d);
                    t[2] = (float)((Sf(0,1) * Sf(1,2) - Sf(0,2) * Sf(1,1)) * d);
                                  
                    t[3] = (float)((Sf(1,2) * Sf(2,0) - Sf(1,0) * Sf(2,2)) * d);
                    t[4] = (float)((Sf(0,0) * Sf(2,2) - Sf(0,2) * Sf(2,0)) * d);
                    t[5] = (float)((Sf(0,2) * Sf(1,0) - Sf(0,0) * Sf(1,2)) * d);
                                  
                    t[6] = (float)((Sf(1,0) * Sf(2,1) - Sf(1,1) * Sf(2,0)) * d);
                    t[7] = (float)((Sf(0,1) * Sf(2,0) - Sf(0,0) * Sf(2,1)) * d);
                    t[8] = (float)((Sf(0,0) * Sf(1,1) - Sf(0,1) * Sf(1,0)) * d);

                    Df(0,0) = t[0]; Df(0,1) = t[1]; Df(0,2) = t[2];
                    Df(1,0) = t[3]; Df(1,1) = t[4]; Df(1,2) = t[5];
                    Df(2,0) = t[6]; Df(2,1) = t[7]; Df(2,2) = t[8];
                }
            }
            else
            {
                double d = det3(Sd);
                if( d != 0. )
                {
                    double t[9];
                    result = d;
                    d = 1./d;

                    t[0] = (Sd(1,1) * Sd(2,2) - Sd(1,2) * Sd(2,1)) * d;
                    t[1] = (Sd(0,2) * Sd(2,1) - Sd(0,1) * Sd(2,2)) * d;
                    t[2] = (Sd(0,1) * Sd(1,2) - Sd(0,2) * Sd(1,1)) * d;
                           
                    t[3] = (Sd(1,2) * Sd(2,0) - Sd(1,0) * Sd(2,2)) * d;
                    t[4] = (Sd(0,0) * Sd(2,2) - Sd(0,2) * Sd(2,0)) * d;
                    t[5] = (Sd(0,2) * Sd(1,0) - Sd(0,0) * Sd(1,2)) * d;
                           
                    t[6] = (Sd(1,0) * Sd(2,1) - Sd(1,1) * Sd(2,0)) * d;
                    t[7] = (Sd(0,1) * Sd(2,0) - Sd(0,0) * Sd(2,1)) * d;
                    t[8] = (Sd(0,0) * Sd(1,1) - Sd(0,1) * Sd(1,0)) * d;

                    Dd(0,0) = t[0]; Dd(0,1) = t[1]; Dd(0,2) = t[2];
                    Dd(1,0) = t[3]; Dd(1,1) = t[4]; Dd(1,2) = t[5];
                    Dd(2,0) = t[6]; Dd(2,1) = t[7]; Dd(2,2) = t[8];
                }
            }
        }
        else
        {
            assert( src->width == 1 );

            if( type == CV_32FC1 )
            {
                double d = Sf(0,0);
                if( d != 0. )
                {
                    result = d;
                    Df(0,0) = (float)(1./d);
                }
            }
            else
            {
                double d = Sd(0,0);
                if( d != 0. )
                {
                    result = d;
                    Dd(0,0) = 1./d;
                }
            }
        }
    }
    else
    {
        CvLUDecompFunc decomp_func;
        CvLUBackFunc back_func;
        CvSize size = cvGetMatSize( src );
        const int worktype = CV_64FC1;
        int buf_size = size.width*size.height*CV_ELEM_SIZE(worktype);
        CvMat tmat;

        if( !lu_inittab )
        {
            icvInitLUTable( &lu_decomp_tab, &lu_back_tab );
            lu_inittab = 1;
        }

        if( size.width <= CV_MAX_LOCAL_MAT_SIZE )
        {
            buffer = (uchar*)cvStackAlloc( buf_size );
            local_alloc = 1;
        }
        else
        {
            CV_CALL( buffer = (uchar*)cvAlloc( buf_size ));
        }

        CV_CALL( cvInitMatHeader( &tmat, size.height, size.width, worktype, buffer,CV_AUTOSTEP ));
        if( type == worktype )
        {
            CV_CALL( cvCopy( src, &tmat,NULL ));
        }
        else
            CV_CALL( cvConvert( src, &tmat ));
        CV_CALL( cvSetIdentity( dst,cvRealScalar(1) ));

        decomp_func = (CvLUDecompFunc)(lu_decomp_tab.fn_2d[CV_MAT_DEPTH(type)-CV_32F]);
        back_func = (CvLUBackFunc)(lu_back_tab.fn_2d[CV_MAT_DEPTH(type)-CV_32F]);
        assert( decomp_func && back_func );

        IPPI_CALL( decomp_func( tmat.data.db, tmat.step, size,
                                dst->data.ptr, dst->step, size, &result ));

        if( result != 0 )
        {
            IPPI_CALL( back_func( tmat.data.db, tmat.step, size,
                                  dst->data.ptr, dst->step, size ));
        }
    }

    if( !result )
        CV_CALL( cvSetZero( dst ));

    __END__;

    if( buffer && !local_alloc )
        cvFree( &buffer );

    if( u || v || w )
    {
        cvReleaseMat( &u );
        cvReleaseMat( &v );
        cvReleaseMat( &w );
    }

    return result;
}

void cvSetIdentity( CvArr* array, CvScalar value )
{
    CV_FUNCNAME( "cvSetIdentity" );

    __BEGIN__;

    CvMat stub, *mat = (CvMat*)array;
    CvSize size;
    int i, k, len, step;
    int type, pix_size;
    uchar* data = 0;
    double buf[4];

    if( !CV_IS_MAT( mat ))
    {
        int coi = 0;
        CV_CALL( mat = cvGetMat( mat, &stub, &coi,0 ));
        if( coi != 0 )
            CV_ERROR( CV_BadCOI, "coi is not supported" );
    }

    size = cvGetMatSize( mat );
    len = CV_IMIN( size.width, size.height );

    type = CV_MAT_TYPE(mat->type);
    pix_size = CV_ELEM_SIZE(type);
    size.width *= pix_size;

    if( CV_IS_MAT_CONT( mat->type ))
    {
        size.width *= size.height;
        size.height = 1;
    }

    data = mat->data.ptr;
    step = mat->step;
    if( step == 0 )
        step = CV_STUB_STEP;
    IPPI_CALL( icvSetZero_8u_C1R( data, step, size ));
    step += pix_size;

    if( type == CV_32FC1 )
    {
        float val = (float)value.val[0];
        float* _data = (float*)data;
        step /= sizeof(_data[0]);
        len *= step;

        for( i = 0; i < len; i += step )
            _data[i] = val;
    }
    else if( type == CV_64FC1 )
    {
        double val = value.val[0];
        double* _data = (double*)data;
        step /= sizeof(_data[0]);
        len *= step;

        for( i = 0; i < len; i += step )
            _data[i] = val;
    }
    else
    {
        uchar* val_ptr = (uchar*)buf;
        cvScalarToRawData( &value, buf, type, 0 );
        len *= step;

        for( i = 0; i < len; i += step )
            for( k = 0; k < pix_size; k++ )
                data[i+k] = val_ptr[k];
    }

    __END__;
}

int array_double( void** array, int n, int size )
{
  void* tmp;

  tmp = realloc( *array, 2 * n * size );
  if( ! tmp )
    {
      fprintf( stderr, "Warning: unable to allocate memory in array_double(),"
	       " %s line %d\n", __FILE__, __LINE__ );
      if( *array )
	free( *array );
      *array = NULL;
      return 0;
    }
  *array = tmp;
  return n*2;
}

#define ICV_LUT_CASE_C1( type )             \
    for( i = 0; i <= size.width-4; i += 4 ) \
    {                                       \
        type t0 = lut[src[i]];              \
        type t1 = lut[src[i+1]];            \
        dst[i] = t0;                        \
        dst[i+1] = t1;                      \
                                            \
        t0 = lut[src[i+2]];                 \
        t1 = lut[src[i+3]];                 \
        dst[i+2] = t0;                      \
        dst[i+3] = t1;                      \
    }                                       \
                                            \
    for( ; i < size.width; i++ )            \
    {                                       \
        type t0 = lut[src[i]];              \
        dst[i] = t0;                        \
    }


#define ICV_LUT_CASE_C2( type )             \
    for( i = 0; i < size.width; i += 2 )    \
    {                                       \
        type t0 = lut[src[i]*2];            \
        type t1 = lut[src[i+1]*2 + 1];      \
        dst[i] = t0;                        \
        dst[i+1] = t1;                      \
    }

#define ICV_LUT_CASE_C3( type )             \
    for( i = 0; i < size.width; i += 3 )    \
    {                                       \
        type t0 = lut[src[i]*3];            \
        type t1 = lut[src[i+1]*3 + 1];      \
        type t2 = lut[src[i+2]*3 + 2];      \
        dst[i] = t0;                        \
        dst[i+1] = t1;                      \
        dst[i+2] = t2;                      \
    }

#define ICV_LUT_CASE_C4( type )             \
    for( i = 0; i < size.width; i += 4 )    \
    {                                       \
        type t0 = lut[src[i]*4];            \
        type t1 = lut[src[i+1]*4 + 1];      \
        dst[i] = t0;                        \
        dst[i+1] = t1;                      \
        t0 = lut[src[i+2]*4 + 2];           \
        t1 = lut[src[i+3]*4 + 3];           \
        dst[i+2] = t0;                      \
        dst[i+3] = t1;                      \
    }


#define  ICV_DEF_LUT_FUNC_8U_CN( flavor, dsttype, cn )      \
CvStatus CV_STDCALL icvLUT_Transform8u_##flavor##_C##cn##R( \
    const uchar* src, int srcstep,                          \
    dsttype* dst, int dststep, CvSize size,                 \
    const dsttype* lut )                                    \
{                                                           \
    size.width *= cn;                                       \
    dststep /= sizeof(dst[0]);                              \
    for( ; size.height--; src += srcstep, dst += dststep )  \
    {                                                       \
        int i;                                              \
        ICV_LUT_CASE_C##cn( dsttype )                       \
    }                                                       \
                                                            \
    return CV_OK;                                           \
}


ICV_DEF_LUT_FUNC_8U_CN( 8u, uchar, 1 )
ICV_DEF_LUT_FUNC_8U_CN( 16u, ushort, 1 )
ICV_DEF_LUT_FUNC_8U_CN( 32s, int, 1 )
ICV_DEF_LUT_FUNC_8U_CN( 64f, double, 1 )

ICV_DEF_LUT_FUNC_8U_CN( 8u, uchar, 2 )
ICV_DEF_LUT_FUNC_8U_CN( 8u, uchar, 3 )
ICV_DEF_LUT_FUNC_8U_CN( 8u, uchar, 4 )


#define  ICV_DEF_LUT_FUNC_8U( flavor, dsttype )             \
static CvStatus CV_STDCALL                                  \
icvLUT_Transform8u_##flavor##_CnR(                          \
    const uchar* src, int srcstep,                          \
    dsttype* dst, int dststep, CvSize size,                 \
    const dsttype* _lut, int cn )                           \
{                                                           \
    int max_block_size = (1 << 10)*cn;                      \
    dsttype lutp[1024];                                     \
    int i, k;                                               \
                                                            \
    size.width *= cn;                                       \
    dststep /= sizeof(dst[0]);                              \
                                                            \
    if( size.width*size.height < 256 )                      \
    {                                                       \
        for( ; size.height--; src+=srcstep, dst+=dststep )  \
            for( k = 0; k < cn; k++ )                       \
                for( i = 0; i < size.width; i += cn )       \
                    dst[i+k] = _lut[src[i+k]*cn+k];         \
        return CV_OK;                                       \
    }                                                       \
                                                            \
    /* repack the lut to planar layout */                   \
    for( k = 0; k < cn; k++ )                               \
        for( i = 0; i < 256; i++ )                          \
            lutp[i+k*256] = _lut[i*cn+k];                   \
                                                            \
    for( ; size.height--; src += srcstep, dst += dststep )  \
    {                                                       \
        for( i = 0; i < size.width; )                       \
        {                                                   \
            int j, limit = MIN(size.width,i+max_block_size);\
            for( k=0; k<cn; k++, src++, dst++ )             \
            {                                               \
                const dsttype* lut = lutp + k*256;          \
                for( j = i; j <= limit - cn*2; j += cn*2 )  \
                {                                           \
                    dsttype t0 = lut[src[j]];               \
                    dsttype t1 = lut[src[j+cn]];            \
                    dst[j] = t0; dst[j+cn] = t1;            \
                }                                           \
                                                            \
                for( ; j < limit; j += cn )                 \
                    dst[j] = lut[src[j]];                   \
            }                                               \
            src -= cn;                                      \
            dst -= cn;                                      \
            i += limit;                                     \
        }                                                   \
    }                                                       \
                                                            \
    return CV_OK;                                           \
}

ICV_DEF_LUT_FUNC_8U( 8u, uchar )
ICV_DEF_LUT_FUNC_8U( 16u, ushort )
ICV_DEF_LUT_FUNC_8U( 32s, int )
ICV_DEF_LUT_FUNC_8U( 64f, double )

#undef   icvLUT_Transform8u_8s_C1R
#undef   icvLUT_Transform8u_16s_C1R
#undef   icvLUT_Transform8u_32f_C1R

#define  icvLUT_Transform8u_8s_C1R    icvLUT_Transform8u_8u_C1R
#define  icvLUT_Transform8u_16s_C1R   icvLUT_Transform8u_16u_C1R
#define  icvLUT_Transform8u_32f_C1R   icvLUT_Transform8u_32s_C1R

#define  icvLUT_Transform8u_8s_CnR    icvLUT_Transform8u_8u_CnR
#define  icvLUT_Transform8u_16s_CnR   icvLUT_Transform8u_16u_CnR
#define  icvLUT_Transform8u_32f_CnR   icvLUT_Transform8u_32s_CnR

CV_DEF_INIT_FUNC_TAB_2D( LUT_Transform8u, C1R )
CV_DEF_INIT_FUNC_TAB_2D( LUT_Transform8u, CnR )

typedef CvStatus (CV_STDCALL * CvLUT_TransformCnFunc)(
    const void* src, int srcstep, void* dst,
    int dststep, CvSize size, const void* lut, int cn );

void cvLUT( const void* srcarr, void* dstarr, const void* lutarr )
{
    static CvFuncTable lut_c1_tab, lut_cn_tab;
    static CvLUT_TransformFunc lut_8u_tab[4];
    static int inittab = 0;

    CV_FUNCNAME( "cvLUT" );

    __BEGIN__;

    int  coi1 = 0, coi2 = 0;
    int  depth, cn, lut_cn;
    CvMat  srcstub, *src = (CvMat*)srcarr;
    CvMat  dststub, *dst = (CvMat*)dstarr;
    CvMat  lutstub, *lut = (CvMat*)lutarr;
    uchar* lut_data;
    uchar* shuffled_lut = 0;
    CvSize size;

    if( !inittab )
    {
        icvInitLUT_Transform8uC1RTable( &lut_c1_tab );
        icvInitLUT_Transform8uCnRTable( &lut_cn_tab );
        lut_8u_tab[0] = (CvLUT_TransformFunc)icvLUT_Transform8u_8u_C1R;
        lut_8u_tab[1] = (CvLUT_TransformFunc)icvLUT_Transform8u_8u_C2R;
        lut_8u_tab[2] = (CvLUT_TransformFunc)icvLUT_Transform8u_8u_C3R;
        lut_8u_tab[3] = (CvLUT_TransformFunc)icvLUT_Transform8u_8u_C4R;
        inittab = 1;
    }

    if( !CV_IS_MAT(src) )
        CV_CALL( src = cvGetMat( src, &srcstub, &coi1,0 ));

    if( !CV_IS_MAT(dst) )
        CV_CALL( dst = cvGetMat( dst, &dststub, &coi2,0 ));

    if( !CV_IS_MAT(lut) )
        CV_CALL( lut = cvGetMat( lut, &lutstub,NULL,0 ));

    if( coi1 != 0 || coi2 != 0 )
        CV_ERROR( CV_BadCOI, "" );

    if( !CV_ARE_SIZES_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    if( !CV_ARE_CNS_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    if( CV_MAT_DEPTH( src->type ) > CV_8S )
        CV_ERROR( CV_StsUnsupportedFormat, "" );

    depth = CV_MAT_DEPTH( dst->type );
    cn = CV_MAT_CN( dst->type );
    lut_cn = CV_MAT_CN( lut->type );

    if( !CV_IS_MAT_CONT(lut->type) || (lut_cn != 1 && lut_cn != cn) ||
        !CV_ARE_DEPTHS_EQ( dst, lut ) || lut->width*lut->height != 256 )
        CV_ERROR( CV_StsBadArg, "The LUT must be continuous array \n"
                                "with 256 elements of the same type as destination" );

    size = cvGetMatSize( src );
    if( lut_cn == 1 )
    {
        size.width *= cn;
        cn = 1;
    }

    if( CV_IS_MAT_CONT( src->type & dst->type ))
    {
        size.width *= size.height;
        size.height = 1;
    }

    lut_data = lut->data.ptr;

    if( CV_MAT_DEPTH( src->type ) == CV_8S )
    {
        int half_size = CV_ELEM_SIZE1(depth)*cn*128;
        shuffled_lut = (uchar*)cvStackAlloc(half_size*2);

        // shuffle lut
        memcpy( shuffled_lut, lut_data + half_size, half_size );
        memcpy( shuffled_lut + half_size, lut_data, half_size );

        lut_data = shuffled_lut;
    }

    if( lut_cn == 1 || lut_cn <= 4 && depth == CV_8U )
    {
        CvLUT_TransformFunc func = depth == CV_8U ? lut_8u_tab[cn-1] :
            (CvLUT_TransformFunc)(lut_c1_tab.fn_2d[depth]);
    
        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        IPPI_CALL( func( src->data.ptr, src->step, dst->data.ptr,
                         dst->step, size, lut_data ));
    }
    else
    {
        CvLUT_TransformCnFunc func =
            (CvLUT_TransformCnFunc)(lut_cn_tab.fn_2d[depth]);
    
        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        IPPI_CALL( func( src->data.ptr, src->step, dst->data.ptr,
                         dst->step, size, lut_data, cn ));
    }

    __END__;
}

const signed char icvDepthToType[] =
{
    -1, -1, CV_8U, CV_8S, CV_16U, CV_16S, -1, -1,
    CV_32F, CV_32S, -1, -1, -1, -1, -1, -1, CV_64F, -1
};

const uchar icvSaturate8u_cv[] = 
{
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
     16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
     32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
     48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
     64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
     80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,
     96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
    112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
    128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
    144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
    160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
    176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
    192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
    208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
    224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
    240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255
};

CvStatus CV_STDCALL
icvJacobiEigens_32f(float *A, float *V, float *E, int n, float eps)
{
    int i, j, k, ind;
    float *AA = A, *VV = V;
    double Amax, anorm = 0, ax;

    if( A == NULL || V == NULL || E == NULL )
        return CV_NULLPTR_ERR;
    if( n <= 0 )
        return CV_BADSIZE_ERR;
    if( eps < 1.0e-7f )
        eps = 1.0e-7f;

    /*-------- Prepare --------*/
    for( i = 0; i < n; i++, VV += n, AA += n )
    {
        for( j = 0; j < i; j++ )
        {
            double Am = AA[j];

            anorm += Am * Am;
        }
        for( j = 0; j < n; j++ )
            VV[j] = 0.f;
        VV[i] = 1.f;
    }

    anorm = sqrt( anorm + anorm );
    ax = anorm * eps / n;
    Amax = anorm;

    while( Amax > ax )
    {
        Amax /= n;
        do                      /* while (ind) */
        {
            int p, q;
            float *V1 = V, *A1 = A;

            ind = 0;
            for( p = 0; p < n - 1; p++, A1 += n, V1 += n )
            {
                float *A2 = A + n * (p + 1), *V2 = V + n * (p + 1);

                for( q = p + 1; q < n; q++, A2 += n, V2 += n )
                {
                    double x, y, c, s, c2, s2, a;
                    float *A3, Apq = A1[q], App, Aqq, Aip, Aiq, Vpi, Vqi;

                    if( fabs( Apq ) < Amax )
                        continue;

                    ind = 1;

                    /*---- Calculation of rotation angle's sine & cosine ----*/
                    App = A1[p];
                    Aqq = A2[q];
                    y = 5.0e-1 * (App - Aqq);
                    x = -Apq / sqrt( (double)Apq * Apq + (double)y * y );
                    if( y < 0.0 )
                        x = -x;
                    s = x / sqrt( 2.0 * (1.0 + sqrt( 1.0 - (double)x * x )));
                    s2 = s * s;
                    c = sqrt( 1.0 - s2 );
                    c2 = c * c;
                    a = 2.0 * Apq * c * s;

                    /*---- Apq annulation ----*/
                    A3 = A;
                    for( i = 0; i < p; i++, A3 += n )
                    {
                        Aip = A3[p];
                        Aiq = A3[q];
                        Vpi = V1[i];
                        Vqi = V2[i];
                        A3[p] = (float) (Aip * c - Aiq * s);
                        A3[q] = (float) (Aiq * c + Aip * s);
                        V1[i] = (float) (Vpi * c - Vqi * s);
                        V2[i] = (float) (Vqi * c + Vpi * s);
                    }
                    for( ; i < q; i++, A3 += n )
                    {
                        Aip = A1[i];
                        Aiq = A3[q];
                        Vpi = V1[i];
                        Vqi = V2[i];
                        A1[i] = (float) (Aip * c - Aiq * s);
                        A3[q] = (float) (Aiq * c + Aip * s);
                        V1[i] = (float) (Vpi * c - Vqi * s);
                        V2[i] = (float) (Vqi * c + Vpi * s);
                    }
                    for( ; i < n; i++ )
                    {
                        Aip = A1[i];
                        Aiq = A2[i];
                        Vpi = V1[i];
                        Vqi = V2[i];
                        A1[i] = (float) (Aip * c - Aiq * s);
                        A2[i] = (float) (Aiq * c + Aip * s);
                        V1[i] = (float) (Vpi * c - Vqi * s);
                        V2[i] = (float) (Vqi * c + Vpi * s);
                    }
                    A1[p] = (float) (App * c2 + Aqq * s2 - a);
                    A2[q] = (float) (App * s2 + Aqq * c2 + a);
                    A1[q] = A2[p] = 0.0f;
                }               /*q */
            }                   /*p */
        }
        while( ind );
        Amax /= n;
    }                           /* while ( Amax > ax ) */

    for( i = 0, k = 0; i < n; i++, k += n + 1 )
        E[i] = A[k];
    /*printf(" M = %d\n", M); */

    /* -------- ordering -------- */
    for( i = 0; i < n; i++ )
    {
        int m = i;
        float Em = (float) fabs( E[i] );

        for( j = i + 1; j < n; j++ )
        {
            float Ej = (float) fabs( E[j] );

            m = (Em < Ej) ? j : m;
            Em = (Em < Ej) ? Ej : Em;
        }
        if( m != i )
        {
            int l;
            float b = E[i];

            E[i] = E[m];
            E[m] = b;
            for( j = 0, k = i * n, l = m * n; j < n; j++, k++, l++ )
            {
                b = V[k];
                V[k] = V[l];
                V[l] = b;
            }
        }
    }

    return CV_NO_ERR;
}

static CvStatus CV_STDCALL
icvJacobiEigens_64d(double *A, double *V, double *E, int n, double eps)
{
    int i, j, k, p, q, ind, iters = 0;
    double *A1 = A, *V1 = V, *A2 = A, *V2 = V;
    double Amax = 0.0, anorm = 0.0, ax;

    if( A == NULL || V == NULL || E == NULL )
        return CV_NULLPTR_ERR;
    if( n <= 0 )
        return CV_BADSIZE_ERR;
    if( eps < DBL_EPSILON )
        eps = DBL_EPSILON;

    /*-------- Prepare --------*/
    for( i = 0; i < n; i++, V1 += n, A1 += n )
    {
        for( j = 0; j < i; j++ )
        {
            double Am = A1[j];

            anorm += Am * Am;
        }
        for( j = 0; j < n; j++ )
            V1[j] = 0.0;
        V1[i] = 1.0;
    }

    anorm = sqrt( anorm + anorm );
    ax = anorm * eps / n;
    Amax = anorm;

    while( Amax > ax && iters++ < 100 )
    {
        Amax /= n;
        do                      /* while (ind) */
        {
            ind = 0;
            A1 = A;
            V1 = V;
            for( p = 0; p < n - 1; p++, A1 += n, V1 += n )
            {
                A2 = A + n * (p + 1);
                V2 = V + n * (p + 1);
                for( q = p + 1; q < n; q++, A2 += n, V2 += n )
                {
                    double x, y, c, s, c2, s2, a;
                    double *A3, Apq, App, Aqq, App2, Aqq2, Aip, Aiq, Vpi, Vqi;

                    if( fabs( A1[q] ) < Amax )
                        continue;
                    Apq = A1[q];

                    ind = 1;

                    /*---- Calculation of rotation angle's sine & cosine ----*/
                    App = A1[p];
                    Aqq = A2[q];
                    y = 5.0e-1 * (App - Aqq);
                    x = -Apq / sqrt( Apq * Apq + (double)y * y );
                    if( y < 0.0 )
                        x = -x;
                    s = x / sqrt( 2.0 * (1.0 + sqrt( 1.0 - (double)x * x )));
                    s2 = s * s;
                    c = sqrt( 1.0 - s2 );
                    c2 = c * c;
                    a = 2.0 * Apq * c * s;

                    /*---- Apq annulation ----*/
                    A3 = A;
                    for( i = 0; i < p; i++, A3 += n )
                    {
                        Aip = A3[p];
                        Aiq = A3[q];
                        Vpi = V1[i];
                        Vqi = V2[i];
                        A3[p] = Aip * c - Aiq * s;
                        A3[q] = Aiq * c + Aip * s;
                        V1[i] = Vpi * c - Vqi * s;
                        V2[i] = Vqi * c + Vpi * s;
                    }
                    for( ; i < q; i++, A3 += n )
                    {
                        Aip = A1[i];
                        Aiq = A3[q];
                        Vpi = V1[i];
                        Vqi = V2[i];
                        A1[i] = Aip * c - Aiq * s;
                        A3[q] = Aiq * c + Aip * s;
                        V1[i] = Vpi * c - Vqi * s;
                        V2[i] = Vqi * c + Vpi * s;
                    }
                    for( ; i < n; i++ )
                    {
                        Aip = A1[i];
                        Aiq = A2[i];
                        Vpi = V1[i];
                        Vqi = V2[i];
                        A1[i] = Aip * c - Aiq * s;
                        A2[i] = Aiq * c + Aip * s;
                        V1[i] = Vpi * c - Vqi * s;
                        V2[i] = Vqi * c + Vpi * s;
                    }
                    App2 = App * c2 + Aqq * s2 - a;
                    Aqq2 = App * s2 + Aqq * c2 + a;
                    A1[p] = App2;
                    A2[q] = Aqq2;
                    A1[q] = A2[p] = 0.0;
                }               /*q */
            }                   /*p */
        }
        while( ind );
    }                           /* while ( Amax > ax ) */

    for( i = 0, k = 0; i < n; i++, k += n + 1 )
        E[i] = A[k];

    /* -------- ordering -------- */
    for( i = 0; i < n; i++ )
    {
        int m = i;
        double Em = fabs( E[i] );

        for( j = i + 1; j < n; j++ )
        {
            double Ej = fabs( E[j] );

            m = (Em < Ej) ? j : m;
            Em = (Em < Ej) ? Ej : Em;
        }
        if( m != i )
        {
            int l;
            double b = E[i];

            E[i] = E[m];
            E[m] = b;
            for( j = 0, k = i * n, l = m * n; j < n; j++, k++, l++ )
            {
                b = V[k];
                V[k] = V[l];
                V[l] = b;
            }
        }
    }

    return CV_NO_ERR;
}

void cvEigenVV( CvArr* srcarr, CvArr* evectsarr, CvArr* evalsarr, double eps )
{

    CV_FUNCNAME( "cvEigenVV" );

    __BEGIN__;

    CvMat sstub, *src = (CvMat*)srcarr;
    CvMat estub1, *evects = (CvMat*)evectsarr;
    CvMat estub2, *evals = (CvMat*)evalsarr;

    if( !CV_IS_MAT( src ))
        CV_CALL( src = cvGetMat( src, &sstub,NULL,0 ));

    if( !CV_IS_MAT( evects ))
        CV_CALL( evects = cvGetMat( evects, &estub1,NULL,0 ));

    if( !CV_IS_MAT( evals ))
        CV_CALL( evals = cvGetMat( evals, &estub2,NULL,0 ));

    if( src->cols != src->rows )
        CV_ERROR( CV_StsUnmatchedSizes, "source is not quadratic matrix" );

    if( !CV_ARE_SIZES_EQ( src, evects) )
        CV_ERROR( CV_StsUnmatchedSizes, "eigenvectors matrix has inappropriate size" );

    if( (evals->rows != src->rows || evals->cols != 1) &&
        (evals->cols != src->rows || evals->rows != 1))
        CV_ERROR( CV_StsBadSize, "eigenvalues vector has inappropriate size" );

    if( !CV_ARE_TYPES_EQ( src, evects ) || !CV_ARE_TYPES_EQ( src, evals ))
        CV_ERROR( CV_StsUnmatchedFormats,
        "input matrix, eigenvalues and eigenvectors must have the same type" );

    if( !CV_IS_MAT_CONT( src->type & evals->type & evects->type ))
        CV_ERROR( CV_BadStep, "all the matrices must be continuous" );

    if( CV_MAT_TYPE(src->type) == CV_32FC1 )
    {
        IPPI_CALL( icvJacobiEigens_32f( src->data.fl,
                                        evects->data.fl,
                                        evals->data.fl, src->cols, (float)eps ));

    }
    else if( CV_MAT_TYPE(src->type) == CV_64FC1 )
    {
        IPPI_CALL( icvJacobiEigens_64d( src->data.db,
                                        evects->data.db,
                                        evals->data.db, src->cols, eps ));
    }
    else
    {
        CV_ERROR( CV_StsUnsupportedFormat, "Only 32fC1 and 64fC1 types are supported" );
    }

    CV_CHECK_NANS( evects );
    CV_CHECK_NANS( evals );

    __END__;
}

int cvClipLine( CvSize img_size, CvPoint* pt1, CvPoint* pt2 )
{
    int result = 0;

    CV_FUNCNAME( "cvClipLine" );

    __BEGIN__;

    int x1, y1, x2, y2;
    int c1, c2;
    int right = img_size.width-1, bottom = img_size.height-1;

    if( !pt1 || !pt2 )
        CV_ERROR( CV_StsNullPtr, "One of point pointers is NULL" );

    if( right < 0 || bottom < 0 )
        CV_ERROR( CV_StsOutOfRange, "Image width or height are negative" );

    x1 = pt1->x; y1 = pt1->y; x2 = pt2->x; y2 = pt2->y;
    c1 = (x1 < 0) + (x1 > right) * 2 + (y1 < 0) * 4 + (y1 > bottom) * 8;
    c2 = (x2 < 0) + (x2 > right) * 2 + (y2 < 0) * 4 + (y2 > bottom) * 8;

    if( (c1 & c2) == 0 && (c1 | c2) != 0 )
    {
        int a;

        if( c1 & 12 )
        {
            a = c1 < 8 ? 0 : bottom;
            x1 += (int) (((int64) (a - y1)) * (x2 - x1) / (y2 - y1));
            y1 = a;
            c1 = (x1 < 0) + (x1 > right) * 2;
        }
        if( c2 & 12 )
        {
            a = c2 < 8 ? 0 : bottom;
            x2 += (int) (((int64) (a - y2)) * (x2 - x1) / (y2 - y1));
            y2 = a;
            c2 = (x2 < 0) + (x2 > right) * 2;
        }
        if( (c1 & c2) == 0 && (c1 | c2) != 0 )
        {
            if( c1 )
            {
                a = c1 == 1 ? 0 : right;
                y1 += (int) (((int64) (a - x1)) * (y2 - y1) / (x2 - x1));
                x1 = a;
                c1 = 0;
            }
            if( c2 )
            {
                a = c2 == 1 ? 0 : right;
                y2 += (int) (((int64) (a - x2)) * (y2 - y1) / (x2 - x1));
                x2 = a;
                c2 = 0;
            }
        }

        assert( (c1 & c2) != 0 || (x1 | y1 | x2 | y2) >= 0 );

        pt1->x = x1;
        pt1->y = y1;
        pt2->x = x2;
        pt2->y = y2;
    }

    result = ( c1 | c2 ) == 0;

    __END__;

    return result;
}

int cvInitLineIterator( const CvArr* img, CvPoint pt1, CvPoint pt2, CvLineIterator* iterator, int connectivity, int left_to_right )
{
    int count = -1;
    
    CV_FUNCNAME( "cvInitLineIterator" );

    __BEGIN__;
    
    CvMat stub, *mat = (CvMat*)img;
    int dx, dy, s;
    int bt_pix, bt_pix0, step;

    if( !CV_IS_MAT(mat) )
        CV_CALL( mat = cvGetMat( mat, &stub,NULL,0 ));

    if( !iterator )
        CV_ERROR( CV_StsNullPtr, "Pointer to the iterator state is NULL" );

    if( connectivity != 8 && connectivity != 4 )
        CV_ERROR( CV_StsBadArg, "Connectivity must be 8 or 4" );

    if( (unsigned)pt1.x >= (unsigned)(mat->width) ||
        (unsigned)pt2.x >= (unsigned)(mat->width) ||
        (unsigned)pt1.y >= (unsigned)(mat->height) ||
        (unsigned)pt2.y >= (unsigned)(mat->height) )
        CV_ERROR( CV_StsBadPoint,
            "One of the ending points is outside of the image, use cvClipLine" );

    bt_pix0 = bt_pix = CV_ELEM_SIZE(mat->type);
    step = mat->step;

    dx = pt2.x - pt1.x;
    dy = pt2.y - pt1.y;
    s = dx < 0 ? -1 : 0;

    if( left_to_right )
    {
        dx = (dx ^ s) - s;
        dy = (dy ^ s) - s;
        pt1.x ^= (pt1.x ^ pt2.x) & s;
        pt1.y ^= (pt1.y ^ pt2.y) & s;
    }
    else
    {
        dx = (dx ^ s) - s;
        bt_pix = (bt_pix ^ s) - s;
    }

    iterator->ptr = (uchar*)(mat->data.ptr + pt1.y * step + pt1.x * bt_pix0);

    s = dy < 0 ? -1 : 0;
    dy = (dy ^ s) - s;
    step = (step ^ s) - s;

    s = dy > dx ? -1 : 0;
    
    /* conditional swaps */
    dx ^= dy & s;
    dy ^= dx & s;
    dx ^= dy & s;

    bt_pix ^= step & s;
    step ^= bt_pix & s;
    bt_pix ^= step & s;

    if( connectivity == 8 )
    {
        assert( dx >= 0 && dy >= 0 );
        
        iterator->err = dx - (dy + dy);
        iterator->plus_delta = dx + dx;
        iterator->minus_delta = -(dy + dy);
        iterator->plus_step = step;
        iterator->minus_step = bt_pix;
        count = dx + 1;
    }
    else /* connectivity == 4 */
    {
        assert( dx >= 0 && dy >= 0 );
        
        iterator->err = 0;
        iterator->plus_delta = (dx + dx) + (dy + dy);
        iterator->minus_delta = -(dy + dy);
        iterator->plus_step = step - bt_pix;
        iterator->minus_step = bt_pix;
        count = dx + dy + 1;
    }

    __END__;

    return count;
}

static void icvLine( CvMat* mat, CvPoint pt1, CvPoint pt2, const void* color, int connectivity = 8 )
{
    if( cvClipLine( cvGetMatSize(mat), &pt1, &pt2 ))
    {
        CvLineIterator iterator;
        int pix_size = CV_ELEM_SIZE(mat->type);
        int i, count;
        
        if( connectivity == 0 )
            connectivity = 8;
        if( connectivity == 1 )
            connectivity = 4;
        
        count = cvInitLineIterator( mat, pt1, pt2, &iterator, connectivity, 1 );

        for( i = 0; i < count; i++ )
        {
            CV_MEMCPY_AUTO( iterator.ptr, color, pix_size );
            CV_NEXT_LINE_POINT( iterator );
        }
    }
}

static const uchar icvSlopeCorrTable[] = {
    181, 181, 181, 182, 182, 183, 184, 185, 187, 188, 190, 192, 194, 196, 198, 201,
    203, 206, 209, 211, 214, 218, 221, 224, 227, 231, 235, 238, 242, 246, 250, 254
};

/* Gaussian for antialiasing filter */
static const int icvFilterTable[] = {
    168, 177, 185, 194, 202, 210, 218, 224, 231, 236, 241, 246, 249, 252, 254, 254,
    254, 254, 252, 249, 246, 241, 236, 231, 224, 218, 210, 202, 194, 185, 177, 168,
    158, 149, 140, 131, 122, 114, 105, 97, 89, 82, 75, 68, 62, 56, 50, 45,
    40, 36, 32, 28, 25, 22, 19, 16, 14, 12, 11, 9, 8, 7, 5, 5
};

static void icvLineAA( CvMat* img, CvPoint pt1, CvPoint pt2, const void* color )
{
    int dx, dy;
    int ecount, scount = 0;
    int slope;
    int ax, ay;
    int x_step, y_step;
    int i, j;
    int ep_table[9];
    int cb = ((uchar*)color)[0], cg = ((uchar*)color)[1], cr = ((uchar*)color)[2];
    int _cb, _cg, _cr;
    int nch = CV_MAT_CN( img->type );
    uchar* ptr = (uchar*)(img->data.ptr);
    int step = img->step;
    CvSize size = cvGetMatSize( img );

    assert( img && (nch == 1 || nch == 3) && CV_MAT_DEPTH(img->type) == CV_8U );

    pt1.x -= XY_ONE*2;
    pt1.y -= XY_ONE*2;
    pt2.x -= XY_ONE*2;
    pt2.y -= XY_ONE*2;
    ptr += img->step*2 + 2*nch;

    size.width = ((size.width - 5) << XY_SHIFT) + 1;
    size.height = ((size.height - 5) << XY_SHIFT) + 1;

    if( !cvClipLine( size, &pt1, &pt2 ))
        return;

    dx = pt2.x - pt1.x;
    dy = pt2.y - pt1.y;

    j = dx < 0 ? -1 : 0;
    ax = (dx ^ j) - j;
    i = dy < 0 ? -1 : 0;
    ay = (dy ^ i) - i;

    if( ax > ay )
    {
        dx = ax;
        dy = (dy ^ j) - j;
        pt1.x ^= pt2.x & j;
        pt2.x ^= pt1.x & j;
        pt1.x ^= pt2.x & j;
        pt1.y ^= pt2.y & j;
        pt2.y ^= pt1.y & j;
        pt1.y ^= pt2.y & j;

        x_step = XY_ONE;
        y_step = (int) (((int64) dy << XY_SHIFT) / (ax | 1));
        pt2.x += XY_ONE;
        ecount = (pt2.x >> XY_SHIFT) - (pt1.x >> XY_SHIFT);
        j = -(pt1.x & (XY_ONE - 1));
        pt1.y += (int) ((((int64) y_step) * j) >> XY_SHIFT) + (XY_ONE >> 1);
        slope = (y_step >> (XY_SHIFT - 5)) & 0x3f;
        slope ^= (y_step < 0 ? 0x3f : 0);

        /* Get 4-bit fractions for end-point adjustments */
        i = (pt1.x >> (XY_SHIFT - 7)) & 0x78;
        j = (pt2.x >> (XY_SHIFT - 7)) & 0x78;
    }
    else
    {
        dy = ay;
        dx = (dx ^ i) - i;
        pt1.x ^= pt2.x & i;
        pt2.x ^= pt1.x & i;
        pt1.x ^= pt2.x & i;
        pt1.y ^= pt2.y & i;
        pt2.y ^= pt1.y & i;
        pt1.y ^= pt2.y & i;

        x_step = (int) (((int64) dx << XY_SHIFT) / (ay | 1));
        y_step = XY_ONE;
        pt2.y += XY_ONE;
        ecount = (pt2.y >> XY_SHIFT) - (pt1.y >> XY_SHIFT);
        j = -(pt1.y & (XY_ONE - 1));
        pt1.x += (int) ((((int64) x_step) * j) >> XY_SHIFT) + (XY_ONE >> 1);
        slope = (x_step >> (XY_SHIFT - 5)) & 0x3f;
        slope ^= (x_step < 0 ? 0x3f : 0);

        /* Get 4-bit fractions for end-point adjustments */
        i = (pt1.y >> (XY_SHIFT - 7)) & 0x78;
        j = (pt2.y >> (XY_SHIFT - 7)) & 0x78;
    }

    slope = (slope & 0x20) ? 0x100 : icvSlopeCorrTable[slope];

    /* Calc end point correction table */
    {
        int t0 = slope << 7;
        int t1 = ((0x78 - i) | 4) * slope;
        int t2 = (j | 4) * slope;

        ep_table[0] = 0;
        ep_table[8] = slope;
        ep_table[1] = ep_table[3] = ((((j - i) & 0x78) | 4) * slope >> 8) & 0x1ff;
        ep_table[2] = (t1 >> 8) & 0x1ff;
        ep_table[4] = ((((j - i) + 0x80) | 4) * slope >> 8) & 0x1ff;
        ep_table[5] = ((t1 + t0) >> 8) & 0x1ff;
        ep_table[6] = (t2 >> 8) & 0x1ff;
        ep_table[7] = ((t2 + t0) >> 8) & 0x1ff;
    }

    if( nch == 3 )
    {
        #define  ICV_PUT_POINT()            \
        {                                   \
            _cb = tptr[0];                  \
            _cb += ((cb - _cb)*a + 127)>> 8;\
            _cg = tptr[1];                  \
            _cg += ((cg - _cg)*a + 127)>> 8;\
            _cr = tptr[2];                  \
            _cr += ((cr - _cr)*a + 127)>> 8;\
            tptr[0] = (uchar)_cb;           \
            tptr[1] = (uchar)_cg;           \
            tptr[2] = (uchar)_cr;           \
        }
        if( ax > ay )
        {
            ptr += (pt1.x >> XY_SHIFT) * 3;

            while( ecount >= 0 )
            {
                uchar *tptr = ptr + ((pt1.y >> XY_SHIFT) - 1) * step;

                int ep_corr = ep_table[(((scount >= 2) + 1) & (scount | 2)) * 3 +
                                       (((ecount >= 2) + 1) & (ecount | 2))];
                int a, dist = (pt1.y >> (XY_SHIFT - 5)) & 31;

                a = (ep_corr * icvFilterTable[dist + 32] >> 8) & 0xff;
                ICV_PUT_POINT();
                ICV_PUT_POINT();

                tptr += step;
                a = (ep_corr * icvFilterTable[dist] >> 8) & 0xff;
                ICV_PUT_POINT();
                ICV_PUT_POINT();

                tptr += step;
                a = (ep_corr * icvFilterTable[63 - dist] >> 8) & 0xff;
                ICV_PUT_POINT();
                ICV_PUT_POINT();

                pt1.y += y_step;
                ptr += 3;
                scount++;
                ecount--;
            }
        }
        else
        {
            ptr += (pt1.y >> XY_SHIFT) * step;

            while( ecount >= 0 )
            {
                uchar *tptr = ptr + ((pt1.x >> XY_SHIFT) - 1) * 3;

                int ep_corr = ep_table[(((scount >= 2) + 1) & (scount | 2)) * 3 +
                                       (((ecount >= 2) + 1) & (ecount | 2))];
                int a, dist = (pt1.x >> (XY_SHIFT - 5)) & 31;

                a = (ep_corr * icvFilterTable[dist + 32] >> 8) & 0xff;
                ICV_PUT_POINT();
                ICV_PUT_POINT();

                tptr += 3;
                a = (ep_corr * icvFilterTable[dist] >> 8) & 0xff;
                ICV_PUT_POINT();
                ICV_PUT_POINT();

                tptr += 3;
                a = (ep_corr * icvFilterTable[63 - dist] >> 8) & 0xff;
                ICV_PUT_POINT();
                ICV_PUT_POINT();

                pt1.x += x_step;
                ptr += step;
                scount++;
                ecount--;
            }
        }
        #undef ICV_PUT_POINT
    }
    else
    {
        #define  ICV_PUT_POINT()            \
        {                                   \
            _cb = tptr[0];                  \
            _cb += ((cb - _cb)*a + 127)>> 8;\
            tptr[0] = (uchar)_cb;           \
        }

        if( ax > ay )
        {
            ptr += (pt1.x >> XY_SHIFT);

            while( ecount >= 0 )
            {
                uchar *tptr = ptr + ((pt1.y >> XY_SHIFT) - 1) * step;

                int ep_corr = ep_table[(((scount >= 2) + 1) & (scount | 2)) * 3 +
                                       (((ecount >= 2) + 1) & (ecount | 2))];
                int a, dist = (pt1.y >> (XY_SHIFT - 5)) & 31;

                a = (ep_corr * icvFilterTable[dist + 32] >> 8) & 0xff;
                ICV_PUT_POINT();
                ICV_PUT_POINT();

                tptr += step;
                a = (ep_corr * icvFilterTable[dist] >> 8) & 0xff;
                ICV_PUT_POINT();
                ICV_PUT_POINT();

                tptr += step;
                a = (ep_corr * icvFilterTable[63 - dist] >> 8) & 0xff;
                ICV_PUT_POINT();
                ICV_PUT_POINT();

                pt1.y += y_step;
                ptr++;
                scount++;
                ecount--;
            }
        }
        else
        {
            ptr += (pt1.y >> XY_SHIFT) * step;

            while( ecount >= 0 )
            {
                uchar *tptr = ptr + ((pt1.x >> XY_SHIFT) - 1);

                int ep_corr = ep_table[(((scount >= 2) + 1) & (scount | 2)) * 3 +
                                       (((ecount >= 2) + 1) & (ecount | 2))];
                int a, dist = (pt1.x >> (XY_SHIFT - 5)) & 31;

                a = (ep_corr * icvFilterTable[dist + 32] >> 8) & 0xff;
                ICV_PUT_POINT();
                ICV_PUT_POINT();

                tptr++;
                a = (ep_corr * icvFilterTable[dist] >> 8) & 0xff;
                ICV_PUT_POINT();
                ICV_PUT_POINT();

                tptr++;
                a = (ep_corr * icvFilterTable[63 - dist] >> 8) & 0xff;
                ICV_PUT_POINT();
                ICV_PUT_POINT();

                pt1.x += x_step;
                ptr += step;
                scount++;
                ecount--;
            }
        }
        #undef ICV_PUT_POINT
    }
}

static void icvLine2( CvMat* img, CvPoint pt1, CvPoint pt2, const void* color )
{
    int dx, dy;
    int ecount;
    int ax, ay;
    int i, j;
    int x_step, y_step;
    int cb = ((uchar*)color)[0], cg = ((uchar*)color)[1], cr = ((uchar*)color)[2];
    int pix_size = CV_ELEM_SIZE( img->type );
    uchar *ptr = (uchar*)(img->data.ptr), *tptr;
    int step = img->step;
    CvSize size = cvGetMatSize( img );

    //assert( img && (nch == 1 || nch == 3) && CV_MAT_DEPTH(img->type) == CV_8U );

    pt1.x -= XY_ONE*2;
    pt1.y -= XY_ONE*2;
    pt2.x -= XY_ONE*2;
    pt2.y -= XY_ONE*2;
    ptr += img->step*2 + 2*pix_size;

    size.width = ((size.width - 5) << XY_SHIFT) + 1;
    size.height = ((size.height - 5) << XY_SHIFT) + 1;

    if( !cvClipLine( size, &pt1, &pt2 ))
        return;

    dx = pt2.x - pt1.x;
    dy = pt2.y - pt1.y;

    j = dx < 0 ? -1 : 0;
    ax = (dx ^ j) - j;
    i = dy < 0 ? -1 : 0;
    ay = (dy ^ i) - i;

    if( ax > ay )
    {
        dx = ax;
        dy = (dy ^ j) - j;
        pt1.x ^= pt2.x & j;
        pt2.x ^= pt1.x & j;
        pt1.x ^= pt2.x & j;
        pt1.y ^= pt2.y & j;
        pt2.y ^= pt1.y & j;
        pt1.y ^= pt2.y & j;

        x_step = XY_ONE;
        y_step = (int) (((int64) dy << XY_SHIFT) / (ax | 1));
        ecount = (pt2.x - pt1.x) >> XY_SHIFT;
    }
    else
    {
        dy = ay;
        dx = (dx ^ i) - i;
        pt1.x ^= pt2.x & i;
        pt2.x ^= pt1.x & i;
        pt1.x ^= pt2.x & i;
        pt1.y ^= pt2.y & i;
        pt2.y ^= pt1.y & i;
        pt1.y ^= pt2.y & i;

        x_step = (int) (((int64) dx << XY_SHIFT) / (ay | 1));
        y_step = XY_ONE;
        ecount = (pt2.y - pt1.y) >> XY_SHIFT;
    }

    pt1.x += (XY_ONE >> 1);
    pt1.y += (XY_ONE >> 1);

    if( pix_size == 3 )
    {
        #define  ICV_PUT_POINT()    \
        {                           \
            tptr[0] = (uchar)cb;    \
            tptr[1] = (uchar)cg;    \
            tptr[2] = (uchar)cr;    \
        }
        
        tptr = ptr + ((pt2.x + (XY_ONE >> 1))>> XY_SHIFT)*3 +
            ((pt2.y + (XY_ONE >> 1)) >> XY_SHIFT)*step;
        ICV_PUT_POINT();
        
        if( ax > ay )
        {
            ptr += (pt1.x >> XY_SHIFT) * 3;

            while( ecount >= 0 )
            {
                tptr = ptr + (pt1.y >> XY_SHIFT) * step;
                ICV_PUT_POINT();
                pt1.y += y_step;
                ptr += 3;
                ecount--;
            }
        }
        else
        {
            ptr += (pt1.y >> XY_SHIFT) * step;

            while( ecount >= 0 )
            {
                tptr = ptr + (pt1.x >> XY_SHIFT) * 3;
                ICV_PUT_POINT();
                pt1.x += x_step;
                ptr += step;
                ecount--;
            }
        }

        #undef ICV_PUT_POINT
    }
    else if( pix_size == 1 )
    {
        #define  ICV_PUT_POINT()            \
        {                                   \
            tptr[0] = (uchar)cb;            \
        }

        tptr = ptr + ((pt2.x + (XY_ONE >> 1))>> XY_SHIFT) +
            ((pt2.y + (XY_ONE >> 1)) >> XY_SHIFT)*step;
        ICV_PUT_POINT();

        if( ax > ay )
        {
            ptr += (pt1.x >> XY_SHIFT);

            while( ecount >= 0 )
            {
                tptr = ptr + (pt1.y >> XY_SHIFT) * step;
                ICV_PUT_POINT();
                pt1.y += y_step;
                ptr++;
                ecount--;
            }
        }
        else
        {
            ptr += (pt1.y >> XY_SHIFT) * step;

            while( ecount >= 0 )
            {
                tptr = ptr + (pt1.x >> XY_SHIFT);
                ICV_PUT_POINT();
                pt1.x += x_step;
                ptr += step;
                ecount--;
            }
        }
        #undef ICV_PUT_POINT
    }
    else
    {
        #define  ICV_PUT_POINT()                \
            for( j = 0; j < pix_size; j++ )     \
                tptr[j] = ((uchar*)color)[j];

        tptr = ptr + ((pt2.x + (XY_ONE >> 1))>> XY_SHIFT)*pix_size +
            ((pt2.y + (XY_ONE >> 1)) >> XY_SHIFT)*step;
        ICV_PUT_POINT();
        
        if( ax > ay )
        {
            ptr += (pt1.x >> XY_SHIFT) * pix_size;

            while( ecount >= 0 )
            {
                tptr = ptr + (pt1.y >> XY_SHIFT) * step;
                ICV_PUT_POINT();
                pt1.y += y_step;
                ptr += pix_size;
                ecount--;
            }
        }
        else
        {
            ptr += (pt1.y >> XY_SHIFT) * step;

            while( ecount >= 0 )
            {
                tptr = ptr + (pt1.x >> XY_SHIFT) * pix_size;
                ICV_PUT_POINT();
                pt1.x += x_step;
                ptr += step;
                ecount--;
            }
        }

        #undef ICV_PUT_POINT
    }
}

static void icvFillConvexPoly( CvMat* img, CvPoint *v, int npts, const void* color, int line_type, int shift )
{
    struct
    {
        int idx, di;
        int x, dx, ye;
    }
    edge[2];

    int delta = shift ? 1 << (shift - 1) : 0;
    int i, y, imin = 0, left = 0, right = 1, x1, x2;
    int edges = npts;
    int xmin, xmax, ymin, ymax;
    uchar* ptr = img->data.ptr;
    CvSize size = cvGetMatSize( img );
    int pix_size = CV_ELEM_SIZE(img->type);
    CvPoint p0;
    int delta1, delta2;

    if( line_type < CV_AA )
        delta1 = delta2 = XY_ONE >> 1;
        //delta1 = 0, delta2 = XY_ONE - 1;
    else
        delta1 = XY_ONE - 1, delta2 = 0;

    p0 = v[npts - 1];
    p0.x <<= XY_SHIFT - shift;
    p0.y <<= XY_SHIFT - shift;

    assert( 0 <= shift && shift <= XY_SHIFT );
    xmin = xmax = v[0].x;
    ymin = ymax = v[0].y;

    for( i = 0; i < npts; i++ )
    {
        CvPoint p = v[i];
        if( p.y < ymin )
        {
            ymin = p.y;
            imin = i;
        }

        ymax = MAX( ymax, p.y );
        xmax = MAX( xmax, p.x );
        xmin = MIN( xmin, p.x );

        p.x <<= XY_SHIFT - shift;
        p.y <<= XY_SHIFT - shift;

        if( line_type <= 8 )
        {
            if( shift == 0 )
            {
                CvPoint pt0, pt1;
                pt0.x = p0.x >> XY_SHIFT;
                pt0.y = p0.y >> XY_SHIFT;
                pt1.x = p.x >> XY_SHIFT;
                pt1.y = p.y >> XY_SHIFT;
                icvLine( img, pt0, pt1, color, line_type );
            }
            else
                icvLine2( img, p0, p, color );
        }
        else
            icvLineAA( img, p0, p, color );
        p0 = p;
    }

    xmin = (xmin + delta) >> shift;
    xmax = (xmax + delta) >> shift;
    ymin = (ymin + delta) >> shift;
    ymax = (ymax + delta) >> shift;

    if( npts < 3 || xmax < 0 || ymax < 0 || xmin >= size.width || ymin >= size.height )
        return;

    ymax = MIN( ymax, size.height - 1 );
    edge[0].idx = edge[1].idx = imin;

    edge[0].ye = edge[1].ye = y = ymin;
    edge[0].di = 1;
    edge[1].di = npts - 1;

    ptr += img->step*y;

    do
    {
        if( line_type < CV_AA || y < ymax || y == ymin )
        {
            for( i = 0; i < 2; i++ )
            {
                if( y >= edge[i].ye )
                {
                    int idx = edge[i].idx, di = edge[i].di;
                    int xs = 0, xe, ye, ty = 0;

                    for(;;)
                    {
                        ty = (v[idx].y + delta) >> shift;
                        if( ty > y || edges == 0 )
                            break;
                        xs = v[idx].x;
                        idx += di;
                        idx -= ((idx < npts) - 1) & npts;   /* idx -= idx >= npts ? npts : 0 */
                        edges--;
                    }

                    ye = ty;
                    xs <<= XY_SHIFT - shift;
                    xe = v[idx].x << (XY_SHIFT - shift);

                    /* no more edges */
                    if( y >= ye )
                        return;

                    edge[i].ye = ye;
                    edge[i].dx = ((xe - xs)*2 + (ye - y)) / (2 * (ye - y));
                    edge[i].x = xs;
                    edge[i].idx = idx;
                }
            }
        }

        if( edge[left].x > edge[right].x )
        {
            left ^= 1;
            right ^= 1;
        }

        x1 = edge[left].x;
        x2 = edge[right].x;

        if( y >= 0 )
        {
            int xx1 = (x1 + delta1) >> XY_SHIFT;
            int xx2 = (x2 + delta2) >> XY_SHIFT;

            if( xx2 >= 0 && xx1 < size.width )
            {
                if( xx1 < 0 )
                    xx1 = 0;
                if( xx2 >= size.width )
                    xx2 = size.width - 1;
                ICV_HLINE( ptr, xx1, xx2, color, pix_size );
            }
        }

        x1 += edge[left].dx;
        x2 += edge[right].dx;

        edge[left].x = x1;
        edge[right].x = x2;
        ptr += img->step;
    }
    while( ++y <= ymax );
}

static void
icvCircle( CvMat* img, CvPoint center, int radius, const void* color, int fill )
{
    CvSize size = cvGetMatSize( img );
    int step = img->step;
    int pix_size = CV_ELEM_SIZE(img->type);
    uchar* ptr = (uchar*)(img->data.ptr);
    int err = 0, dx = radius, dy = 0, plus = 1, minus = (radius << 1) - 1;
    int inside = center.x >= radius && center.x < size.width - radius &&
        center.y >= radius && center.y < size.height - radius;

    #define ICV_PUT_POINT( ptr, x )     \
        CV_MEMCPY_CHAR( ptr + (x)*pix_size, color, pix_size );

    while( dx >= dy )
    {
        int mask;
        int y11 = center.y - dy, y12 = center.y + dy, y21 = center.y - dx, y22 = center.y + dx;
        int x11 = center.x - dx, x12 = center.x + dx, x21 = center.x - dy, x22 = center.x + dy;

        if( inside )
        {
            uchar *tptr0 = ptr + y11 * step;
            uchar *tptr1 = ptr + y12 * step;
            
            if( !fill )
            {
                ICV_PUT_POINT( tptr0, x11 );
                ICV_PUT_POINT( tptr1, x11 );
                ICV_PUT_POINT( tptr0, x12 );
                ICV_PUT_POINT( tptr1, x12 );
            }
            else
            {
                ICV_HLINE( tptr0, x11, x12, color, pix_size );
                ICV_HLINE( tptr1, x11, x12, color, pix_size );
            }

            tptr0 = ptr + y21 * step;
            tptr1 = ptr + y22 * step;

            if( !fill )
            {
                ICV_PUT_POINT( tptr0, x21 );
                ICV_PUT_POINT( tptr1, x21 );
                ICV_PUT_POINT( tptr0, x22 );
                ICV_PUT_POINT( tptr1, x22 );
            }
            else
            {
                ICV_HLINE( tptr0, x21, x22, color, pix_size );
                ICV_HLINE( tptr1, x21, x22, color, pix_size );
            }
        }
        else if( x11 < size.width && x12 >= 0 && y21 < size.height && y22 >= 0 )
        {
            if( fill )
            {
                x11 = MAX( x11, 0 );
                x12 = MIN( x12, size.width - 1 );
            }
            
            if( (unsigned)y11 < (unsigned)size.height )
            {
                uchar *tptr = ptr + y11 * step;

                if( !fill )
                {
                    if( x11 >= 0 )
                        ICV_PUT_POINT( tptr, x11 );
                    if( x12 < size.width )
                        ICV_PUT_POINT( tptr, x12 );
                }
                else
                    ICV_HLINE( tptr, x11, x12, color, pix_size );
            }

            if( (unsigned)y12 < (unsigned)size.height )
            {
                uchar *tptr = ptr + y12 * step;

                if( !fill )
                {
                    if( x11 >= 0 )
                        ICV_PUT_POINT( tptr, x11 );
                    if( x12 < size.width )
                        ICV_PUT_POINT( tptr, x12 );
                }
                else
                    ICV_HLINE( tptr, x11, x12, color, pix_size );
            }

            if( x21 < size.width && x22 >= 0 )
            {
                if( fill )
                {
                    x21 = MAX( x21, 0 );
                    x22 = MIN( x22, size.width - 1 );
                }

                if( (unsigned)y21 < (unsigned)size.height )
                {
                    uchar *tptr = ptr + y21 * step;

                    if( !fill )
                    {
                        if( x21 >= 0 )
                            ICV_PUT_POINT( tptr, x21 );
                        if( x22 < size.width )
                            ICV_PUT_POINT( tptr, x22 );
                    }
                    else
                        ICV_HLINE( tptr, x21, x22, color, pix_size );
                }

                if( (unsigned)y22 < (unsigned)size.height )
                {
                    uchar *tptr = ptr + y22 * step;

                    if( !fill )
                    {
                        if( x21 >= 0 )
                            ICV_PUT_POINT( tptr, x21 );
                        if( x22 < size.width )
                            ICV_PUT_POINT( tptr, x22 );
                    }
                    else
                        ICV_HLINE( tptr, x21, x22, color, pix_size );
                }
            }
        }
        dy++;
        err += plus;
        plus += 2;

        mask = (err <= 0) - 1;

        err -= minus & mask;
        dx += mask;
        minus -= mask & 2;
    }

    #undef  ICV_PUT_POINT
}

static void icvCollectPolyEdges( CvMat* img, CvSeq* v, CvContour* edges, const void* color, int line_type, int shift, CvPoint offset )
{
    int  i, count = v->total;
    CvRect bounds = edges->rect;
    int delta = offset.y + (shift ? 1 << (shift - 1) : 0);
    int elem_type = CV_MAT_TYPE(v->flags);

    CvSeqReader reader;
    CvSeqWriter writer;

    cvStartReadSeq( v, &reader,0 );
    cvStartAppendToSeq( (CvSeq*)edges, &writer );

    for( i = 0; i < count; i++ )
    {
        CvPoint pt0, pt1, t0, t1;
        CvPolyEdge edge;
        CV_READ_EDGE( pt0, pt1, reader );
        
        if( elem_type == CV_32SC2 )
        {
            pt0.x = (pt0.x + offset.x) << (XY_SHIFT - shift);
            pt0.y = (pt0.y + delta) >> shift;
            pt1.x = (pt1.x + offset.x) << (XY_SHIFT - shift);
            pt1.y = (pt1.y + delta) >> shift;
        }
        else
        {
            Cv32suf x, y;
            assert( shift == 0 );

            x.i = pt0.x; y.i = pt0.y;
            pt0.x = cvRound((x.f + offset.x) * XY_ONE);
            pt0.y = cvRound(y.f + offset.y);
            x.i = pt1.x; y.i = pt1.y;
            pt1.x = cvRound((x.f + offset.x) * XY_ONE);
            pt1.y = cvRound(y.f + offset.y);
        }

        if( line_type < CV_AA )
        {
            t0.y = pt0.y; t1.y = pt1.y;
            t0.x = (pt0.x + (XY_ONE >> 1)) >> XY_SHIFT;
            t1.x = (pt1.x + (XY_ONE >> 1)) >> XY_SHIFT;
            icvLine( img, t0, t1, color, line_type );
        }
        else
        {
            t0.x = pt0.x; t1.x = pt1.x;
            t0.y = pt0.y << XY_SHIFT;
            t1.y = pt1.y << XY_SHIFT;
            icvLineAA( img, t0, t1, color );
        }

        if( pt0.y == pt1.y )
            continue;

        if( pt0.y > pt1.y )
            CV_SWAP( pt0, pt1, t0 );

        bounds.y = MIN( bounds.y, pt0.y );
        bounds.height = MAX( bounds.height, pt1.y );

        if( pt0.x < pt1.x )
        {
            bounds.x = MIN( bounds.x, pt0.x );
            bounds.width = MAX( bounds.width, pt1.x );
        }
        else
        {
            bounds.x = MIN( bounds.x, pt1.x );
            bounds.width = MAX( bounds.width, pt0.x );
        }

        edge.y0 = pt0.y;
        edge.y1 = pt1.y;
        edge.x = pt0.x;
        edge.dx = (pt1.x - pt0.x) / (pt1.y - pt0.y);
        assert( edge.y0 < edge.y1 );

        CV_WRITE_SEQ_ELEM( edge, writer );
    }

    edges->rect = bounds;
    cvEndWriteSeq( &writer );
}

static int icvCmpEdges( const void* _e1, const void* _e2, void* /*userdata*/ )
{
    CvPolyEdge *e1 = (CvPolyEdge*)_e1, *e2 = (CvPolyEdge*)_e2;
    return e1->y0 - e2->y0 ? e1->y0 - e2->y0 :
           e1->x - e2->x ? e1->x - e2->x : e1->dx - e2->dx;
}

static void icvFillEdgeCollection( CvMat* img, CvContour* edges, const void* color )
{
    CvPolyEdge tmp;
    int i, y, total = edges->total;
    CvSeqReader reader;
    CvSize size = cvGetMatSize(img);
    CvPolyEdge* e;
    int y_max = INT_MIN;
    int pix_size = CV_ELEM_SIZE(img->type);

    __BEGIN__;
    
    memset( &tmp, 0, sizeof(tmp));
    
    /* check parameters */
    if( edges->total < 2 || edges->rect.height < 0 || edges->rect.y >= size.height ||
        edges->rect.width < 0 || edges->rect.x >= size.width )
        EXIT;

    cvSeqSort( (CvSeq*)edges, icvCmpEdges, 0 );
    cvStartReadSeq( (CvSeq*)edges, &reader,0 );

#ifdef _DEBUG
    e = &tmp;
    tmp.y0 = INT_MIN;
#endif

    for( i = 0; i < total; i++ )
    {
        CvPolyEdge* e1 = (CvPolyEdge*)(reader.ptr);

#ifdef _DEBUG
        assert( e1->y0 < e1->y1 && (i == 0 || icvCmpEdges( e, e1, 0 ) <= 0) );
        e = e1;
#endif
        y_max = MAX( y_max, e1->y1 );

        CV_NEXT_SEQ_ELEM( sizeof(CvPolyEdge), reader );
    }

    /* start drawing */
    tmp.y0 = INT_MAX;
    cvSeqPush( (CvSeq*)edges, &tmp );

    i = 0;
    tmp.next = 0;
    cvStartReadSeq( (CvSeq*)edges, &reader,0 );
    e = (CvPolyEdge*)(reader.ptr);
    y_max = MIN( y_max, size.height );

    for( y = e->y0; y < y_max; y++ )
    {
        CvPolyEdge *last, *prelast, *keep_prelast;
        int sort_flag = 0;
        int draw = 0;
        int clipline = y < 0;

        prelast = &tmp;
        last = tmp.next;
        while( last || e->y0 == y )
        {
            if( last && last->y1 == y )
            {
                /* exlude edge if y reachs its lower point */
                prelast->next = last->next;
                last = last->next;
                continue;
            }
            keep_prelast = prelast;
            if( last && (e->y0 > y || last->x < e->x) )
            {
                /* go to the next edge in active list */
                prelast = last;
                last = last->next;
            }
            else if( i < total )
            {
                /* insert new edge into active list if y reachs its upper point */
                prelast->next = e;
                e->next = last;
                prelast = e;
                CV_NEXT_SEQ_ELEM( edges->elem_size, reader );
                e = (CvPolyEdge*)(reader.ptr);
                i++;
            }
            else
                break;

            if( draw )
            {
                if( !clipline )
                {
                    /* convert x's from fixed-point to image coordinates */
                    uchar *timg = (uchar*)(img->data.ptr) + y * img->step;
                    int x1 = keep_prelast->x;
                    int x2 = prelast->x;

                    if( x1 > x2 )
                    {
                        int t = x1;

                        x1 = x2;
                        x2 = t;
                    }

                    x1 = (x1 + XY_ONE - 1) >> XY_SHIFT;
                    x2 = x2 >> XY_SHIFT;

                    /* clip and draw the line */
                    if( x1 < size.width && x2 >= 0 )
                    {
                        if( x1 < 0 )
                            x1 = 0;
                        if( x2 >= size.width )
                            x2 = size.width - 1;
                        ICV_HLINE( timg, x1, x2, color, pix_size );
                    }
                }
                keep_prelast->x += keep_prelast->dx;
                prelast->x += prelast->dx;
            }
            draw ^= 1;
        }

        /* sort edges (bubble sort on list) */
        keep_prelast = 0;

        do
        {
            prelast = &tmp;
            last = tmp.next;

            while( last != keep_prelast && last->next != 0 )
            {
                CvPolyEdge *te = last->next;

                /* swap edges */
                if( last->x > te->x )
                {
                    prelast->next = te;
                    last->next = te->next;
                    te->next = last;
                    prelast = te;
                    sort_flag = 1;
                }
                else
                {
                    prelast = last;
                    last = te;
                }
            }
            keep_prelast = prelast;
        }
        while( sort_flag && keep_prelast != tmp.next && keep_prelast != &tmp );
    }

    __END__;
}



static void icvEllipseEx( CvMat* img, CvPoint center, CvSize axes, int angle, int arc_start, int arc_end, const void* color, int thickness, int line_type )
{
    CvMemStorage* st = 0;

    CV_FUNCNAME( "icvEllipseEx" );
    
    __BEGIN__;

    CvPoint v[1 << 8];
    int count, delta;

    if( axes.width < 0 || axes.height < 0 )
        CV_ERROR( CV_StsBadSize, "" );

    delta = (MAX(axes.width,axes.height)+(XY_ONE>>1))>>XY_SHIFT;
    delta = delta < 3 ? 90 : delta < 10 ? 30 : delta < 15 ? 18 : 5;

    count = cvEllipse2Poly( center, axes, angle, arc_start, arc_end, v, delta );

    if( thickness >= 0 )
    {
        icvPolyLine( img, v, count, 0, color, thickness, line_type, XY_SHIFT );
    }
    else if( arc_end - arc_start >= 360 )
    {
        icvFillConvexPoly( img, v, count, color, line_type, XY_SHIFT );
    }
    else
    {
        CvContour* edges;
        CvSeq vtx;
        CvSeqBlock block;
        
        CV_CALL( st = cvCreateMemStorage( CV_DRAWING_STORAGE_BLOCK ));
        CV_CALL( edges = (CvContour*)cvCreateSeq( 0, sizeof(CvContour), sizeof(CvPolyEdge), st ));
        v[count++] = center;

        CV_CALL( cvMakeSeqHeaderForArray( CV_32SC2, sizeof(CvSeq), sizeof(CvPoint),
                                          v, count, &vtx, &block ));

        CV_CALL( icvCollectPolyEdges( img, &vtx, edges, color, line_type, XY_SHIFT,cvPoint(0,0) ));
        CV_CALL( icvFillEdgeCollection( img, edges, color ));
    }

    __END__;

    if( st )
        cvReleaseMemStorage( &st );
}

static void icvThickLine( CvMat* img, CvPoint p0, CvPoint p1, const void* color, int thickness, int line_type, int flags, int shift )
{
    static const double INV_XY_ONE = 1./XY_ONE;

    p0.x <<= XY_SHIFT - shift;
    p0.y <<= XY_SHIFT - shift;
    p1.x <<= XY_SHIFT - shift;
    p1.y <<= XY_SHIFT - shift;

    if( thickness <= 1 )
    {
        if( line_type < CV_AA )
        {
            if( line_type == 1 || line_type == 4 || shift == 0 )
            {
                p0.x = (p0.x + (XY_ONE>>1)) >> XY_SHIFT;
                p0.y = (p0.y + (XY_ONE>>1)) >> XY_SHIFT;
                p1.x = (p1.x + (XY_ONE>>1)) >> XY_SHIFT;
                p1.y = (p1.y + (XY_ONE>>1)) >> XY_SHIFT;
                icvLine( img, p0, p1, color, line_type );
            }
            else
                icvLine2( img, p0, p1, color );
        }
        else
            icvLineAA( img, p0, p1, color );
    }
    else
    {
        CvPoint pt[4], dp = {0,0};
        double dx = (p0.x - p1.x)*INV_XY_ONE, dy = (p1.y - p0.y)*INV_XY_ONE;
        double r = dx * dx + dy * dy;
        int i;
        thickness <<= XY_SHIFT - 1;

        if( fabs(r) > DBL_EPSILON )
        {
            r = thickness * cvInvSqrt( (float) r );
            dp.x = cvRound( dy * r );
            dp.y = cvRound( dx * r );
        }

        pt[0].x = p0.x + dp.x;
        pt[0].y = p0.y + dp.y;
        pt[1].x = p0.x - dp.x;
        pt[1].y = p0.y - dp.y;
        pt[2].x = p1.x - dp.x;
        pt[2].y = p1.y - dp.y;
        pt[3].x = p1.x + dp.x;
        pt[3].y = p1.y + dp.y;

        icvFillConvexPoly( img, pt, 4, color, line_type, XY_SHIFT );

        for( i = 0; i < 2; i++ )
        {
            if( flags & (i+1) )
            {
                if( line_type < CV_AA )
                {
                    CvPoint center;
                    center.x = (p0.x + (XY_ONE>>1)) >> XY_SHIFT;
                    center.y = (p0.y + (XY_ONE>>1)) >> XY_SHIFT;
                    icvCircle( img, center, thickness >> XY_SHIFT, color, 1 ); 
                }
                else
                {
                    icvEllipseEx( img, p0, cvSize(thickness, thickness),
                                  0, 0, 360, color, -1, line_type );
                }
            }
            p0 = p1;
        }
    }
}

void icvPolyLine( CvMat* img, CvPoint *v, int count, int is_closed, const void* color, int thickness, int line_type, int shift )
{
    CV_FUNCNAME("icvPolyLine");

    __BEGIN__;
    
    if( count > 0 )
    {
        int i = is_closed ? count - 1 : 0;
        int flags = 2 + !is_closed;
        CvPoint p0;
        assert( 0 <= shift && shift <= XY_SHIFT );
        assert( img && thickness >= 0 ); 
        assert( v && count >= 0 );

        if( !v )
            CV_ERROR( CV_StsNullPtr, "" );

        p0 = v[i];
        for( i = !is_closed; i < count; i++ )
        {
            CvPoint p = v[i];
            icvThickLine( img, p0, p, color, thickness, line_type, flags, shift );
            p0 = p;
            flags = 2;
        }
    }

    __END__;
}


















void cvEllipse( void *img, CvPoint center, CvSize axes, double angle, double start_angle, double end_angle, CvScalar color, int thickness, int line_type, int shift )
{
    CV_FUNCNAME( "cvEllipse" );

    __BEGIN__;

    int coi = 0;
    CvMat stub, *mat = (CvMat*)img;
    double buf[4];

    CV_CALL( mat = cvGetMat( mat, &stub, &coi,0 ));

    if( line_type == CV_AA && CV_MAT_DEPTH(mat->type) != CV_8U )
        line_type = 8;

    if( coi != 0 )
        CV_ERROR( CV_BadCOI, cvUnsupportedFormat );

    if( axes.width < 0 || axes.height < 0 )
        CV_ERROR( CV_StsOutOfRange, "" );

    if( thickness < -1 || thickness > 255 )
        CV_ERROR( CV_StsOutOfRange, "" );

    if( shift < 0 || XY_SHIFT < shift )
        CV_ERROR( CV_StsOutOfRange, "shift must be between 0 and 16" );

    CV_CALL( cvScalarToRawData( &color, buf, mat->type, 0 ));

    {
        int _angle = cvRound(angle);
        int _start_angle = cvRound(start_angle);
        int _end_angle = cvRound(end_angle);
        center.x <<= XY_SHIFT - shift;
        center.y <<= XY_SHIFT - shift;
        axes.width <<= XY_SHIFT - shift;
        axes.height <<= XY_SHIFT - shift;

        CV_CALL( icvEllipseEx( mat, center, axes, _angle, _start_angle,
                               _end_angle, buf, thickness, line_type ));
    }

    __END__;
}

static const float icvSinTable[] =
    { 0.0000000f, 0.0174524f, 0.0348995f, 0.0523360f, 0.0697565f, 0.0871557f,
    0.1045285f, 0.1218693f, 0.1391731f, 0.1564345f, 0.1736482f, 0.1908090f,
    0.2079117f, 0.2249511f, 0.2419219f, 0.2588190f, 0.2756374f, 0.2923717f,
    0.3090170f, 0.3255682f, 0.3420201f, 0.3583679f, 0.3746066f, 0.3907311f,
    0.4067366f, 0.4226183f, 0.4383711f, 0.4539905f, 0.4694716f, 0.4848096f,
    0.5000000f, 0.5150381f, 0.5299193f, 0.5446390f, 0.5591929f, 0.5735764f,
    0.5877853f, 0.6018150f, 0.6156615f, 0.6293204f, 0.6427876f, 0.6560590f,
    0.6691306f, 0.6819984f, 0.6946584f, 0.7071068f, 0.7193398f, 0.7313537f,
    0.7431448f, 0.7547096f, 0.7660444f, 0.7771460f, 0.7880108f, 0.7986355f,
    0.8090170f, 0.8191520f, 0.8290376f, 0.8386706f, 0.8480481f, 0.8571673f,
    0.8660254f, 0.8746197f, 0.8829476f, 0.8910065f, 0.8987940f, 0.9063078f,
    0.9135455f, 0.9205049f, 0.9271839f, 0.9335804f, 0.9396926f, 0.9455186f,
    0.9510565f, 0.9563048f, 0.9612617f, 0.9659258f, 0.9702957f, 0.9743701f,
    0.9781476f, 0.9816272f, 0.9848078f, 0.9876883f, 0.9902681f, 0.9925462f,
    0.9945219f, 0.9961947f, 0.9975641f, 0.9986295f, 0.9993908f, 0.9998477f,
    1.0000000f, 0.9998477f, 0.9993908f, 0.9986295f, 0.9975641f, 0.9961947f,
    0.9945219f, 0.9925462f, 0.9902681f, 0.9876883f, 0.9848078f, 0.9816272f,
    0.9781476f, 0.9743701f, 0.9702957f, 0.9659258f, 0.9612617f, 0.9563048f,
    0.9510565f, 0.9455186f, 0.9396926f, 0.9335804f, 0.9271839f, 0.9205049f,
    0.9135455f, 0.9063078f, 0.8987940f, 0.8910065f, 0.8829476f, 0.8746197f,
    0.8660254f, 0.8571673f, 0.8480481f, 0.8386706f, 0.8290376f, 0.8191520f,
    0.8090170f, 0.7986355f, 0.7880108f, 0.7771460f, 0.7660444f, 0.7547096f,
    0.7431448f, 0.7313537f, 0.7193398f, 0.7071068f, 0.6946584f, 0.6819984f,
    0.6691306f, 0.6560590f, 0.6427876f, 0.6293204f, 0.6156615f, 0.6018150f,
    0.5877853f, 0.5735764f, 0.5591929f, 0.5446390f, 0.5299193f, 0.5150381f,
    0.5000000f, 0.4848096f, 0.4694716f, 0.4539905f, 0.4383711f, 0.4226183f,
    0.4067366f, 0.3907311f, 0.3746066f, 0.3583679f, 0.3420201f, 0.3255682f,
    0.3090170f, 0.2923717f, 0.2756374f, 0.2588190f, 0.2419219f, 0.2249511f,
    0.2079117f, 0.1908090f, 0.1736482f, 0.1564345f, 0.1391731f, 0.1218693f,
    0.1045285f, 0.0871557f, 0.0697565f, 0.0523360f, 0.0348995f, 0.0174524f,
    0.0000000f, -0.0174524f, -0.0348995f, -0.0523360f, -0.0697565f, -0.0871557f,
    -0.1045285f, -0.1218693f, -0.1391731f, -0.1564345f, -0.1736482f, -0.1908090f,
    -0.2079117f, -0.2249511f, -0.2419219f, -0.2588190f, -0.2756374f, -0.2923717f,
    -0.3090170f, -0.3255682f, -0.3420201f, -0.3583679f, -0.3746066f, -0.3907311f,
    -0.4067366f, -0.4226183f, -0.4383711f, -0.4539905f, -0.4694716f, -0.4848096f,
    -0.5000000f, -0.5150381f, -0.5299193f, -0.5446390f, -0.5591929f, -0.5735764f,
    -0.5877853f, -0.6018150f, -0.6156615f, -0.6293204f, -0.6427876f, -0.6560590f,
    -0.6691306f, -0.6819984f, -0.6946584f, -0.7071068f, -0.7193398f, -0.7313537f,
    -0.7431448f, -0.7547096f, -0.7660444f, -0.7771460f, -0.7880108f, -0.7986355f,
    -0.8090170f, -0.8191520f, -0.8290376f, -0.8386706f, -0.8480481f, -0.8571673f,
    -0.8660254f, -0.8746197f, -0.8829476f, -0.8910065f, -0.8987940f, -0.9063078f,
    -0.9135455f, -0.9205049f, -0.9271839f, -0.9335804f, -0.9396926f, -0.9455186f,
    -0.9510565f, -0.9563048f, -0.9612617f, -0.9659258f, -0.9702957f, -0.9743701f,
    -0.9781476f, -0.9816272f, -0.9848078f, -0.9876883f, -0.9902681f, -0.9925462f,
    -0.9945219f, -0.9961947f, -0.9975641f, -0.9986295f, -0.9993908f, -0.9998477f,
    -1.0000000f, -0.9998477f, -0.9993908f, -0.9986295f, -0.9975641f, -0.9961947f,
    -0.9945219f, -0.9925462f, -0.9902681f, -0.9876883f, -0.9848078f, -0.9816272f,
    -0.9781476f, -0.9743701f, -0.9702957f, -0.9659258f, -0.9612617f, -0.9563048f,
    -0.9510565f, -0.9455186f, -0.9396926f, -0.9335804f, -0.9271839f, -0.9205049f,
    -0.9135455f, -0.9063078f, -0.8987940f, -0.8910065f, -0.8829476f, -0.8746197f,
    -0.8660254f, -0.8571673f, -0.8480481f, -0.8386706f, -0.8290376f, -0.8191520f,
    -0.8090170f, -0.7986355f, -0.7880108f, -0.7771460f, -0.7660444f, -0.7547096f,
    -0.7431448f, -0.7313537f, -0.7193398f, -0.7071068f, -0.6946584f, -0.6819984f,
    -0.6691306f, -0.6560590f, -0.6427876f, -0.6293204f, -0.6156615f, -0.6018150f,
    -0.5877853f, -0.5735764f, -0.5591929f, -0.5446390f, -0.5299193f, -0.5150381f,
    -0.5000000f, -0.4848096f, -0.4694716f, -0.4539905f, -0.4383711f, -0.4226183f,
    -0.4067366f, -0.3907311f, -0.3746066f, -0.3583679f, -0.3420201f, -0.3255682f,
    -0.3090170f, -0.2923717f, -0.2756374f, -0.2588190f, -0.2419219f, -0.2249511f,
    -0.2079117f, -0.1908090f, -0.1736482f, -0.1564345f, -0.1391731f, -0.1218693f,
    -0.1045285f, -0.0871557f, -0.0697565f, -0.0523360f, -0.0348995f, -0.0174524f,
    -0.0000000f, 0.0174524f, 0.0348995f, 0.0523360f, 0.0697565f, 0.0871557f,
    0.1045285f, 0.1218693f, 0.1391731f, 0.1564345f, 0.1736482f, 0.1908090f,
    0.2079117f, 0.2249511f, 0.2419219f, 0.2588190f, 0.2756374f, 0.2923717f,
    0.3090170f, 0.3255682f, 0.3420201f, 0.3583679f, 0.3746066f, 0.3907311f,
    0.4067366f, 0.4226183f, 0.4383711f, 0.4539905f, 0.4694716f, 0.4848096f,
    0.5000000f, 0.5150381f, 0.5299193f, 0.5446390f, 0.5591929f, 0.5735764f,
    0.5877853f, 0.6018150f, 0.6156615f, 0.6293204f, 0.6427876f, 0.6560590f,
    0.6691306f, 0.6819984f, 0.6946584f, 0.7071068f, 0.7193398f, 0.7313537f,
    0.7431448f, 0.7547096f, 0.7660444f, 0.7771460f, 0.7880108f, 0.7986355f,
    0.8090170f, 0.8191520f, 0.8290376f, 0.8386706f, 0.8480481f, 0.8571673f,
    0.8660254f, 0.8746197f, 0.8829476f, 0.8910065f, 0.8987940f, 0.9063078f,
    0.9135455f, 0.9205049f, 0.9271839f, 0.9335804f, 0.9396926f, 0.9455186f,
    0.9510565f, 0.9563048f, 0.9612617f, 0.9659258f, 0.9702957f, 0.9743701f,
    0.9781476f, 0.9816272f, 0.9848078f, 0.9876883f, 0.9902681f, 0.9925462f,
    0.9945219f, 0.9961947f, 0.9975641f, 0.9986295f, 0.9993908f, 0.9998477f,
    1.0000000f
};

static void icvSinCos( int angle, float *cosval, float *sinval )
{
    angle += (angle < 0 ? 360 : 0);
    *sinval = icvSinTable[angle];
    *cosval = icvSinTable[450 - angle];
}

int cvEllipse2Poly( CvPoint center, CvSize axes, int angle, int arc_start, int arc_end, CvPoint* pts, int delta )
{
    float alpha, beta;
    double size_a = axes.width, size_b = axes.height;
    double cx = center.x, cy = center.y;
    CvPoint *pts_origin = pts;
    int i;

    while( angle < 0 )
        angle += 360;
    while( angle > 360 )
        angle -= 360;

    if( arc_start > arc_end )
    {
        i = arc_start;
        arc_start = arc_end;
        arc_end = i;
    }
    while( arc_start < 0 )
    {
        arc_start += 360;
        arc_end += 360;
    }
    while( arc_end > 360 )
    {
        arc_end -= 360;
        arc_start -= 360;
    }
    if( arc_end - arc_start > 360 )
    {
        arc_start = 0;
        arc_end = 360;
    }
    icvSinCos( angle, &alpha, &beta );

    for( i = arc_start; i < arc_end + delta; i += delta )
    {
        double x, y;
        angle = i;
        if( angle > arc_end )
            angle = arc_end;
        if( angle < 0 )
            angle += 360;
        
        x = size_a * icvSinTable[450-angle];
        y = size_b * icvSinTable[angle];
        pts->x = cvRound( cx + x * alpha - y * beta );
        pts->y = cvRound( cy - x * beta - y * alpha );
        pts += i == arc_start || pts->x != pts[-1].x || pts->y != pts[-1].y;
    }

    i = (int)(pts - pts_origin);
    for( ; i < 2; i++ )
        pts_origin[i] = pts_origin[i-1];
    return i;
}

void cvStartAppendToSeq( CvSeq *seq, CvSeqWriter * writer )
{
    CV_FUNCNAME( "cvStartAppendToSeq" );

    __BEGIN__;

    if( !seq || !writer )
        CV_ERROR( CV_StsNullPtr, "" );

    memset( writer, 0, sizeof( *writer ));
    writer->header_size = sizeof( CvSeqWriter );

    writer->seq = seq;
    writer->block = seq->first ? seq->first->prev : 0;
    writer->ptr = seq->ptr;
    writer->block_max = seq->block_max;

    __END__;
}

void cvCreateSeqBlock( CvSeqWriter * writer )
{
    CV_FUNCNAME( "cvCreateSeqBlock" );

    __BEGIN__;

    CvSeq *seq;

    if( !writer || !writer->seq )
        CV_ERROR( CV_StsNullPtr, "" );

    seq = writer->seq;

    cvFlushSeqWriter( writer );

    CV_CALL( icvGrowSeq( seq, 0 ));

    writer->block = seq->first->prev;
    writer->ptr = seq->ptr;
    writer->block_max = seq->block_max;

    __END__;
}

CvSeq * cvEndWriteSeq( CvSeqWriter * writer )
{
    CvSeq *seq = 0;

    CV_FUNCNAME( "cvEndWriteSeq" );

    __BEGIN__;

    if( !writer )
        CV_ERROR( CV_StsNullPtr, "" );

    CV_CALL( cvFlushSeqWriter( writer ));
    seq = writer->seq;

    /* truncate the last block */
    if( writer->block && writer->seq->storage )
    {
        CvMemStorage *storage = seq->storage;
        char *storage_block_max = (char *) storage->top + storage->block_size;

        assert( writer->block->count > 0 );

        if( (unsigned)((storage_block_max - storage->free_space)
            - seq->block_max) < CV_STRUCT_ALIGN )
        {
            storage->free_space = cvAlignLeft((int)(storage_block_max - seq->ptr), CV_STRUCT_ALIGN);
            seq->block_max = seq->ptr;
        }
    }

    writer->ptr = 0;

    __END__;

    return seq;
}

void cvFlushSeqWriter( CvSeqWriter * writer )
{
    CvSeq *seq = 0;

    CV_FUNCNAME( "cvFlushSeqWriter" );

    __BEGIN__;

    if( !writer )
        CV_ERROR( CV_StsNullPtr, "" );

    seq = writer->seq;
    seq->ptr = writer->ptr;

    if( writer->block )
    {
        int total = 0;
        CvSeqBlock *first_block = writer->seq->first;
        CvSeqBlock *block = first_block;

        writer->block->count = (int)((writer->ptr - writer->block->data) / seq->elem_size);
        assert( writer->block->count > 0 );

        do
        {
            total += block->count;
            block = block->next;
        }
        while( block != first_block );

        writer->seq->total = total;
    }

    __END__;
}

void cvLine( void* img, CvPoint pt1, CvPoint pt2, CvScalar color, int thickness, int line_type, int shift )
{
    CV_FUNCNAME( "cvLine" );

    __BEGIN__;

    int coi = 0;
    CvMat stub, *mat = (CvMat*)img;
    double buf[4];

    CV_CALL( mat = cvGetMat( img, &stub, &coi,0 ));

    if( line_type == CV_AA && CV_MAT_DEPTH(mat->type) != CV_8U )
        line_type = 8;

    if( coi != 0 )
        CV_ERROR( CV_BadCOI, cvUnsupportedFormat );

    if( (unsigned)thickness > 255  )
        CV_ERROR( CV_StsOutOfRange, "" );

    if( shift < 0 || XY_SHIFT < shift )
        CV_ERROR( CV_StsOutOfRange, "shift must be between 0 and 16" );

    CV_CALL( cvScalarToRawData( &color, buf, mat->type, 0 ));
    icvThickLine( mat, pt1, pt2, buf, thickness, line_type, 3, shift ); 

    __END__;
}

void cvSetImageROI( IplImage* image, CvRect rect )
{
    CV_FUNCNAME( "cvSetImageROI" );

    __BEGIN__;

    if( !image )
        CV_ERROR( CV_HeaderIsNull, "" );

    if( rect.x > image->width || rect.y > image->height )
        CV_ERROR( CV_BadROISize, "" );

    if( rect.x + rect.width < 0 || rect.y + rect.height < 0 )
        CV_ERROR( CV_BadROISize, "" );

    if( rect.x < 0 )
    {
        rect.width += rect.x;
        rect.x = 0;
    }

    if( rect.y < 0 )
    {
        rect.height += rect.y;
        rect.y = 0;
    }

    if( rect.x + rect.width > image->width )
        rect.width = image->width - rect.x;

    if( rect.y + rect.height > image->height )
        rect.height = image->height - rect.y;

    if( image->roi )
    {
        image->roi->xOffset = rect.x;
        image->roi->yOffset = rect.y;
        image->roi->width = rect.width;
        image->roi->height = rect.height;
    }
    else
    {
        CV_CALL( image->roi = icvCreateROI( 0, rect.x, rect.y, rect.width, rect.height ));
    }

    __END__;
}


void cvResetImageROI( IplImage* image )
{
    CV_FUNCNAME( "cvResetImageROI" );

    __BEGIN__;

    if( !image )
        CV_ERROR( CV_HeaderIsNull, "" );

    if( image->roi )
    {
        if( !CvIPL.deallocate )
        {
            cvFree( &image->roi );
        }
        else
        {
            CvIPL.deallocate( image, IPL_IMAGE_ROI );
            image->roi = 0;
        }
    }

    __END__;
}

void cvAdd( const void* srcarr1, const void* srcarr2, void* dstarr, const void* maskarr )
{
    static CvFuncTable add_tab;
    static int inittab = 0;
    int local_alloc = 1;
    uchar* buffer = 0;

    CV_FUNCNAME( "cvAdd" );

    __BEGIN__;

    int y, dy, type, depth, cn, cont_flag = 0;
    int src1_step, src2_step, dst_step, tdst_step, mask_step;
    CvMat srcstub1, *src1 = (CvMat*)srcarr1;
    CvMat srcstub2, *src2 = (CvMat*)srcarr2;
    CvMat dststub,  *dst = (CvMat*)dstarr;
    CvMat maskstub, *mask = (CvMat*)maskarr;
    CvMat dstbuf, *tdst;
    CvFunc2D_3A func;
    CvFunc2D_3A1I func_sfs;
    CvCopyMaskFunc copym_func;
    CvSize size, tsize;

    if( !CV_IS_MAT(src1) || !CV_IS_MAT(src2) || !CV_IS_MAT(dst))
    {
        if( CV_IS_MATND(src1) || CV_IS_MATND(src2) || CV_IS_MATND(dst))
        {
            CvArr* arrs[] = { src1, src2, dst };
            CvMatND stubs[3];
            CvNArrayIterator iterator;

            if( maskarr )
                CV_ERROR( CV_StsBadMask,
                "This operation on multi-dimensional arrays does not support mask" );

            CV_CALL( cvInitNArrayIterator( 3, arrs, 0, stubs, &iterator,0 ));

            type = iterator.hdr[0]->type;
            iterator.size.width *= CV_MAT_CN(type);

            if( !inittab )
            {
                icvInitAddC1RTable( &add_tab );
                inittab = 1;
            }

            depth = CV_MAT_DEPTH(type);
            if( depth <= CV_16S )
            {
                func_sfs = (CvFunc2D_3A1I)(add_tab.fn_2d[depth]);
                if( !func_sfs )
                    CV_ERROR( CV_StsUnsupportedFormat, "" );

                do
                {
                    IPPI_CALL( func_sfs( iterator.ptr[0], CV_STUB_STEP,
                                         iterator.ptr[1], CV_STUB_STEP,
                                         iterator.ptr[2], CV_STUB_STEP,
                                         iterator.size, 0 ));
                }
                while( cvNextNArraySlice( &iterator ));
            }
            else
            {
                func = (CvFunc2D_3A)(add_tab.fn_2d[depth]);
                if( !func )
                    CV_ERROR( CV_StsUnsupportedFormat, "" );

                do
                {
                    IPPI_CALL( func( iterator.ptr[0], CV_STUB_STEP,
                                     iterator.ptr[1], CV_STUB_STEP,
                                     iterator.ptr[2], CV_STUB_STEP,
                                     iterator.size ));
                }
                while( cvNextNArraySlice( &iterator ));
            }
            EXIT;
        }
        else
        {
            int coi1 = 0, coi2 = 0, coi3 = 0;
            
            CV_CALL( src1 = cvGetMat( src1, &srcstub1, &coi1,0 ));
            CV_CALL( src2 = cvGetMat( src2, &srcstub2, &coi2,0 ));
            CV_CALL( dst = cvGetMat( dst, &dststub, &coi3,0 ));
            if( coi1 + coi2 + coi3 != 0 )
                CV_ERROR( CV_BadCOI, "" );
        }
    }

    if( !CV_ARE_TYPES_EQ( src1, src2 ) || !CV_ARE_TYPES_EQ( src1, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedFormats );

    if( !CV_ARE_SIZES_EQ( src1, src2 ) || !CV_ARE_SIZES_EQ( src1, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedSizes );

    type = CV_MAT_TYPE(src1->type);
    size = cvGetMatSize( src1 );
    depth = CV_MAT_DEPTH(type);
    cn = CV_MAT_CN(type);

    if( !mask )
    {
        if( CV_IS_MAT_CONT( src1->type & src2->type & dst->type ))
        {
            int len = size.width*size.height*cn;

            if( len <= CV_MAX_INLINE_MAT_OP_SIZE*CV_MAX_INLINE_MAT_OP_SIZE )
            {
                if( depth == CV_32F )
                {
                    const float* src1data = (const float*)(src1->data.ptr);
                    const float* src2data = (const float*)(src2->data.ptr);
                    float* dstdata = (float*)(dst->data.ptr);

                    do
                    {
                        dstdata[len-1] = (float)(src1data[len-1] + src2data[len-1]);
                    }
                    while( --len );

                    EXIT;
                }

                if( depth == CV_64F )
                {
                    const double* src1data = (const double*)(src1->data.ptr);
                    const double* src2data = (const double*)(src2->data.ptr);
                    double* dstdata = (double*)(dst->data.ptr);

                    do
                    {
                        dstdata[len-1] = src1data[len-1] + src2data[len-1];
                    }
                    while( --len );

                    EXIT;
                }
            }
            cont_flag = 1;
        }

        dy = size.height;
        copym_func = 0;
        tdst = dst;
    }
    else
    {
        int buf_size, elem_size;
        
        if( !CV_IS_MAT(mask) )
            CV_CALL( mask = cvGetMat( mask, &maskstub,NULL,0 ));

        if( !CV_IS_MASK_ARR(mask))
            CV_ERROR( CV_StsBadMask, "" );

        if( !CV_ARE_SIZES_EQ( mask, dst ))
            CV_ERROR( CV_StsUnmatchedSizes, "" );

        cont_flag = CV_IS_MAT_CONT( src1->type & src2->type & dst->type & mask->type );
        elem_size = CV_ELEM_SIZE(type);

        dy = CV_MAX_LOCAL_SIZE/(elem_size*size.height);
        dy = MAX(dy,1);
        dy = MIN(dy,size.height);
        dstbuf = cvMat( dy, size.width, type,NULL );
        if( !cont_flag )
            dstbuf.step = cvAlign( dstbuf.step, 8 );
        buf_size = dstbuf.step ? dstbuf.step*dy : size.width*elem_size;
        if( buf_size > CV_MAX_LOCAL_SIZE )
        {
            CV_CALL( buffer = (uchar*)cvAlloc( buf_size ));
            local_alloc = 0;
        }
        else
            buffer = (uchar*)cvStackAlloc( buf_size );
        dstbuf.data.ptr = buffer;
        tdst = &dstbuf;
        
        copym_func = icvGetCopyMaskFunc( elem_size );
    }

    if( !inittab )
    {
        icvInitAddC1RTable( &add_tab );
        inittab = 1;
    }

    if( depth <= CV_16S )
    {
        func = 0;
        func_sfs = (CvFunc2D_3A1I)(add_tab.fn_2d[depth]);
        if( !func_sfs )
            CV_ERROR( CV_StsUnsupportedFormat, "" );
    }
    else
    {
        func_sfs = 0;
        func = (CvFunc2D_3A)(add_tab.fn_2d[depth]);
        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );
    }

    src1_step = src1->step;
    src2_step = src2->step;
    dst_step = dst->step;
    tdst_step = tdst->step;
    mask_step = mask ? mask->step : 0;

    for( y = 0; y < size.height; y += dy )
    {
        tsize.width = size.width;
        tsize.height = dy;
        if( y + dy > size.height )
            tsize.height = size.height - y;
        if( cont_flag || tsize.height == 1 )
        {
            tsize.width *= tsize.height;
            tsize.height = 1;
            src1_step = src2_step = tdst_step = dst_step = mask_step = CV_STUB_STEP;
        }

        IPPI_CALL( depth <= CV_16S ?
            func_sfs( src1->data.ptr + y*src1->step, src1_step,
                      src2->data.ptr + y*src2->step, src2_step,
                      tdst->data.ptr, tdst_step,
                      cvSize( tsize.width*cn, tsize.height ), 0 ) :
            func( src1->data.ptr + y*src1->step, src1_step,
                  src2->data.ptr + y*src2->step, src2_step,
                  tdst->data.ptr, tdst_step,
                  cvSize( tsize.width*cn, tsize.height )));

        if( mask )
        {
            IPPI_CALL( copym_func( tdst->data.ptr, tdst_step, dst->data.ptr + y*dst->step,
                                   dst_step, tsize, mask->data.ptr + y*mask->step, mask_step ));
        }
    }

    __END__;

    if( !local_alloc )
        cvFree( &buffer );
}

#define ICV_DEF_WARP_PERSPECTIVE_BILINEAR_FUNC( flavor, arrtype, load_macro, cast_macro )\
static CvStatus CV_STDCALL                                                  \
icvWarpPerspective_Bilinear_##flavor##_CnR(                                 \
    const arrtype* src, int step, CvSize ssize,                             \
    arrtype* dst, int dststep, CvSize dsize,                                \
    const double* matrix, int cn,                                           \
    const arrtype* fillval )                                                \
{                                                                           \
    int x, y, k;                                                            \
    float A11 = (float)matrix[0], A12 = (float)matrix[1], A13 = (float)matrix[2];\
    float A21 = (float)matrix[3], A22 = (float)matrix[4], A23 = (float)matrix[5];\
    float A31 = (float)matrix[6], A32 = (float)matrix[7], A33 = (float)matrix[8];\
                                                                            \
    step /= sizeof(src[0]);                                                 \
    dststep /= sizeof(dst[0]);                                              \
                                                                            \
    for( y = 0; y < dsize.height; y++, dst += dststep )                     \
    {                                                                       \
        float xs0 = A12*y + A13;                                            \
        float ys0 = A22*y + A23;                                            \
        float ws = A32*y + A33;                                             \
                                                                            \
        for( x = 0; x < dsize.width; x++, xs0 += A11, ys0 += A21, ws += A31 )\
        {                                                                   \
            float inv_ws = 1.f/ws;                                          \
            float xs = xs0*inv_ws;                                          \
            float ys = ys0*inv_ws;                                          \
            int ixs = floor(xs);                                          \
            int iys = floor(ys);                                          \
            float a = xs - ixs;                                             \
            float b = ys - iys;                                             \
            float p0, p1;                                                   \
                                                                            \
            if( (unsigned)ixs < (unsigned)(ssize.width - 1) &&              \
                (unsigned)iys < (unsigned)(ssize.height - 1) )              \
            {                                                               \
                const arrtype* ptr = src + step*iys + ixs*cn;               \
                                                                            \
                for( k = 0; k < cn; k++ )                                   \
                {                                                           \
                    p0 = load_macro(ptr[k]) +                               \
                        a * (load_macro(ptr[k+cn]) - load_macro(ptr[k]));   \
                    p1 = load_macro(ptr[k+step]) +                          \
                        a * (load_macro(ptr[k+cn+step]) -                   \
                             load_macro(ptr[k+step]));                      \
                    dst[x*cn+k] = (arrtype)cast_macro(p0 + b*(p1 - p0));    \
                }                                                           \
            }                                                               \
            else if( (unsigned)(ixs+1) < (unsigned)(ssize.width+1) &&       \
                     (unsigned)(iys+1) < (unsigned)(ssize.height+1))        \
            {                                                               \
                int x0 = ICV_WARP_CLIP_X( ixs );                            \
                int y0 = ICV_WARP_CLIP_Y( iys );                            \
                int x1 = ICV_WARP_CLIP_X( ixs + 1 );                        \
                int y1 = ICV_WARP_CLIP_Y( iys + 1 );                        \
                const arrtype* ptr0, *ptr1, *ptr2, *ptr3;                   \
                                                                            \
                ptr0 = src + y0*step + x0*cn;                               \
                ptr1 = src + y0*step + x1*cn;                               \
                ptr2 = src + y1*step + x0*cn;                               \
                ptr3 = src + y1*step + x1*cn;                               \
                                                                            \
                for( k = 0; k < cn; k++ )                                   \
                {                                                           \
                    p0 = load_macro(ptr0[k]) +                              \
                        a * (load_macro(ptr1[k]) - load_macro(ptr0[k]));    \
                    p1 = load_macro(ptr2[k]) +                              \
                        a * (load_macro(ptr3[k]) - load_macro(ptr2[k]));    \
                    dst[x*cn+k] = (arrtype)cast_macro(p0 + b*(p1 - p0));    \
                }                                                           \
            }                                                               \
            else if( fillval )                                              \
                for( k = 0; k < cn; k++ )                                   \
                    dst[x*cn+k] = fillval[k];                               \
        }                                                                   \
    }                                                                       \
                                                                            \
    return CV_OK;                                                           \
}


#define ICV_WARP_SCALE_ALPHA(x) ((x)*(1./(ICV_WARP_MASK+1)))

ICV_DEF_WARP_PERSPECTIVE_BILINEAR_FUNC( 8u, uchar, CV_8TO32F, cvRound )
ICV_DEF_WARP_PERSPECTIVE_BILINEAR_FUNC( 16u, ushort, CV_NOP, cvRound )
ICV_DEF_WARP_PERSPECTIVE_BILINEAR_FUNC( 32f, float, CV_NOP, CV_NOP )

static void icvInitWarpPerspectiveTab( CvFuncTable* bilin_tab )
{
    bilin_tab->fn_2d[CV_8U] = (void*)icvWarpPerspective_Bilinear_8u_CnR;
    bilin_tab->fn_2d[CV_16U] = (void*)icvWarpPerspective_Bilinear_16u_CnR;
    bilin_tab->fn_2d[CV_32F] = (void*)icvWarpPerspective_Bilinear_32f_CnR;
}

icvWarpPerspectiveBack_8u_C1R_t icvWarpPerspectiveBack_8u_C1R_p = 0;
icvWarpPerspectiveBack_8u_C3R_t icvWarpPerspectiveBack_8u_C3R_p = 0;
icvWarpPerspectiveBack_8u_C4R_t icvWarpPerspectiveBack_8u_C4R_p = 0;
icvWarpPerspectiveBack_32f_C1R_t icvWarpPerspectiveBack_32f_C1R_p = 0;
icvWarpPerspectiveBack_32f_C3R_t icvWarpPerspectiveBack_32f_C3R_p = 0;
icvWarpPerspectiveBack_32f_C4R_t icvWarpPerspectiveBack_32f_C4R_p = 0;

icvWarpPerspective_8u_C1R_t icvWarpPerspective_8u_C1R_p = 0;
icvWarpPerspective_8u_C3R_t icvWarpPerspective_8u_C3R_p = 0;
icvWarpPerspective_8u_C4R_t icvWarpPerspective_8u_C4R_p = 0;
icvWarpPerspective_32f_C1R_t icvWarpPerspective_32f_C1R_p = 0;
icvWarpPerspective_32f_C3R_t icvWarpPerspective_32f_C3R_p = 0;
icvWarpPerspective_32f_C4R_t icvWarpPerspective_32f_C4R_p = 0;

void cvWarpPerspective( const CvArr* srcarr, CvArr* dstarr, const CvMat* matrix, int flags, CvScalar fillval )
{
    static CvFuncTable bilin_tab;
    static int inittab = 0;

    CV_FUNCNAME( "cvWarpPerspective" );

    __BEGIN__;
    
    CvMat srcstub, *src = (CvMat*)srcarr;
    CvMat dststub, *dst = (CvMat*)dstarr;
    int type, depth, cn;
    int method = flags & 3;
    double src_matrix[9], dst_matrix[9];
    double fillbuf[4];
    CvMat A = cvMat( 3, 3, CV_64F, src_matrix ),
          invA = cvMat( 3, 3, CV_64F, dst_matrix );
    CvWarpPerspectiveFunc func;
    CvSize ssize, dsize;

    if( method == CV_INTER_NN || method == CV_INTER_AREA )
        method = CV_INTER_LINEAR;
    
    if( !inittab )
    {
        icvInitWarpPerspectiveTab( &bilin_tab );
        inittab = 1;
    }

    CV_CALL( src = cvGetMat( srcarr, &srcstub,NULL,0 ));
    CV_CALL( dst = cvGetMat( dstarr, &dststub,NULL,0 ));
    
    if( !CV_ARE_TYPES_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    if( !CV_IS_MAT(matrix) || CV_MAT_CN(matrix->type) != 1 ||
        CV_MAT_DEPTH(matrix->type) < CV_32F || matrix->rows != 3 || matrix->cols != 3 )
        CV_ERROR( CV_StsBadArg,
        "Transformation matrix should be 3x3 floating-point single-channel matrix" );

    if( flags & CV_WARP_INVERSE_MAP )
        cvConvertScale( matrix, &invA,1,0 );
    else
    {
        cvConvertScale( matrix, &A,1,0 );
        cvInvert( &A, &invA, CV_SVD );
    }

    type = CV_MAT_TYPE(src->type);
    depth = CV_MAT_DEPTH(type);
    cn = CV_MAT_CN(type);
    if( cn > 4 )
        CV_ERROR( CV_BadNumChannels, "" );
    
    ssize = cvGetMatSize(src);
    dsize = cvGetMatSize(dst);
    
    if( icvWarpPerspectiveBack_8u_C1R_p )
    {
        CvWarpPerspectiveBackIPPFunc ipp_func =
            type == CV_8UC1 ? icvWarpPerspectiveBack_8u_C1R_p :
            type == CV_8UC3 ? icvWarpPerspectiveBack_8u_C3R_p :
            type == CV_8UC4 ? icvWarpPerspectiveBack_8u_C4R_p :
            type == CV_32FC1 ? icvWarpPerspectiveBack_32f_C1R_p :
            type == CV_32FC3 ? icvWarpPerspectiveBack_32f_C3R_p :
            type == CV_32FC4 ? icvWarpPerspectiveBack_32f_C4R_p : 0;
        
        if( ipp_func && CV_INTER_NN <= method && method <= CV_INTER_AREA &&
            MIN(ssize.width,ssize.height) >= 4 && MIN(dsize.width,dsize.height) >= 4 )
        {
            int srcstep = src->step ? src->step : CV_STUB_STEP;
            int dststep = dst->step ? dst->step : CV_STUB_STEP;
            CvStatus status;
            CvRect srcroi = {0, 0, ssize.width, ssize.height};
            CvRect dstroi = {0, 0, dsize.width, dsize.height};

            // this is not the most efficient way to fill outliers
            if( flags & CV_WARP_FILL_OUTLIERS )
                cvSet( dst, fillval,NULL );

            status = ipp_func( src->data.ptr, ssize, srcstep, srcroi,
                               dst->data.ptr, dststep, dstroi,
                               invA.data.db, 1 << method );
            if( status >= 0 )
                EXIT;

            ipp_func = type == CV_8UC1 ? icvWarpPerspective_8u_C1R_p :
                type == CV_8UC3 ? icvWarpPerspective_8u_C3R_p :
                type == CV_8UC4 ? icvWarpPerspective_8u_C4R_p :
                type == CV_32FC1 ? icvWarpPerspective_32f_C1R_p :
                type == CV_32FC3 ? icvWarpPerspective_32f_C3R_p :
                type == CV_32FC4 ? icvWarpPerspective_32f_C4R_p : 0;

            if( ipp_func )
            {
                if( flags & CV_WARP_INVERSE_MAP )
                    cvInvert( &invA, &A, CV_SVD );

                status = ipp_func( src->data.ptr, ssize, srcstep, srcroi,
                               dst->data.ptr, dststep, dstroi,
                               A.data.db, 1 << method );
                if( status >= 0 )
                    EXIT;
            }
        }
    }

    cvScalarToRawData( &fillval, fillbuf, CV_MAT_TYPE(src->type), 0 );

    /*if( method == CV_INTER_LINEAR )*/
    {
        func = (CvWarpPerspectiveFunc)bilin_tab.fn_2d[depth];
        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        IPPI_CALL( func( src->data.ptr, src->step, ssize, dst->data.ptr,
                         dst->step, dsize, dst_matrix, cn,
                         flags & CV_WARP_FILL_OUTLIERS ? fillbuf : 0 ));
    }

    __END__;
}


















