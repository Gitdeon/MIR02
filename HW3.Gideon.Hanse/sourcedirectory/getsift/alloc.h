#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "global.h"


#ifndef ALLOC_H
#define ALLOC_H

typedef void* (CV_CDECL *CvAllocFunc)(size_t size, void* userdata);
typedef int (CV_CDECL *CvFreeFunc)(void* pptr, void* userdata);

#define  CV_MALLOC_ALIGN    32
#define  CV_MAX_ALLOC_SIZE    (((size_t)1 << (sizeof(size_t)*8-2)))
#define  CV_DEFAULT_IMAGE_ROW_ALIGN  4

typedef enum CvStatus
{         
    CV_BADMEMBLOCK_ERR          = -113,
    CV_INPLACE_NOT_SUPPORTED_ERR= -112,
    CV_UNMATCHED_ROI_ERR        = -111,
    CV_NOTFOUND_ERR             = -110,
    CV_BADCONVERGENCE_ERR       = -109,

    CV_BADDEPTH_ERR             = -107,
    CV_BADROI_ERR               = -106,
    CV_BADHEADER_ERR            = -105,
    CV_UNMATCHED_FORMATS_ERR    = -104,
    CV_UNSUPPORTED_COI_ERR      = -103,
    CV_UNSUPPORTED_CHANNELS_ERR = -102,
    CV_UNSUPPORTED_DEPTH_ERR    = -101,
    CV_UNSUPPORTED_FORMAT_ERR   = -100,

    CV_BADARG_ERR      = -49,  //ipp comp
    CV_NOTDEFINED_ERR  = -48,  //ipp comp

    CV_BADCHANNELS_ERR = -47,  //ipp comp
    CV_BADRANGE_ERR    = -44,  //ipp comp
    CV_BADSTEP_ERR     = -29,  //ipp comp

    CV_BADFLAG_ERR     =  -12,
    CV_DIV_BY_ZERO_ERR =  -11, //ipp comp
    CV_BADCOEF_ERR     =  -10,

    CV_BADFACTOR_ERR   =  -7,
    CV_BADPOINT_ERR    =  -6,
    CV_BADSCALE_ERR    =  -4,
    CV_OUTOFMEM_ERR    =  -3,
    CV_NULLPTR_ERR     =  -2,
    CV_BADSIZE_ERR     =  -1,
    CV_NO_ERR          =   0,
    CV_OK              =   CV_NO_ERR
}
CvStatus;


#define CV_ERROR_FROM_STATUS( result )                \
    CV_ERROR( cvErrorFromIppStatus( result ), "OpenCV function failed" )

#define IPPI_CALL( Func )                                              \
{                                                                      \
      CvStatus  ippi_call_result;                                      \
      ippi_call_result = Func;                                         \
                                                                       \
      if( ippi_call_result < 0 )                                       \
            CV_ERROR_FROM_STATUS( (ippi_call_result));                 \
}

#define  CV_ORIGIN_TL  0
#define  CV_ORIGIN_BL  1

inline void* cvAlignPtr( const void* ptr, int align=32 )
{
    assert( (align & (align-1)) == 0 );
    return (void*)( ((size_t)ptr + align - 1) & ~(size_t)(align-1) );
}

CvStatus CV_STDCALL icvSetZero_8u_C1R( uchar* dst, int dststep, CvSize size );


IPCVAPI_EX( CvStatus, icvCopy_8u_C1R, "ippiCopy_8u_C1R", 0/*CV_PLUGINS1(CV_PLUGIN_IPPI)*/,
                  ( const uchar* src, int src_step,
                    uchar* dst, int dst_step, CvSize size ))

typedef CvStatus (CV_STDCALL * CvCopyMaskFunc)(const void* src, int src_step,void* dst, int dst_step, CvSize size,const void* mask, int mask_step);
CvCopyMaskFunc icvGetCopyMaskFunc( int elem_size );

CvStatus CV_STDCALL icvLUT_Transform8u_8u_C1R( const uchar* src, int srcstep, uchar* dst,
                                               int dststep, CvSize size, const uchar* lut );

