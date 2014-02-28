#include "parser.h"
#include "queue.h"

#include "protocol/constants.h"
#include "protocol/message_structs.h"
#include "hardware.h"
#include "parameters.h"
#include "stepper.h"
#include "control_isr.h"
#include "i2c_slave.h"
#include <usb_serial.h>
#include <mk20dx128.h>
#include <pin_config.h>

void reset_state(void){
  initialize_motion_queue();
  initialize_parser();
  reset_parameters();
  initialize_stepper_state();
}

int main(void){
  initialize_i2c(2);
  // Configure all of the hardware and internal state
  reset_state();
  reset_hardware();
  while(1){
    if(parser.status == PARSER_NEW_EVENT){
      switch(parser.packet_type){
      case IMC_MSG_INITIALIZE:
	reset_state();
	enable_sync_interrupt();
	STEPPER_PORT(SOR) = STEP_BIT;

 	response.init.slave_hw_ver = 0;
	response.init.slave_fw_ver = 0;
	response.init.queue_depth = MOTION_QUEUE_LENGTH;
	send_response(IMC_RSP_OK,sizeof(rsp_initialize_t));
	break;
      case IMC_MSG_GETPARAM:
	handle_get_parameter(&parser.packet.get_param, &response.param);
	send_response(IMC_RSP_OK,sizeof(rsp_get_param_t));
	break;
      case IMC_MSG_SETPARAM:
	handle_set_parameter(&parser.packet.set_param);
	send_response(IMC_RSP_OK,0);
	break;	
      case IMC_MSG_QUEUEMOVE:
	{
	  msg_queue_move_t test;
	  int space;
	  test.length = 100;
	  test.total_length = 100;
	  test.initial_rate = 100;
	  test.final_rate = 100;
	  test.nominal_rate = 4000;
	  test.acceleration = 128000;
	  test.stop_accelerating = 20;
	  test.start_decelerating = 80;

	  space = enqueue_block(&test);

	  send_response(space < 0 ? IMC_RSP_QUEUEFULL : IMC_RSP_OK,0);
	  // If we're adding moves in idle state, make sure that the sync interface is listening
	  if(st.state == STATE_IDLE)
	    enable_sync_interrupt();
	}
	break;
      case IMC_MSG_STATUS:
	response.status.location = get_position();
	response.status.sync_error = parameters.sync_error;
	response.status.queued_moves = queue_length();
	if(st.state == STATE_ERROR){
	  response.status.status = parameters.error_low;
	}else{
	  response.status.status = IMC_ERR_NONE;
	}
	send_response(IMC_RSP_OK,sizeof(rsp_status_t));
	break;
      case IMC_MSG_HOME:
      case IMC_MSG_QUICKSTOP:
	send_response(IMC_RSP_ERROR,0);
	break;
      }
      initialize_parser();
    }
  }
}
