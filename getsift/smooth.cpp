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
#include "smooth.h"

static void default_x_filter_func( const uchar*, uchar*, void* )
{
}

static void default_y_filter_func( uchar**, uchar*, int, int, void* )
{
}

CvBaseImageFilter::CvBaseImageFilter()
{
    min_depth = CV_8U;
    buffer = 0;
    rows = 0;
    max_width = 0;
    x_func = default_x_filter_func;
    y_func = default_y_filter_func;
}


CvBaseImageFilter::CvBaseImageFilter( int _max_width, int _src_type, int _dst_type,
                                      bool _is_separable, CvSize _ksize, CvPoint _anchor,
                                      int _border_mode, CvScalar _border_value )
{
    min_depth = CV_8U;
    buffer = 0;
    rows = 0;
    max_width = 0;
    x_func = default_x_filter_func;
    y_func = default_y_filter_func;

    init( _max_width, _src_type, _dst_type, _is_separable,
          _ksize, _anchor, _border_mode, _border_value );
}


void CvBaseImageFilter::clear()
{
    cvFree( &buffer );
    rows = 0;
}


CvBaseImageFilter::~CvBaseImageFilter()
{
    clear();
}


void CvBaseImageFilter::get_work_params()
{
    int min_rows = max_ky*2 + 3, rows = MAX(min_rows,10), row_sz;
    int width = max_width, trow_sz = 0;

    if( is_separable )
    {
        int max_depth = MAX(CV_MAT_DEPTH(src_type), CV_MAT_DEPTH(dst_type));
        int max_cn = MAX(CV_MAT_CN(src_type), CV_MAT_CN(dst_type));
        max_depth = MAX( max_depth, min_depth );
        work_type = CV_MAKETYPE( max_depth, max_cn );
        trow_sz = cvAlign( (max_width + ksize.width - 1)*CV_ELEM_SIZE(src_type), ALIGN );
    }
    else
    {
        work_type = src_type;
        width += ksize.width - 1;
    }
    row_sz = cvAlign( width*CV_ELEM_SIZE(work_type), ALIGN );
    buf_size = rows*row_sz;
    buf_size = MIN( buf_size, 1 << 16 );
    buf_size = MAX( buf_size, min_rows*row_sz );
    max_rows = (buf_size/row_sz)*3 + max_ky*2 + 8;
    buf_size += trow_sz;
}


void CvBaseImageFilter::init( int _max_width, int _src_type, int _dst_type,
                              bool _is_separable, CvSize _ksize, CvPoint _anchor,
                              int _border_mode, CvScalar _border_value )
{
    CV_FUNCNAME( "CvBaseImageFilter::init" );

    __BEGIN__;

    int total_buf_sz, src_pix_sz, row_tab_sz, bsz;
    uchar* ptr;

    if( !(buffer && _max_width <= max_width && _src_type == src_type &&
        _dst_type == dst_type && _is_separable == is_separable &&
        _ksize.width == ksize.width && _ksize.height == ksize.height &&
        _anchor.x == anchor.x && _anchor.y == anchor.y) )
        clear();

    is_separable = _is_separable != 0;
    max_width = _max_width; //MAX(_max_width,_ksize.width);
    src_type = CV_MAT_TYPE(_src_type);
    dst_type = CV_MAT_TYPE(_dst_type);
    ksize = _ksize;
    anchor = _anchor;

    if( anchor.x == -1 )
        anchor.x = ksize.width / 2;
    if( anchor.y == -1 )
        anchor.y = ksize.height / 2;

    max_ky = MAX( anchor.y, ksize.height - anchor.y - 1 ); 
    border_mode = _border_mode;
    border_value = _border_value;

    if( ksize.width <= 0 || ksize.height <= 0 ||
        (unsigned)anchor.x >= (unsigned)ksize.width ||
        (unsigned)anchor.y >= (unsigned)ksize.height )
        CV_ERROR( CV_StsOutOfRange, "invalid kernel size and/or anchor position" );

    if( border_mode != IPL_BORDER_CONSTANT && border_mode != IPL_BORDER_REPLICATE &&
        border_mode != IPL_BORDER_REFLECT && border_mode != IPL_BORDER_REFLECT_101 )
        CV_ERROR( CV_StsBadArg, "Invalid/unsupported border mode" );

    get_work_params();

    prev_width = 0;
    prev_x_range = cvSlice(0,0);

    buf_size = cvAlign( buf_size, ALIGN );

    src_pix_sz = CV_ELEM_SIZE(src_type);
    border_tab_sz1 = anchor.x*src_pix_sz;
    border_tab_sz = (ksize.width-1)*src_pix_sz;
    bsz = cvAlign( border_tab_sz*sizeof(int), ALIGN );

    assert( max_rows > max_ky*2 );
    row_tab_sz = cvAlign( max_rows*sizeof(uchar*), ALIGN );
    total_buf_sz = buf_size + row_tab_sz + bsz;
    
    CV_CALL( ptr = buffer = (uchar*)cvAlloc( total_buf_sz ));
    
    rows = (uchar**)ptr;
    ptr += row_tab_sz;
    border_tab = (int*)ptr;
    ptr += bsz;

    buf_start = ptr;
    const_row = 0;

    if( border_mode == IPL_BORDER_CONSTANT )
        cvScalarToRawData( &border_value, border_tab, src_type, 0 );

    __END__;
}


void CvBaseImageFilter::start_process( CvSlice x_range, int width )
{
    int mode = border_mode;
    int pix_sz = CV_ELEM_SIZE(src_type), work_pix_sz = CV_ELEM_SIZE(work_type);
    int bsz = buf_size, bw = x_range.end_index - x_range.start_index, bw1 = bw + ksize.width - 1;
    int tr_step = cvAlign(bw1*pix_sz, ALIGN );
    int i, j, k, ofs;
    
    if( x_range.start_index == prev_x_range.start_index &&
        x_range.end_index == prev_x_range.end_index &&
        width == prev_width )
        return;

    prev_x_range = x_range;
    prev_width = width;

    if( !is_separable )
        bw = bw1;
    else
        bsz -= tr_step;

    buf_step = cvAlign(bw*work_pix_sz, ALIGN);

    if( mode == IPL_BORDER_CONSTANT )
        bsz -= buf_step;
    buf_max_count = bsz/buf_step;
    buf_max_count = MIN( buf_max_count, max_rows - max_ky*2 );
    buf_end = buf_start + buf_max_count*buf_step;

    if( mode == IPL_BORDER_CONSTANT )
    {
        int i, tab_len = ksize.width*pix_sz;
        uchar* bt = (uchar*)border_tab;
        uchar* trow = buf_end;
        const_row = buf_end + (is_separable ? 1 : 0)*tr_step;

        for( i = pix_sz; i < tab_len; i++ )
            bt[i] = bt[i - pix_sz];
        for( i = 0; i < pix_sz; i++ )
            trow[i] = bt[i];
        for( i = pix_sz; i < tr_step; i++ )
            trow[i] = trow[i - pix_sz];
        if( is_separable )
            x_func( trow, const_row, this );
        return;
    }

    if( x_range.end_index - x_range.start_index <= 1 )
        mode = IPL_BORDER_REPLICATE;

    width = (width - 1)*pix_sz;
    ofs = (anchor.x-x_range.start_index)*pix_sz;

    for( k = 0; k < 2; k++ )
    {
        int idx, delta;
        int i1, i2, di;

        if( k == 0 )
        {
            idx = (x_range.start_index - 1)*pix_sz;
            delta = di = -pix_sz;
            i1 = border_tab_sz1 - pix_sz;
            i2 = -pix_sz;
        }
        else
        {
            idx = x_range.end_index*pix_sz;
            delta = di = pix_sz;
            i1 = border_tab_sz1;
            i2 = border_tab_sz;
        }

        if( (unsigned)idx > (unsigned)width )
        {
            int shift = mode == IPL_BORDER_REFLECT_101 ? pix_sz : 0;
            idx = k == 0 ? shift : width - shift;
            delta = -delta;
        }

        for( i = i1; i != i2; i += di )
        {
            for( j = 0; j < pix_sz; j++ )
                border_tab[i + j] = idx + ofs + j;
            if( mode != IPL_BORDER_REPLICATE )
            {
                if( delta > 0 && idx == width ||
                    delta < 0 && idx == 0 )
                {
                    if( mode == IPL_BORDER_REFLECT_101 )
                        idx -= delta*2;
                    delta = -delta;
                }
                else
                    idx += delta;
            }
        }
    }
}


void CvBaseImageFilter::make_y_border( int row_count, int top_rows, int bottom_rows )
{
    int i;
    
    if( border_mode == IPL_BORDER_CONSTANT ||
        border_mode == IPL_BORDER_REPLICATE )
    {
        uchar* row1 = border_mode == IPL_BORDER_CONSTANT ? const_row : rows[max_ky];
        
        for( i = 0; i < top_rows && rows[i] == 0; i++ )
            rows[i] = row1;

        row1 = border_mode == IPL_BORDER_CONSTANT ? const_row : rows[row_count-1];
        for( i = 0; i < bottom_rows; i++ )
            rows[i + row_count] = row1;
    }
    else
    {
        int j, dj = 1, shift = border_mode == IPL_BORDER_REFLECT_101;

        for( i = top_rows-1, j = top_rows+shift; i >= 0; i-- )
        {
            if( rows[i] == 0 )
                rows[i] = rows[j];
            j += dj;
            if( dj > 0 && j >= row_count )
            {
                if( !bottom_rows )
                    break;
                j -= 1 + shift;
                dj = -dj;
            }
        }

        for( i = 0, j = row_count-1-shift; i < bottom_rows; i++, j-- )
            rows[i + row_count] = rows[j];
    }
}


