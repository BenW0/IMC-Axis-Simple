#include "message_structs.h"
  
const uint8_t imc_message_type_count = 6;    // number of message types defined.

const uint8_t imc_message_length[7] = {0, sizeof(msg_initialize_t), 
                              0/*sizeof(msg_status_t)*/, 0/*sizeof(msg_home_t)*/, sizeof(msg_queue_move_t), 
                              sizeof(msg_get_param_t), sizeof(msg_set_param_t)};

const uint8_t imc_resp_length[7] = {0, sizeof(rsp_initialize_t), 
                              sizeof(rsp_status_t), sizeof(rsp_home_t), 0/*sizeof(rsp_queue_move_t)*/,
                              sizeof(rsp_get_param_t), 0/*sizeof(rsp_set_param_t)*/};
