#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <assert.h>
#include <math.h>
#include <limits.h>


#ifndef GLOBAL_H
#define GLOBAL_H

#define __BEGIN__       {
#define __END__         goto exit; exit: ; }
#define __CLEANUP__
#define EXIT            goto exit


#if defined WIN32 || defined WIN64
    #define CV_CDECL __cdecl
    #define CV_STDCALL __stdcall
#else
    #define CV_CDECL
    #define CV_STDCALL
#endif




typedef long long int64;
typedef unsigned long long uint64;
typedef unsigned char uchar;
typedef unsigned short ushort;

#define IPL_DEPTH_SIGN 0x80000000

#define IPL_DEPTH_1U     1
#define IPL_DEPTH_8U     8
#define IPL_DEPTH_16U   16
#define IPL_DEPTH_32F   32
#define IPL_DEPTH_64F  64

#define IPL_DEPTH_8S  (IPL_DEPTH_SIGN| 8)
#define IPL_DEPTH_16S (IPL_DEPTH_SIGN|16)
#define IPL_DEPTH_32S (IPL_DEPTH_SIGN|32)

#define IPL_DATA_ORDER_PIXEL  0
#define IPL_DATA_ORDER_PLANE  1

#define IPL_ORIGIN_TL 0
#define IPL_ORIGIN_BL 1

#define IPL_ALIGN_4BYTES   4
#define IPL_ALIGN_8BYTES   8
#define IPL_ALIGN_16BYTES 16
#define IPL_ALIGN_32BYTES 32

#define IPL_ALIGN_DWORD   IPL_ALIGN_4BYTES
#define IPL_ALIGN_QWORD   IPL_ALIGN_8BYTES

#define IPL_BORDER_CONSTANT   0
#define IPL_BORDER_REPLICATE  1
#define IPL_BORDER_REFLECT    2
#define IPL_BORDER_WRAP       3

#define IPL_IMAGE_HEADER 1
#define IPL_IMAGE_DATA   2
#define IPL_IMAGE_ROI    4

#define CV_PI   3.1415926535897932384626433832795
#define CV_LOG2 0.69314718055994530941723212145818

#define CV_SWAP(a,b,t) ((t) = (a), (a) = (b), (b) = (t))

#ifndef MIN
#define MIN(a,b)  ((a) > (b) ? (b) : (a))
#endif

#ifndef MAX
#define MAX(a,b)  ((a) < (b) ? (b) : (a))
#endif

#define  CV_IMIN(a, b)  ((a) ^ (((a)^(b)) & (((a) < (b)) - 1)))


typedef struct _IplImage
{
    int  nSize;         /* sizeof(IplImage) */
    int  ID;            /* version (=0)*/
    int  nChannels;     /* Most of OpenCV functions support 1,2,3 or 4 channels */
    int  alphaChannel;  /* ignored by OpenCV */
    int  depth;         /* pixel depth in bits: IPL_DEPTH_8U, IPL_DEPTH_8S, IPL_DEPTH_16S,
                           IPL_DEPTH_32S, IPL_DEPTH_32F and IPL_DEPTH_64F are supported */
    char colorModel[4]; /* ignored by OpenCV */
    char channelSeq[4]; /* ditto */
    int  dataOrder;     /* 0 - interleaved color channels, 1 - separate color channels.
                           cvCreateImage can only create interleaved images */
    int  origin;        /* 0 - top-left origin,
                           1 - bottom-left origin (Windows bitmaps style) */
    int  align;         /* Alignment of image rows (4 or 8).
                           OpenCV ignores it and uses widthStep instead */
    int  width;         /* image width in pixels */
    int  height;        /* image height in pixels */
    struct _IplROI *roi;/* image ROI. if NULL, the whole image is selected */
    struct _IplImage *maskROI; /* must be NULL */
    void  *imageId;     /* ditto */
    struct _IplTileInfo *tileInfo; /* ditto */
    int  imageSize;     /* image data size in bytes
                           (==image->height*image->widthStep
                           in case of interleaved data)*/
    char *imageData;  /* pointer to aligned image data */
    int  widthStep;   /* size of aligned image row in bytes */
    int  BorderMode[4]; /* ignored by OpenCV */
    int  BorderConst[4]; /* ditto */
    char *imageDataOrigin; /* pointer to very origin of image data
                              (not necessarily aligned) -
                              needed for correct deallocation */
}
IplImage;

typedef struct _IplTileInfo IplTileInfo;

typedef struct _IplROI
{
    int  coi; /* 0 - no COI (all channels are selected), 1 - 0th channel is selected ...*/
    int  xOffset;
    int  yOffset;
    int  width;
    int  height;
}
IplROI;

typedef struct
{
    int width;
    int height;
}
CvSize;

/******************************* CvPoint and variants ***********************************/

typedef struct CvPoint
{
    int x;
    int y;
}
CvPoint;

inline CvPoint  cvPoint( int x, int y )
{
    CvPoint p;

    p.x = x;
    p.y = y;

    return p;
}

typedef struct CvPoint2D32f
{
    float x;
    float y;
}
CvPoint2D32f;


typedef struct CvPoint3D32f
{
    float x;
    float y;
    float z;
}
CvPoint3D32f;



typedef struct CvPoint2D64f
{
    double x;
    double y;
}
CvPoint2D64f;


typedef struct CvPoint3D64f
{
    double x;
    double y;
    double z;
}
CvPoint3D64f;


typedef void CvArr;

#define CV_CN_MAX     64
#define CV_CN_SHIFT   3
#define CV_DEPTH_MAX  (1 << CV_CN_SHIFT)

#define CV_8U   0
#define CV_8S   1
#define CV_16U  2
#define CV_16S  3
#define CV_32S  4
#define CV_32F  5
#define CV_64F  6
#define CV_USRTYPE1 7

#define CV_MAKETYPE(depth,cn) ((depth) + (((cn)-1) << CV_CN_SHIFT))
#define CV_MAKE_TYPE CV_MAKETYPE

#define CV_8UC1 CV_MAKETYPE(CV_8U,1)
#define CV_8UC2 CV_MAKETYPE(CV_8U,2)
#define CV_8UC3 CV_MAKETYPE(CV_8U,3)
#define CV_8UC4 CV_MAKETYPE(CV_8U,4)
#define CV_8UC(n) CV_MAKETYPE(CV_8U,(n))

#define CV_8SC1 CV_MAKETYPE(CV_8S,1)
#define CV_8SC2 CV_MAKETYPE(CV_8S,2)
#define CV_8SC3 CV_MAKETYPE(CV_8S,3)
#define CV_8SC4 CV_MAKETYPE(CV_8S,4)
#define CV_8SC(n) CV_MAKETYPE(CV_8S,(n))

#define CV_16UC1 CV_MAKETYPE(CV_16U,1)
#define CV_16UC2 CV_MAKETYPE(CV_16U,2)
#define CV_16UC3 CV_MAKETYPE(CV_16U,3)
#define CV_16UC4 CV_MAKETYPE(CV_16U,4)
#define CV_16UC(n) CV_MAKETYPE(CV_16U,(n))

#define CV_16SC1 CV_MAKETYPE(CV_16S,1)
#define CV_16SC2 CV_MAKETYPE(CV_16S,2)
#define CV_16SC3 CV_MAKETYPE(CV_16S,3)
#define CV_16SC4 CV_MAKETYPE(CV_16S,4)
#define CV_16SC(n) CV_MAKETYPE(CV_16S,(n))

#define CV_32SC1 CV_MAKETYPE(CV_32S,1)
#define CV_32SC2 CV_MAKETYPE(CV_32S,2)
#define CV_32SC3 CV_MAKETYPE(CV_32S,3)
#define CV_32SC4 CV_MAKETYPE(CV_32S,4)
#define CV_32SC(n) CV_MAKETYPE(CV_32S,(n))

#define CV_32FC1 CV_MAKETYPE(CV_32F,1)
#define CV_32FC2 CV_MAKETYPE(CV_32F,2)
#define CV_32FC3 CV_MAKETYPE(CV_32F,3)
#define CV_32FC4 CV_MAKETYPE(CV_32F,4)
#define CV_32FC(n) CV_MAKETYPE(CV_32F,(n))

#define CV_64FC1 CV_MAKETYPE(CV_64F,1)
#define CV_64FC2 CV_MAKETYPE(CV_64F,2)
#define CV_64FC3 CV_MAKETYPE(CV_64F,3)
#define CV_64FC4 CV_MAKETYPE(CV_64F,4)
#define CV_64FC(n) CV_MAKETYPE(CV_64F,(n))

#define CV_AUTO_STEP  0x7fffffff
#define CV_WHOLE_ARR  cvSlice( 0, 0x3fffffff )