int CvBaseImageFilter::fill_cyclic_buffer( const uchar* src, int src_step,
                                           int y0, int y1, int y2 )
{
    int i, y = y0, bsz1 = border_tab_sz1, bsz = border_tab_sz;
    int pix_size = CV_ELEM_SIZE(src_type);
    int width = prev_x_range.end_index - prev_x_range.start_index, width_n = width*pix_size;
    bool can_use_src_as_trow = is_separable && width >= ksize.width;

    // fill the cyclic buffer
    for( ; buf_count < buf_max_count && y < y2; buf_count++, y++, src += src_step )
    {
        uchar* trow = is_separable ? buf_end : buf_tail;
        uchar* bptr = can_use_src_as_trow && y1 < y && y+1 < y2 ? (uchar*)(src - bsz1) : trow;

        if( bptr != trow )
        {
            for( i = 0; i < bsz1; i++ )
                trow[i] = bptr[i];
            for( ; i < bsz; i++ )
                trow[i] = bptr[i + width_n];
        }
        else if( !(((size_t)(bptr + bsz1)|(size_t)src|width_n) & (sizeof(int)-1)) )
            for( i = 0; i < width_n; i += sizeof(int) )
                *(int*)(bptr + i + bsz1) = *(int*)(src + i);
        else
            for( i = 0; i < width_n; i++ )
                bptr[i + bsz1] = src[i];

        if( border_mode != IPL_BORDER_CONSTANT )
        {
            for( i = 0; i < bsz1; i++ )
            {
                int j = border_tab[i];
                bptr[i] = bptr[j];
            }
            for( ; i < bsz; i++ )
            {
                int j = border_tab[i];
                bptr[i + width_n] = bptr[j];
            }
        }
        else
        {
            const uchar *bt = (uchar*)border_tab; 
            for( i = 0; i < bsz1; i++ )
                bptr[i] = bt[i];

            for( ; i < bsz; i++ )
                bptr[i + width_n] = bt[i];
        }

        if( is_separable )
        {
            x_func( bptr, buf_tail, this );
            if( bptr != trow )
            {
                for( i = 0; i < bsz1; i++ )
                    bptr[i] = trow[i];
                for( ; i < bsz; i++ )
                    bptr[i + width_n] = trow[i];
            }
        }

        buf_tail += buf_step;
        if( buf_tail >= buf_end )
            buf_tail = buf_start;
    }

    return y - y0;
}

int CvBaseImageFilter::process( const CvMat* src, CvMat* dst,
                                CvRect src_roi, CvPoint dst_origin, int flags )
{
    int rows_processed = 0;

    /*
        check_parameters
        initialize_horizontal_border_reloc_tab_if_not_initialized_yet
        
        for_each_source_row: src starts from src_roi.y, buf starts with the first available row
            1) if separable,
                   1a.1) copy source row to temporary buffer, form a border using border reloc tab.
                   1a.2) apply row-wise filter (symmetric, asymmetric or generic)
               else
                   1b.1) copy source row to the buffer, form a border
            2) if the buffer is full, or it is the last source row:
                 2.1) if stage != middle, form the pointers to other "virtual" rows.
                 if separable
                    2a.2) apply column-wise filter, store the results.
                 else
                    2b.2) form a sparse (offset,weight) tab
                    2b.3) apply generic non-separable filter, store the results
            3) update row pointers etc.
    */

    CV_FUNCNAME( "CvBaseImageFilter::process" );

    __BEGIN__;

    int i, width, _src_y1, _src_y2;
    int src_x, src_y, src_y1, src_y2, dst_y;
    int pix_size = CV_ELEM_SIZE(src_type);
    uchar *sptr = 0, *dptr;
    int phase = flags & (CV_START|CV_END|CV_MIDDLE);
    bool isolated_roi = (flags & CV_ISOLATED_ROI) != 0;

    if( !CV_IS_MAT(src) )
        CV_ERROR( CV_StsBadArg, "" );

    if( CV_MAT_TYPE(src->type) != src_type )
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    width = src->cols;

    if( src_roi.width == -1 && src_roi.x == 0 )
        src_roi.width = width;

    if( src_roi.height == -1 && src_roi.y == 0 )
    {
        src_roi.y = 0;
        src_roi.height = src->rows;
    }

    if( src_roi.width > max_width ||
        src_roi.x < 0 || src_roi.width < 0 ||
        src_roi.y < 0 || src_roi.height < 0 ||
        src_roi.x + src_roi.width > width ||
        src_roi.y + src_roi.height > src->rows )
        CV_ERROR( CV_StsOutOfRange, "Too large source image or its ROI" );

    src_x = src_roi.x;
    _src_y1 = 0;
    _src_y2 = src->rows;

    if( isolated_roi )
    {
        src_roi.x = 0;
        width = src_roi.width;
        _src_y1 = src_roi.y;
        _src_y2 = src_roi.y + src_roi.height;
    }

    if( !CV_IS_MAT(dst) )
        CV_ERROR( CV_StsBadArg, "" );

    if( CV_MAT_TYPE(dst->type) != dst_type )
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    if( dst_origin.x < 0 || dst_origin.y < 0 )
        CV_ERROR( CV_StsOutOfRange, "Incorrect destination ROI origin" );

    if( phase == CV_WHOLE )
        phase = CV_START | CV_END;
    phase &= CV_START | CV_END | CV_MIDDLE;

    // initialize horizontal border relocation tab if it is not initialized yet
    if( phase & CV_START )
        start_process( cvSlice(src_roi.x, src_roi.x + src_roi.width), width );
    else if( prev_width != width || prev_x_range.start_index != src_roi.x ||
        prev_x_range.end_index != src_roi.x + src_roi.width )
        CV_ERROR( CV_StsBadArg,
            "In a middle or at the end the horizontal placement of the stripe can not be changed" );

    dst_y = dst_origin.y;
    src_y1 = src_roi.y;
    src_y2 = src_roi.y + src_roi.height;

    if( phase & CV_START )
    {
        for( i = 0; i <= max_ky*2; i++ )
            rows[i] = 0;
        
        src_y1 -= max_ky;
        top_rows = bottom_rows = 0;

        if( src_y1 < _src_y1 )
        {
            top_rows = _src_y1 - src_y1;
            src_y1 = _src_y1;
        }

        buf_head = buf_tail = buf_start;
        buf_count = 0;
    }

    if( phase & CV_END )
    {
        src_y2 += max_ky;

        if( src_y2 > _src_y2 )
        {
            bottom_rows = src_y2 - _src_y2;
            src_y2 = _src_y2;
        }
    }
    
    dptr = dst->data.ptr + dst_origin.y*dst->step + dst_origin.x*CV_ELEM_SIZE(dst_type);
    sptr = src->data.ptr + src_y1*src->step + src_x*pix_size;
        
    for( src_y = src_y1; src_y < src_y2; )
    {
        uchar* bptr;
        int row_count, delta;

        delta = fill_cyclic_buffer( sptr, src->step, src_y, src_y1, src_y2 );

        src_y += delta;
        sptr += src->step*delta;

        // initialize the cyclic buffer row pointers
        bptr = buf_head;
        for( i = 0; i < buf_count; i++ )
        {
            rows[i+top_rows] = bptr;
            bptr += buf_step;
            if( bptr >= buf_end )
                bptr = buf_start;
        }

        row_count = top_rows + buf_count;
        
        if( !rows[0] || (phase & CV_END) && src_y == src_y2 )
        {
            int br = (phase & CV_END) && src_y == src_y2 ? bottom_rows : 0;
            make_y_border( row_count, top_rows, br );
            row_count += br;
        }

        if( rows[0] && row_count > max_ky*2 )
        {
            int count = row_count - max_ky*2;
            if( dst_y + count > dst->rows )
                CV_ERROR( CV_StsOutOfRange, "The destination image can not fit the result" );

            assert( count >= 0 );
            y_func( rows + max_ky - anchor.y, dptr, dst->step, count, this );
            row_count -= count;
            dst_y += count;
            dptr += dst->step*count;
            for( bptr = row_count > 0 ?rows[count] : 0; buf_head != bptr && buf_count > 0; buf_count-- )
            {
                buf_head += buf_step;
                if( buf_head >= buf_end )
                    buf_head = buf_start;
            }
            rows_processed += count;
            top_rows = MAX(top_rows - count, 0);
        }
    }

    __END__;

    return rows_processed;
}


void cvSmooth( const void* srcarr, void* dstarr, int smooth_type, int param1, int param2, double param3, double param4 )
{

    CvSepFilter gaussian_filter;
    CvMat* temp = 0;
    CV_FUNCNAME( "cvSmooth" );
    __BEGIN__;
    int coi1 = 0, coi2 = 0;
    CvMat srcstub, *src = (CvMat*)srcarr;
    CvMat dststub, *dst = (CvMat*)dstarr;
    CvSize size;
    int src_type, dst_type, depth, cn;
    double sigma1 = 0, sigma2 = 0;
   
    CV_CALL( src = cvGetMat( src, &srcstub, &coi1,0 ));
    CV_CALL( dst = cvGetMat( dst, &dststub, &coi2,0 ));

    if( coi1 != 0 || coi2 != 0 )
        CV_ERROR( CV_BadCOI, "" );

    src_type = CV_MAT_TYPE( src->type );
    dst_type = CV_MAT_TYPE( dst->type );
    depth = CV_MAT_DEPTH(src_type);
    cn = CV_MAT_CN(src_type);
    size = cvGetMatSize(src);

    if( !CV_ARE_SIZES_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedSizes, "" );
   
        // automatic detection of kernel size from sigma
        if( smooth_type == CV_GAUSSIAN )
        {
            sigma1 = param3;
            sigma2 = param4 ? param4 : param3;

            if( param1 == 0 && sigma1 > 0 )
                param1 = cvRound(sigma1*(depth == CV_8U ? 3 : 4)*2 + 1)|1;
            if( param2 == 0 && sigma2 > 0 )
                param2 = cvRound(sigma2*(depth == CV_8U ? 3 : 4)*2 + 1)|1;
        }

        if( param2 == 0 )
            param2 = size.height == 1 ? 1 : param1;
        if( param1 < 1 || (param1 & 1) == 0 || param2 < 1 || (param2 & 1) == 0 )
            CV_ERROR( CV_StsOutOfRange,
                "Both mask width and height must be >=1 and odd" );

        if( param1 == 1 && param2 == 1 )
        {
            cvConvert( src, dst );
            EXIT;
        }   

    if( smooth_type == CV_GAUSSIAN )
    {
        CvSize ksize = { param1, param2 };
        float* kx = (float*)cvStackAlloc( ksize.width*sizeof(kx[0]) );
        float* ky = (float*)cvStackAlloc( ksize.height*sizeof(ky[0]) );
        CvMat KX = cvMat( 1, ksize.width, CV_32F, kx );
        CvMat KY = cvMat( 1, ksize.height, CV_32F, ky );
        
        CvSepFilter::init_gaussian_kernel( &KX, sigma1 );
        if( ksize.width != ksize.height || fabs(sigma1 - sigma2) > FLT_EPSILON )
            CvSepFilter::init_gaussian_kernel( &KY, sigma2 );
        else
            KY.data.fl = kx;        

        CV_CALL( gaussian_filter.init( src->cols, src_type, dst_type, &KX, &KY ));
        CV_CALL( gaussian_filter.process( src, dst ));
    }    

    __END__;

    cvReleaseMat( &temp );
}