CvStatus CV_STDCALL icvLUT_Transform8u_8u_C1R( const uchar* src, int srcstep, uchar* dst,
                                               int dststep, CvSize size, const uchar* lut );
CvStatus CV_STDCALL icvLUT_Transform8u_16u_C1R( const uchar* src, int srcstep, ushort* dst,
                                                int dststep, CvSize size, const ushort* lut );
CvStatus CV_STDCALL icvLUT_Transform8u_32s_C1R( const uchar* src, int srcstep, int* dst,
                                                int dststep, CvSize size, const int* lut );
CvStatus CV_STDCALL icvLUT_Transform8u_64f_C1R( const uchar* src, int srcstep, double* dst,
                                                int dststep, CvSize size, const double* lut );

CvStatus CV_STDCALL icvLUT_Transform8u_8u_C2R( const uchar* src, int srcstep, uchar* dst,
                                               int dststep, CvSize size, const uchar* lut );
CvStatus CV_STDCALL icvLUT_Transform8u_8u_C3R( const uchar* src, int srcstep, uchar* dst,
                                               int dststep, CvSize size, const uchar* lut );
CvStatus CV_STDCALL icvLUT_Transform8u_8u_C4R( const uchar* src, int srcstep, uchar* dst,
                                               int dststep, CvSize size, const uchar* lut );

typedef CvStatus (CV_STDCALL * CvLUT_TransformFunc)( const void* src, int srcstep, void* dst,
                                                     int dststep, CvSize size, const void* lut );

inline CvStatus
icvLUT_Transform8u_8s_C1R( const uchar* src, int srcstep, char* dst,
                            int dststep, CvSize size, const char* lut )
{
    return icvLUT_Transform8u_8u_C1R( src, srcstep, (uchar*)dst,
                                      dststep, size, (const uchar*)lut );
}

inline CvStatus
icvLUT_Transform8u_16s_C1R( const uchar* src, int srcstep, short* dst,
                            int dststep, CvSize size, const short* lut )
{
    return icvLUT_Transform8u_16u_C1R( src, srcstep, (ushort*)dst,
                                       dststep, size, (const ushort*)lut );
}
inline CvStatus
icvLUT_Transform8u_32f_C1R( const uchar* src, int srcstep, float* dst,
                            int dststep, CvSize size, const float* lut )
{
    return icvLUT_Transform8u_32s_C1R( src, srcstep, (int*)dst,
                                       dststep, size, (const int*)lut );
}

typedef  CvStatus (CV_STDCALL * CvLUDecompFunc)( double* A, int stepA, CvSize sizeA,
                                                 void* B, int stepB, CvSize sizeB,
                                                 double* det );

typedef  CvStatus (CV_STDCALL * CvLUBackFunc)( double* A, int stepA, CvSize sizeA,
                                               void* B, int stepB, CvSize sizeB );

typedef CvStatus (CV_STDCALL *CvGEMMSingleMulFunc)( const void* src1, size_t step1,
                   const void* src2, size_t step2, const void* src3, size_t step3,
                   void* dst, size_t dststep, CvSize srcsize, CvSize dstsize,
                   double alpha, double beta, int flags );

typedef CvStatus (CV_STDCALL *CvGEMMBlockMulFunc)( const void* src1, size_t step1,
                   const void* src2, size_t step2, void* dst, size_t dststep,
                   CvSize srcsize, CvSize dstsize, int flags );

typedef CvStatus (CV_STDCALL *CvGEMMStoreFunc)( const void* src1, size_t step1,
                   const void* src2, size_t step2, void* dst, size_t dststep,
                   CvSize dstsize, double alpha, double beta, int flags );

#define IPCV_RESIZE( flavor, cn )                                           \
IPCVAPI_EX( CvStatus, icvResize_##flavor##_C##cn##R,                        \
            "ippiResize_" #flavor "_C" #cn "R", CV_PLUGINS1(CV_PLUGIN_IPPI),\
           (const void* src, CvSize srcsize, int srcstep, CvRect srcroi,    \
            void* dst, int dststep, CvSize dstroi,                          \
            double xfactor, double yfactor, int interpolation ))