#define CV_MAT_CN_MASK          ((CV_CN_MAX - 1) << CV_CN_SHIFT)
#define CV_MAT_CN(flags)        ((((flags) & CV_MAT_CN_MASK) >> CV_CN_SHIFT) + 1)
#define CV_MAT_DEPTH_MASK       (CV_DEPTH_MAX - 1)
#define CV_MAT_DEPTH(flags)     ((flags) & CV_MAT_DEPTH_MASK)
#define CV_MAT_TYPE_MASK        (CV_DEPTH_MAX*CV_CN_MAX - 1)
#define CV_MAT_TYPE(flags)      ((flags) & CV_MAT_TYPE_MASK)
#define CV_MAT_CONT_FLAG_SHIFT  14
#define CV_MAT_CONT_FLAG        (1 << CV_MAT_CONT_FLAG_SHIFT)
#define CV_IS_MAT_CONT(flags)   ((flags) & CV_MAT_CONT_FLAG)
#define CV_IS_CONT_MAT          CV_IS_MAT_CONT
#define CV_MAT_TEMP_FLAG_SHIFT  15
#define CV_MAT_TEMP_FLAG        (1 << CV_MAT_TEMP_FLAG_SHIFT)
#define CV_IS_TEMP_MAT(flags)   ((flags) & CV_MAT_TEMP_FLAG)

#define CV_MAGIC_MASK       0xFFFF0000
#define CV_MAT_MAGIC_VAL    0x42420000
#define CV_TYPE_NAME_MAT    "opencv-matrix"

typedef struct CvMat
{
    int type;
    int step;

    /* for internal use only */
    int* refcount;
    int hdr_refcount;

    union
    {
        uchar* ptr;
        short* s;
        int* i;
        float* fl;
        double* db;
    } data;

#ifdef __cplusplus
    union
    {
        int rows;
        int height;
    };

    union
    {
        int cols;
        int width;
    };
#else
    int rows;
    int cols;
#endif

}
CvMat;


#define CV_MAT_ELEM_PTR_FAST( mat, row, col, pix_size )  \
    (assert( (unsigned)(row) < (unsigned)(mat).rows &&   \
             (unsigned)(col) < (unsigned)(mat).cols ),   \
     (mat).data.ptr + (size_t)(mat).step*(row) + (pix_size)*(col))

#define CV_MAT_ELEM_PTR( mat, row, col )                 \
    CV_MAT_ELEM_PTR_FAST( mat, row, col, CV_ELEM_SIZE((mat).type) )

#define CV_MAT_ELEM( mat, elemtype, row, col )           \
    (*(elemtype*)CV_MAT_ELEM_PTR_FAST( mat, row, col, sizeof(elemtype)))


#define CV_ELEM_SIZE(type) \
    (CV_MAT_CN(type) << ((((sizeof(size_t)/4+1)*16384|0x3a50) >> CV_MAT_DEPTH(type)*2) & 3))

#define CV_IS_MAT_HDR(mat) \
    ((mat) != NULL && \
    (((const CvMat*)(mat))->type & CV_MAGIC_MASK) == CV_MAT_MAGIC_VAL && \
    ((const CvMat*)(mat))->cols > 0 && ((const CvMat*)(mat))->rows > 0)

#define CV_IS_IMAGE_HDR(img) \
    ((img) != NULL && ((const IplImage*)(img))->nSize == sizeof(IplImage))

#define CV_IS_MAT(mat) \
    (CV_IS_MAT_HDR(mat) && ((const CvMat*)(mat))->data.ptr != NULL)


/////////////////////////////////////////////////////////////////////////////////////

typedef IplImage* (CV_STDCALL* Cv_iplCreateImageHeader)
                            (int,int,int,char*,char*,int,int,int,int,int,
                            IplROI*,IplImage*,void*,IplTileInfo*);
typedef void (CV_STDCALL* Cv_iplAllocateImageData)(IplImage*,int,int);
typedef void (CV_STDCALL* Cv_iplDeallocate)(IplImage*,int);
typedef IplROI* (CV_STDCALL* Cv_iplCreateROI)(int,int,int,int,int);
typedef IplImage* (CV_STDCALL* Cv_iplCloneImage)(const IplImage*);

static struct
{
    Cv_iplCreateImageHeader  createHeader;
    Cv_iplAllocateImageData  allocateData;
    Cv_iplDeallocate  deallocate;
    Cv_iplCreateROI  createROI;
    Cv_iplCloneImage  cloneImage;
}
CvIPL;

/****************************************************************************************\
*                       Multi-dimensional dense array (CvMatND)                          *
\****************************************************************************************/

#define CV_MATND_MAGIC_VAL    0x42430000
#define CV_TYPE_NAME_MATND    "opencv-nd-matrix"

#define CV_MAX_DIM            32
#define CV_MAX_DIM_HEAP       (1 << 16)

typedef struct CvMatND
{
    int type;
    int dims;

    int* refcount;
    int hdr_refcount;

    union
    {
        uchar* ptr;
        float* fl;
        double* db;
        int* i;
        short* s;
    } data;

    struct
    {
        int size;
        int step;
    }
    dim[CV_MAX_DIM];
}
CvMatND;

#define CV_IS_MATND_HDR(mat) \
    ((mat) != NULL && (((const CvMatND*)(mat))->type & CV_MAGIC_MASK) == CV_MATND_MAGIC_VAL)

#define CV_IS_MATND(mat) \
    (CV_IS_MATND_HDR(mat) && ((const CvMatND*)(mat))->data.ptr != NULL)

#define cvZero  cvSetZero

#define CV_SVD_MODIFY_A   1
#define CV_SVD_U_T        2
#define CV_SVD_V_T        4
#define CV_LU  0
#define CV_SVD 1
#define CV_SVD_SYM 2

#define cvConvert( src, dst )  cvConvertScale( (src), (dst), 1, 0 )
#define CV_AUTOSTEP  0x7fffffff

#define cvMatMulAdd( src1, src2, src3, dst ) cvGEMM( (src1), (src2), 1., (src3), 1., (dst), 0 )
#define cvMatMul( src1, src2, dst )  cvMatMulAdd( (src1), (src2), NULL, (dst))

typedef struct CvRect
{
    int x;
    int y;
    int width;
    int height;
}
CvRect;

/******************************** Memory storage ****************************************/

typedef struct CvMemBlock
{
    struct CvMemBlock*  prev;
    struct CvMemBlock*  next;
}
CvMemBlock;

#define CV_STORAGE_MAGIC_VAL    0x42890000

typedef struct CvMemStorage
{
    int signature;
    CvMemBlock* bottom;/* first allocated block */
    CvMemBlock* top;   /* current memory block - top of the stack */
    struct  CvMemStorage* parent; /* borrows new blocks from */
    int block_size;  /* block size */
    int free_space;  /* free space in the current block */
}
CvMemStorage;

#define CV_IS_STORAGE(storage)  \
    ((storage) != NULL &&       \
    (((CvMemStorage*)(storage))->signature & CV_MAGIC_MASK) == CV_STORAGE_MAGIC_VAL)


typedef struct CvMemStoragePos
{
    CvMemBlock* top;
    int free_space;
}
CvMemStoragePos;


/*********************************** Sequence *******************************************/

typedef struct CvSeqBlock
{
    struct CvSeqBlock*  prev; /* previous sequence block */
    struct CvSeqBlock*  next; /* next sequence block */
    int    start_index;       /* index of the first element in the block +
                                 sequence->first->start_index */
    int    count;             /* number of elements in the block */
    char*  data;              /* pointer to the first element of the block */
}
CvSeqBlock;


#define CV_TREE_NODE_FIELDS(node_type)                          \
    int       flags;         /* micsellaneous flags */          \
    int       header_size;   /* size of sequence header */      \
    struct    node_type* h_prev; /* previous sequence */        \
    struct    node_type* h_next; /* next sequence */            \
    struct    node_type* v_prev; /* 2nd previous sequence */    \
    struct    node_type* v_next  /* 2nd next sequence */

/*
   Read/Write sequence.
   Elements can be dynamically inserted to or deleted from the sequence.
*/
#define CV_SEQUENCE_FIELDS()                                            \
    CV_TREE_NODE_FIELDS(CvSeq);                                         \
    int       total;          /* total number of elements */            \
    int       elem_size;      /* size of sequence element in bytes */   \
    char*     block_max;      /* maximal bound of the last block */     \
    char*     ptr;            /* current write pointer */               \
    int       delta_elems;    /* how many elements allocated when the seq grows */  \
    CvMemStorage* storage;    /* where the seq is stored */             \
    CvSeqBlock* free_blocks;  /* free blocks list */                    \
    CvSeqBlock* first; /* pointer to the first sequence block */