/****************************************************************************************\
                                    Separable Linear Filter
\****************************************************************************************/

static void icvFilterRowSymm_8u32s( const uchar* src, int* dst, void* params );
static void icvFilterColSymm_32s8u( const int** src, uchar* dst, int dst_step,
                                    int count, void* params );
static void icvFilterColSymm_32s16s( const int** src, short* dst, int dst_step,
                                     int count, void* params );
static void icvFilterRowSymm_8u32f( const uchar* src, float* dst, void* params );
static void icvFilterRow_8u32f( const uchar* src, float* dst, void* params );
static void icvFilterRowSymm_16s32f( const short* src, float* dst, void* params );
static void icvFilterRow_16s32f( const short* src, float* dst, void* params );
static void icvFilterRowSymm_16u32f( const ushort* src, float* dst, void* params );
static void icvFilterRow_16u32f( const ushort* src, float* dst, void* params );
static void icvFilterRowSymm_32f( const float* src, float* dst, void* params );
static void icvFilterRow_32f( const float* src, float* dst, void* params );

static void icvFilterColSymm_32f8u( const float** src, uchar* dst, int dst_step,
                                    int count, void* params );
static void icvFilterCol_32f8u( const float** src, uchar* dst, int dst_step,
                                int count, void* params );
static void icvFilterColSymm_32f16s( const float** src, short* dst, int dst_step,
                                     int count, void* params );
static void icvFilterCol_32f16s( const float** src, short* dst, int dst_step,
                                 int count, void* params );
static void icvFilterColSymm_32f16u( const float** src, ushort* dst, int dst_step,
                                     int count, void* params );
static void icvFilterCol_32f16u( const float** src, ushort* dst, int dst_step,
                                 int count, void* params );
static void icvFilterColSymm_32f( const float** src, float* dst, int dst_step,
                                  int count, void* params );
static void icvFilterCol_32f( const float** src, float* dst, int dst_step,
                              int count, void* params );

CvSepFilter::CvSepFilter()
{
    min_depth = CV_32F;
    kx = ky = 0;
    kx_flags = ky_flags = 0;
}


CvSepFilter::CvSepFilter( int _max_width, int _src_type, int _dst_type,
                          const CvMat* _kx, const CvMat* _ky,
                          CvPoint _anchor, int _border_mode,
                          CvScalar _border_value )
{
    min_depth = CV_32F;
    kx = ky = 0;
    init( _max_width, _src_type, _dst_type, _kx, _ky, _anchor, _border_mode, _border_value );
}


void CvSepFilter::clear()
{
    cvReleaseMat( &kx );
    cvReleaseMat( &ky );
    CvBaseImageFilter::clear();
}


CvSepFilter::~CvSepFilter()
{
    clear();
}


#undef FILTER_BITS
#define FILTER_BITS 8

void CvSepFilter::init( int _max_width, int _src_type, int _dst_type,
                        const CvMat* _kx, const CvMat* _ky,
                        CvPoint _anchor, int _border_mode,
                        CvScalar _border_value )
{
    CV_FUNCNAME( "CvSepFilter::init" );

    __BEGIN__;

    CvSize _ksize;
    int filter_type;
    int i, xsz, ysz;
    int convert_filters = 0;
    double xsum = 0, ysum = 0;
    const float eps = FLT_EPSILON*100.f;

    if( !CV_IS_MAT(_kx) || !CV_IS_MAT(_ky) ||
        _kx->cols != 1 && _kx->rows != 1 ||
        _ky->cols != 1 && _ky->rows != 1 ||
        CV_MAT_CN(_kx->type) != 1 || CV_MAT_CN(_ky->type) != 1 ||
        !CV_ARE_TYPES_EQ(_kx,_ky) )
        CV_ERROR( CV_StsBadArg,
        "Both kernels must be valid 1d single-channel vectors of the same types" );

    if( CV_MAT_CN(_src_type) != CV_MAT_CN(_dst_type) )
        CV_ERROR( CV_StsUnmatchedFormats, "Input and output must have the same number of channels" );

    filter_type = MAX( CV_32F, CV_MAT_DEPTH(_kx->type) );

    _ksize.width = _kx->rows + _kx->cols - 1;
    _ksize.height = _ky->rows + _ky->cols - 1;

    CV_CALL( CvBaseImageFilter::init( _max_width, _src_type, _dst_type, 1, _ksize,
                                      _anchor, _border_mode, _border_value ));

    if( !(kx && CV_ARE_SIZES_EQ(kx,_kx)) )
    {
        cvReleaseMat( &kx );
        CV_CALL( kx = cvCreateMat( _kx->rows, _kx->cols, filter_type ));
    }

    if( !(ky && CV_ARE_SIZES_EQ(ky,_ky)) )
    {
        cvReleaseMat( &ky );
        CV_CALL( ky = cvCreateMat( _ky->rows, _ky->cols, filter_type ));
    }

    CV_CALL( cvConvert( _kx, kx ));
    CV_CALL( cvConvert( _ky, ky ));

    xsz = kx->rows + kx->cols - 1;
    ysz = ky->rows + ky->cols - 1;
    kx_flags = ky_flags = ASYMMETRICAL + SYMMETRICAL + POSITIVE + SUM_TO_1 + INTEGER;
    
    if( !(xsz & 1) )
        kx_flags &= ~(ASYMMETRICAL + SYMMETRICAL);
    if( !(ysz & 1) )
        ky_flags &= ~(ASYMMETRICAL + SYMMETRICAL);

    for( i = 0; i < xsz; i++ )
    {
        float v = kx->data.fl[i];
        xsum += v;
        if( v < 0 )
            kx_flags &= ~POSITIVE;
        if( fabs(v - cvRound(v)) > eps )
            kx_flags &= ~INTEGER;
        if( fabs(v - kx->data.fl[xsz - i - 1]) > eps )
            kx_flags &= ~SYMMETRICAL;
        if( fabs(v + kx->data.fl[xsz - i - 1]) > eps )
            kx_flags &= ~ASYMMETRICAL;
    }

    if( fabs(xsum - 1.) > eps )
        kx_flags &= ~SUM_TO_1;
    
    for( i = 0; i < ysz; i++ )
    {
        float v = ky->data.fl[i];
        ysum += v;
        if( v < 0 )
            ky_flags &= ~POSITIVE;
        if( fabs(v - cvRound(v)) > eps )
            ky_flags &= ~INTEGER;
        if( fabs(v - ky->data.fl[ysz - i - 1]) > eps )
            ky_flags &= ~SYMMETRICAL;
        if( fabs(v + ky->data.fl[ysz - i - 1]) > eps )
            ky_flags &= ~ASYMMETRICAL;
    }

    if( fabs(ysum - 1.) > eps )
        ky_flags &= ~SUM_TO_1;

    x_func = 0;
    y_func = 0;

    if( CV_MAT_DEPTH(src_type) == CV_8U )
    {
        if( CV_MAT_DEPTH(dst_type) == CV_8U &&
            ((kx_flags&ky_flags) & (SYMMETRICAL + POSITIVE + SUM_TO_1)) == SYMMETRICAL + POSITIVE + SUM_TO_1 )
        {
            x_func = (CvRowFilterFunc)icvFilterRowSymm_8u32s;
            y_func = (CvColumnFilterFunc)icvFilterColSymm_32s8u;
            kx_flags &= ~INTEGER;
            ky_flags &= ~INTEGER;
            convert_filters = 1;
        }
        else if( CV_MAT_DEPTH(dst_type) == CV_16S &&
            (kx_flags & (SYMMETRICAL + ASYMMETRICAL)) && (kx_flags & INTEGER) &&
            (ky_flags & (SYMMETRICAL + ASYMMETRICAL)) && (ky_flags & INTEGER) )
        {
            x_func = (CvRowFilterFunc)icvFilterRowSymm_8u32s;
            y_func = (CvColumnFilterFunc)icvFilterColSymm_32s16s;
            convert_filters = 1;
        }
        else
        {
            if( CV_MAT_DEPTH(dst_type) > CV_32F )
                CV_ERROR( CV_StsUnsupportedFormat, "8u->64f separable filtering is not supported" );

            if( kx_flags & (SYMMETRICAL + ASYMMETRICAL) )
                x_func = (CvRowFilterFunc)icvFilterRowSymm_8u32f;
            else
                x_func = (CvRowFilterFunc)icvFilterRow_8u32f;
        }
    }
    else if( CV_MAT_DEPTH(src_type) == CV_16U )
    {
        if( CV_MAT_DEPTH(dst_type) > CV_32F )
            CV_ERROR( CV_StsUnsupportedFormat, "16u->64f separable filtering is not supported" );

        if( kx_flags & (SYMMETRICAL + ASYMMETRICAL) )
            x_func = (CvRowFilterFunc)icvFilterRowSymm_16u32f;
        else
            x_func = (CvRowFilterFunc)icvFilterRow_16u32f;
    }
    else if( CV_MAT_DEPTH(src_type) == CV_16S )
    {
        if( CV_MAT_DEPTH(dst_type) > CV_32F )
            CV_ERROR( CV_StsUnsupportedFormat, "16s->64f separable filtering is not supported" );

        if( kx_flags & (SYMMETRICAL + ASYMMETRICAL) )
            x_func = (CvRowFilterFunc)icvFilterRowSymm_16s32f;
        else
            x_func = (CvRowFilterFunc)icvFilterRow_16s32f;
    }
    else if( CV_MAT_DEPTH(src_type) == CV_32F )
    {
        if( CV_MAT_DEPTH(dst_type) != CV_32F )
            CV_ERROR( CV_StsUnsupportedFormat, "When the input has 32f data type, the output must also have 32f type" );

        if( kx_flags & (SYMMETRICAL + ASYMMETRICAL) )
            x_func = (CvRowFilterFunc)icvFilterRowSymm_32f;
        else
            x_func = (CvRowFilterFunc)icvFilterRow_32f;
    }
    else
        CV_ERROR( CV_StsUnsupportedFormat, "Unknown or unsupported input data type" );

    if( !y_func )
    {
        if( CV_MAT_DEPTH(dst_type) == CV_8U )
        {
            if( ky_flags & (SYMMETRICAL + ASYMMETRICAL) )
                y_func = (CvColumnFilterFunc)icvFilterColSymm_32f8u;
            else
                y_func = (CvColumnFilterFunc)icvFilterCol_32f8u;
        }
        else if( CV_MAT_DEPTH(dst_type) == CV_16U )
        {
            if( ky_flags & (SYMMETRICAL + ASYMMETRICAL) )
                y_func = (CvColumnFilterFunc)icvFilterColSymm_32f16u;
            else
                y_func = (CvColumnFilterFunc)icvFilterCol_32f16u;
        }
        else if( CV_MAT_DEPTH(dst_type) == CV_16S )
        {
            if( ky_flags & (SYMMETRICAL + ASYMMETRICAL) )
                y_func = (CvColumnFilterFunc)icvFilterColSymm_32f16s;
            else
                y_func = (CvColumnFilterFunc)icvFilterCol_32f16s;
        }
        else if( CV_MAT_DEPTH(dst_type) == CV_32F )
        {
            if( ky_flags & (SYMMETRICAL + ASYMMETRICAL) )
                y_func = (CvColumnFilterFunc)icvFilterColSymm_32f;
            else
                y_func = (CvColumnFilterFunc)icvFilterCol_32f;
        }
        else
            CV_ERROR( CV_StsUnsupportedFormat, "Unknown or unsupported input data type" );
    }

    if( convert_filters )
    {
        int scale = kx_flags & ky_flags & INTEGER ? 1 : (1 << FILTER_BITS);
        int sum;
        
        for( i = sum = 0; i < xsz; i++ )
        {
            int t = cvRound(kx->data.fl[i]*scale);
            kx->data.i[i] = t;
            sum += t;
        }
        if( scale > 1 )
            kx->data.i[xsz/2] += scale - sum;

        for( i = sum = 0; i < ysz; i++ )
        {
            int t = cvRound(ky->data.fl[i]*scale);
            ky->data.i[i] = t;
            sum += t;
        }
        if( scale > 1 )
            ky->data.i[ysz/2] += scale - sum;
        kx->type = (kx->type & ~CV_MAT_DEPTH_MASK) | CV_32S;
        ky->type = (ky->type & ~CV_MAT_DEPTH_MASK) | CV_32S;
    }

    __END__;
}


