/*
 ** Author: Elina Lijouvni, Owen Cliffe
 ** License: GPL v3
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <cv.h>
#include <highgui.h>

#include "low-level-leap.h"
typedef struct ctx_s ctx_t;
struct ctx_s
{
  int quit;
};


#define VFRAME_WIDTH  640
#define VFRAME_HEIGHT 240
#define VFRAME_SIZE   (VFRAME_WIDTH * VFRAME_HEIGHT)




typedef struct stereoframe_s stereoframe_t; 
struct stereoframe_s{
  IplImage * left; 
  IplImage * right; 
  CvMat *cv_image_depth;
  IplImage *cv_image_depth_aux;	
  CvStereoBMState *stereo_state; /* Block Matching State */	

};

#define UVC_STREAM_EOF                                  (1 << 1)


/* Function to compute StereoBM and update the result on the window */
static void computeStereoBM ( stereoframe_t *data )
{

  cvSmooth(data->left,data->left,CV_GAUSSIAN,3,0,0,0);
  cvSmooth(data->right,data->right,CV_GAUSSIAN,3,0,0,0);
  //adaptiveBilateralFilter(data->left, data-> left, 15,80,80);
  //adaptiveBilateralFilter(data->right, data-> right, 15,80,80);
       cvCanny( data->left, data->left, 10, 100, 3 );
       cvCanny( data->right, data->right, 10, 100, 3 );

	int i, j, aux;
	uchar *ptr_dst;
	cvFindStereoCorrespondenceBM ( 
		data->left,
		data->right, 
		data->cv_image_depth, 
		data->stereo_state
	);
	//Normalize the result so we can display it
	//		cvNormalize( data->cv_image_depth, data->cv_image_depth, 0, 256, CV_MINMAX, NULL );
	for ( i = 0; i < data->cv_image_depth->rows; i++)
	{
		aux = data->cv_image_depth->cols * i;
		ptr_dst = (uchar*)(data->cv_image_depth_aux->imageData + i*data->cv_image_depth_aux->widthStep);
		for ( j = 0; j < data->cv_image_depth->cols; j++ )
		{
			//((float*)(mat->data.ptr + mat->step*i))[j]
			ptr_dst[3*j] = (uchar)((short int*)(data->cv_image_depth->data.ptr + data->cv_image_depth->step*i))[j];
			ptr_dst[3*j+1] = (uchar)((short int*)(data->cv_image_depth->data.ptr + data->cv_image_depth->step*i))[j];
			ptr_dst[3*j+2] = (uchar)((short int*)(data->cv_image_depth->data.ptr + data->cv_image_depth->step*i))[j];
		}
	}
}


#define WINDOWNAME ("SimpleStereo") 
static void
process_video_frame(ctx_t *ctx, stereoframe_t *frame)
{
  int key;
  
  cvShowImage("left", frame->left );
  cvShowImage("right", frame->right );
  //  cvShowImage("mat", frame->cv_image_depth );
  // cvShowImage(WINDOWNAME, frame->cv_image_depth_aux );

  key = cvWaitKey(1);

if(key > 0){
  
  switch(key){
    printf("got key %d\n", key) ;  

    case 1048689: // q 
	    ctx->quit = 1;
            break; 
     case 1048695: 
       if(frame->stereo_state->preFilterSize < 251){
	frame->stereo_state->preFilterSize+=2;
       }
        break; 
     case 1048691: 
       if(frame->stereo_state->preFilterSize > 7){
	frame->stereo_state->preFilterSize-=2;
       }
       break; 
  case 1048677: 
    if(frame->stereo_state->preFilterCap < 63){
      frame->stereo_state->preFilterCap++;
    }
    break; 
  case 1048676: 
    if(frame->stereo_state->preFilterCap > 1){
      frame->stereo_state->preFilterCap--;
    }
    break; 

  case 1048690: 
    if(frame->stereo_state->SADWindowSize < 251){
      frame->stereo_state->SADWindowSize+=2;
    }
    break; 
  case 1048678: 
    if(frame->stereo_state->SADWindowSize > 7 ){
      frame->stereo_state->SADWindowSize-=2;
    }
    break; 

  case 1048692: 
    frame->stereo_state->minDisparity+=1;
    break; 
  case 1048679: 
    frame->stereo_state->minDisparity-=1;
    break; 
		
  }

  printf("k: %d pfs:%d, pfc:%d, sws:%d  md:%d\n", key,
	 frame->stereo_state->preFilterSize,frame->stereo_state->preFilterCap,frame->stereo_state->SADWindowSize,frame->stereo_state->minDisparity);
 }
}

