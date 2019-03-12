#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "error.h"



#if defined WIN32 || defined WIN64
#define CV_DEFAULT_ERROR_CALLBACK  cvGuiBoxReport
#else
#define CV_DEFAULT_ERROR_CALLBACK  cvStdErrReport
#endif

#define CV_ErrModeLeaf     0   /* Print error and exit program */
#define CV_ErrModeParent   1   /* Print error and continue */
#define CV_ErrModeSilent   2   /* Don't print and continue */

static CvContext* icvCreateContext(void)
{
    CvContext* context = (CvContext*)malloc( sizeof(*context) );

    context->err_mode = CV_ErrModeLeaf;
    context->err_code = CV_StsOk;

    context->error_callback = CV_DEFAULT_ERROR_CALLBACK;
    context->userdata = 0;

    return context;
}

static void icvDestroyContext(CvContext* context)
{
    free(context);
}


static CvContext* icvGetContext(void)
{
#ifdef CV_DLL
#if defined WIN32 || defined WIN64
    CvContext* context;

    //assert(g_TlsIndex != TLS_OUT_OF_INDEXES);
    if( g_TlsIndex == TLS_OUT_OF_INDEXES )
    {
        g_TlsIndex = TlsAlloc();
        if( g_TlsIndex == TLS_OUT_OF_INDEXES )
            FatalAppExit( 0, "Only set CV_DLL for DLL usage" );
    }

    context = (CvContext*)TlsGetValue( g_TlsIndex );
    if( !context )
    {
        context = icvCreateContext();
        if( !context )
            FatalAppExit( 0, "OpenCV. Problem to allocate memory for TLS OpenCV context." );

        TlsSetValue( g_TlsIndex, context );
    }
    return context;
#else
    CvContext* context = (CvContext*)pthread_getspecific( g_TlsIndex );
    if( !context )
    {
    context = icvCreateContext();
    if( !context )
    {
            fprintf(stderr,"OpenCV. Problem to allocate memory for OpenCV context.");
        exit(1);
    }
    pthread_setspecific( g_TlsIndex, context );
    }
    return context;
#endif
#else /* static single-thread library case */
    static CvContext* context = 0;

    if( !context )
        context = icvCreateContext();

    return context;
#endif
}


int cvStdErrReport( int code, const char *func_name, const char *err_msg,const char *file, int line, void* )
{
    if( code == CV_StsBackTrace || code == CV_StsAutoTrace )
        fprintf( stderr, "\tcalled from " );
    else
        fprintf( stderr, "OpenCV ERROR: %s (%s)\n\tin function ",
                 cvErrorStr(code), err_msg ? err_msg : "no description" );

    fprintf( stderr, "%s, %s(%d)\n", func_name ? func_name : "<unknown>",
             file != NULL ? file : "", line );

    if( cvGetErrMode() == CV_ErrModeLeaf )
    {
        fprintf( stderr, "Terminating the application...\n" );
        return 1;
    }
    else
        return 0;
}


int cvGuiBoxReport( int code, const char *func_name, const char *err_msg,const char *file, int line, void* )
{
#if !defined WIN32 && !defined WIN64
    return cvStdErrReport( code, func_name, err_msg, file, line, 0 );
#else
    if( code != CV_StsBackTrace && code != CV_StsAutoTrace )
    {
        size_t msg_len = strlen(err_msg ? err_msg : "") + 1024;
        char* message = (char*)alloca(msg_len);
        char title[100];

        wsprintf( message, "%s (%s)\nin function %s, %s(%d)\n\n"
                  "Press \"Abort\" to terminate application.\n"
                  "Press \"Retry\" to debug (if the app is running under debugger).\n"
                  "Press \"Ignore\" to continue (this is not safe).\n",
                  cvErrorStr(code), err_msg ? err_msg : "no description",
                  func_name, file, line );

        wsprintf( title, "OpenCV GUI Error Handler" );

        int answer = MessageBox( NULL, message, title, MB_ICONERROR|MB_ABORTRETRYIGNORE|MB_SYSTEMMODAL );

        if( answer == IDRETRY )
        {
            CV_DBG_BREAK();
        }
        return answer != IDIGNORE;
    }
    return 0;
#endif
}


int cvNulDevReport( int /*code*/, const char* /*func_name*/,const char* /*err_msg*/, const char* /*file*/, int /*line*/, void* )
{
    return cvGetErrMode() == CV_ErrModeLeaf;
}