void CvSepFilter::init( int _max_width, int _src_type, int _dst_type,
                        bool _is_separable, CvSize _ksize,
                        CvPoint _anchor, int _border_mode,
                        CvScalar _border_value )
{
    CvBaseImageFilter::init( _max_width, _src_type, _dst_type, _is_separable,
                             _ksize, _anchor, _border_mode, _border_value );
}


static void
icvFilterRowSymm_8u32s( const uchar* src, int* dst, void* params )
{
    const CvSepFilter* state = (const CvSepFilter*)params;
    const CvMat* _kx = state->get_x_kernel();
    const int* kx = _kx->data.i;
    int ksize = _kx->cols + _kx->rows - 1;
    int i = 0, j, k, width = state->get_width();
    int cn = CV_MAT_CN(state->get_src_type());
    int ksize2 = ksize/2, ksize2n = ksize2*cn;
    int is_symm = state->get_x_kernel_flags() & CvSepFilter::SYMMETRICAL;
    const uchar* s = src + ksize2n;

    kx += ksize2;
    width *= cn;

    if( is_symm )
    {
        if( ksize == 1 && kx[0] == 1 )
        {
            for( i = 0; i <= width - 2; i += 2 )
            {
                int s0 = s[i], s1 = s[i+1];
                dst[i] = s0; dst[i+1] = s1;
            }
            s += i;
        }
        else if( ksize == 3 )
        {
            if( kx[0] == 2 && kx[1] == 1 )
                for( ; i <= width - 2; i += 2, s += 2 )
                {
                    int s0 = s[-cn] + s[0]*2 + s[cn], s1 = s[1-cn] + s[1]*2 + s[1+cn];
                    dst[i] = s0; dst[i+1] = s1;
                }
            else if( kx[0] == 10 && kx[1] == 3 )
                for( ; i <= width - 2; i += 2, s += 2 )
                {
                    int s0 = s[0]*10 + (s[-cn] + s[cn])*3, s1 = s[1]*10 + (s[1-cn] + s[1+cn])*3;
                    dst[i] = s0; dst[i+1] = s1;
                }
            else if( kx[0] == 2*64 && kx[1] == 1*64 )
                for( ; i <= width - 2; i += 2, s += 2 )
                {
                    int s0 = (s[0]*2 + s[-cn] + s[cn]) << 6;
                    int s1 = (s[1]*2 + s[1-cn] + s[1+cn]) << 6;
                    dst[i] = s0; dst[i+1] = s1;
                }
            else
            {
                int k0 = kx[0], k1 = kx[1];
                for( ; i <= width - 2; i += 2, s += 2 )
                {
                    int s0 = s[0]*k0 + (s[-cn] + s[cn])*k1, s1 = s[1]*k0 + (s[1-cn] + s[1+cn])*k1;
                    dst[i] = s0; dst[i+1] = s1;
                }
            }
        }
        else if( ksize == 5 )
        {
            int k0 = kx[0], k1 = kx[1], k2 = kx[2];
            if( k0 == 6*16 && k1 == 4*16 && k2 == 1*16 )
                for( ; i <= width - 2; i += 2, s += 2 )
                {
                    int s0 = (s[0]*6 + (s[-cn] + s[cn])*4 + (s[-cn*2] + s[cn*2])*1) << 4;
                    int s1 = (s[1]*6 + (s[1-cn] + s[1+cn])*4 + (s[1-cn*2] + s[1+cn*2])*1) << 4;
                    dst[i] = s0; dst[i+1] = s1;
                }
            else
                for( ; i <= width - 2; i += 2, s += 2 )
                {
                    int s0 = s[0]*k0 + (s[-cn] + s[cn])*k1 + (s[-cn*2] + s[cn*2])*k2;
                    int s1 = s[1]*k0 + (s[1-cn] + s[1+cn])*k1 + (s[1-cn*2] + s[1+cn*2])*k2;
                    dst[i] = s0; dst[i+1] = s1;
                }
        }
        else
            for( ; i <= width - 4; i += 4, s += 4 )
            {
                int f = kx[0];
                int s0 = f*s[0], s1 = f*s[1], s2 = f*s[2], s3 = f*s[3];
                for( k = 1, j = cn; k <= ksize2; k++, j += cn )
                {
                    f = kx[k];
                    s0 += f*(s[j] + s[-j]); s1 += f*(s[j+1] + s[-j+1]);
                    s2 += f*(s[j+2] + s[-j+2]); s3 += f*(s[j+3] + s[-j+3]);
                }

                dst[i] = s0; dst[i+1] = s1;
                dst[i+2] = s2; dst[i+3] = s3;
            }

        for( ; i < width; i++, s++ )
        {
            int s0 = kx[0]*s[0];
            for( k = 1, j = cn; k <= ksize2; k++, j += cn )
                s0 += kx[k]*(s[j] + s[-j]);
            dst[i] = s0;
        }
    }
    else
    {
        if( ksize == 3 && kx[0] == 0 && kx[1] == 1 )
            for( ; i <= width - 2; i += 2, s += 2 )
            {
                int s0 = s[cn] - s[-cn], s1 = s[1+cn] - s[1-cn];
                dst[i] = s0; dst[i+1] = s1;
            }
        else
            for( ; i <= width - 4; i += 4, s += 4 )
            {
                int s0 = 0, s1 = 0, s2 = 0, s3 = 0;
                for( k = 1, j = cn; k <= ksize2; k++, j += cn )
                {
                    int f = kx[k];
                    s0 += f*(s[j] - s[-j]); s1 += f*(s[j+1] - s[-j+1]);
                    s2 += f*(s[j+2] - s[-j+2]); s3 += f*(s[j+3] - s[-j+3]);
                }

                dst[i] = s0; dst[i+1] = s1;
                dst[i+2] = s2; dst[i+3] = s3;
            }

        for( ; i < width; i++, s++ )
        {
            int s0 = kx[0]*s[0];
            for( k = 1, j = cn; k <= ksize2; k++, j += cn )
                s0 += kx[k]*(s[j] - s[-j]);
            dst[i] = s0;
        }
    }
}


#define ICV_FILTER_ROW( flavor, srctype, dsttype, load_macro )      \
static void                                                         \
icvFilterRow_##flavor(const srctype* src, dsttype* dst, void*params)\
{                                                                   \
    const CvSepFilter* state = (const CvSepFilter*)params;          \
    const CvMat* _kx = state->get_x_kernel();                       \
    const dsttype* kx = (const dsttype*)(_kx->data.ptr);            \
    int ksize = _kx->cols + _kx->rows - 1;                          \
    int i = 0, k, width = state->get_width();                       \
    int cn = CV_MAT_CN(state->get_src_type());                      \
    const srctype* s;                                               \
                                                                    \
    width *= cn;                                                    \
                                                                    \
    for( ; i <= width - 4; i += 4 )                                 \
    {                                                               \
        double f = kx[0];                                           \
        double s0=f*load_macro(src[i]), s1=f*load_macro(src[i+1]),  \
                s2=f*load_macro(src[i+2]), s3=f*load_macro(src[i+3]);\
        for( k = 1, s = src + i + cn; k < ksize; k++, s += cn )     \
        {                                                           \
            f = kx[k];                                              \
            s0 += f*load_macro(s[0]);                               \
            s1 += f*load_macro(s[1]);                               \
            s2 += f*load_macro(s[2]);                               \
            s3 += f*load_macro(s[3]);                               \
        }                                                           \
        dst[i] = (dsttype)s0; dst[i+1] = (dsttype)s1;               \
        dst[i+2] = (dsttype)s2; dst[i+3] = (dsttype)s3;             \
    }                                                               \
                                                                    \
    for( ; i < width; i++ )                                         \
    {                                                               \
        double s0 = (double)kx[0]*load_macro(src[i]);               \
        for( k = 1, s = src + i + cn; k < ksize; k++, s += cn )     \
            s0 += (double)kx[k]*load_macro(s[0]);                   \
        dst[i] = (dsttype)s0;                                       \
    }                                                               \
}


ICV_FILTER_ROW( 8u32f, uchar, float, CV_8TO32F )
ICV_FILTER_ROW( 16s32f, short, float, CV_NOP )
ICV_FILTER_ROW( 16u32f, ushort, float, CV_NOP )
ICV_FILTER_ROW( 32f, float, float, CV_NOP )