typedef struct CvSeq
{
    CV_SEQUENCE_FIELDS()
}
CvSeq;

typedef int (CV_CDECL* CvCmpFunc)(const void* a, const void* b, void* userdata );
typedef struct CvSlice
{
    int  start_index, end_index;
}
CvSlice;


#define CV_WHOLE_SEQ_END_INDEX 0x3fffffff
#define CV_WHOLE_SEQ  cvSlice(0, CV_WHOLE_SEQ_END_INDEX)

#define  CV_SEQ_ELEM( seq, elem_type, index )                    \
/* assert gives some guarantee that <seq> parameter is valid */  \
(   assert(sizeof((seq)->first[0]) == sizeof(CvSeqBlock) &&      \
    (seq)->elem_size == sizeof(elem_type)),                      \
    (elem_type*)((seq)->first && (unsigned)index <               \
    (unsigned)((seq)->first->count) ?                            \
    (seq)->first->data + (index) * sizeof(elem_type) :           \
    cvGetSeqElem( (CvSeq*)(seq), (index) )))
#define CV_GET_SEQ_ELEM( elem_type, seq, index ) CV_SEQ_ELEM( (seq), elem_type, (index) )

#define CV_GAUSSIAN  2

#define CV_GEMM_A_T 1
#define CV_GEMM_B_T 2
#define CV_GEMM_C_T 4

#define  CV_INTER_NN        0
#define  CV_INTER_LINEAR    1
#define  CV_INTER_CUBIC     2
#define  CV_INTER_AREA      3

#define CV_MAX_ARR 10

typedef struct CvNArrayIterator
{
    int count; /* number of arrays */
    int dims; /* number of dimensions to iterate */
    CvSize size; /* maximal common linear size: { width = size, height = 1 } */
    uchar* ptr[CV_MAX_ARR]; /* pointers to the array slices */
    int stack[CV_MAX_DIM]; /* for internal use */
    CvMatND* hdr[CV_MAX_ARR]; /* pointers to the headers of the
                                 matrices that are processed */
}
CvNArrayIterator;

#define  CV_MAX_INLINE_MAT_OP_SIZE  10
#define  CV_STUB_STEP     (1 << 30)

#define CV_SPARSE_MAT_MAGIC_VAL    0x42440000
#define CV_TYPE_NAME_SPARSE_MAT    "opencv-sparse-matrix"

#define CV_SET_ELEM_FIELDS(elem_type)   \
    int  flags;                         \
    struct elem_type* next_free;

typedef struct CvSetElem
{
    CV_SET_ELEM_FIELDS(CvSetElem)
}
CvSetElem;

#define CV_SET_FIELDS()      \
    CV_SEQUENCE_FIELDS()     \
    CvSetElem* free_elems;   \
    int active_count;

typedef struct CvSet
{
    CV_SET_FIELDS()
}
CvSet;

typedef struct CvSparseMat
{
    int type;
    int dims;
    int* refcount;
    int hdr_refcount;

    struct CvSet* heap;
    void** hashtable;
    int hashsize;
    int valoffset;
    int idxoffset;
    int size[CV_MAX_DIM];
}
CvSparseMat;

typedef struct CvSparseNode
{
    unsigned hashval;
    struct CvSparseNode* next;
}
CvSparseNode;

typedef struct CvSparseMatIterator
{
    CvSparseMat* mat;
    CvSparseNode* node;
    int curidx;
}
CvSparseMatIterator;


#define CV_IS_SPARSE_MAT_HDR(mat) \
    ((mat) != NULL && \
    (((const CvSparseMat*)(mat))->type & CV_MAGIC_MASK) == CV_SPARSE_MAT_MAGIC_VAL)

#define CV_IS_SPARSE_MAT(mat) \
    CV_IS_SPARSE_MAT_HDR(mat)

#define CV_ARE_TYPES_EQ(mat1, mat2) \
    ((((mat1)->type ^ (mat2)->type) & CV_MAT_TYPE_MASK) == 0)

#define  CV_MAX_LOCAL_MAT_SIZE  32

/* maximal size of local memory storage */
#define  CV_MAX_LOCAL_SIZE  \
    (CV_MAX_LOCAL_MAT_SIZE*CV_MAX_LOCAL_MAT_SIZE*(int)sizeof(double))

#define cvStackAlloc(size) cvAlignPtr( alloca((size) + CV_MALLOC_ALIGN), CV_MALLOC_ALIGN )

#define cvT cvTranspose

#if 0 /*def  CV_CHECK_FOR_NANS*/
    #define CV_CHECK_NANS( arr ) cvCheckArray((arr))  
#else
    #define CV_CHECK_NANS( arr )
#endif

 #define CV_DEFAULT(val) = val

typedef struct CvScalar
{
    double val[4];
}
CvScalar;

inline  CvScalar  cvScalar( double val0, double val1 CV_DEFAULT(0),double val2 CV_DEFAULT(0), double val3 CV_DEFAULT(0))
{
    CvScalar scalar;
    scalar.val[0] = val0; scalar.val[1] = val1;
    scalar.val[2] = val2; scalar.val[3] = val3;
    return scalar;
}

#define  CV_SPARSE_HASH_RATIO    3

#define CV_MEMCPY_AUTO( dst, src, len )                                             \
{                                                                                   \
    size_t _icv_memcpy_i_, _icv_memcpy_len_ = (len);                                \
    char* _icv_memcpy_dst_ = (char*)(dst);                                          \
    const char* _icv_memcpy_src_ = (const char*)(src);                              \
    if( (_icv_memcpy_len_ & (sizeof(int)-1)) == 0 )                                 \
    {                                                                               \
        assert( ((size_t)_icv_memcpy_src_&(sizeof(int)-1)) == 0 &&                  \
                ((size_t)_icv_memcpy_dst_&(sizeof(int)-1)) == 0 );                  \
        for( _icv_memcpy_i_ = 0; _icv_memcpy_i_ < _icv_memcpy_len_;                 \
            _icv_memcpy_i_+=sizeof(int) )                                           \
        {                                                                           \
            *(int*)(_icv_memcpy_dst_+_icv_memcpy_i_) =                              \
            *(const int*)(_icv_memcpy_src_+_icv_memcpy_i_);                         \
        }                                                                           \
    }                                                                               \
    else                                                                            \
    {                                                                               \
        for(_icv_memcpy_i_ = 0; _icv_memcpy_i_ < _icv_memcpy_len_; _icv_memcpy_i_++)\
            _icv_memcpy_dst_[_icv_memcpy_i_] = _icv_memcpy_src_[_icv_memcpy_i_];    \
    }                                                                               \
}

#define CV_ARE_SIZES_EQ(mat1, mat2) \
    ((mat1)->height == (mat2)->height && (mat1)->width == (mat2)->width)

#define CV_IS_MASK_ARR(mat) \
    (((mat)->type & (CV_MAT_TYPE_MASK & ~CV_8SC1)) == 0)

typedef struct CvFuncTable
{
    void*   fn_2d[CV_DEPTH_MAX];
}
CvFuncTable;


#define CV_NO_DEPTH_CHECK     1
#define CV_NO_CN_CHECK        2
#define CV_NO_SIZE_CHECK      4

#define CV_DEF_INIT_FUNC_TAB_2D( FUNCNAME, FLAG )                   \
static void  icvInit##FUNCNAME##FLAG##Table( CvFuncTable* tab )     \
{                                                                   \
    assert( tab );                                                  \
                                                                    \
    tab->fn_2d[CV_8U]  = (void*)icv##FUNCNAME##_8u_##FLAG;          \
    tab->fn_2d[CV_8S]  = (void*)icv##FUNCNAME##_8s_##FLAG;          \
    tab->fn_2d[CV_16U] = (void*)icv##FUNCNAME##_16u_##FLAG;         \
    tab->fn_2d[CV_16S] = (void*)icv##FUNCNAME##_16s_##FLAG;         \
    tab->fn_2d[CV_32S] = (void*)icv##FUNCNAME##_32s_##FLAG;         \
    tab->fn_2d[CV_32F] = (void*)icv##FUNCNAME##_32f_##FLAG;         \
    tab->fn_2d[CV_64F] = (void*)icv##FUNCNAME##_64f_##FLAG;         \
}

#define  CV_NOP(a)      (a)
#define  CV_ADD(a, b)   ((a) + (b))
#define  CV_SUB(a, b)   ((a) - (b))
#define  CV_MUL(a, b)   ((a) * (b))
#define  CV_AND(a, b)   ((a) & (b))
#define  CV_OR(a, b)    ((a) | (b))
#define  CV_XOR(a, b)   ((a) ^ (b))
#define  CV_ANDN(a, b)  (~(a) & (b))
#define  CV_ORN(a, b)   (~(a) | (b))
#define  CV_SQR(a)      ((a) * (a))
#define CV_SUB_R(a,b) ((b) - (a))

