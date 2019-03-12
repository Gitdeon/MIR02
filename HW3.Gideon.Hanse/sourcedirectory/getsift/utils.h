#ifndef UTILS_H
#define UTILS_H
#include <stdio.h>
#include "global.h"

/**
   A function to get a pixel value from an 8-bit unsigned image.
   
   @param img an image
   @param r row
   @param c column
   @return Returns the value of the pixel at (\a r, \a c) in \a img
*/
static inline int pixval8( IplImage* img, int r, int c )
{
  return (int)( ( (uchar*)(img->imageData + img->widthStep*r) )[c] );
}


/**
   A function to set a pixel value in an 8-bit unsigned image.
   
   @param img an image
   @param r row
   @param c column
   @param val pixel value
*/
static inline void setpix8( IplImage* img, int r, int c, uchar val)
{
  ( (uchar*)(img->imageData + img->widthStep*r) )[c] = val;
}
/**
   A function to set a pixel value in a 32-bit floating-point image.
   
   @param img an image
   @param r row
   @param c column
   @param val pixel value
*/
static inline void setpix32f( IplImage* img, int r, int c, float val )
{
  ( (float*)(img->imageData + img->widthStep*r) )[c] = val;
}
/**
   A function to get a pixel value from a 64-bit floating-point image.
   
   @param img an image
   @param r row
   @param c column
   @return Returns the value of the pixel at (\a r, \a c) in \a img
*/
static inline double pixval64f( IplImage* img, int r, int c )
{
  return (double)( ( (double*)(img->imageData + img->widthStep*r) )[c] );
}
/**
   A function to set a pixel value in a 64-bit floating-point image.
   
   @param img an image
   @param r row
   @param c column
   @param val pixel value
*/
static inline void setpix64f( IplImage* img, int r, int c, double val )
{
  ( (double*)(img->imageData + img->widthStep*r) )[c] = val;
}
/**************************** Function Prototypes ****************************/

/**
   Replaces a file's extension, which is assumed to be everything after the
   last dot ('.') character.

   @param file the name of a file

   @param extn a new extension for \a file; should not include a dot (i.e.
     \c "jpg", not \c ".jpg") unless the new file extension should contain
     two dots.

   @return Returns a new string formed as described above.  If \a file does
     not have an extension, this function simply adds one.
*/
extern char* replace_extension( const char* file, const char* extn );


/**
   Prepends a path to a filename.

   @param path a path
   @param file a file name

   @return Returns a new string containing a full path name consisting of
     \a path prepended to \a file.
*/
extern char* prepend_path( const char* path, const char* file );



/**
   Displays progress in the console with a spinning pinwheel.  Every time this
   function is called, the state of the pinwheel is incremented.  The pinwheel
   has four states that loop indefinitely: '|', '/', '-', '\'.
   
   @param done if 0, this function simply increments the state of the pinwheel;
     otherwise it prints "done"
*/
//extern void progress( int done );


/**
   Erases a specified number of characters from a stream.
   
   @param stream the stream from which to erase characters
   @param n the number of characters to erase
*/
extern void erase_from_stream( FILE* stream, int n );






/**
   Draws an x on an image.

   @param img an image
   @param pt the center point of the x
   @param r the x's radius
   @param w the x's line weight
   @param color the color of the x
*/
extern void draw_x( IplImage* img, CvPoint pt, int r, int w, CvScalar color );


/**
   Combines two images by scacking one on top of the other
   
   @param img1 top image
   @param img2 bottom image

   @return Returns the image resulting from stacking \a img1 on top if \a img2
*/
IplImage* stack_imgs( IplImage* img1, IplImage* img2 );



#endif
