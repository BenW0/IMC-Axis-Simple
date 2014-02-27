#include "protocol/message_structs.h"
#include "protocol/constants.h"
#include "print.h"
#include "parser.h"
#include "parameters.h"
#include <usb_serial.h>

rsp_get_param_t resp;

int main(void){
  initialize_parser();
  reset_parameters();

  while(1){
    if(usb_serial_available()){
      uint8_t c = usb_serial_getchar();
      feed_data(&c,1);
    }
    if(parser.status == PARSER_NEW_EVENT){
      uint8_t* v = (uint8_t*) &resp;
      int i;
      switch(parser.packet_type){
      case IMC_MSG_GETPARAM:
	handle_get_parameter(&parser.packet.get_param, &resp);
	usb_serial_putchar(IMC_RSP_OK);
	for(i = 0; i < sizeof(resp); i++)
	  usb_serial_putchar(v[i]);
	break;
      case IMC_MSG_SETPARAM:
	handle_set_parameter(&parser.packet.set_param);
	usb_serial_putchar(IMC_RSP_OK);
	break;
      default:;
      }
      initialize_parser();
    }
  }
}