#define  CV_CAST_8U(t)  (uchar)(!((t) & ~255) ? (t) : (t) > 0 ? 255 : 0)
#define  CV_CAST_8S(t)  (char)(!(((t)+128) & ~255) ? (t) : (t) > 0 ? 127 : -128)
#define  CV_CAST_16U(t) (ushort)(!((t) & ~65535) ? (t) : (t) > 0 ? 65535 : 0)
#define  CV_CAST_16S(t) (short)(!(((t)+32768) & ~65535) ? (t) : (t) > 0 ? 32767 : -32768)
#define  CV_CAST_32S(t) (int)(t)
#define  CV_CAST_64S(t) (int64)(t)
#define  CV_CAST_32F(t) (float)(t)
#define  CV_CAST_64F(t) (double)(t)

// -256.f ... 511.f
extern const float icv8x32fTab_cv[];
#define CV_8TO32F(x)  icv8x32fTab_cv[(x)+256]

#define ICV_FIX_SHIFT  15
#define ICV_SCALE(x)   (((x) + (1 << (ICV_FIX_SHIFT-1))) >> ICV_FIX_SHIFT)

#define CV_ARE_CNS_EQ(mat1, mat2) \
    ((((mat1)->type ^ (mat2)->type) & CV_MAT_CN_MASK) == 0)

#define det2(m)   (m(0,0)*m(1,1) - m(0,1)*m(1,0))
#define det3(m)   (m(0,0)*(m(1,1)*m(2,2) - m(1,2)*m(2,1)) -  \
                   m(0,1)*(m(1,0)*m(2,2) - m(1,2)*m(2,0)) +  \
                   m(0,2)*(m(1,0)*m(2,1) - m(1,1)*m(2,0)))

#define Sf( y, x ) ((float*)(srcdata + y*srcstep))[x]
#define Sd( y, x ) ((double*)(srcdata + y*srcstep))[x]
#define Df( y, x ) ((float*)(dstdata + y*dststep))[x]
#define Dd( y, x ) ((double*)(dstdata + y*dststep))[x]

typedef struct CvBigFuncTable
{
    void*   fn_2d[CV_DEPTH_MAX*CV_CN_MAX];
}
CvBigFuncTable;

#ifndef IPCVAPI
#define IPCVAPI(type,declspec,name,args)                        \
    /* function pointer */                                      \
    typedef type (declspec* name##_t) args;                     \
    extern name##_t name##_p;                                   \
    type declspec name args;
#endif


#define IPCVAPI_EX(type,name,ipp_name,ipp_search_modules,args)  \
    IPCVAPI(type,CV_STDCALL,name,args) 

#define IPCVAPI_C_EX(type,name,ipp_name,ipp_search_modules,args)\
    IPCVAPI(type,CV_CDECL,name,args)

#ifndef IPCVAPI_IMPL
#define IPCVAPI_IMPL(type,name,args,arg_names)                  \
    static type CV_STDCALL name##_f args;                       \
    name##_t name##_p = name##_f;                               \
    type CV_STDCALL name args { return name##_p arg_names; }    \
    static type CV_STDCALL name##_f args
#endif

#define  CV_STRUCT_ALIGN    ((int)sizeof(double))
inline int cvAlign( int size, int align )
{
    assert( (align & (align-1)) == 0 && size < INT_MAX );
    return (size + align - 1) & -align;
}

struct CvComplex32f;
struct CvComplex64f;

struct CvComplex32f
{
    float re, im;

    CvComplex32f() {}
    CvComplex32f( float _re, float _im=0 ) : re(_re), im(_im) {}
    explicit CvComplex32f( const CvComplex64f& v );
    //CvComplex32f( const CvComplex32f& v ) : re(v.re), im(v.im) {}
    //CvComplex32f& operator = (const CvComplex32f& v ) { re = v.re; im = v.im; return *this; }
    operator CvComplex64f() const;
};

struct CvComplex64f
{
    double re, im;

    CvComplex64f() {}
    CvComplex64f( double _re, double _im=0 ) : re(_re), im(_im) {}
    explicit CvComplex64f( const CvComplex32f& v );
    //CvComplex64f( const CvComplex64f& v ) : re(v.re), im(v.im) {}
    //CvComplex64f& operator = (const CvComplex64f& v ) { re = v.re; im = v.im; return *this; }
    operator CvComplex32f() const;
};

inline CvComplex32f::CvComplex32f( const CvComplex64f& v ) : re((float)v.re), im((float)v.im) {}
inline CvComplex64f::CvComplex64f( const CvComplex32f& v ) : re(v.re), im(v.im) {}
inline CvComplex32f operator + (CvComplex32f a, CvComplex32f b)
{
    return CvComplex32f( a.re + b.re, a.im + b.im );
}

inline CvComplex32f& operator += (CvComplex32f& a, CvComplex32f b)
{
    a.re += b.re;
    a.im += b.im;
    return a;
}

inline CvComplex32f operator - (CvComplex32f a, CvComplex32f b)
{
    return CvComplex32f( a.re - b.re, a.im - b.im );
}

inline CvComplex32f& operator -= (CvComplex32f& a, CvComplex32f b)
{
    a.re -= b.re;
    a.im -= b.im;
    return a;
}

inline CvComplex32f operator - (CvComplex32f a)
{
    return CvComplex32f( -a.re, -a.im );
}

inline CvComplex32f operator * (CvComplex32f a, CvComplex32f b)
{
    return CvComplex32f( a.re*b.re - a.im*b.im, a.re*b.im + a.im*b.re );
}

inline double abs(CvComplex32f a)
{
    return sqrt( (double)a.re*a.re + (double)a.im*a.im );
}

inline CvComplex32f conj(CvComplex32f a)
{
    return CvComplex32f( a.re, -a.im );
}


inline CvComplex32f operator / (CvComplex32f a, CvComplex32f b)
{
    double t = 1./((double)b.re*b.re + (double)b.im*b.im);
    return CvComplex32f( (float)((a.re*b.re + a.im*b.im)*t),
                         (float)((-a.re*b.im + a.im*b.re)*t) );
}

inline CvComplex32f operator * (double a, CvComplex32f b)
{
    return CvComplex32f( (float)(a*b.re), (float)(a*b.im) );
}

inline CvComplex32f operator * (CvComplex32f a, double b)
{
    return CvComplex32f( (float)(a.re*b), (float)(a.im*b) );
}

inline CvComplex32f::operator CvComplex64f() const
{
    return CvComplex64f(re,im);
}


inline CvComplex64f operator + (CvComplex64f a, CvComplex64f b)
{
    return CvComplex64f( a.re + b.re, a.im + b.im );
}

inline CvComplex64f& operator += (CvComplex64f& a, CvComplex64f b)
{
    a.re += b.re;
    a.im += b.im;
    return a;
}

inline CvComplex64f operator - (CvComplex64f a, CvComplex64f b)
{
    return CvComplex64f( a.re - b.re, a.im - b.im );
}

inline CvComplex64f& operator -= (CvComplex64f& a, CvComplex64f b)
{
    a.re -= b.re;
    a.im -= b.im;
    return a;
}

inline CvComplex64f operator - (CvComplex64f a)
{
    return CvComplex64f( -a.re, -a.im );
}

inline CvComplex64f operator * (CvComplex64f a, CvComplex64f b)
{
    return CvComplex64f( a.re*b.re - a.im*b.im, a.re*b.im + a.im*b.re );
}

inline double abs(CvComplex64f a)
{
    return sqrt( (double)a.re*a.re + (double)a.im*a.im );
}

inline CvComplex64f operator / (CvComplex64f a, CvComplex64f b)
{
    double t = 1./((double)b.re*b.re + (double)b.im*b.im);
    return CvComplex64f( (a.re*b.re + a.im*b.im)*t,
                         (-a.re*b.im + a.im*b.re)*t );
}

inline CvComplex64f operator * (double a, CvComplex64f b)
{
    return CvComplex64f( a*b.re, a*b.im );
}

inline CvComplex64f operator * (CvComplex64f a, double b)
{
    return CvComplex64f( a.re*b, a.im*b );
}

inline CvComplex64f::operator CvComplex32f() const
{
    return CvComplex32f((float)re,(float)im);
}

inline CvComplex64f conj(CvComplex64f a)
{
    return CvComplex64f( a.re, -a.im );
}

inline CvComplex64f operator + (CvComplex64f a, CvComplex32f b)
{
    return CvComplex64f( a.re + b.re, a.im + b.im );
}

inline CvComplex64f operator + (CvComplex32f a, CvComplex64f b)
{
    return CvComplex64f( a.re + b.re, a.im + b.im );
}

inline CvComplex64f operator - (CvComplex64f a, CvComplex32f b)
{
    return CvComplex64f( a.re - b.re, a.im - b.im );
}

inline CvComplex64f operator - (CvComplex32f a, CvComplex64f b)
{
    return CvComplex64f( a.re - b.re, a.im - b.im );
}

inline CvComplex64f operator * (CvComplex64f a, CvComplex32f b)
{
    return CvComplex64f( a.re*b.re - a.im*b.im, a.re*b.im + a.im*b.re );
}

inline CvComplex64f operator * (CvComplex32f a, CvComplex64f b)
{
    return CvComplex64f( a.re*b.re - a.im*b.im, a.re*b.im + a.im*b.re );
}

#define  CV_STORAGE_BLOCK_SIZE   ((1<<16) - 128)

#define ICV_FREE_PTR(storage)  \
    ((char*)(storage)->top + (storage)->block_size - (storage)->free_space)

#define CV_SEQ_READER_FIELDS()                                      \
    int          header_size;                                       \
    CvSeq*       seq;        /* sequence, beign read */             \
    CvSeqBlock*  block;      /* current block */                    \
    char*        ptr;        /* pointer to element be read next */  \
    char*        block_min;  /* pointer to the beginning of block */\
    char*        block_max;  /* pointer to the end of block */      \
    int          delta_index;/* = seq->first->start_index   */      \
    char*        prev_elem;  /* pointer to previous element */

typedef struct CvSeqReader
{
    CV_SEQ_READER_FIELDS()
}
CvSeqReader;

typedef struct CvSeqReaderPos
{
    CvSeqBlock* block;
    char* ptr;
    char* block_min;
    char* block_max;
}
CvSeqReaderPos;

#define CV_SAVE_READER_POS( reader, pos )   \
{                                           \
    (pos).block = (reader).block;           \
    (pos).ptr = (reader).ptr;               \
    (pos).block_min = (reader).block_min;   \
    (pos).block_max = (reader).block_max;   \
}

#define CV_RESTORE_READER_POS( reader, pos )\
{                                           \
    (reader).block = (pos).block;           \
    (reader).ptr = (pos).ptr;               \
    (reader).block_min = (pos).block_min;   \
    (reader).block_max = (pos).block_max;   \
}

#define CV_SEQ_WRITER_FIELDS()                                     \
    int          header_size;                                      \
    CvSeq*       seq;        /* the sequence written */            \
    CvSeqBlock*  block;      /* current block */                   \
    char*        ptr;        /* pointer to free space */           \
    char*        block_min;  /* pointer to the beginning of block*/\
    char*        block_max;  /* pointer to the end of block */

typedef struct CvSeqWriter
{
    CV_SEQ_WRITER_FIELDS()
}
CvSeqWriter;





#define CV_SEQ_MAGIC_VAL             0x42990000
#define CV_IS_SEQ(seq) \
    ((seq) != NULL && (((CvSeq*)(seq))->flags & CV_MAGIC_MASK) == CV_SEQ_MAGIC_VAL)

#define CV_PREV_SEQ_ELEM( elem_size, reader )                \
{                                                            \
    if( ((reader).ptr -= (elem_size)) < (reader).block_min ) \
    {                                                        \
        cvChangeSeqBlock( &(reader), -1 );                   \
    }                                                        \
}
#define CV_NEXT_SEQ_ELEM( elem_size, reader )                 \
{                                                             \
    if( ((reader).ptr += (elem_size)) >= (reader).block_max ) \
    {                                                         \
        cvChangeSeqBlock( &(reader), 1 );                     \
    }                                                         \
}
#define CV_SWAP_ELEMS(a,b,elem_size)  \
{                                     \
    int k;                            \
    for( k = 0; k < elem_size; k++ )  \
    {                                 \
        char t0 = (a)[k];             \
        char t1 = (b)[k];             \
        (a)[k] = t1;                  \
        (b)[k] = t0;                  \
    }                                 \
}

#define CV_SEQ_ELTYPE_GENERIC        0
#define ICV_SHIFT_TAB_MAX 32
static const char icvPower2ShiftTab[] =
{
    0, 1, -1, 2, -1, -1, -1, 3, -1, -1, -1, -1, -1, -1, -1, 4,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 5
};

#define ICV_ALIGNED_SEQ_BLOCK_SIZE  \
    (int)cvAlign(sizeof(CvSeqBlock), CV_STRUCT_ALIGN)

#define  CV_DESCALE(x,n)     (((x) + (1 << ((n)-1))) >> (n))
#define ICV_WARP_SHIFT          10
#define ICV_WARP_MUL_ONE_8U(x)  ((x) << ICV_WARP_SHIFT)
#define ICV_WARP_DESCALE_8U(x)  CV_DESCALE((x), ICV_WARP_SHIFT*2)

typedef struct CvResizeAlpha
{
    int idx;
    union
    {
        float alpha;
        int ialpha;
    };
}
CvResizeAlpha;
#define ICV_WARP_MASK           ((1 << ICV_WARP_SHIFT) - 1)
#define ICV_CUBIC_TAB_SIZE   (ICV_WARP_MASK+1)
extern float icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE+1)*2];