IPCV_RESIZE( 8u, 1 )
IPCV_RESIZE( 8u, 3 )
IPCV_RESIZE( 8u, 4 )

IPCV_RESIZE( 16u, 1 )
IPCV_RESIZE( 16u, 3 )
IPCV_RESIZE( 16u, 4 )

IPCV_RESIZE( 32f, 1 )
IPCV_RESIZE( 32f, 3 )
IPCV_RESIZE( 32f, 4 )

typedef CvStatus (CV_STDCALL * CvResizeBilinearFunc)
                    ( const void* src, int srcstep, CvSize ssize,
                      void* dst, int dststep, CvSize dsize,
                      int cn, int xmax, const CvResizeAlpha* xofs,
                      const CvResizeAlpha* yofs, float* buf0, float* buf1 );

typedef CvStatus (CV_STDCALL * CvResizeBicubicFunc)
                    ( const void* src, int srcstep, CvSize ssize,
                      void* dst, int dststep, CvSize dsize,
                      int cn, int xmin, int xmax,
                      const CvResizeAlpha* xofs, float** buf );

typedef CvStatus (CV_STDCALL * CvResizeAreaFastFunc)
                    ( const void* src, int srcstep, CvSize ssize,
                      void* dst, int dststep, CvSize dsize,
                      int cn, const int* ofs, const int *xofs );
typedef CvStatus (CV_STDCALL * CvResizeAreaFunc)
                    ( const void* src, int srcstep, CvSize ssize,
                      void* dst, int dststep, CvSize dsize,
                      int cn, const CvDecimateAlpha* xofs,
                      int xofs_count, float* buf, float* sum );

IPCVAPI_EX( CvStatus, icvSetByte_8u_C1R, "ippiSet_8u_C1R", 0/*CV_PLUGINS1(CV_PLUGIN_IPPI)*/,
                  ( uchar value, uchar* dst, int dst_step, CvSize size ))

typedef CvStatus (CV_STDCALL *CvFunc2D_1A)(void* arr, int step, CvSize size);
typedef CvStatus (CV_STDCALL *CvFunc2D_2A)( void* arr0, int step0,
                                            void* arr1, int step1, CvSize size );
typedef CvStatus (CV_STDCALL *CvFunc2D_2A1P)( void* arr0, int step0,
                                              void* arr1, int step1,
                                              CvSize size, void* param );

#define CV_PLUGINS1(lib1) ((lib1)&15)
#define CV_PLUGIN_IPPI      3 /* IPP: image processing */

#define IPCV_COPYSET( flavor, arrtype, scalartype )                                 \
IPCVAPI_EX( CvStatus, icvCopy##flavor, "ippiCopy" #flavor,                          \
                                    CV_PLUGINS1(CV_PLUGIN_IPPI),                    \
                                   ( const arrtype* src, int srcstep,               \
                                     arrtype* dst, int dststep, CvSize size,        \
                                     const uchar* mask, int maskstep ))             \
IPCVAPI_EX( CvStatus, icvSet##flavor, "ippiSet" #flavor,                            \
                                    0/*CV_PLUGINS1(CV_PLUGIN_OPTCV)*/,              \
                                  ( arrtype* dst, int dststep,                      \
                                    const uchar* mask, int maskstep,                \
                                    CvSize size, const arrtype* scalar ))

#define IPCV_PIX_PLANE( flavor, arrtype )                                           \
IPCVAPI_EX( CvStatus, icvCopy_##flavor##_C2P2R,                                     \
    "ippiCopy_" #flavor "_C2P2R", 0/*CV_PLUGINS1(CV_PLUGIN_IPPI)*/,                 \
    ( const arrtype* src, int srcstep, arrtype** dst, int dststep, CvSize size ))   \
