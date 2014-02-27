#include "protocol/message_structs.h"
#include "protocol/constants.h"
#include "print.h"
#include "parser.h"
#include "parameters.h"
#include <usb_serial.h>

int main(void){
  initialize_parser();
  reset_parameters();

  while(1){
    if(usb_serial_available()){
      uint8_t c = usb_serial_getchar();
      feed_data(&c,1);
    }
    if(parser.status == PARSER_NEW_EVENT){
      switch(parser.packet_type){
      case IMC_MSG_GETPARAM:
	handle_get_parameter(&parser.packet.get_param, &response.param);
	send_response(IMC_RSP_OK,sizeof(rsp_get_param_t));
	break;
      case IMC_MSG_SETPARAM:
	handle_set_parameter(&parser.packet.set_param);
	send_response(IMC_RSP_OK,0);
	break;
      default:;
      }
      initialize_parser();
    }
  }
}
