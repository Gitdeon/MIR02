#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "alloc.h"
#include "global.h"
#include "error.h"
static void* icvDefaultAlloc( size_t size, void* )
{
    char *ptr, *ptr0 = (char*)malloc(
        (size_t)(size + CV_MALLOC_ALIGN*((size >= 4096) + 1) + sizeof(char*)));

    if( !ptr0 )
        return 0;

    // align the pointer
    ptr = (char*)cvAlignPtr(ptr0 + sizeof(char*) + 1, CV_MALLOC_ALIGN);
    *(char**)(ptr - sizeof(char*)) = ptr0;

    return ptr;
}


// default <free>
static int icvDefaultFree( void* ptr, void* )
{
    // Pointer must be aligned by CV_MALLOC_ALIGN
    if( ((size_t)ptr & (CV_MALLOC_ALIGN-1)) != 0 )
        return CV_BADARG_ERR;
    free( *((char**)ptr - 1) );

    return CV_OK;
}


// pointers to allocation functions, initially set to default
static CvAllocFunc p_cvAlloc = icvDefaultAlloc;
static CvFreeFunc p_cvFree = icvDefaultFree;
static void* p_cvAllocUserData = 0;

void cvSetMemoryManager( CvAllocFunc alloc_func, CvFreeFunc free_func, void* userdata )
{
    CV_FUNCNAME( "cvSetMemoryManager" );

    __BEGIN__;
    
    if( (alloc_func == 0) ^ (free_func == 0) )
        CV_ERROR( CV_StsNullPtr, "Either both pointers should be NULL or none of them");

    p_cvAlloc = alloc_func ? alloc_func : icvDefaultAlloc;
    p_cvFree = free_func ? free_func : icvDefaultFree;
    p_cvAllocUserData = userdata;

    __END__;
}


void*  cvAlloc( size_t size )
{
    void* ptr = 0;
    
    CV_FUNCNAME( "cvAlloc" );

    __BEGIN__;

    if( (size_t)size > CV_MAX_ALLOC_SIZE )
        CV_ERROR( CV_StsOutOfRange,
                  "Negative or too large argument of cvAlloc function" );

    ptr = p_cvAlloc( size, p_cvAllocUserData );
    if( !ptr )
        CV_ERROR( CV_StsNoMem, "Out of memory" );

    __END__;

    return ptr;
}


void  cvFree_( void* ptr )
{
    CV_FUNCNAME( "cvFree_" );

    __BEGIN__;

    if( ptr )
    {
        CVStatus status = p_cvFree( ptr, p_cvAllocUserData );
        if( status < 0 )
            CV_ERROR( status, "Deallocation error" );
    }

    __END__;
}

/* End of file. */
int cvErrorFromIppStatus( int status )
{
    switch (status)
    {
    case CV_BADSIZE_ERR: return CV_StsBadSize;
    case CV_BADMEMBLOCK_ERR: return CV_StsBadMemBlock;
    case CV_NULLPTR_ERR: return CV_StsNullPtr;
    case CV_DIV_BY_ZERO_ERR: return CV_StsDivByZero;
    case CV_BADSTEP_ERR: return CV_BadStep ;
    case CV_OUTOFMEM_ERR: return CV_StsNoMem;
    case CV_BADARG_ERR: return CV_StsBadArg;
    case CV_NOTDEFINED_ERR: return CV_StsError;
    case CV_INPLACE_NOT_SUPPORTED_ERR: return CV_StsInplaceNotSupported;
    case CV_NOTFOUND_ERR: return CV_StsObjectNotFound;
    case CV_BADCONVERGENCE_ERR: return CV_StsNoConv;
    case CV_BADDEPTH_ERR: return CV_BadDepth;
    case CV_UNMATCHED_FORMATS_ERR: return CV_StsUnmatchedFormats;
    case CV_UNSUPPORTED_COI_ERR: return CV_BadCOI;
    case CV_UNSUPPORTED_CHANNELS_ERR: return CV_BadNumChannels;
    case CV_BADFLAG_ERR: return CV_StsBadFlag;
    case CV_BADRANGE_ERR: return CV_StsBadArg;
    case CV_BADCOEF_ERR: return CV_StsBadArg;
    case CV_BADFACTOR_ERR: return CV_StsBadArg;
    case CV_BADPOINT_ERR: return CV_StsBadPoint;

    default: return CV_StsError;
    }
}
