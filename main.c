#include "parser.h"
#include "queue.h"

#include "protocol/constants.h"
#include "protocol/message_structs.h"
#include "hardware.h"
#include "parameters.h"
#include <usb_serial.h>
#include <mk20dx128.h>
#include <pin_config.h>
void reset_state(void){
  initialize_motion_queue();
  initialize_parser();
  reset_parameters();
}

int main(void){
  // Configure all of the hardware and internal state
  reset_state();
  reset_hardware();

  while(1){
    // In reality, we'll be feeding this from i2c, probably in an interrupt
    if(usb_serial_available()){
      uint8_t c = usb_serial_getchar();
      feed_data(&c,1);
    }
    if(parser.status == PARSER_NEW_EVENT){
      switch(parser.packet_type){
      case IMC_MSG_INITIALIZE:
	reset_state();
	// Also, reset sync line
	response.init.slave_hw_ver = 0;
	response.init.slave_fw_ver = 0;
	response.init.queue_depth = MOTION_QUEUE_LENGTH;
	send_response(IMC_RSP_OK,sizeof(rsp_initialize_t));
	break;
      case IMC_MSG_GETPARAM:
	// 	handle_get_parameter(&parser.packet.get_param, &response.param);
	send_response(IMC_RSP_OK,sizeof(rsp_get_param_t));
	break;
      case IMC_MSG_SETPARAM:
	//	handle_set_parameter(&parser.packet.set_param);
	send_response(IMC_RSP_OK,0);
	break;	
      case IMC_MSG_QUEUEMOVE:
	{
	  int space = enqueue_block(&parser.packet.move);
	  send_response(space < 0 ? IMC_RSP_QUEUEFULL : IMC_RSP_OK,0);
	}
	break;
      case IMC_MSG_STATUS:
      case IMC_MSG_HOME:
      case IMC_MSG_QUICKSTOP:
	send_response(IMC_RSP_ERROR,0);
	break;
      }
      initialize_parser();
    }
  }
}
