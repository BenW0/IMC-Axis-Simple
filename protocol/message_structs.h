#ifndef message_structs_h
#define message_structs_h

#include <stdint.h>
#include "constants.h"

////////////////////// Message Packet Structures /////////////////////////////////
// All
typedef struct __attribute__((__packed__)){
  uint16_t host_revision;
  uint8_t  reserved[6];
} msg_initialize_t;

typedef struct __attribute__((__packed__)){
  ;  // no fields in message
} msg_status_t;

typedef struct __attribute__((__packed__)){
  ;  // no fields in message
} msg_home_t;

typedef struct { // Assume we don't have to pad this on 32-bit systems
  int32_t length;
  uint32_t total_length;
  uint32_t initial_rate;
  uint32_t final_rate;
  uint32_t acceleration;
  uint32_t stop_accelerating;
  uint32_t start_decelerating;
} msg_queue_move_t;

typedef struct __attribute__((__packed__)){
  uint8_t param_id;
} msg_get_param_t;

// Note from Ben: I think we should swap this struct to have the value first to enhance memory alignment on 32-bit systems
typedef struct __attribute__((__packed__)){
  uint8_t param_id;
  uint32_t param_value;
} msg_set_param_t;


///////////////////////// Response Packet Structures /////////////////////////////////
typedef struct __attribute__((__packed__)){
  imc_response_type response;
  uint16_t slave_hw_ver;
  uint16_t slave_fw_ver;
} rsp_initialize_t;

typedef struct __attribute__(__packed__)){
  imc_response_type response;
  imc_axis_error status;
  int32_t location;
  uint8_t queued_moves;
  uint32_t sync_error;
} rsp_status_t;

typedef struct __attribute__((__packed__)){
  imc_response_type response;
  int32_t old_position;
} rsp_home_t;

typedef struct __attribute__ ((__packed__)){
  imc_response_type response;
} rsp_queue_move_t;

typedef struct __attribute__((__packed__)){
  imc_response_type response;
  uint32_t value;
} rsp_get_param_t;

typedef struct __attribute__((__packed__)){
  imc_response_type response;
} rsp_set_param_t;
  
  
const uint8_t imc_message_type_count = 6;    // number of message types defined.

const uint8_t imc_message_length[imc_message_type_count + 1] = {0, sizeof(msg_initialize_t), 
                              sizeof(msg_status_t), sizeof(msg_home_t), sizeof(msg_queue_move_t), 
                              sizeof(msg_get_param_t), sizeof(msg_set_param_t)};

const uint8_t imc_resp_length[imc_message_type_count + 1] = {0, sizeof(rsp_initialize_t), 
                              sizeof(rsp_status_t), sizeof(rsp_home_t), sizeof(rsp_queue_move_t),
                              sizeof(rsp_get_param_t), sizeof(rsp_set_param_t)};
#endif


