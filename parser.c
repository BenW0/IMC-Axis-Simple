#include "protocol/message_structs.h"
#include "protocol/constants.h"
#include "parser.h"

volatile parser_state_t parser;

void initialize_parser(void){
  parser.status = PARSER_EMPTY;
  parser.packet_type = 0; // 0 is guaranteed to not be a packet type byte
  parser.head = (uint8_t*) &parser.packet;
}

uint32_t feed_data(const uint8_t* input, uint32_t length){ 
  uint32_t consumed = 0;
  if(parser.status == PARSER_NEW_EVENT)
    return consumed;
  
  if(0 == parser.packet_type){
    imc_message_type type = (imc_message_type) input[0];
    consumed++;
    
    if(type < IMC_MSG_INITIALIZE || IMC_MSG_QUICKSTOP < type){ // First and last members of  imc_message_type
      parser.status = PARSER_ERR;
      return consumed;
    }
    // Otherwise, we have a valid message type. Figure out how long it should be.
    parser.packet_type = type;
    parser.remaining = imc_message_length[type];
    parser.head = (uint8_t*) &parser.packet;
  }
  // Now start consuming bytes until we either run out or have a complete packet
  while(consumed < length && parser.remaining > 0){
    *parser.head++ = input[consumed++];
    parser.remaining--;
  }
  // If we've finished an entire packet, signal that
  if(parser.remaining == 0)
    parser.status = PARSER_NEW_EVENT;
  return consumed;
}