#define CV_MEMCPY_INT( dst, src, len )                                              \
{                                                                                   \
    size_t _icv_memcpy_i_, _icv_memcpy_len_ = (len);                                \
    int* _icv_memcpy_dst_ = (int*)(dst);                                            \
    const int* _icv_memcpy_src_ = (const int*)(src);                                \
    assert( ((size_t)_icv_memcpy_src_&(sizeof(int)-1)) == 0 &&                      \
            ((size_t)_icv_memcpy_dst_&(sizeof(int)-1)) == 0 );                      \
                                                                                    \
    for(_icv_memcpy_i_=0;_icv_memcpy_i_<_icv_memcpy_len_;_icv_memcpy_i_++)          \
        _icv_memcpy_dst_[_icv_memcpy_i_] = _icv_memcpy_src_[_icv_memcpy_i_];        \
}

typedef struct CvDecimateAlpha
{
    int si, di;
    float alpha;
}
CvDecimateAlpha;

#define  CV_FLT_TO_FIX(x,n)  cvRound((x)*(1<<(n)))
#define  CV_DEFAULT_MAT_ROW_ALIGN  1

extern const signed char icvDepthToType[];

#define icvIplToCvDepth( depth ) \
    icvDepthToType[(((depth) & 255) >> 2) + ((depth) < 0)]

typedef struct CvBtFuncTable
{
    void*   fn_2d[33];
}
CvBtFuncTable;

#define CV_DEF_INIT_PIXSIZE_TAB_2D( FUNCNAME, FLAG )                \
static void icvInit##FUNCNAME##FLAG##Table( CvBtFuncTable* table )  \
{                                                                   \
    table->fn_2d[1]  = (void*)icv##FUNCNAME##_8u_C1##FLAG;          \
    table->fn_2d[2]  = (void*)icv##FUNCNAME##_8u_C2##FLAG;          \
    table->fn_2d[3]  = (void*)icv##FUNCNAME##_8u_C3##FLAG;          \
    table->fn_2d[4]  = (void*)icv##FUNCNAME##_16u_C2##FLAG;         \
    table->fn_2d[6]  = (void*)icv##FUNCNAME##_16u_C3##FLAG;         \
    table->fn_2d[8]  = (void*)icv##FUNCNAME##_32s_C2##FLAG;         \
    table->fn_2d[12] = (void*)icv##FUNCNAME##_32s_C3##FLAG;         \
    table->fn_2d[16] = (void*)icv##FUNCNAME##_64s_C2##FLAG;         \
    table->fn_2d[24] = (void*)icv##FUNCNAME##_64s_C3##FLAG;         \
    table->fn_2d[32] = (void*)icv##FUNCNAME##_64s_C4##FLAG;         \
}

#define icvGivens_64f( n, x, y, c, s ) \
{                                      \
    int _i;                            \
    double* _x = (x);                  \
    double* _y = (y);                  \
                                       \
    for( _i = 0; _i < n; _i++ )        \
    {                                  \
        double t0 = _x[_i];            \
        double t1 = _y[_i];            \
        _x[_i] = t0*c + t1*s;          \
        _y[_i] = -t0*s + t1*c;         \
    }                                  \
}

#define CV_ELEM_SIZE1(type) \
    ((((sizeof(size_t)<<28)|0x8442211) >> CV_MAT_DEPTH(type)*4) & 15)

#define arrtype float
#define temptype double

#define  CV_UN_ENTRY_C1(worktype)           \
    worktype s0 = scalar[0]
    
#define  CV_UN_ENTRY_C2(worktype)           \
    worktype s0 = scalar[0], s1 = scalar[1]

