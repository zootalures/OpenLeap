/*
 ** Author: Elina Lijouvni
 ** License: GPL v3
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <cv.h>
#include <highgui.h>

typedef struct ctx_s ctx_t;
struct ctx_s
{
  int quit;
};


#define VFRAME_WIDTH  640
#define VFRAME_HEIGHT 240
#define VFRAME_SIZE   (VFRAME_WIDTH * VFRAME_HEIGHT)

typedef struct frame_s frame_t;
struct frame_s
{

  uint32_t id;
  uint32_t data_len;
  uint32_t state;
};


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
	int i, j, aux;
	uchar *ptr_dst;
	cvFindStereoCorrespondenceBM ( 
		data->left,
		data->right, 
		data->cv_image_depth, 
		data->stereo_state
	);
	//Normalize the result so we can display it
	cvNormalize( data->cv_image_depth, data->cv_image_depth, 0, 256, CV_MINMAX, NULL );
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

  cvShowImage(WINDOWNAME, frame->cv_image_depth_aux );
  key = cvWaitKey(1);

if(key > 0){
  
  switch(key){
    
     case 1048689: 
	    ctx->quit = 1;
            break; 
     case 1048695: 
	frame->stereo_state->preFilterSize+=2;
        break; 
     case 1048691: 
	frame->stereo_state->preFilterSize-=2;
        break; 

    case 1048677: 
	frame->stereo_state->preFilterCap++;
        break; 
     case 1048676: 
	frame->stereo_state->preFilterCap--;
        break; 

    case 1048690: 
	frame->stereo_state->SADWindowSize+=2;
        break; 
     case 1048678: 
	frame->stereo_state->SADWindowSize-=2;
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
process_usb_frame(ctx_t *ctx,stereoframe_t * stereo,  frame_t *frame, unsigned char *data, int size)
{
  int i;

  int bHeaderLen = data[0];
  int bmHeaderInfo = data[1];

  uint32_t dwPresentationTime = *( (uint32_t *) &data[2] );
  //printf("frame time: %u\n", dwPresentationTime);

  if (frame->id == 0)
    frame->id = dwPresentationTime;

  for (i = bHeaderLen; i < size ; i += 2) {
    if (frame->data_len >= VFRAME_SIZE)
      break ;

    CvScalar l;
    l.val[2] = data[i];
    l.val[1] = data[i];
    l.val[0] = data[i]; 
    CvScalar r;
    r.val[2] = data[i+1];
    r.val[1] = data[i+1];
    r.val[0] = data[i+1]; 

    int x = frame->data_len % VFRAME_WIDTH;
    int y = frame->data_len / VFRAME_WIDTH;
    cvSet2D(stereo->left, y, x, l);
    cvSet2D(stereo->right, y, x, r);


    frame->data_len++;
  }

  if (bmHeaderInfo & UVC_STREAM_EOF) {
    //printf("End-of-Frame.  Got %i\n", frame->data_len);

    if (frame->data_len != VFRAME_SIZE) {
      //printf("wrong frame size got %i expected %i\n", frame->data_len, VFRAME_SIZE);
      frame->data_len = 0;
      frame->id = 0;
      return ;
    }

    computeStereoBM(stereo);
    process_video_frame(ctx, stereo);
    frame->data_len = 0;
    frame->id = 0;
  }
  else {
    if (dwPresentationTime != frame->id && frame->id > 0) {
      //printf("mixed frame TS: dropping frame\n");
      frame->id = dwPresentationTime;
      /* frame->data_len = 0; */
      /* frame->id = 0; */
      /* return ; */
    }
  }
}


void init_stereo_frame(stereoframe_t *frame){
   memset(frame,0,sizeof(stereoframe_t));
   frame->stereo_state =  cvCreateStereoBMState(CV_STEREO_BM_BASIC, 64);
   frame->cv_image_depth = cvCreateMat (  VFRAME_HEIGHT,VFRAME_WIDTH, CV_16S);
   frame->cv_image_depth_aux = cvCreateImage (cvSize(VFRAME_WIDTH, 2 * VFRAME_HEIGHT),IPL_DEPTH_8U, 1);
   frame->left = cvCreateImage (cvSize(VFRAME_WIDTH, VFRAME_HEIGHT),IPL_DEPTH_8U, 1);
   frame->right = cvCreateImage (cvSize(VFRAME_WIDTH, VFRAME_HEIGHT),IPL_DEPTH_8U, 1);

   frame->stereo_state->preFilterSize 		= 49;
   frame->stereo_state->preFilterCap 		= 63;
   frame->stereo_state->SADWindowSize 		= 11;
   frame->stereo_state->minDisparity 		= 2;
   frame->stereo_state->numberOfDisparities 	= 32;
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

  cvNamedWindow(WINDOWNAME, 0);
  cvResizeWindow(WINDOWNAME, VFRAME_WIDTH, VFRAME_HEIGHT * 2);

  frame_t frame;
  memset(&frame, 0, sizeof (frame));


  stereoframe_t stereo; 

  init_stereo_frame(&stereo);

  for ( ; ; ) {
    unsigned char data[16384];
    int usb_frame_size;

    if ( feof(stdin) || ctx->quit )
      break ;

    fread(&usb_frame_size, sizeof (usb_frame_size), 1, stdin);
    fread(data, usb_frame_size, 1, stdin);

    process_usb_frame(ctx,&stereo, &frame, data, usb_frame_size);
  }

  return (0);
}

