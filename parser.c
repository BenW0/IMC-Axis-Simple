#include "protocol/message_structs.h"
#include "protocol/constants.h"
#include "parser.h"
#include <usb_serial.h>

#include "hardware.h"
#include <mk20dx128.h>

volatile parser_state_t parser;
generic_response response;

void initialize_parser(void){
  parser.status = PARSER_EMPTY;
  parser.packet_type = 0; // 0 is guaranteed to not be a packet type byte
  parser.head = (uint8_t*) &parser.packet;
}

void feed_data(uint8_t input){
  if(parser.status == PARSER_NEW_EVENT){
    // Haven't dealt with the event soon enough, and we have no queue,
    // so this is an error. I2C shouldn't be prone to this, however
    parser.status = PARSER_ERR; 
    return;
  }
  if(0 == parser.packet_type){
    imc_message_type type = (imc_message_type) input;
    if(type < IMC_MSG_INITIALIZE || IMC_MSG_QUICKSTOP < type){ // First and last members of imc_message_type
      parser.status = PARSER_ERR;
      return;
    }
    parser.packet_type = type;
    parser.remaining = imc_message_length[type];
    parser.head = (uint8_t*) &parser.packet;
  }
  
  if(parser.remaining-- > 0){
    *parser.head++ = input;
  }
  // If we've finished an entire packet, signal that
  if(parser.remaining == 0)
    parser.status = PARSER_NEW_EVENT;
}

void send_response(imc_response_type status,uint32_t size){
  // At the moment, we're just going through usb serial. In reality, we should
  // write this to the i2c interface
  usb_serial_putchar(status);
  usb_serial_write(&response, size);
}
