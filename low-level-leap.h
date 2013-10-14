#ifndef LOW_LEVEL_LEAP_H
#define LOW_LEVEL_LEAP_H

typedef struct leap_ctx_s leap_ctx_t;
typedef struct leap_frame_s leap_frame_t;

enum leap_result_enum{
  PARTIAL_RESULT, 
  FRAME_TRANSFERED, 
  LEAP_ERROR
};

typedef enum leap_result_enum leap_result_t;

/**
 * create a leap context 
 */
leap_ctx_t * leap_create();

/**
 * Performs a transfer,  returns 1 if a new frame is ready 
 */
leap_result_t  leap_transfer(leap_ctx_t *ctx); 


leap_frame_t *  leap_get_current_frame(leap_ctx_t * ctx);


size_t leap_frame_get_size(leap_frame_t * frame);

int leap_frame_get_id(leap_frame_t * frame);

enum leap_frame_type{ 
  LEFT,
  RIGHT
};

typedef enum leap_frame_type leap_frame_side_t;

unsigned char  leap_frame_get_pixel(leap_frame_t * frame, leap_frame_side_t type, int x, int y);



void leap_close(leap_ctx_t * ctx);
#endif 
