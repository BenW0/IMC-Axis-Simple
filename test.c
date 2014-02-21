#include "hardware.h"
#include "stepper.h"
#include "queue.h"
#include "sync.h"
#include "protocol/message_structs.h"
#include <pin_config.h>


int main(void){
  msg_queue_move_t test;
  
  test.length = 100;
  test.total_length = 100;
  test.initial_rate = 800;
  test.final_rate = 900;
  test.nominal_rate = 4000;
  test.acceleration = 128000;
  test.stop_accelerating = 10;
  test.start_decelerating = 90;

  SYNC_CTRL = STANDARD_OUTPUT;
  CONTROL_DDR |= SYNC_BIT;
  CONTROL_PORT(COR) = SYNC_BIT;  

  configure_stepper_gpio();
  initialize_motion_queue();
  enqueue_block(&test);
  enqueue_block(&test);
  enqueue_block(&test);
  enqueue_block(&test);
  enqueue_block(&test);
  initialize_stepper();
  enable_stepper_int();
  enable_sync_isr();

  while(1);
}