#define  CV_UN_ENTRY_C3(worktype)           \
    worktype s0 = scalar[0], s1 = scalar[1], s2 = scalar[2]

#define  CV_UN_ENTRY_C4(worktype)           \
    worktype s0 = scalar[0], s1 = scalar[1], s2 = scalar[2], s3 = scalar[3]
#define CV_SET_ELEM_IDX_MASK   ((1 << 26) - 1)

#define CV_ARE_DEPTHS_EQ(mat1, mat2) \
    ((((mat1)->type ^ (mat2)->type) & CV_MAT_DEPTH_MASK) == 0)

#define CV_GET_LAST_ELEM( seq, block ) \
    ((block)->data + ((block)->count - 1)*((seq)->elem_size))
#define CV_SET_ELEM_FREE_FLAG  (1 << (sizeof(int)*8-1))

typedef struct CvGenericHash
{
    CV_SET_FIELDS()
    int tab_size;
    void** table;
}
CvGenericHash;

typedef CvGenericHash CvStringHash;

typedef struct CvGenericHash CvFileNodeHash;

typedef struct CvString
{
    int len;
    char* ptr;
}
CvString;

typedef struct CvFileNode
{
    int tag;
    struct CvTypeInfo* info; /* type information
            (only for user-defined object, for others it is 0) */
    union
    {
        double f; /* scalar floating-point number */
        int i;    /* scalar integer number */
        CvString str; /* text string */
        CvSeq* seq; /* sequence (ordered collection of file nodes) */
        CvFileNodeHash* map; /* map (collection of named file nodes) */
    } data;
}
CvFileNode;

//typedef void (*CvParse)( struct CvFileStorage* fs );
typedef void (*CvStartWriteStruct)( struct CvFileStorage* fs, const char* key,
                                    int struct_flags, const char* type_name );
typedef void (*CvEndWriteStruct)( struct CvFileStorage* fs );
typedef void (*CvWriteInt)( struct CvFileStorage* fs, const char* key, int value );
typedef void (*CvWriteReal)( struct CvFileStorage* fs, const char* key, double value );
typedef void (*CvWriteString)( struct CvFileStorage* fs, const char* key,
                               const char* value, int quote );
typedef void (*CvWriteComment)( struct CvFileStorage* fs, const char* comment, int eol_comment );
typedef void (*CvStartNextStream)( struct CvFileStorage* fs );

typedef struct CvFileStorage
{
    int flags;
    int is_xml;
    int write_mode;
    int is_first;
    CvMemStorage* memstorage;
    CvMemStorage* dststorage;
    CvMemStorage* strstorage;
    CvStringHash* str_hash;
    CvSeq* roots;
    CvSeq* write_stack;
    int struct_indent;
    int struct_flags;
    CvString struct_tag;
    int space;
    char* filename;
    FILE* file;
    char* buffer;
    char* buffer_start;
    char* buffer_end;
    int wrap_margin;
    int lineno;
    int dummy_eof;
    const char* errmsg;
    char errmsgbuf[128];

    CvStartWriteStruct start_write_struct;
    CvEndWriteStruct end_write_struct;
    CvWriteInt write_int;
    CvWriteReal write_real;
    CvWriteString write_string;
    CvWriteComment write_comment;
    CvStartNextStream start_next_stream;
    //CvParse parse;
}
CvFileStorage;

/* list of attributes */
typedef struct CvAttrList
{
    const char** attr; /* NULL-terminated array of (attribute_name,attribute_value) pairs */
    struct CvAttrList* next; /* pointer to next chunk of the attributes list */
}
CvAttrList;

#ifdef __cplusplus
extern "C" {
#endif
typedef int (CV_CDECL *CvIsInstanceFunc)( const void* struct_ptr );
typedef void (CV_CDECL *CvReleaseFunc)( void** struct_dblptr );
typedef void* (CV_CDECL *CvReadFunc)( CvFileStorage* storage, CvFileNode* node );
typedef void (CV_CDECL *CvWriteFunc)( CvFileStorage* storage, const char* name,
                                      const void* struct_ptr, CvAttrList attributes );
typedef void* (CV_CDECL *CvCloneFunc)( const void* struct_ptr );
#ifdef __cplusplus
}
#endif

typedef struct CvTypeInfo
{
    int flags;
    int header_size;
    struct CvTypeInfo* prev;
    struct CvTypeInfo* next;
    const char* type_name;
    CvIsInstanceFunc is_instance;
    CvReleaseFunc release;
    CvReadFunc read;
    CvWriteFunc write;
    CvCloneFunc clone;
}
CvTypeInfo;

#if (defined WIN32 || defined WIN64) && defined CVAPI_EXPORTS
    #define CV_EXPORTS __declspec(dllexport)
#else
    #define CV_EXPORTS
#endif


#define ABS(x) ( ( (x) < 0 )? -(x) : (x) )

/* Constants for color conversion */
#define  CV_BGR2BGRA    0
#define  CV_RGB2RGBA    CV_BGR2BGRA

#define  CV_BGRA2BGR    1
#define  CV_RGBA2RGB    CV_BGRA2BGR

#define  CV_BGR2RGBA    2
#define  CV_RGB2BGRA    CV_BGR2RGBA

#define  CV_RGBA2BGR    3
#define  CV_BGRA2RGB    CV_RGBA2BGR

#define  CV_BGR2RGB     4
#define  CV_RGB2BGR     CV_BGR2RGB

#define  CV_BGRA2RGBA   5
#define  CV_RGBA2BGRA   CV_BGRA2RGBA

#define  CV_BGR2GRAY    6
#define  CV_RGB2GRAY    7
#define  CV_GRAY2BGR    8
#define  CV_GRAY2RGB    CV_GRAY2BGR
#define  CV_GRAY2BGRA   9
#define  CV_GRAY2RGBA   CV_GRAY2BGRA
#define  CV_BGRA2GRAY   10
#define  CV_RGBA2GRAY   11

#define  CV_BGR2BGR565  12
#define  CV_RGB2BGR565  13
#define  CV_BGR5652BGR  14
#define  CV_BGR5652RGB  15
#define  CV_BGRA2BGR565 16
#define  CV_RGBA2BGR565 17
#define  CV_BGR5652BGRA 18
#define  CV_BGR5652RGBA 19

#define  CV_GRAY2BGR565 20
#define  CV_BGR5652GRAY 21

#define  CV_BGR2BGR555  22
#define  CV_RGB2BGR555  23
#define  CV_BGR5552BGR  24
#define  CV_BGR5552RGB  25
#define  CV_BGRA2BGR555 26
#define  CV_RGBA2BGR555 27
#define  CV_BGR5552BGRA 28
#define  CV_BGR5552RGBA 29

#define  CV_GRAY2BGR555 30
#define  CV_BGR5552GRAY 31

#define  CV_BGR2XYZ     32
#define  CV_RGB2XYZ     33
#define  CV_XYZ2BGR     34
#define  CV_XYZ2RGB     35

#define  CV_BGR2YCrCb   36
#define  CV_RGB2YCrCb   37
#define  CV_YCrCb2BGR   38
#define  CV_YCrCb2RGB   39

#define  CV_BGR2HSV     40
#define  CV_RGB2HSV     41

#define  CV_BGR2Lab     44
#define  CV_RGB2Lab     45

#define  CV_BayerBG2BGR 46
#define  CV_BayerGB2BGR 47
#define  CV_BayerRG2BGR 48
#define  CV_BayerGR2BGR 49

#define  CV_BayerBG2RGB CV_BayerRG2BGR
#define  CV_BayerGB2RGB CV_BayerGR2BGR
#define  CV_BayerRG2RGB CV_BayerBG2BGR
#define  CV_BayerGR2RGB CV_BayerGB2BGR

#define  CV_BGR2Luv     50
#define  CV_RGB2Luv     51
#define  CV_BGR2HLS     52
#define  CV_RGB2HLS     53

#define  CV_HSV2BGR     54
#define  CV_HSV2RGB     55

#define  CV_Lab2BGR     56
#define  CV_Lab2RGB     57
#define  CV_Luv2BGR     58
#define  CV_Luv2RGB     59
#define  CV_HLS2BGR     60
#define  CV_HLS2RGB     61

#define fix(x,n)      (int)((x)*(1 << (n)) + 0.5)
#define descale       CV_DESCALE

#define cscGr_32f  0.299f
#define cscGg_32f  0.587f
#define cscGb_32f  0.114f

/* BGR/RGB -> Gray */
#define csc_shift  14
#define cscGr  fix(cscGr_32f,csc_shift) 
#define cscGg  fix(cscGg_32f,csc_shift)
#define cscGb  /*fix(cscGb_32f,csc_shift)*/ ((1 << csc_shift) - cscGr - cscGg)

