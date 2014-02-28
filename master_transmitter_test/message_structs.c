#include "message_structs.h"
  
const uint8_t imc_message_length[IMC_MESSAGE_TYPE_COUNT + 1] = {0, sizeof(msg_initialize_t)+1, 
                              0+1/*sizeof(msg_status_t)*/, 0+1/*sizeof(msg_home_t)*/, sizeof(msg_queue_move_t)+1, 
                              sizeof(msg_get_param_t)+1, sizeof(msg_set_param_t), 0+1};

const uint8_t imc_resp_length[IMC_MESSAGE_TYPE_COUNT + 1] = {0+1, sizeof(rsp_initialize_t)+1, 
                              sizeof(rsp_status_t)+1, sizeof(rsp_home_t)+1, 0+1/*sizeof(rsp_queue_move_t)*/,
                              sizeof(rsp_get_param_t)+1, 0+1/*sizeof(rsp_set_param_t)*/, 0+1};
