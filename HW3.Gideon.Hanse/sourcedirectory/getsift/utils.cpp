/*
  Miscellaneous utility functions.
  
  Copyright (C) 2006-2012  Rob Hess <rob@iqengines.com>

  @version 1.1.2-20100521
*/

#include "utils.h"

//#include <gdk/gdk.h>
//#include <gtk/gtk.h>

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "global.h"
#include "cv.h"


/*************************** Function Definitions ****************************/


/*
  Replaces a file's extension, which is assumed to be everything after the
  last dot ('.') character.
  
  @param file the name of a file
  
  @param extn a new extension for \a file; should not include a dot (i.e.
    \c "jpg", not \c ".jpg") unless the new file extension should contain
    two dots.
    
  @return Returns a new string formed as described above.  If \a file does
    not have an extension, this function simply adds one.
*/
char* replace_extension( const char* file, const char* extn )
{
  char* new_file, * lastdot;

  new_file = (char *)calloc( strlen( file ) + strlen( extn ) + 2,  sizeof( char ) );
  strcpy( new_file, file );
  lastdot = strrchr( new_file, '.' );
  if( lastdot )
    *(lastdot + 1) = '\0';
  else
    strcat( new_file, "." );
  strcat( new_file, extn );

  return new_file;
}



/*
  Prepends a path to a filename.
  
  @param path a path
  @param file a file name
  
  @return Returns a new string containing a full path name consisting of
    \a path prepended to \a file.
*/
char* prepend_path( const char* path, const char* file )
{
  int n = strlen(path) + strlen(file) + 2;
  char* pathname = (char *)calloc( n, sizeof(char) );

  return pathname;
}


/*
  Displays progress in the console with a spinning pinwheel.  Every time this
  function is called, the state of the pinwheel is incremented.  The pinwheel
  has four states that loop indefinitely: '|', '/', '-', '\'.
  
  @param done if 0, this function simply increments the state of the pinwheel;
    otherwise it prints "done"
*/
void progress( int done )
{
  char state[4] = { '|', '/', '-', '\\' };
  static int cur = -1;
  
  if( cur == -1 )
    fprintf( stderr, "  " );

  if( done )
    {
      fprintf( stderr, "\b\bdone\n");
      cur = -1;
    }
  else
    {
      cur = ( cur + 1 ) % 4;
      fprintf( stdout, "\b\b%c ", state[cur] );
      fflush(stderr);
    }
}



/*
  Erases a specified number of characters from a stream.

  @param stream the stream from which to erase characters
  @param n the number of characters to erase
*/
void erase_from_stream( FILE* stream, int n )
{
  int j;
  for( j = 0; j < n; j++ )
    fprintf( stream, "\b" );
  for( j = 0; j < n; j++ )
    fprintf( stream, " " );
  for( j = 0; j < n; j++ )
    fprintf( stream, "\b" );
}





/*
  Draws an x on an image.
  
  @param img an image
  @param pt the center point of the x
  @param r the x's radius
  @param w the x's line weight
  @param color the color of the x
*/
void draw_x( IplImage* img, CvPoint pt, int r, int w, CvScalar color )
{
  cvLine( img, pt, cvPoint( pt.x + r, pt.y + r), color, w, 8, 0 );
  cvLine( img, pt, cvPoint( pt.x - r, pt.y + r), color, w, 8, 0 );
  cvLine( img, pt, cvPoint( pt.x + r, pt.y - r), color, w, 8, 0 );
  cvLine( img, pt, cvPoint( pt.x - r, pt.y - r), color, w, 8, 0 );
}



/*
  Combines two images by scacking one on top of the other
  
  @param img1 top image
  @param img2 bottom image
  
  @return Returns the image resulting from stacking \a img1 on top if \a img2
*/
IplImage* stack_imgs( IplImage* img1, IplImage* img2 )
{
  IplImage* stacked = cvCreateImage( cvSize( MAX(img1->width, img2->width),img1->height + img2->height ),IPL_DEPTH_8U, 3 );
  cvZero( stacked );
  cvSetImageROI( stacked, cvRect( 0, 0, img1->width, img1->height ) );
  cvAdd( img1, stacked, stacked, NULL );
  cvSetImageROI( stacked, cvRect(0, img1->height, img2->width, img2->height) );
  cvAdd( img2, stacked, stacked, NULL );
  cvResetImageROI( stacked );
  return stacked;
}