#define yuvYr_32f cscGr_32f
#define yuvYg_32f cscGg_32f
#define yuvYb_32f cscGb_32f
#define yuvCr_32f 0.713f
#define yuvCb_32f 0.564f

#define yuv_shift 14
#define yuvYr  fix(yuvYr_32f,yuv_shift)
#define yuvYg  fix(yuvYg_32f,yuv_shift)
#define yuvYb  fix(yuvYb_32f,yuv_shift)
#define yuvCr  fix(yuvCr_32f,yuv_shift)
#define yuvCb  fix(yuvCb_32f,yuv_shift)

#define yuv_descale(x)  CV_DESCALE((x), yuv_shift)
#define yuv_prescale(x) ((x) << yuv_shift)

#define  yuvRCr_32f   1.403f
#define  yuvGCr_32f   (-0.714f)
#define  yuvGCb_32f   (-0.344f)
#define  yuvBCb_32f   1.773f

#define  yuvRCr   fix(yuvRCr_32f,yuv_shift)
#define  yuvGCr   (-fix(-yuvGCr_32f,yuv_shift))
#define  yuvGCb   (-fix(-yuvGCb_32f,yuv_shift))
#define  yuvBCb   fix(yuvBCb_32f,yuv_shift)

/////////////////////////////////////////////
#define xyzXr_32f  0.412453f
#define xyzXg_32f  0.357580f
#define xyzXb_32f  0.180423f

#define xyzYr_32f  0.212671f
#define xyzYg_32f  0.715160f
#define xyzYb_32f  0.072169f

#define xyzZr_32f  0.019334f
#define xyzZg_32f  0.119193f
#define xyzZb_32f  0.950227f

#define xyzRx_32f  3.240479f
#define xyzRy_32f  (-1.53715f)
#define xyzRz_32f  (-0.498535f)

#define xyzGx_32f  (-0.969256f)
#define xyzGy_32f  1.875991f
#define xyzGz_32f  0.041556f

#define xyzBx_32f  0.055648f
#define xyzBy_32f  (-0.204043f)
#define xyzBz_32f  1.057311f

#define xyz_shift  10
#define xyzXr_32s  fix(xyzXr_32f, xyz_shift )
#define xyzXg_32s  fix(xyzXg_32f, xyz_shift )
#define xyzXb_32s  fix(xyzXb_32f, xyz_shift )

#define xyzYr_32s  fix(xyzYr_32f, xyz_shift )
#define xyzYg_32s  fix(xyzYg_32f, xyz_shift )
#define xyzYb_32s  fix(xyzYb_32f, xyz_shift )

#define xyzZr_32s  fix(xyzZr_32f, xyz_shift )
#define xyzZg_32s  fix(xyzZg_32f, xyz_shift )
#define xyzZb_32s  fix(xyzZb_32f, xyz_shift )

#define xyzRx_32s  fix(3.240479f, xyz_shift )
#define xyzRy_32s  -fix(1.53715f, xyz_shift )
#define xyzRz_32s  -fix(0.498535f, xyz_shift )

#define xyzGx_32s  -fix(0.969256f, xyz_shift )
#define xyzGy_32s  fix(1.875991f, xyz_shift )
#define xyzGz_32s  fix(0.041556f, xyz_shift )

#define xyzBx_32s  fix(0.055648f, xyz_shift )
#define xyzBy_32s  -fix(0.204043f, xyz_shift )
#define xyzBz_32s  fix(1.057311f, xyz_shift )

#define xyz_descale(x) CV_DESCALE((x),xyz_shift)

extern const uchar icvSaturate8u_cv[];
#define CV_FAST_CAST_8U(t)  (assert(-256 <= (t) || (t) <= 512), icvSaturate8u_cv[(t)+256])
#define CV_CALC_MIN_8U(a,b) (a) -= CV_FAST_CAST_8U((a) - (b))
#define CV_CALC_MAX_8U(a,b) (a) += CV_FAST_CAST_8U((b) - (a))

typedef union Cv32suf
{
    int i;
    unsigned u;
    float f;
}
Cv32suf;


inline  CvScalar  cvRealScalar( double val0 )
{
    CvScalar scalar;
    scalar.val[0] = val0;
    scalar.val[1] = scalar.val[2] = scalar.val[3] = 0;
    return scalar;
}

inline  CvScalar  cvScalarAll( double val0123 )
{
    CvScalar scalar;
    scalar.val[0] = val0123;
    scalar.val[1] = val0123;
    scalar.val[2] = val0123;
    scalar.val[3] = val0123;
    return scalar;
}

#define SMALL_GAUSSIAN_SIZE  7

typedef void (*CvRowFilterFunc)( const uchar* src, uchar* dst, void* params );
typedef void (*CvColumnFilterFunc)( uchar** src, uchar* dst, int dst_step, int count, void* params );
#define IPL_BORDER_REFLECT_101    4
#define CV_RGB( r, g, b )  cvScalar( (b), (g), (r), 0 )
#define CV_AA 16
#define  cvUnsupportedFormat "Unsupported format"
#define  XY_SHIFT  16
#define  XY_ONE    (1 << XY_SHIFT)

/* Line iterator state */
typedef struct CvLineIterator
{
    /* pointer to the current point */
    uchar* ptr;

    /* Bresenham algorithm state */
    int  err;
    int  plus_delta;
    int  minus_delta;
    int  plus_step;
    int  minus_step;
}
CvLineIterator;

/* Moves iterator to the next line point */
#define CV_NEXT_LINE_POINT( line_iterator )                     \
{                                                               \
    int _line_iterator_mask = (line_iterator).err < 0 ? -1 : 0; \
    (line_iterator).err += (line_iterator).minus_delta +        \
        ((line_iterator).plus_delta & _line_iterator_mask);     \
    (line_iterator).ptr += (line_iterator).minus_step +         \
        ((line_iterator).plus_step & _line_iterator_mask);      \
}

#define cvInvSqrt(value) ((float)(1./sqrt(value)))
#define cvSqrt(value)  ((float)sqrt(value))

/* helper macros: filling horizontal row */
#define ICV_HLINE( ptr, xl, xr, color, pix_size )            \
{                                                            \
    uchar* hline_ptr = (uchar*)(ptr) + (xl)*(pix_size);      \
    uchar* hline_max_ptr = (uchar*)(ptr) + (xr)*(pix_size);  \
                                                             \
    for( ; hline_ptr <= hline_max_ptr; hline_ptr += (pix_size))\
    {                                                        \
        int hline_j;                                         \
        for( hline_j = 0; hline_j < (pix_size); hline_j++ )  \
        {                                                    \
            hline_ptr[hline_j] = ((uchar*)color)[hline_j];   \
        }                                                    \
    }                                                        \
}

#define CV_MEMCPY_CHAR( dst, src, len )                                             \
{                                                                                   \
    size_t _icv_memcpy_i_, _icv_memcpy_len_ = (len);                                \
    char* _icv_memcpy_dst_ = (char*)(dst);                                          \
    const char* _icv_memcpy_src_ = (const char*)(src);                              \
                                                                                    \
    for( _icv_memcpy_i_ = 0; _icv_memcpy_i_ < _icv_memcpy_len_; _icv_memcpy_i_++ )  \
        _icv_memcpy_dst_[_icv_memcpy_i_] = _icv_memcpy_src_[_icv_memcpy_i_];        \
}

#define CV_CONTOUR_FIELDS()  \
    CV_SEQUENCE_FIELDS()     \
    CvRect rect;             \
    int color;               \
    int reserved[3];

typedef struct CvContour
{
    CV_CONTOUR_FIELDS()
}
CvContour;

#define CV_DRAWING_STORAGE_BLOCK ((1 << 12) - 256)

typedef struct CvPolyEdge
{
    int x, dx;
    union
    {
        struct CvPolyEdge *next;
        int y0;
    };
    int y1;
}
CvPolyEdge;

#define CV_CURRENT_POINT( reader )  (*((CvPoint*)((reader).ptr)))
#define CV_PREV_POINT( reader )     (*((CvPoint*)((reader).prev_elem)))

#define CV_READ_EDGE( pt1, pt2, reader )               \
{                                                      \
    assert( sizeof(pt1) == sizeof(CvPoint) &&          \
            sizeof(pt2) == sizeof(CvPoint) &&          \
            reader.seq->elem_size == sizeof(CvPoint)); \
    (pt1) = CV_PREV_POINT( reader );                   \
    (pt2) = CV_CURRENT_POINT( reader );                \
    (reader).prev_elem = (reader).ptr;                 \
    CV_NEXT_SEQ_ELEM( sizeof(CvPoint), (reader));      \
}

