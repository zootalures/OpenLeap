

#include <stdio.h> 
#include "low-level-leap.h" 

int main(int argc, const char** argv){
  leap_ctx_t * leap = leap_create();
  
  if(leap == 0){
    fprintf(stderr, "failed to set up device\n");
    return 1;
  }
  fprintf(stderr, "Set up device\n");
  while(PARTIAL_RESULT == leap_transfer(leap)){  
  }

  leap_frame_t * frame = leap_get_current_frame(leap);

  fprintf(stderr, "Frame %d transfered, size : %d \n", 
	  leap_frame_get_id(frame),
	  leap_frame_get_size(frame));

}