#define ICV_FILTER_ROW_SYMM( flavor, srctype, dsttype, load_macro ) \
static void                                                         \
icvFilterRowSymm_##flavor( const srctype* src,                      \
                           dsttype* dst, void* params )             \
{                                                                   \
    const CvSepFilter* state = (const CvSepFilter*)params;          \
    const CvMat* _kx = state->get_x_kernel();                       \
    const dsttype* kx = (const dsttype*)(_kx->data.ptr);            \
    int ksize = _kx->cols + _kx->rows - 1;                          \
    int i = 0, j, k, width = state->get_width();                    \
    int cn = CV_MAT_CN(state->get_src_type());                      \
    int is_symm=state->get_x_kernel_flags()&CvSepFilter::SYMMETRICAL;\
    int ksize2 = ksize/2, ksize2n = ksize2*cn;                      \
    const srctype* s = src + ksize2n;                               \
                                                                    \
    kx += ksize2;                                                   \
    width *= cn;                                                    \
                                                                    \
    if( is_symm )                                                   \
    {                                                               \
        for( ; i <= width - 4; i += 4, s += 4 )                     \
        {                                                           \
            double f = kx[0];                                       \
            double s0=f*load_macro(s[0]), s1=f*load_macro(s[1]),    \
                   s2=f*load_macro(s[2]), s3=f*load_macro(s[3]);    \
            for( k = 1, j = cn; k <= ksize2; k++, j += cn )         \
            {                                                       \
                f = kx[k];                                          \
                s0 += f*load_macro(s[j] + s[-j]);                   \
                s1 += f*load_macro(s[j+1] + s[-j+1]);               \
                s2 += f*load_macro(s[j+2] + s[-j+2]);               \
                s3 += f*load_macro(s[j+3] + s[-j+3]);               \
            }                                                       \
                                                                    \
            dst[i] = (dsttype)s0; dst[i+1] = (dsttype)s1;           \
            dst[i+2] = (dsttype)s2; dst[i+3] = (dsttype)s3;         \
        }                                                           \
                                                                    \
        for( ; i < width; i++, s++ )                                \
        {                                                           \
            double s0 = (double)kx[0]*load_macro(s[0]);             \
            for( k = 1, j = cn; k <= ksize2; k++, j += cn )         \
                s0 += (double)kx[k]*load_macro(s[j] + s[-j]);       \
            dst[i] = (dsttype)s0;                                   \
        }                                                           \
    }                                                               \
    else                                                            \
    {                                                               \
        for( ; i <= width - 4; i += 4, s += 4 )                     \
        {                                                           \
            double s0 = 0, s1 = 0, s2 = 0, s3 = 0;                  \
            for( k = 1, j = cn; k <= ksize2; k++, j += cn )         \
            {                                                       \
                double f = kx[k];                                   \
                s0 += f*load_macro(s[j] - s[-j]);                   \
                s1 += f*load_macro(s[j+1] - s[-j+1]);               \
                s2 += f*load_macro(s[j+2] - s[-j+2]);               \
                s3 += f*load_macro(s[j+3] - s[-j+3]);               \
            }                                                       \
                                                                    \
            dst[i] = (dsttype)s0; dst[i+1] = (dsttype)s1;           \
            dst[i+2] = (dsttype)s2; dst[i+3] = (dsttype)s3;         \
        }                                                           \
                                                                    \
        for( ; i < width; i++, s++ )                                \
        {                                                           \
            double s0 = 0;                                          \
            for( k = 1, j = cn; k <= ksize2; k++, j += cn )         \
                s0 += (double)kx[k]*load_macro(s[j] - s[-j]);       \
            dst[i] = (dsttype)s0;                                   \
        }                                                           \
    }                                                               \
}


ICV_FILTER_ROW_SYMM( 8u32f, uchar, float, CV_8TO32F )
ICV_FILTER_ROW_SYMM( 16s32f, short, float, CV_NOP )
ICV_FILTER_ROW_SYMM( 16u32f, ushort, float, CV_NOP )

static void
icvFilterRowSymm_32f( const float* src, float* dst, void* params )
{
    const CvSepFilter* state = (const CvSepFilter*)params;
    const CvMat* _kx = state->get_x_kernel();
    const float* kx = _kx->data.fl;
    int ksize = _kx->cols + _kx->rows - 1;
    int i = 0, j, k, width = state->get_width();
    int cn = CV_MAT_CN(state->get_src_type());
    int ksize2 = ksize/2, ksize2n = ksize2*cn;
    int is_symm = state->get_x_kernel_flags() & CvSepFilter::SYMMETRICAL;
    const float* s = src + ksize2n;

    kx += ksize2;
    width *= cn;

    if( is_symm )
    {
        if( ksize == 3 && fabs(kx[0]-2.) <= FLT_EPSILON && fabs(kx[1]-1.) <= FLT_EPSILON )
            for( ; i <= width - 2; i += 2, s += 2 )
            {
                float s0 = s[-cn] + s[0]*2 + s[cn], s1 = s[1-cn] + s[1]*2 + s[1+cn];
                dst[i] = s0; dst[i+1] = s1;
            }
        else if( ksize == 3 && fabs(kx[0]-10.) <= FLT_EPSILON && fabs(kx[1]-3.) <= FLT_EPSILON )
            for( ; i <= width - 2; i += 2, s += 2 )
            {
                float s0 = s[0]*10 + (s[-cn] + s[cn])*3, s1 = s[1]*10 + (s[1-cn] + s[1+cn])*3;
                dst[i] = s0; dst[i+1] = s1;
            }
        else
            for( ; i <= width - 4; i += 4, s += 4 )
            {
                double f = kx[0];
                double s0 = f*s[0], s1 = f*s[1], s2 = f*s[2], s3 = f*s[3];
                for( k = 1, j = cn; k <= ksize2; k++, j += cn )
                {
                    f = kx[k];
                    s0 += f*(s[j] + s[-j]); s1 += f*(s[j+1] + s[-j+1]);
                    s2 += f*(s[j+2] + s[-j+2]); s3 += f*(s[j+3] + s[-j+3]);
                }

                dst[i] = (float)s0; dst[i+1] = (float)s1;
                dst[i+2] = (float)s2; dst[i+3] = (float)s3;
            }

        for( ; i < width; i++, s++ )
        {
            double s0 = (double)kx[0]*s[0];
            for( k = 1, j = cn; k <= ksize2; k++, j += cn )
                s0 += (double)kx[k]*(s[j] + s[-j]);
            dst[i] = (float)s0;
        }
    }
    else
    {
        if( ksize == 3 && fabs(kx[0]) <= FLT_EPSILON && fabs(kx[1]-1.) <= FLT_EPSILON )
            for( ; i <= width - 2; i += 2, s += 2 )
            {
                float s0 = s[cn] - s[-cn], s1 = s[1+cn] - s[1-cn];
                dst[i] = s0; dst[i+1] = s1;
            }
        else
            for( ; i <= width - 4; i += 4, s += 4 )
            {
                double s0 = 0, s1 = 0, s2 = 0, s3 = 0;
                for( k = 1, j = cn; k <= ksize2; k++, j += cn )
                {
                    double f = kx[k];
                    s0 += f*(s[j] - s[-j]); s1 += f*(s[j+1] - s[-j+1]);
                    s2 += f*(s[j+2] - s[-j+2]); s3 += f*(s[j+3] - s[-j+3]);
                }

                dst[i] = (float)s0; dst[i+1] = (float)s1;
                dst[i+2] = (float)s2; dst[i+3] = (float)s3;
            }

        for( ; i < width; i++, s++ )
        {
            double s0 = (double)kx[0]*s[0];
            for( k = 1, j = cn; k <= ksize2; k++, j += cn )
                s0 += (double)kx[k]*(s[j] - s[-j]);
            dst[i] = (float)s0;
        }
    }
}


static void
icvFilterColSymm_32s8u( const int** src, uchar* dst, int dst_step, int count, void* params )
{
    const CvSepFilter* state = (const CvSepFilter*)params;
    const CvMat* _ky = state->get_y_kernel();
    const int* ky = _ky->data.i;
    int ksize = _ky->cols + _ky->rows - 1, ksize2 = ksize/2;
    int i, k, width = state->get_width();
    int cn = CV_MAT_CN(state->get_src_type());

    width *= cn;
    src += ksize2;
    ky += ksize2;

    for( ; count--; dst += dst_step, src++ )
    {
        if( ksize == 3 )
        {
            const int* sptr0 = src[-1], *sptr1 = src[0], *sptr2 = src[1];
            int k0 = ky[0], k1 = ky[1];
            for( i = 0; i <= width - 2; i += 2 )
            {
                int s0 = sptr1[i]*k0 + (sptr0[i] + sptr2[i])*k1;
                int s1 = sptr1[i+1]*k0 + (sptr0[i+1] + sptr2[i+1])*k1;
                s0 = CV_DESCALE(s0, FILTER_BITS*2);
                s1 = CV_DESCALE(s1, FILTER_BITS*2);
                dst[i] = (uchar)s0; dst[i+1] = (uchar)s1;
            }
        }
        else if( ksize == 5 )
        {
            const int* sptr0 = src[-2], *sptr1 = src[-1],
                *sptr2 = src[0], *sptr3 = src[1], *sptr4 = src[2];
            int k0 = ky[0], k1 = ky[1], k2 = ky[2];
            for( i = 0; i <= width - 2; i += 2 )
            {
                int s0 = sptr2[i]*k0 + (sptr1[i] + sptr3[i])*k1 + (sptr0[i] + sptr4[i])*k2;
                int s1 = sptr2[i+1]*k0 + (sptr1[i+1] + sptr3[i+1])*k1 + (sptr0[i+1] + sptr4[i+1])*k2;
                s0 = CV_DESCALE(s0, FILTER_BITS*2);
                s1 = CV_DESCALE(s1, FILTER_BITS*2);
                dst[i] = (uchar)s0; dst[i+1] = (uchar)s1;
            }
        }
        else
            for( i = 0; i <= width - 4; i += 4 )
            {
                int f = ky[0];
                const int* sptr = src[0] + i, *sptr2;
                int s0 = f*sptr[0], s1 = f*sptr[1], s2 = f*sptr[2], s3 = f*sptr[3];
                for( k = 1; k <= ksize2; k++ )
                {
                    sptr = src[k] + i;
                    sptr2 = src[-k] + i;
                    f = ky[k];
                    s0 += f*(sptr[0] + sptr2[0]);
                    s1 += f*(sptr[1] + sptr2[1]);
                    s2 += f*(sptr[2] + sptr2[2]);
                    s3 += f*(sptr[3] + sptr2[3]);
                }

                s0 = CV_DESCALE(s0, FILTER_BITS*2);
                s1 = CV_DESCALE(s1, FILTER_BITS*2);
                dst[i] = (uchar)s0; dst[i+1] = (uchar)s1;
                s2 = CV_DESCALE(s2, FILTER_BITS*2);
                s3 = CV_DESCALE(s3, FILTER_BITS*2);
                dst[i+2] = (uchar)s2; dst[i+3] = (uchar)s3;
            }

        for( ; i < width; i++ )
        {
            int s0 = ky[0]*src[0][i];
            for( k = 1; k <= ksize2; k++ )
                s0 += ky[k]*(src[k][i] + src[-k][i]);

            s0 = CV_DESCALE(s0, FILTER_BITS*2);
            dst[i] = (uchar)s0;
        }
    }
}


