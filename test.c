#include "hardware.h"
#include "stepper.h"
#include "queue.h"
#include "sync.h"
#include "protocol/message_structs.h"



int main(void){
  msg_queue_move_t test;
  
  test.length = 500;
  test.total_length = 500;
  test.initial_rate = 0;
  test.final_rate = 0;
  test.nominal_rate = 4000;
  test.acceleration = 128000;
  test.stop_accelerating = 120;
  test.start_decelerating = 350;
  
  configure_stepper_gpio();
  initialize_motion_queue();
  enqueue_block(&test);
  enqueue_block(&test);
  enqueue_block(&test);
  initialize_stepper();
  enable_stepper_int();
  enable_sync_isr();

  while(1);
}