#define CV_WRITE_SEQ_ELEM( elem, writer )             \
{                                                     \
    assert( (writer).seq->elem_size == sizeof(elem)); \
    if( (writer).ptr >= (writer).block_max )          \
    {                                                 \
        cvCreateSeqBlock( &writer);                   \
    }                                                 \
    assert( (writer).ptr <= (writer).block_max - sizeof(elem));\
    memcpy((writer).ptr, &(elem), sizeof(elem));      \
    (writer).ptr += sizeof(elem);                     \
}

#define ICV_WARP_CLIP_X(x)      ((unsigned)(x) < (unsigned)ssize.width ? \
                                (x) : (x) < 0 ? 0 : ssize.width - 1)
#define ICV_WARP_CLIP_Y(y)      ((unsigned)(y) < (unsigned)ssize.height ? \
                                (y) : (y) < 0 ? 0 : ssize.height - 1)

#define  CV_WARP_INVERSE_MAP  16
#define  CV_WARP_FILL_OUTLIERS 8


#endif

CvSize  cvSize( int width, int height );
CvMat* cvCreateMat( int height, int width, int type );
CvMat* cvInitMatHeader( CvMat* arr, int rows, int cols,int type, void* data, int step );
void cvReleaseMat( CvMat** array );
CvMat cvMat( int rows, int cols, int type, void* data);
void cvSetZero( CvArr* arr );
void  cvmSet( CvMat* mat, int row, int col, double value );
void cvSVD( CvArr* aarr, CvArr* warr, CvArr* uarr, CvArr* varr, int flags );
CvMat*  cvGetRow( const CvArr* arr, CvMat* submat, int row );
void cvCopy( const void* srcarr, void* dstarr, const void* maskarr );
void cvConvertScale( const void* srcarr, void* dstarr, double scale, double shift );
int cvSolve( const CvArr* A, const CvArr* b, CvArr* x, int method );
CvPoint2D64f  cvPoint2D64f( double x, double y );
void cvGEMM( const CvArr* Aarr, const CvArr* Barr, double alpha,const CvArr* Carr, double beta, CvArr* Darr, int flags );
CvRect  cvRect( int x, int y, int width, int height );
static void icvInitMemStorage( CvMemStorage* storage, int block_size );
CvMemStorage* cvCreateMemStorage( int block_size );
CvMemStorage * cvCreateChildMemStorage( CvMemStorage * parent );
static void icvDestroyMemStorage( CvMemStorage* storage );
void cvReleaseMemStorage( CvMemStorage** storage );
void cvClearMemStorage( CvMemStorage * storage );
static void icvGoNextMemBlock( CvMemStorage * storage );
void cvSaveMemStoragePos( const CvMemStorage * storage, CvMemStoragePos * pos );
void cvRestoreMemStoragePos( CvMemStorage * storage, CvMemStoragePos * pos );
void* cvMemStorageAlloc( CvMemStorage* storage, size_t size );
void cvSeqSort( CvSeq* seq, CvCmpFunc cmp_func, void* aux );
CvSeq * cvCreateSeq( int seq_flags, int header_size, int elem_size, CvMemStorage * storage );
void cvSetSeqBlockSize( CvSeq *seq, int delta_elements );
char* cvGetSeqElem( const CvSeq *seq, int index );
int cvSeqElemIdx( const CvSeq* seq, const void* _element, CvSeqBlock** _block );
int cvSliceLength( CvSlice slice, const CvSeq* seq );
void* cvCvtSeqToArray( const CvSeq *seq, void *array, CvSlice slice );
CvSeq* cvMakeSeqHeaderForArray( int seq_flags, int header_size, int elem_size,void *array, int total, CvSeq *seq, CvSeqBlock * block );
static void icvGrowSeq( CvSeq *seq, int in_front_of );
static void icvFreeSeqBlock( CvSeq *seq, int in_front_of );
CvSlice  cvSlice( int start, int end );
void cvResize( const CvArr* srcarr, CvArr* dstarr, int method );
CvSize cvGetSize( const CvArr* arr );
CvMat* cvCreateMatHeader( int rows, int cols, int type );
int cvInitNArrayIterator( int count, CvArr** arrs,const CvArr* mask, CvMatND* stubs,CvNArrayIterator* iterator, int flags );
int  cvNextNArraySlice( CvNArrayIterator* iterator );
void cvClearSet( CvSet* set );
CvMat* cvGetMat( const CvArr* array, CvMat* mat,int* pCOI, int allowND );
 CvSize  cvGetMatSize( const CvMat* mat );
 void cvTranspose( const CvArr* srcarr, CvArr* dstarr );
 static void icvSVD_32f( float* a, int lda, int m, int n, float* w, float* uT, int lduT, int nu, float* vT, int ldvT, float* buffer );
 static void icvSVD_64f( double* a, int lda, int m, int n, double* w, double* uT, int lduT, int nu, double* vT, int ldvT, double* buffer );
 CvMat* cvGetRows( const CvArr* arr, CvMat* submat,int start_row, int end_row, int delta_row );
 void cvSet( void* arr, CvScalar value, const void* maskarr );
 CvSparseNode* cvInitSparseMatIterator( const CvSparseMat* mat, CvSparseMatIterator* iterator );
 CvSparseNode* cvGetNextSparseNode( CvSparseMatIterator* mat_iterator );
 CvSetElem* cvSetNew( CvSet* set_header );
 void cvSplit( const CvArr* src, CvArr* dst0, CvArr* dst1, CvArr* dst2, CvArr* dst3 );
 void cvMerge( const CvArr* src0, const CvArr* src1,const CvArr* src2, const CvArr* src3,CvArr* dst );
int  cvRound( double value );
void cvSVBkSb( const CvArr* W, const CvArr* U,const CvArr* V, const CvArr* B,CvArr* X, int flags );
int cvAlignLeft( int size, int align );
void cvStartReadSeq( const CvSeq *seq, CvSeqReader * reader, int reverse );
void cvChangeSeqBlock( void* _reader, int direction );
int cvGetSeqReaderPos( CvSeqReader* reader );
void cvSetSeqReaderPos( CvSeqReader* reader, int index, int is_relative );
void icvInitCubicCoeffTab();
void cvClearSeq( CvSeq *seq );
void cvScalarToRawData( const CvScalar* scalar, void* data, int type, int extend_to_12 );
int cvSetAdd( CvSet* set, CvSetElem* element, CvSetElem** inserted_element );
void cvSeqPopMulti( CvSeq *seq, void *_elements, int count, int front );
void* cvClone( const void* struct_ptr );
CvTypeInfo* cvTypeOf( const void* struct_ptr );
void cvCvtColor( const CvArr* srcarr, CvArr* dstarr, int code );
float  cvCbrt( float value );
IplImage* cvCloneImage( const IplImage* src );
void cvSub( const void* srcarr1, const void* srcarr2,void* dstarr, const void* maskarr );
char* cvSeqPush( CvSeq *seq, void *element );
void cvSeqPopFront( CvSeq *seq, void *element );
double cvInvert( const CvArr* srcarr, CvArr* dstarr, int method );
void cvSetIdentity( CvArr* array, CvScalar value );
int array_double( void** array, int n, int size );
void cvSmooth( const void* srcarr, void* dstarr, int smooth_type, int param1, int param2, double param3, double param4 );
void cvLUT( const void* srcarr, void* dstarr, const void* lutarr );
void cvEigenVV( CvArr* srcarr, CvArr* evectsarr, CvArr* evalsarr, double eps );
void cvEllipse( void *img, CvPoint center, CvSize axes, double angle, double start_angle, double end_angle, CvScalar color, int thickness, int line_type, int shift );
int cvEllipse2Poly( CvPoint center, CvSize axes, int angle, int arc_start, int arc_end, CvPoint* pts, int delta );
int cvClipLine( CvSize img_size, CvPoint* pt1, CvPoint* pt2 );
int cvInitLineIterator( const CvArr* img, CvPoint pt1, CvPoint pt2, CvLineIterator* iterator, int connectivity, int left_to_right );
void cvStartAppendToSeq( CvSeq *seq, CvSeqWriter * writer );
void cvCreateSeqBlock( CvSeqWriter * writer );
CvSeq * cvEndWriteSeq( CvSeqWriter * writer );
void cvFlushSeqWriter( CvSeqWriter * writer );
void cvLine( void* img, CvPoint pt1, CvPoint pt2, CvScalar color, int thickness, int line_type, int shift );
void cvSetImageROI( IplImage* image, CvRect rect );
void cvAdd( const void* srcarr1, const void* srcarr2, void* dstarr, const void* maskarr );
void cvResetImageROI( IplImage* image );
void icvPolyLine( CvMat* img, CvPoint *v, int count, int closed, const void* color, int thickness, int line_type, int shift );
void cvWarpPerspective( const CvArr* srcarr, CvArr* dstarr, const CvMat* matrix, int flags, CvScalar fillval );