static void
icvFilterColSymm_32s16s( const int** src, short* dst,
                         int dst_step, int count, void* params )
{
    const CvSepFilter* state = (const CvSepFilter*)params;
    const CvMat* _ky = state->get_y_kernel();
    const int* ky = (const int*)_ky->data.ptr;
    int ksize = _ky->cols + _ky->rows - 1, ksize2 = ksize/2;
    int i = 0, k, width = state->get_width();
    int cn = CV_MAT_CN(state->get_src_type());
    int is_symm = state->get_y_kernel_flags() & CvSepFilter::SYMMETRICAL;
    int is_1_2_1 = is_symm && ksize == 3 && ky[1] == 2 && ky[2] == 1;
    int is_3_10_3 = is_symm && ksize == 3 && ky[1] == 10 && ky[2] == 3;
    int is_m1_0_1 = !is_symm && ksize == 3 && ky[1] == 0 &&
        ky[2]*ky[2] == 1 ? (ky[2] > 0 ? 1 : -1) : 0;

    width *= cn;
    src += ksize2;
    ky += ksize2;
    dst_step /= sizeof(dst[0]);

    if( is_symm )
    {
        for( ; count--; dst += dst_step, src++ )
        {
            if( is_1_2_1 )
            {
                const int *src0 = src[-1], *src1 = src[0], *src2 = src[1];

                for( i = 0; i <= width - 2; i += 2 )
                {
                    int s0 = src0[i] + src1[i]*2 + src2[i],
                        s1 = src0[i+1] + src1[i+1]*2 + src2[i+1];

                    dst[i] = (short)s0; dst[i+1] = (short)s1;
                }
            }
            else if( is_3_10_3 )
            {
                const int *src0 = src[-1], *src1 = src[0], *src2 = src[1];

                for( i = 0; i <= width - 2; i += 2 )
                {
                    int s0 = src1[i]*10 + (src0[i] + src2[i])*3,
                        s1 = src1[i+1]*10 + (src0[i+1] + src2[i+1])*3;

                    dst[i] = (short)s0; dst[i+1] = (short)s1;
                }
            }
            else
                for( i = 0; i <= width - 4; i += 4 )
                {
                    int f = ky[0];
                    const int* sptr = src[0] + i, *sptr2;
                    int s0 = f*sptr[0], s1 = f*sptr[1],
                        s2 = f*sptr[2], s3 = f*sptr[3];
                    for( k = 1; k <= ksize2; k++ )
                    {
                        sptr = src[k] + i; sptr2 = src[-k] + i; f = ky[k];
                        s0 += f*(sptr[0] + sptr2[0]); s1 += f*(sptr[1] + sptr2[1]);
                        s2 += f*(sptr[2] + sptr2[2]); s3 += f*(sptr[3] + sptr2[3]);
                    }

                    dst[i] = CV_CAST_16S(s0); dst[i+1] = CV_CAST_16S(s1);
                    dst[i+2] = CV_CAST_16S(s2); dst[i+3] = CV_CAST_16S(s3);
                }

            for( ; i < width; i++ )
            {
                int s0 = ky[0]*src[0][i];
                for( k = 1; k <= ksize2; k++ )
                    s0 += ky[k]*(src[k][i] + src[-k][i]);
                dst[i] = CV_CAST_16S(s0);
            }
        }
    }
    else
    {
        for( ; count--; dst += dst_step, src++ )
        {
            if( is_m1_0_1 )
            {
                const int *src0 = src[-is_m1_0_1], *src2 = src[is_m1_0_1];

                for( i = 0; i <= width - 2; i += 2 )
                {
                    int s0 = src2[i] - src0[i], s1 = src2[i+1] - src0[i+1];
                    dst[i] = (short)s0; dst[i+1] = (short)s1;
                }
            }
            else
                for( i = 0; i <= width - 4; i += 4 )
                {
                    int f = ky[0];
                    const int* sptr = src[0] + i, *sptr2;
                    int s0 = 0, s1 = 0, s2 = 0, s3 = 0;
                    for( k = 1; k <= ksize2; k++ )
                    {
                        sptr = src[k] + i; sptr2 = src[-k] + i; f = ky[k];
                        s0 += f*(sptr[0] - sptr2[0]); s1 += f*(sptr[1] - sptr2[1]);
                        s2 += f*(sptr[2] - sptr2[2]); s3 += f*(sptr[3] - sptr2[3]);
                    }

                    dst[i] = CV_CAST_16S(s0); dst[i+1] = CV_CAST_16S(s1);
                    dst[i+2] = CV_CAST_16S(s2); dst[i+3] = CV_CAST_16S(s3);
                }

            for( ; i < width; i++ )
            {
                int s0 = ky[0]*src[0][i];
                for( k = 1; k <= ksize2; k++ )
                    s0 += ky[k]*(src[k][i] - src[-k][i]);
                dst[i] = CV_CAST_16S(s0);
            }
        }
    }
}


#define ICV_FILTER_COL( flavor, srctype, dsttype, worktype,     \
                        cast_macro1, cast_macro2 )              \
static void                                                     \
icvFilterCol_##flavor( const srctype** src, dsttype* dst,       \
                       int dst_step, int count, void* params )  \
{                                                               \
    const CvSepFilter* state = (const CvSepFilter*)params;      \
    const CvMat* _ky = state->get_y_kernel();                   \
    const srctype* ky = (const srctype*)_ky->data.ptr;          \
    int ksize = _ky->cols + _ky->rows - 1;                      \
    int i, k, width = state->get_width();                       \
    int cn = CV_MAT_CN(state->get_src_type());                  \
                                                                \
    width *= cn;                                                \
    dst_step /= sizeof(dst[0]);                                 \
                                                                \
    for( ; count--; dst += dst_step, src++ )                    \
    {                                                           \
        for( i = 0; i <= width - 4; i += 4 )                    \
        {                                                       \
            double f = ky[0];                                   \
            const srctype* sptr = src[0] + i;                   \
            double s0 = f*sptr[0], s1 = f*sptr[1],              \
                   s2 = f*sptr[2], s3 = f*sptr[3];              \
            worktype t0, t1;                                    \
            for( k = 1; k < ksize; k++ )                        \
            {                                                   \
                sptr = src[k] + i; f = ky[k];                   \
                s0 += f*sptr[0]; s1 += f*sptr[1];               \
                s2 += f*sptr[2]; s3 += f*sptr[3];               \
            }                                                   \
                                                                \
            t0 = cast_macro1(s0); t1 = cast_macro1(s1);         \
            dst[i]=cast_macro2(t0); dst[i+1]=cast_macro2(t1);   \
            t0 = cast_macro1(s2); t1 = cast_macro1(s3);         \
            dst[i+2]=cast_macro2(t0); dst[i+3]=cast_macro2(t1); \
        }                                                       \
                                                                \
        for( ; i < width; i++ )                                 \
        {                                                       \
            double s0 = (double)ky[0]*src[0][i];                \
            worktype t0;                                        \
            for( k = 1; k < ksize; k++ )                        \
                s0 += (double)ky[k]*src[k][i];                  \
            t0 = cast_macro1(s0);                               \
            dst[i] = cast_macro2(t0);                           \
        }                                                       \
    }                                                           \
}


ICV_FILTER_COL( 32f8u, float, uchar, int, cvRound, CV_CAST_8U )
ICV_FILTER_COL( 32f16s, float, short, int, cvRound, CV_CAST_16S )
ICV_FILTER_COL( 32f16u, float, ushort, int, cvRound, CV_CAST_16U )

#define ICV_FILTER_COL_SYMM( flavor, srctype, dsttype, worktype,    \
                             cast_macro1, cast_macro2 )             \
