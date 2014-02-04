#ifndef message_structs_h
#define message_structs_h

#include <stdint.h>

typedef struct __attribute__((__packed__)){
  uint16_t host_revision;
  uint8_t  reserved[6];
} msg_initialize_t;

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
  uint32_t param_value;
} msg_get_set_param_t;

#endif