IPCVAPI_EX( CvStatus, icvCopy_##flavor##_C3P3R,                                     \
    "ippiCopy_" #flavor "_C3P3R", CV_PLUGINS1(CV_PLUGIN_IPPI),                      \
    ( const arrtype* src, int srcstep, arrtype** dst, int dststep, CvSize size ))   \
IPCVAPI_EX( CvStatus, icvCopy_##flavor##_C4P4R,                                     \
    "ippiCopy_" #flavor "_C4P4R", CV_PLUGINS1(CV_PLUGIN_IPPI),                      \
    ( const arrtype* src, int srcstep, arrtype** dst, int dststep, CvSize size ))   \
IPCVAPI_EX( CvStatus, icvCopy_##flavor##_CnC1CR,                                    \
    "ippiCopy_" #flavor "_CnC1CR", 0/*CV_PLUGINS1(CV_PLUGIN_OPTCV)*/,               \
    ( const arrtype* src, int srcstep, arrtype* dst, int dststep,                   \
      CvSize size, int cn, int coi ))                                               \
IPCVAPI_EX( CvStatus, icvCopy_##flavor##_C1CnCR,                                    \
    "ippiCopy_" #flavor "_CnC1CR", 0/*CV_PLUGINS1(CV_PLUGIN_OPTCV)*/,               \
    ( const arrtype* src, int srcstep, arrtype* dst, int dststep,                   \
      CvSize size, int cn, int coi ))                                               \
IPCVAPI_EX( CvStatus, icvCopy_##flavor##_P2C2R,                                     \
    "ippiCopy_" #flavor "_P2C2R", 0/*CV_PLUGINS1(CV_PLUGIN_IPPI)*/,                 \
    ( const arrtype** src, int srcstep, arrtype* dst, int dststep, CvSize size ))   \
IPCVAPI_EX( CvStatus, icvCopy_##flavor##_P3C3R,                                     \
    "ippiCopy_" #flavor "_P3C3R", CV_PLUGINS1(CV_PLUGIN_IPPI),                      \
    ( const arrtype** src, int srcstep, arrtype* dst, int dststep, CvSize size ))   \
IPCVAPI_EX( CvStatus, icvCopy_##flavor##_P4C4R,                                     \
    "ippiCopy_" #flavor "_P4C4R", CV_PLUGINS1(CV_PLUGIN_IPPI),                      \
    ( const arrtype** src, int srcstep, arrtype* dst, int dststep, CvSize size ))

IPCV_PIX_PLANE( 8u, uchar )
IPCV_PIX_PLANE( 16s, ushort )
IPCV_PIX_PLANE( 32f, int )
IPCV_PIX_PLANE( 64f, int64 )
#undef IPCV_PIX_PLANE

IPCV_COPYSET( _8u_C1MR, uchar, int )
IPCV_COPYSET( _16s_C1MR, ushort, int )
IPCV_COPYSET( _8u_C3MR, uchar, int )
IPCV_COPYSET( _8u_C4MR, int, int )
IPCV_COPYSET( _16s_C3MR, ushort, int )
IPCV_COPYSET( _16s_C4MR, int64, int64 )
IPCV_COPYSET( _32f_C3MR, int, int )
IPCV_COPYSET( _32f_C4MR, int, int )
IPCV_COPYSET( _64s_C3MR, int64, int64 )
IPCV_COPYSET( _64s_C4MR, int64, int64 )

typedef CvStatus (CV_STDCALL *CvSplitFunc)( const void* src, int srcstep,
                                                    void** dst, int dststep, CvSize size);
typedef CvStatus (CV_STDCALL *CvExtractPlaneFunc)( const void* src, int srcstep,
                                                   void* dst, int dststep,
                                                   CvSize size, int cn, int coi );
typedef CvStatus (CV_STDCALL *CvMergeFunc)( const void** src, int srcstep,
                                                    void* dst, int dststep, CvSize size);

typedef CvStatus (CV_STDCALL *CvInsertPlaneFunc)( const void* src, int srcstep,
                                                  void* dst, int dststep,
                                                  CvSize size, int cn, int coi );