#define DIF(x,y) ((x<y)?(y-x):(x-y))

static void
process_usb_frame(ctx_t *ctx,stereoframe_t * stereo,  leap_frame_t * leap_frame)
{
  int i;


  for(int yv = 0; yv < VFRAME_HEIGHT; yv ++){
    for(int xv = 0; xv < VFRAME_WIDTH; xv ++){
      unsigned char lval = leap_frame_get_pixel(leap_frame,
						LEFT,
						xv,yv);
      unsigned char rval = leap_frame_get_pixel(leap_frame,
						RIGHT,
						xv,yv);
      
      CvScalar l;
      l.val[2] = lval;
      l.val[1] = lval;
      l.val[0] = lval; 
      CvScalar r;
      r.val[2] = rval;
      r.val[1] = rval;
      r.val[0] = rval; 
      cvSet2D(stereo->left, yv, xv, l);
      cvSet2D(stereo->right, yv, xv, r);
      
    }
  }
  
  computeStereoBM(stereo);
}


void init_stereo_frame(stereoframe_t *frame){
  memset(frame,0,sizeof(stereoframe_t));
  frame->stereo_state =  cvCreateStereoBMState(CV_STEREO_BM_BASIC, 64);
  frame->cv_image_depth = cvCreateMat (  VFRAME_HEIGHT,VFRAME_WIDTH, CV_16S);
  frame->cv_image_depth_aux = cvCreateImage (cvSize(VFRAME_WIDTH, 2 * VFRAME_HEIGHT),IPL_DEPTH_8U, 1);
  frame->left = cvCreateImage (cvSize(VFRAME_WIDTH, VFRAME_HEIGHT),IPL_DEPTH_8U, 1);
  frame->right = cvCreateImage (cvSize(VFRAME_WIDTH, VFRAME_HEIGHT),IPL_DEPTH_8U, 1);

  frame->stereo_state->preFilterSize 		= 61;
  frame->stereo_state->preFilterCap 		= 63;
  frame->stereo_state->SADWindowSize 		= 47;
  frame->stereo_state->minDisparity 		= -11;
  frame->stereo_state->numberOfDisparities 	= 64;
  frame->stereo_state->textureThreshold 	= 0;
  frame->stereo_state->uniquenessRatio 	= 0;
  frame->stereo_state->speckleWindowSize 	= 0;
  frame->stereo_state->speckleRange		= 0;
}


int
main(int argc, char *argv[])
{
  ctx_t ctx_data;

  memset(&ctx_data, 0, sizeof (ctx_data));

  ctx_t *ctx = &ctx_data;

  //  cvNamedWindow(WINDOWNAME, 0);
  //cvResizeWindow(WINDOWNAME, VFRAME_WIDTH, VFRAME_HEIGHT * 2);

  stereoframe_t stereo; 

  init_stereo_frame(&stereo);
  leap_ctx_t * leap = leap_create();
  
  if(leap ==0){
    printf("Couldn't access leap\n");
    return 1;
  }


 for ( ; ; ) {

    if ( ctx->quit )
      break ;
    leap_result_t ret;
    do{
      ret = leap_transfer(leap);
      if(ret == LEAP_ERROR){
	printf("error transfering frame\n");
	goto NEXT_FRAME; 
      }
    }while(ret != FRAME_TRANSFERED);
    
    process_usb_frame(ctx,&stereo, leap_get_current_frame(leap));
    process_video_frame(ctx, &stereo);
 NEXT_FRAME:;
  }

  return (0);
}