static void                                                         \
icvFilterColSymm_##flavor( const srctype** src, dsttype* dst,       \
                           int dst_step, int count, void* params )  \
{                                                                   \
    const CvSepFilter* state = (const CvSepFilter*)params;          \
    const CvMat* _ky = state->get_y_kernel();                       \
    const srctype* ky = (const srctype*)_ky->data.ptr;              \
    int ksize = _ky->cols + _ky->rows - 1, ksize2 = ksize/2;        \
    int i, k, width = state->get_width();                           \
    int cn = CV_MAT_CN(state->get_src_type());                      \
    int is_symm = state->get_y_kernel_flags() & CvSepFilter::SYMMETRICAL;\
                                                                    \
    width *= cn;                                                    \
    src += ksize2;                                                  \
    ky += ksize2;                                                   \
    dst_step /= sizeof(dst[0]);                                     \
                                                                    \
    if( is_symm )                                                   \
    {                                                               \
        for( ; count--; dst += dst_step, src++ )                    \
        {                                                           \
            for( i = 0; i <= width - 4; i += 4 )                    \
            {                                                       \
                double f = ky[0];                                   \
                const srctype* sptr = src[0] + i, *sptr2;           \
                double s0 = f*sptr[0], s1 = f*sptr[1],              \
                       s2 = f*sptr[2], s3 = f*sptr[3];              \
                worktype t0, t1;                                    \
                for( k = 1; k <= ksize2; k++ )                      \
                {                                                   \
                    sptr = src[k] + i;                              \
                    sptr2 = src[-k] + i;                            \
                    f = ky[k];                                      \
                    s0 += f*(sptr[0] + sptr2[0]);                   \
                    s1 += f*(sptr[1] + sptr2[1]);                   \
                    s2 += f*(sptr[2] + sptr2[2]);                   \
                    s3 += f*(sptr[3] + sptr2[3]);                   \
                }                                                   \
                                                                    \
                t0 = cast_macro1(s0); t1 = cast_macro1(s1);         \
                dst[i]=cast_macro2(t0); dst[i+1]=cast_macro2(t1);   \
                t0 = cast_macro1(s2); t1 = cast_macro1(s3);         \
                dst[i+2]=cast_macro2(t0); dst[i+3]=cast_macro2(t1); \
            }                                                       \
                                                                    \
            for( ; i < width; i++ )                                 \
            {                                                       \
                double s0 = (double)ky[0]*src[0][i];                \
                worktype t0;                                        \
                for( k = 1; k <= ksize2; k++ )                      \
                    s0 += (double)ky[k]*(src[k][i] + src[-k][i]);   \
                t0 = cast_macro1(s0);                               \
                dst[i] = cast_macro2(t0);                           \
            }                                                       \
        }                                                           \
    }                                                               \
    else                                                            \
    {                                                               \
        for( ; count--; dst += dst_step, src++ )                    \
        {                                                           \
            for( i = 0; i <= width - 4; i += 4 )                    \
            {                                                       \
                double f = ky[0];                                   \
                const srctype* sptr = src[0] + i, *sptr2;           \
                double s0 = 0, s1 = 0, s2 = 0, s3 = 0;              \
                worktype t0, t1;                                    \
                for( k = 1; k <= ksize2; k++ )                      \
                {                                                   \
                    sptr = src[k] + i;                              \
                    sptr2 = src[-k] + i;                            \
                    f = ky[k];                                      \
                    s0 += f*(sptr[0] - sptr2[0]);                   \
                    s1 += f*(sptr[1] - sptr2[1]);                   \
                    s2 += f*(sptr[2] - sptr2[2]);                   \
                    s3 += f*(sptr[3] - sptr2[3]);                   \
                }                                                   \
                                                                    \
                t0 = cast_macro1(s0); t1 = cast_macro1(s1);         \
                dst[i]=cast_macro2(t0); dst[i+1]=cast_macro2(t1);   \
                t0 = cast_macro1(s2); t1 = cast_macro1(s3);         \
                dst[i+2]=cast_macro2(t0); dst[i+3]=cast_macro2(t1); \
            }                                                       \
                                                                    \
            for( ; i < width; i++ )                                 \
            {                                                       \
                double s0 = (double)ky[0]*src[0][i];                \
                worktype t0;                                        \
                for( k = 1; k <= ksize2; k++ )                      \
                    s0 += (double)ky[k]*(src[k][i] - src[-k][i]);   \
                t0 = cast_macro1(s0);                               \
                dst[i] = cast_macro2(t0);                           \
            }                                                       \
        }                                                           \
    }                                                               \
}


ICV_FILTER_COL_SYMM( 32f8u, float, uchar, int, cvRound, CV_CAST_8U )
ICV_FILTER_COL_SYMM( 32f16s, float, short, int, cvRound, CV_CAST_16S )
ICV_FILTER_COL_SYMM( 32f16u, float, ushort, int, cvRound, CV_CAST_16U )


static void
icvFilterCol_32f( const float** src, float* dst,
                  int dst_step, int count, void* params )
{
    const CvSepFilter* state = (const CvSepFilter*)params;
    const CvMat* _ky = state->get_y_kernel();
    const float* ky = (const float*)_ky->data.ptr;
    int ksize = _ky->cols + _ky->rows - 1;
    int i, k, width = state->get_width();
    int cn = CV_MAT_CN(state->get_src_type());

    width *= cn;
    dst_step /= sizeof(dst[0]);

    for( ; count--; dst += dst_step, src++ )
    {
        for( i = 0; i <= width - 4; i += 4 )
        {
            double f = ky[0];
            const float* sptr = src[0] + i;
            double s0 = f*sptr[0], s1 = f*sptr[1],
                   s2 = f*sptr[2], s3 = f*sptr[3];
            for( k = 1; k < ksize; k++ )
            {
                sptr = src[k] + i; f = ky[k];
                s0 += f*sptr[0]; s1 += f*sptr[1];
                s2 += f*sptr[2]; s3 += f*sptr[3];
            }

            dst[i] = (float)s0; dst[i+1] = (float)s1;
            dst[i+2] = (float)s2; dst[i+3] = (float)s3;
        }

        for( ; i < width; i++ )
        {
            double s0 = (double)ky[0]*src[0][i];
            for( k = 1; k < ksize; k++ )
                s0 += (double)ky[k]*src[k][i];
            dst[i] = (float)s0;
        }
    }
}


static void
icvFilterColSymm_32f( const float** src, float* dst,
                      int dst_step, int count, void* params )
{
    const CvSepFilter* state = (const CvSepFilter*)params;
    const CvMat* _ky = state->get_y_kernel();
    const float* ky = (const float*)_ky->data.ptr;
    int ksize = _ky->cols + _ky->rows - 1, ksize2 = ksize/2;
    int i = 0, k, width = state->get_width();
    int cn = CV_MAT_CN(state->get_src_type());
    int is_symm = state->get_y_kernel_flags() & CvSepFilter::SYMMETRICAL;
    int is_1_2_1 = is_symm && ksize == 3 &&
        fabs(ky[1] - 2.) <= FLT_EPSILON && fabs(ky[2] - 1.) <= FLT_EPSILON;
    int is_3_10_3 = is_symm && ksize == 3 &&
            fabs(ky[1] - 10.) <= FLT_EPSILON && fabs(ky[2] - 3.) <= FLT_EPSILON;
    int is_m1_0_1 = !is_symm && ksize == 3 &&
            fabs(ky[1]) <= FLT_EPSILON && fabs(ky[2]*ky[2] - 1.) <= FLT_EPSILON ?
            (ky[2] > 0 ? 1 : -1) : 0;

    width *= cn;
    src += ksize2;
    ky += ksize2;
    dst_step /= sizeof(dst[0]);

    if( is_symm )
    {
        for( ; count--; dst += dst_step, src++ )
        {
            if( is_1_2_1 )
            {
                const float *src0 = src[-1], *src1 = src[0], *src2 = src[1];

                for( i = 0; i <= width - 4; i += 4 )
                {
                    float s0 = src0[i] + src1[i]*2 + src2[i],
                          s1 = src0[i+1] + src1[i+1]*2 + src2[i+1],
                          s2 = src0[i+2] + src1[i+2]*2 + src2[i+2],
                          s3 = src0[i+3] + src1[i+3]*2 + src2[i+3];

                    dst[i] = s0; dst[i+1] = s1;
                    dst[i+2] = s2; dst[i+3] = s3;
                }
            }
            else if( is_3_10_3 )
            {
                const float *src0 = src[-1], *src1 = src[0], *src2 = src[1];

                for( i = 0; i <= width - 4; i += 4 )
                {
                    float s0 = src1[i]*10 + (src0[i] + src2[i])*3,
                          s1 = src1[i+1]*10 + (src0[i+1] + src2[i+1])*3,
                          s2 = src1[i+2]*10 + (src0[i+2] + src2[i+2])*3,
                          s3 = src1[i+3]*10 + (src0[i+3] + src2[i+3])*3;

                    dst[i] = s0; dst[i+1] = s1;
                    dst[i+2] = s2; dst[i+3] = s3;
                }
            }
            else
                for( i = 0; i <= width - 4; i += 4 )
                {
                    double f = ky[0];
                    const float* sptr = src[0] + i, *sptr2;
                    double s0 = f*sptr[0], s1 = f*sptr[1],
                           s2 = f*sptr[2], s3 = f*sptr[3];
                    for( k = 1; k <= ksize2; k++ )
                    {
                        sptr = src[k] + i; sptr2 = src[-k] + i; f = ky[k];
                        s0 += f*(sptr[0] + sptr2[0]); s1 += f*(sptr[1] + sptr2[1]);
                        s2 += f*(sptr[2] + sptr2[2]); s3 += f*(sptr[3] + sptr2[3]);
                    }

                    dst[i] = (float)s0; dst[i+1] = (float)s1;
                    dst[i+2] = (float)s2; dst[i+3] = (float)s3;
                }

            for( ; i < width; i++ )
            {
                double s0 = (double)ky[0]*src[0][i];
                for( k = 1; k <= ksize2; k++ )
                    s0 += (double)ky[k]*(src[k][i] + src[-k][i]);
                dst[i] = (float)s0;
            }
        }
    }
    else
    {
        for( ; count--; dst += dst_step, src++ )
        {
            if( is_m1_0_1 )
            {
                const float *src0 = src[-is_m1_0_1], *src2 = src[is_m1_0_1];

                for( i = 0; i <= width - 4; i += 4 )
                {
                    float s0 = src2[i] - src0[i], s1 = src2[i+1] - src0[i+1],
                          s2 = src2[i+2] - src0[i+2], s3 = src2[i+3] - src0[i+3];
                    dst[i] = s0; dst[i+1] = s1;
                    dst[i+2] = s2; dst[i+3] = s3;
                }
            }
            else
                for( i = 0; i <= width - 4; i += 4 )
                {
                    double f = ky[0];
                    const float* sptr = src[0] + i, *sptr2;
                    double s0 = 0, s1 = 0, s2 = 0, s3 = 0;
                    for( k = 1; k <= ksize2; k++ )
                    {
                        sptr = src[k] + i; sptr2 = src[-k] + i; f = ky[k];
                        s0 += f*(sptr[0] - sptr2[0]); s1 += f*(sptr[1] - sptr2[1]);
                        s2 += f*(sptr[2] - sptr2[2]); s3 += f*(sptr[3] - sptr2[3]);
                    }

                    dst[i] = (float)s0; dst[i+1] = (float)s1;
                    dst[i+2] = (float)s2; dst[i+3] = (float)s3;
                }

            for( ; i < width; i++ )
            {
                double s0 = (double)ky[0]*src[0][i];
                for( k = 1; k <= ksize2; k++ )
                    s0 += (double)ky[k]*(src[k][i] - src[-k][i]);
                dst[i] = (float)s0;
            }
        }
    }
}


#define SMALL_GAUSSIAN_SIZE  7