CvErrorCallback cvRedirectError( CvErrorCallback func, void* userdata, void** prev_userdata )
{
    CvContext* context = icvGetContext();

    CvErrorCallback old = context->error_callback;
    if( prev_userdata )
        *prev_userdata = context->userdata;
    if( func )
    {
        context->error_callback = func;
        context->userdata = userdata;
    }
    else
    {
        context->error_callback = CV_DEFAULT_ERROR_CALLBACK;
        context->userdata = 0;
    }

    return old;
}


int cvGetErrInfo( const char** errorcode_desc, const char** description,const char** filename, int* line )
{
    int code = cvGetErrStatus();

    if( errorcode_desc )
        *errorcode_desc = cvErrorStr( code );

    if( code >= 0 )
    {
        if( description )
            *description = 0;
        if( filename )
            *filename = 0;
        if( line )
            *line = 0;
    }
    else
    {
        CvContext* ctx = icvGetContext();

        if( description )
            *description = ctx->err_msg;
        if( filename )
            *filename = ctx->err_ctx.file;
        if( line )
            *line = ctx->err_ctx.line;
    }

    return code;
}


const char* cvErrorStr( int status )
{
    static char buf[256];

    switch (status)
    {
    case CV_StsOk :        return "No Error";
    case CV_StsBackTrace : return "Backtrace";
    case CV_StsError :     return "Unspecified error";
    case CV_StsInternal :  return "Internal error";
    case CV_StsNoMem :     return "Insufficient memory";
    case CV_StsBadArg :    return "Bad argument";
    case CV_StsNoConv :    return "Iterations do not converge";
    case CV_StsAutoTrace : return "Autotrace call";
    case CV_StsBadSize :   return "Incorrect size of input array";
    case CV_StsNullPtr :   return "Null pointer";
    case CV_StsDivByZero : return "Divizion by zero occured";
    case CV_BadStep :      return "Image step is wrong";
    case CV_StsInplaceNotSupported : return "Inplace operation is not supported";
    case CV_StsObjectNotFound :      return "Requested object was not found";
    case CV_BadDepth :     return "Input image depth is not supported by function";
    case CV_StsUnmatchedFormats : return "Formats of input arguments do not match";
    case CV_StsUnmatchedSizes :  return "Sizes of input arguments do not match";
    case CV_StsOutOfRange : return "One of arguments\' values is out of range";
    case CV_StsUnsupportedFormat : return "Unsupported format or combination of formats";
    case CV_BadCOI :      return "Input COI is not supported";
    case CV_BadNumChannels : return "Bad number of channels";
    case CV_StsBadFlag :   return "Bad flag (parameter or structure field)";
    case CV_StsBadPoint :  return "Bad parameter of type CvPoint";
    case CV_StsBadMask : return "Bad type of mask argument";
    case CV_StsParseError : return "Parsing error";
    case CV_StsNotImplemented : return "The function/feature is not implemented";
    case CV_StsBadMemBlock :  return "Memory block has been corrupted";
    };

    sprintf(buf, "Unknown %s code %d", status >= 0 ? "status":"error", status);
    return buf;
}

int cvGetErrMode(void)
{
    return icvGetContext()->err_mode;
}

int cvSetErrMode( int mode )
{
    CvContext* context = icvGetContext();
    int prev_mode = context->err_mode;
    context->err_mode = mode;
    return prev_mode;
}

int cvGetErrStatus()
{
    return icvGetContext()->err_code;
}

void cvSetErrStatus( int code )
{
    icvGetContext()->err_code = code;
}


void cvError( int code, const char* func_name, const char* err_msg, const char* file_name, int line )
{
    if( code == CV_StsOk )
        cvSetErrStatus( code );
    else
    {
        CvContext* context = icvGetContext();

        if( code != CV_StsBackTrace && code != CV_StsAutoTrace )
        {
            char* message = context->err_msg;
            context->err_code = code;

            strcpy( message, err_msg );
            context->err_ctx.file = file_name;
            context->err_ctx.line = line;
        }

        if( context->err_mode != CV_ErrModeSilent )
        {
            int terminate = context->error_callback( code, func_name, err_msg,
                                                    file_name, line, context->userdata );
            if( terminate )
            {
#if !defined WIN32 && !defined WIN64
                assert(0); // for post-mortem analysis with GDB
#endif
                exit(-abs(terminate));
            }
        }
    }
}