typedef CvStatus (CV_STDCALL * CvColorCvtFunc0)(
    const void* src, int srcstep, void* dst, int dststep, CvSize size );

typedef CvStatus (CV_STDCALL * CvColorCvtFunc1)(
    const void* src, int srcstep, void* dst, int dststep,
    CvSize size, int param0 );

typedef CvStatus (CV_STDCALL * CvColorCvtFunc2)(
    const void* src, int srcstep, void* dst, int dststep,
    CvSize size, int param0, int param1 );

typedef CvStatus (CV_STDCALL * CvColorCvtFunc3)(
    const void* src, int srcstep, void* dst, int dststep,
    CvSize size, int param0, int param1, int param2 );

#define IPCV_COLOR( funcname, ipp_funcname, flavor )                            \
IPCVAPI_EX( CvStatus, icv##funcname##_##flavor##_C3R,                           \
        "ippi" #ipp_funcname "_" #flavor "_C3R,"                                \
        "ippi" #ipp_funcname "_" #flavor "_C3R",                                \
        CV_PLUGINS2(CV_PLUGIN_IPPI,CV_PLUGIN_IPPCC),                            \
        ( const void* src, int srcstep, void* dst, int dststep, CvSize size ))

IPCV_COLOR( RGB2XYZ, RGBToXYZ, 8u )
IPCV_COLOR( RGB2XYZ, RGBToXYZ, 16u )
IPCV_COLOR( RGB2XYZ, RGBToXYZ, 32f )
IPCV_COLOR( XYZ2RGB, XYZToRGB, 8u )
IPCV_COLOR( XYZ2RGB, XYZToRGB, 16u )
IPCV_COLOR( XYZ2RGB, XYZToRGB, 32f )

IPCV_COLOR( RGB2HSV, RGBToHSV, 8u )
IPCV_COLOR( HSV2RGB, HSVToRGB, 8u )

IPCV_COLOR( RGB2HLS, RGBToHLS, 8u )
IPCV_COLOR( RGB2HLS, RGBToHLS, 32f )
IPCV_COLOR( HLS2RGB, HLSToRGB, 8u )
IPCV_COLOR( HLS2RGB, HLSToRGB, 32f )

IPCV_COLOR( BGR2Lab, BGRToLab, 8u )
IPCV_COLOR( Lab2BGR, LabToBGR, 8u )

IPCV_COLOR( RGB2Luv, RGBToLUV, 8u )
/*IPCV_COLOR( RGB2Luv, RGBToLUV, 32f )*/
IPCV_COLOR( Luv2RGB, LUVToRGB, 8u )
/*IPCV_COLOR( Luv2RGB, LUVToRGB, 32f )*/

typedef CvStatus (CV_STDCALL *CvFunc2D_3A)( void* arr0, int step0,
                                            void* arr1, int step1,
                                            void* arr2, int step2, CvSize size );
typedef CvStatus (CV_STDCALL *CvFunc2D_3A1I)( void* arr0, int step0,
                                              void* arr1, int step1,
                                              void* arr2, int step2,
                                              CvSize size, int flag );

#define IPCV_BIN_ARITHM( name )                                     \
IPCVAPI_EX( CvStatus, icv##name##_8u_C1R,                           \
    "ippi" #name "_8u_C1RSfs", CV_PLUGINS1(CV_PLUGIN_IPPI),         \
( const uchar* src1, int srcstep1, const uchar* src2, int srcstep2, \
  uchar* dst, int dststep, CvSize size, int scalefactor ))          \
IPCVAPI_EX( CvStatus, icv##name##_16u_C1R,                          \
    "ippi" #name "_16u_C1RSfs", CV_PLUGINS1(CV_PLUGIN_IPPI),        \
( const ushort* src1, int srcstep1, const ushort* src2, int srcstep2,\
  ushort* dst, int dststep, CvSize size, int scalefactor ))         \
IPCVAPI_EX( CvStatus, icv##name##_16s_C1R,                          \
    "ippi" #name "_16s_C1RSfs", CV_PLUGINS1(CV_PLUGIN_IPPI),        \
( const short* src1, int srcstep1, const short* src2, int srcstep2, \
  short* dst, int dststep, CvSize size, int scalefactor ))          \