void CvSepFilter::init_gaussian_kernel( CvMat* kernel, double sigma )
{
    static const float small_gaussian_tab[][SMALL_GAUSSIAN_SIZE/2+1] =
    {
        {1.f},
        {0.5f, 0.25f},
        {0.375f, 0.25f, 0.0625f},
        {0.28125f, 0.21875f, 0.109375f, 0.03125f}
    };

    CV_FUNCNAME( "CvSepFilter::init_gaussian_kernel" );

    __BEGIN__;

    int type, i, n, step;
    const float* fixed_kernel = 0;
    double sigmaX, scale2X, sum;
    float* cf;
    double* cd;

    if( !CV_IS_MAT(kernel) )
        CV_ERROR( CV_StsBadArg, "kernel is not a valid matrix" );

    type = CV_MAT_TYPE(kernel->type);
    
    if( kernel->cols != 1 && kernel->rows != 1 ||
        (kernel->cols + kernel->rows - 1) % 2 == 0 ||
        type != CV_32FC1 && type != CV_64FC1 )
        CV_ERROR( CV_StsBadSize, "kernel should be 1D floating-point vector of odd (2*k+1) size" );

    n = kernel->cols + kernel->rows - 1;

    if( n <= SMALL_GAUSSIAN_SIZE && sigma <= 0 )
        fixed_kernel = small_gaussian_tab[n>>1];

    sigmaX = sigma > 0 ? sigma : (n/2 - 1)*0.3 + 0.8;
    scale2X = -0.5/(sigmaX*sigmaX);
    step = kernel->rows == 1 ? 1 : kernel->step/CV_ELEM_SIZE1(type);
    cf = kernel->data.fl;
    cd = kernel->data.db;

    sum = fixed_kernel ? -fixed_kernel[0] : -1.;

    for( i = 0; i <= n/2; i++ )
    {
        double t = fixed_kernel ? (double)fixed_kernel[i] : exp(scale2X*i*i);
        if( type == CV_32FC1 )
        {
            cf[(n/2+i)*step] = (float)t;
            sum += cf[(n/2+i)*step]*2;
        }
        else
        {
            cd[(n/2+i)*step] = t;
            sum += cd[(n/2+i)*step]*2;
        }
    }

    sum = 1./sum;
    for( i = 0; i <= n/2; i++ )
    {
        if( type == CV_32FC1 )
            cf[(n/2+i)*step] = cf[(n/2-i)*step] = (float)(cf[(n/2+i)*step]*sum);
        else
            cd[(n/2+i)*step] = cd[(n/2-i)*step] = cd[(n/2+i)*step]*sum;
    }

    __END__;
}


void CvSepFilter::init_sobel_kernel( CvMat* _kx, CvMat* _ky, int dx, int dy, int flags )
{
    CV_FUNCNAME( "CvSepFilter::init_sobel_kernel" );

    __BEGIN__;

    int i, j, k, msz;
    int* kerI;
    bool normalize = (flags & NORMALIZE_KERNEL) != 0;
    bool flip = (flags & FLIP_KERNEL) != 0;

    if( !CV_IS_MAT(_kx) || !CV_IS_MAT(_ky) )
        CV_ERROR( CV_StsBadArg, "One of the kernel matrices is not valid" );

    msz = MAX( _kx->cols + _kx->rows, _ky->cols + _ky->rows );
    if( msz > 32 )
        CV_ERROR( CV_StsOutOfRange, "Too large kernel size" );

    kerI = (int*)cvStackAlloc( msz*sizeof(kerI[0]) );

    if( dx < 0 || dy < 0 || dx+dy <= 0 )
        CV_ERROR( CV_StsOutOfRange,
            "Both derivative orders (dx and dy) must be non-negative "
            "and at least one of them must be positive." );

    for( k = 0; k < 2; k++ )
    {
        CvMat* kernel = k == 0 ? _kx : _ky;
        int order = k == 0 ? dx : dy;
        int n = kernel->cols + kernel->rows - 1, step;
        int type = CV_MAT_TYPE(kernel->type);
        double scale = !normalize ? 1. : 1./(1 << (n-order-1));
        int iscale = 1;

        if( kernel->cols != 1 && kernel->rows != 1 ||
            (kernel->cols + kernel->rows - 1) % 2 == 0 ||
            type != CV_32FC1 && type != CV_64FC1 && type != CV_32SC1 )
            CV_ERROR( CV_StsOutOfRange,
            "Both kernels must be 1D floating-point or integer vectors of odd (2*k+1) size." );

        if( normalize && n > 1 && type == CV_32SC1 )
            CV_ERROR( CV_StsBadArg, "Integer kernel can not be normalized" );

        if( n <= order )
            CV_ERROR( CV_StsOutOfRange,
            "Derivative order must be smaller than the corresponding kernel size" );

        if( n == 1 )
            kerI[0] = 1;
        else if( n == 3 )
        {
            if( order == 0 )
                kerI[0] = 1, kerI[1] = 2, kerI[2] = 1;
            else if( order == 1 )
                kerI[0] = -1, kerI[1] = 0, kerI[2] = 1;
            else
                kerI[0] = 1, kerI[1] = -2, kerI[2] = 1;
        }
        else
        {
            int oldval, newval;
            kerI[0] = 1;
            for( i = 0; i < n; i++ )
                kerI[i+1] = 0;

            for( i = 0; i < n - order - 1; i++ )
            {
                oldval = kerI[0];
                for( j = 1; j <= n; j++ )
                {
                    newval = kerI[j]+kerI[j-1];
                    kerI[j-1] = oldval;
                    oldval = newval;
                }
            }

            for( i = 0; i < order; i++ )
            {
                oldval = -kerI[0];
                for( j = 1; j <= n; j++ )
                {
                    newval = kerI[j-1] - kerI[j];
                    kerI[j-1] = oldval;
                    oldval = newval;
                }
            }
        }

        step = kernel->rows == 1 ? 1 : kernel->step/CV_ELEM_SIZE1(type);
        if( flip && (order & 1) && k )
            iscale = -iscale, scale = -scale;

        for( i = 0; i < n; i++ )
        {
            if( type == CV_32SC1 )
                kernel->data.i[i*step] = kerI[i]*iscale;
            else if( type == CV_32FC1 )
                kernel->data.fl[i*step] = (float)(kerI[i]*scale);
            else
                kernel->data.db[i*step] = kerI[i]*scale;
        }
    }

    __END__;
}


void CvSepFilter::init_scharr_kernel( CvMat* _kx, CvMat* _ky, int dx, int dy, int flags )
{
    CV_FUNCNAME( "CvSepFilter::init_scharr_kernel" );

    __BEGIN__;

    int i, k;
    int kerI[3];
    bool normalize = (flags & NORMALIZE_KERNEL) != 0;
    bool flip = (flags & FLIP_KERNEL) != 0;

    if( !CV_IS_MAT(_kx) || !CV_IS_MAT(_ky) )
        CV_ERROR( CV_StsBadArg, "One of the kernel matrices is not valid" );

    if( ((dx|dy)&~1) || dx+dy != 1 )
        CV_ERROR( CV_StsOutOfRange,
            "Scharr kernel can only be used for 1st order derivatives" );

    for( k = 0; k < 2; k++ )
    {
        CvMat* kernel = k == 0 ? _kx : _ky;
        int order = k == 0 ? dx : dy;
        int n = kernel->cols + kernel->rows - 1, step;
        int type = CV_MAT_TYPE(kernel->type);
        double scale = !normalize ? 1. : order == 0 ? 1./16 : 1./2;
        int iscale = 1;

        if( kernel->cols != 1 && kernel->rows != 1 ||
            kernel->cols + kernel->rows - 1 != 3 ||
            type != CV_32FC1 && type != CV_64FC1 && type != CV_32SC1 )
            CV_ERROR( CV_StsOutOfRange,
            "Both kernels must be 1D floating-point or integer vectors containing 3 elements each." );

        if( normalize && type == CV_32SC1 )
            CV_ERROR( CV_StsBadArg, "Integer kernel can not be normalized" );

        if( order == 0 )
            kerI[0] = 3, kerI[1] = 10, kerI[2] = 3;
        else
            kerI[0] = -1, kerI[1] = 0, kerI[2] = 1;

        step = kernel->rows == 1 ? 1 : kernel->step/CV_ELEM_SIZE1(type);
        if( flip && (order & 1) && k )
            iscale = -iscale, scale = -scale;

        for( i = 0; i < n; i++ )
        {
            if( type == CV_32SC1 )
                kernel->data.i[i*step] = kerI[i]*iscale;
            else if( type == CV_32FC1 )
                kernel->data.fl[i*step] = (float)(kerI[i]*scale);
            else
                kernel->data.db[i*step] = kerI[i]*scale;
        }
    }

    __END__;
}


void CvSepFilter::init_deriv( int _max_width, int _src_type, int _dst_type,
                              int dx, int dy, int aperture_size, int flags )
{
    CV_FUNCNAME( "CvSepFilter::init_deriv" );

    __BEGIN__;

    int kx_size = aperture_size == CV_SCHARR ? 3 : aperture_size, ky_size = kx_size;
    float kx_data[CV_MAX_SOBEL_KSIZE], ky_data[CV_MAX_SOBEL_KSIZE];
    CvMat _kx, _ky;

    if( kx_size <= 0 || ky_size > CV_MAX_SOBEL_KSIZE )
        CV_ERROR( CV_StsOutOfRange, "Incorrect aperture_size" );

    if( kx_size == 1 && dx )
        kx_size = 3;
    if( ky_size == 1 && dy )
        ky_size = 3;

    _kx = cvMat( 1, kx_size, CV_32FC1, kx_data );
    _ky = cvMat( 1, ky_size, CV_32FC1, ky_data );

    if( aperture_size == CV_SCHARR )
    {
        CV_CALL( init_scharr_kernel( &_kx, &_ky, dx, dy, flags ));
    }
    else
    {
        CV_CALL( init_sobel_kernel( &_kx, &_ky, dx, dy, flags ));
    }

    CV_CALL( init( _max_width, _src_type, _dst_type, &_kx, &_ky ));

    __END__;
}


void CvSepFilter::init_gaussian( int _max_width, int _src_type, int _dst_type,
                                 int gaussian_size, double sigma )
{
    float* kdata = 0;
    
    CV_FUNCNAME( "CvSepFilter::init_gaussian" );

    __BEGIN__;

    CvMat _kernel;

    if( gaussian_size <= 0 || gaussian_size > 1024 )
        CV_ERROR( CV_StsBadSize, "Incorrect size of gaussian kernel" );

    kdata = (float*)cvStackAlloc(gaussian_size*sizeof(kdata[0]));
    _kernel = cvMat( 1, gaussian_size, CV_32F, kdata );

    CV_CALL( init_gaussian_kernel( &_kernel, sigma ));
    CV_CALL( init( _max_width, _src_type, _dst_type, &_kernel, &_kernel )); 

    __END__;
}



