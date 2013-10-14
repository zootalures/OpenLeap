/*
 ** Author: Elina Lijouvni,Owen Cliffe
 ** License: GPL v3
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <libusb.h>

#include "low-level-leap.h" 
#ifdef NDEBUG
#define debug_printf(...)
#else
#define debug_printf(...) fprintf(stderr, __VA_ARGS__)
#endif

#define VFRAME_WIDTH  (640)
#define VFRAME_HEIGHT (240)
#define VFRAME_SIZE   (VFRAME_WIDTH * VFRAME_HEIGHT)
#define BOTHFRAME_SIZE   (VFRAME_SIZE * 4)

#define MAX_PACKET_SIZE (16384)
#define LEAP_VID (0xf182)
#define LEAP_PID (0x0003)
#define UVC_STREAM_EOF                                  (1 << 1)


/**
 * A single bulk transfer buffer off the device 
 */
typedef struct usb_message_s usb_message_t;
struct usb_message_s{
  uint32_t presentationTime;
  unsigned char data[MAX_PACKET_SIZE];
};

/**
 * a whole video frame in memory (interleaved) 
 */
typedef struct leap_frame_s leap_frame_t;
struct leap_frame_s 
{
  unsigned int id;
  size_t  frame_size;
  unsigned char data[BOTHFRAME_SIZE];
};


typedef struct leap_ctx_s leap_ctx_t;

struct leap_ctx_s
{
  libusb_context       *libusb_ctx;
  libusb_device_handle *dev_handle;

  int current_frame;
  leap_frame_t frames[2];
  usb_message_t last_message;
};



void
fprintf_data(FILE *fp, const char * title, unsigned char *data, size_t size)
{
  int i;

  debug_printf("%s", title);
  for (i = 0; i < size; i++) {
    if ( ! (i % 16) )
      debug_printf("\n");
    debug_printf("%02x ", data[i]);
  }
  debug_printf("\n");
}




leap_frame_t *  leap_get_current_frame(leap_ctx_t * ctx){
  return &(ctx->frames[ctx->current_frame]);
}

leap_frame_t *  leap_get_next_frame(leap_ctx_t * ctx){
  return &(ctx->frames[((ctx->current_frame) + 1 )% 2 ]);
}



static void leap_init_device(leap_ctx_t *ctx)
{
  int ret;
  unsigned char data[256];

#include "leap_libusb_init.c.inc"
}




leap_ctx_t * leap_create(){
  leap_ctx_t *ctx = (leap_ctx_t*) malloc(sizeof(leap_ctx_t));
  int ret;
  memset(ctx, 0, sizeof (leap_ctx_t));

  libusb_init( & ctx->libusb_ctx );

  ctx->dev_handle = libusb_open_device_with_vid_pid(ctx->libusb_ctx, LEAP_VID, LEAP_PID);
  if (ctx->dev_handle == NULL) {
    fprintf(stderr, "ERROR: can't find leap.\n");
    return 0;
  }

  debug_printf("Found leap\n");


  ret = libusb_reset_device(ctx->dev_handle);
  debug_printf("libusb_reset_device() ret: %i: %s\n", ret, libusb_error_name(ret));

  ret = libusb_kernel_driver_active(ctx->dev_handle, 0);
  if ( ret == 1 ) {
    debug_printf("kernel active for interface 0\n");
    libusb_detach_kernel_driver(ctx->dev_handle, 0);
  }
  else if (ret != 0) {
    printf("error\n");
    return 0;
  }

  ret = libusb_kernel_driver_active(ctx->dev_handle, 1);
  if ( ret == 1 ) {
    debug_printf("kernel active\n");
    libusb_detach_kernel_driver(ctx->dev_handle, 1);
  }
  else if (ret != 0) {
    printf("error\n");
    return 0;
  }

  ret = libusb_claim_interface(ctx->dev_handle, 0);
  debug_printf("libusb_claim_interface() ret: %i: %s\n", ret, libusb_error_name(ret));

  ret = libusb_claim_interface(ctx->dev_handle, 1);
  debug_printf("libusb_claim_interface() ret: %i: %s\n", ret, libusb_error_name(ret));

  leap_init_device(ctx);

  debug_printf( "max %i\n",  libusb_get_max_packet_size(libusb_get_device( ctx->dev_handle ), 0x83));
  return ctx;
}

void reset_frame(leap_frame_t * frame){
    frame->id =0;
    frame->frame_size  =0;
}

void swap_frames(leap_ctx_t * ctx){
  // swap buffers
  ctx->current_frame = (ctx->current_frame + 1)%2;  
}

leap_result_t leap_transfer(leap_ctx_t *ctx){
  int ret;
  int read_size = 0;
  usb_message_t * message = &(ctx->last_message);
  unsigned char * data = message->data;
  
  ret = libusb_bulk_transfer(ctx->dev_handle, 0x83, message->data, 
			     MAX_PACKET_SIZE,
			     &read_size, 1000);
  
  if (ret != 0) {
    printf("libusb_bulk_transfer(): %i: %s\n", ret, libusb_error_name(ret));
    return LEAP_ERROR;
  }
  
  int bHeaderLen = data[0];
  int bmHeaderInfo = data[1];
  uint32_t dwPresentationTime = *( (uint32_t *) &data[2] );


  debug_printf("read usb frame packet of %i bytes\n", read_size);


  leap_frame_t * frame;
  frame = leap_get_next_frame(ctx);
  if (frame->id == 0){
    frame->id = dwPresentationTime;
  }

  int data_size = read_size - bHeaderLen;
  if(frame->frame_size +data_size  > BOTHFRAME_SIZE){
    printf("Too many bytes in a frame ");
    return LEAP_ERROR;
  }
  
  memcpy(frame->data + frame->frame_size, data  + bHeaderLen,data_size);
  frame->frame_size += (data_size);
  
  if (bmHeaderInfo & UVC_STREAM_EOF) {  
    
    if(frame->frame_size != VFRAME_SIZE * 2){
      debug_printf("Invalid frame length, got %d, expecting %d , skipping frame\n", frame->frame_size, VFRAME_SIZE * 2);
      reset_frame(frame);
      return PARTIAL_RESULT;    
    }else{
      swap_frames(ctx);
      reset_frame(leap_get_next_frame(ctx));      
      return FRAME_TRANSFERED;
    }
  }else{
    return PARTIAL_RESULT;
  }
}


void leap_close_context(leap_ctx_t * ctx){
  libusb_exit(ctx->libusb_ctx);
  free(ctx);
}


size_t leap_frame_get_size(leap_frame_t * frame){
  return frame->frame_size;
}

int leap_frame_get_id(leap_frame_t * frame){
  return frame->id;
}

unsigned char  
  leap_frame_get_pixel(leap_frame_t * frame, 
		       leap_frame_side_t type, 
		       int x, 
		       int y){
  
  return frame->data[((y * VFRAME_WIDTH) + x) * 2 + (type == RIGHT?1:0)];
  
}