IPCVAPI_EX( CvStatus, icv##name##_32s_C1R,                          \
    "ippi" #name "_32s_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),           \
( const int* src1, int srcstep1, const int* src2, int srcstep2,     \
  int* dst, int dststep, CvSize size ))                             \
IPCVAPI_EX( CvStatus, icv##name##_32f_C1R,                          \
    "ippi" #name "_32f_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),           \
( const float* src1, int srcstep1, const float* src2, int srcstep2, \
  float* dst, int dststep, CvSize size ))                           \
IPCVAPI_EX( CvStatus, icv##name##_64f_C1R,                          \
    "ippi" #name "_64f_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),           \
( const double* src1, int srcstep1, const double* src2, int srcstep2,\
  double* dst, int dststep, CvSize size ))


IPCV_BIN_ARITHM( Add )
IPCV_BIN_ARITHM( Sub )

typedef CvStatus (CV_STDCALL * CvWarpPerspectiveFunc)(
    const void* src, int srcstep, CvSize ssize,
    void* dst, int dststep, CvSize dsize,
    const double* matrix, int cn, const void* fillval );

#define IPCV_WARPPERSPECTIVE_BACK( flavor, cn )                             \
IPCVAPI_EX( CvStatus, icvWarpPerspectiveBack_##flavor##_C##cn##R,           \
    "ippiWarpPerspectiveBack_" #flavor "_C" #cn "R", CV_PLUGINS1(CV_PLUGIN_IPPI),\
    (const void* src, CvSize srcsize, int srcstep, CvRect srcroi,           \
    void* dst, int dststep, CvRect dstroi,                                  \
    const double* coeffs, int interpolate ))

IPCV_WARPPERSPECTIVE_BACK( 8u, 1 )
IPCV_WARPPERSPECTIVE_BACK( 8u, 3 )
IPCV_WARPPERSPECTIVE_BACK( 8u, 4 )

IPCV_WARPPERSPECTIVE_BACK( 32f, 1 )
IPCV_WARPPERSPECTIVE_BACK( 32f, 3 )
IPCV_WARPPERSPECTIVE_BACK( 32f, 4 )

#undef IPCV_WARPPERSPECTIVE_BACK

#define IPCV_WARPPERSPECTIVE( flavor, cn )                                  \
IPCVAPI_EX( CvStatus, icvWarpPerspective_##flavor##_C##cn##R,               \
    "ippiWarpPerspective_" #flavor "_C" #cn "R", CV_PLUGINS1(CV_PLUGIN_IPPI),\
    (const void* src, CvSize srcsize, int srcstep, CvRect srcroi,           \
    void* dst, int dststep, CvRect dstroi,                                  \
    const double* coeffs, int interpolate ))

IPCV_WARPPERSPECTIVE( 8u, 1 )
IPCV_WARPPERSPECTIVE( 8u, 3 )
IPCV_WARPPERSPECTIVE( 8u, 4 )

IPCV_WARPPERSPECTIVE( 32f, 1 )
IPCV_WARPPERSPECTIVE( 32f, 3 )
IPCV_WARPPERSPECTIVE( 32f, 4 )

#undef IPCV_WARPPERSPECTIVE

typedef CvStatus (CV_STDCALL * CvWarpPerspectiveBackIPPFunc)
( const void* src, CvSize srcsize, int srcstep, CvRect srcroi,
  void* dst, int dststep, CvRect dstroi,
  const double* coeffs, int interpolation );

#endif

static void* icvDefaultAlloc( size_t size, void* );
static int icvDefaultFree( void* ptr, void* );
void cvSetMemoryManager( CvAllocFunc alloc_func, CvFreeFunc free_func, void* userdata );
int cvErrorFromIppStatus( int status );;
void*  cvAlloc( size_t size );
void  cvFree_( void* ptr );
#define cvFree(ptr) (cvFree_(*(ptr)), *(ptr)=0)