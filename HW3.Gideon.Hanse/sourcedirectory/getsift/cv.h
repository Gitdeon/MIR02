#include <stdlib.h>
#include <string.h>
#include <stdio.h> 
#include "global.h"


#ifndef CV_H
#define CV_H




#endif

IplImage* read_jpeg_file(char * filename);
//FILE* convert2jpeg(IplImage* frame);
void write_jpeg_file(char *filename, const IplImage* img_);
IplImage * cvCreateImage( CvSize size, int depth, int channels );
IplImage * cvCreateImageHeader( CvSize size, int depth, int channels );
void cvCreateData( CvArr* arr );
IplImage* cvInitImageHeader( IplImage * image, CvSize size, int depth,int channels, int origin, int align );
void cvReleaseImageHeader( IplImage** image );
void cvReleaseImage( IplImage ** image );
void cvReleaseData( CvArr* arr );
static  void icvGetColorModel( int nchannels, char** colorModel, char** channelSeq );
void  cvDecRefData( CvArr* arr );
