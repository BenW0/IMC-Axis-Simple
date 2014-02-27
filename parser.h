#ifndef parser_h
#define parser_h

#include "protocol/message_structs.h"
#include "protocol/constants.h"

#define PARSER_NEW_EVENT 0
#define PARSER_EMPTY 1
#define PARSER_ERR 2

typedef struct {
  uint32_t status;
  imc_message_type packet_type;
  uint32_t remaining;
  uint8_t* head;
  union {
    msg_initialize_t init;
    msg_queue_move_t move;    
    msg_get_param_t get_param;
    msg_set_param_t set_param;
  } packet;
} parser_state_t;

void initialize_parser(void);
uint32_t feed_data(const uint8_t*, uint32_t);

extern volatile parser_state_t parser;
#endif
