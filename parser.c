#include "protocol/message_structs.h"
#include "protocol/constants.h"
#include "parser.h"
#include "i2c_slave.h"
#include <usb_serial.h>

#include "hardware.h"
volatile parser_state_t parser;
generic_response response;

void initialize_parser(void){
  parser.status = PARSER_EMPTY;
  parser.packet_type = 0; // 0 is guaranteed to not be a packet type byte
  parser.head = (uint8_t*) &parser.packet;
  memset(&parser.packet, 0, sizeof(parser.packet));
}

void feed_data(uint8_t input){
  if(parser.status == PARSER_NEW_EVENT || parser.status == PARSER_ERR){
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
    parser.remaining = imc_message_length[type] + 1; // Include an extra for the checksum
    parser.head = (uint8_t*) &parser.packet;
    return;
  }
  if(parser.remaining-- > 0){
    //   usb_serial_putchar(input + 'a');
    *parser.head++ = input;
  }
  // If we've finished an entire packet, signal that
  if(parser.remaining == 0){
    //   usb_serial_putchar('D');
    uint8_t sum = parser.packet_type; 
    uint32_t size = imc_message_length[sum];
    uint32_t i;
    for(i = 0; i < size; i++){
      //   usb_serial_putchar(((uint8_t*) &parser.packet)[i] + 'a');
      sum ^= ((uint8_t*) &parser.packet)[i];
    }

    // usb_serial_putchar('0' + size);
      
    if(sum == (((uint8_t*) &parser.packet)[size])){
      //   usb_serial_putchar('K');
      parser.status = PARSER_NEW_EVENT;
    }else{
      //     usb_serial_putchar('E');
    
      parser.status = PARSER_ERR;
    }
  }
}


void send_response(imc_response_type status,uint32_t size){
  uint8_t checksum = status;
  uint32_t i;
  txBufferLength = size+2;
  txBuffer[0] = status;

  for(i = 0; i < size; i++){
    uint8_t byte = ((uint8_t*) &response)[i];
    checksum ^= byte;
    txBuffer[i+1] = byte;
  }
  txBuffer[size+1] = checksum;
}


