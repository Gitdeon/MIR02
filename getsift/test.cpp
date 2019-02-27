#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include "cv.h"
#include "sift.h"
#include "imgfeatures.h"
#include "kdtree.h"
#include "xform.h"
#include "utils.h"

/* the maximum number of keypoint NN candidates to check during BBF search */
#define KDTREE_BBF_MAX_NN_CHKS 200

/* threshold on squared ratio of distances between NN and 2nd NN */
#define NN_SQ_DIST_RATIO_THR 0.5

int main(int argc, char *argv[])
{
	IplImage* img1, * img2, * stacked1, *stacked2;
        char stemp[1024];

// printf("Reading images: %s and %s\n",argv[1],argv[2]);
  if(argc != 3) {printf("\n\nUsage: getsift [image1.jpg] [image2.jpg]\n\n"); exit(0);}

	img1=read_jpeg_file(argv[1]);
	img2=read_jpeg_file(argv[2]);
	stacked1 = stack_imgs( img1, img2 );
	stacked2 = stack_imgs( img1, img2 );
	
	struct feature* feat1, * feat2, * feat;
	struct feature** nbrs;
	struct feature** RANnb;
	struct kd_node* kd_root;
	CvPoint pt1, pt2;
	double d0, d1;
	int n1, n2, k, i,j, m = 0, n=0;
	
	printf("SIFT Features Extraction: %s\n", argv[1]);
	n1 = sift_features( img1, &feat1 );
	printf("Numbers of Features from %s: %d\n",argv[1], n1);

	printf("SIFT Features Extraction: %s\n", argv[2]);
	n2 = sift_features( img2, &feat2 );
	printf("Numbers of Features from %s: %d\n",argv[2], n2);

        sprintf(stemp,"%s.sift.jpg",argv[1]);
	draw_keypoint(  img1,  feat1, n1 );
	write_jpeg_file(stemp,img1);

        sprintf(stemp,"%s.sift.jpg",argv[2]);
	draw_keypoint(  img2,  feat2, n2 );
	write_jpeg_file(stemp,img2);

	FILE * feat1file;
	FILE * feat2file;

	feat1file=fopen("features1.txt","w+");
	for(i=0;i<n1;i++)
	{
		fprintf(feat1file,"(%lf,%lf): {",(feat1+i)->x,(feat1+i)->y);	
		for(j=0;j<FEATURE_MAX_D;j++)
			fprintf(feat1file,"% lf ",(feat1+i)->descr[j]);
		fprintf(feat1file,"}\n");
	}
    printf("coordinate and descriptor of %s keypoints have been written in featfile1.txt\n",argv[1]);

	feat2file=fopen("features2.txt","w+");
	for(i=0;i<n2;i++)
	{
		fprintf(feat2file,"(%lf,%lf): {",(feat2+i)->x,(feat2+i)->y);
		for(j=0;j<FEATURE_MAX_D;j++)
			fprintf(feat2file,"% lf ",(feat2+i)->descr[j]);
		fprintf(feat2file,"}\n");
	}
	printf("coordinate and descriptor of %s keypoints have been written in featfile2.txt\n",argv[2]);

	kd_root = kdtree_build( feat2, n2 );	

	for( i = 0; i < n1; i++ )
	{
		feat = feat1 + i;
		k = kdtree_bbf_knn( kd_root, feat, 2, &nbrs, KDTREE_BBF_MAX_NN_CHKS );
		if( k == 2 )
		{
			d0 = descr_dist_sq( feat, nbrs[0] );
			d1 = descr_dist_sq( feat, nbrs[1] );
			if( d0 < d1 * NN_SQ_DIST_RATIO_THR )
			{
				pt1 = cvPoint( cvRound( feat->x ), cvRound( feat->y ) );
				pt2 = cvPoint( cvRound( nbrs[0]->x ), cvRound( nbrs[0]->y ) );
				pt2.y += img1->height;
				cvLine( stacked1, pt1, pt2, CV_RGB(255,0,255), 1, 8, 0 );
				m++;
				feat1[i].fwd_match = nbrs[0];
			}
		}
		free( nbrs );
	}

	printf("Found %d total matches\n", m );
	write_jpeg_file("matches.jpg",stacked1);

  
    CvMat* H;
    int number=0;

    H = ransac_xform( feat1, n1, FEATURE_FWD_MATCH, lsq_homog, 4, 0.25, homog_xfer_err, 27.0, &RANnb, &number );	

   	for( i = 0; i < number; i++ )
	{
		pt1 = cvPoint( cvRound( RANnb[i]->x ), cvRound( RANnb[i]->y ) );
		pt2 = cvPoint( cvRound( RANnb[i]->fwd_match->x ), cvRound( RANnb[i]->fwd_match->y ) );
		pt2.y += img1->height;
		cvLine( stacked2, pt1, pt2, CV_RGB(255,0,255), 1, 8, 0 );
		n++;		
	}

	printf("Found %d total matches after RANSAC\n", n );
	write_jpeg_file("matches.ransac.jpg",stacked2);
	
       cvReleaseImage( &img1 );
	cvReleaseImage( &img2 );
	kdtree_release( kd_root );
	free( feat1 );
	free( feat2 );
	return 0;
}
