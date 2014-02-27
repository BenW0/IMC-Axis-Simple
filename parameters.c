#include "parameters.h"
#include "protocol/constants.h"
#include "protocol/message_structs.h"

parameters_t parameters;

void reset_parameters(void){
  parameters.error_low = 0;
  parameters.error_high = 0;
  parameters.homing = ENABLE_MIN; // Don't flip, home low, no software max, don't invert
  parameters.min_pos = 0;
  parameters.max_pos = 0;

  parameters.home_pos = 0;
  parameters.homing_feedrate = 60000; // Arbitrary, but reasonable
  parameters.motor_on = 0;
  parameters.motor_timeout = 1<<30; // Big number is big
  // Position lives in the stepper state, as it is volatile
  parameters.slowdown = 0;
  parameters.sync_error = 0;
}

uint32_t const_to_mask(imc_axis_parameter c){
  switch(c){
  case IMC_PARAM_FLIP_AXIS:
    return FLIP_AXIS;
  case IMC_PARAM_HOME_DIR:
    return HOME_DIR;
  case IMC_PARAM_MIN_SOFTWARE_ENDSTOPS:
    return MIN_SOFTWARE;
  case IMC_PARAM_MAX_SOFTWARE_ENDSTOPS:
    return MAX_SOFTWARE;
  case IMC_PARAM_MIN_LIMIT_EN:
    return ENABLE_MIN;
  case IMC_PARAM_MAX_LIMIT_EN:
    return ENABLE_MAX;
  case IMC_PARAM_MIN_LIMIT_INV:
    return INVERT_MIN;
  case IMC_PARAM_MAX_LIMIT_INV:
    return INVERT_MAX;
  default:
    return 0;
  }
}

void handle_get_parameter(msg_get_param_t* msg,rsp_get_param_t* rsp){
  switch(msg->param_id){
  case IMC_PARAM_ERROR_INFO1:
    rsp->value = parameters.error_low;
    break;
  case IMC_PARAM_ERROR_INFO2:
    rsp->value = parameters.error_high;
    break;
  case IMC_PARAM_FLIP_AXIS:
  case IMC_PARAM_HOME_DIR:
  case IMC_PARAM_MIN_SOFTWARE_ENDSTOPS:
  case IMC_PARAM_MAX_SOFTWARE_ENDSTOPS:
  case IMC_PARAM_MIN_LIMIT_EN:
  case IMC_PARAM_MAX_LIMIT_EN:
  case IMC_PARAM_MIN_LIMIT_INV:
  case IMC_PARAM_MAX_LIMIT_INV:
    rsp->value = const_to_mask(msg->param_id) & parameters.homing ? 1 : 0;
    break;
  case IMC_PARAM_MIN_POS:
    rsp->value = parameters.min_pos;
    break;
  case IMC_PARAM_MAX_POS:
    rsp->value = parameters.max_pos;
    break;
  case IMC_PARAM_HOME_POS:
    rsp->value = parameters.home_pos;
    break;
  case IMC_PARAM_HOMING_FEEDRATE:
    rsp->value = parameters.homing_feedrate;
    break;
  case IMC_PARAM_MOTOR_ON:
    rsp->value = parameters.motor_on;
    break;
  case IMC_PARAM_MOTOR_IDLE_TIMEOUT:
    rsp->value = parameters.motor_timeout;
    break;
  case IMC_PARAM_LOCATION:
    // Consult the stepper driver state
    ;
    break;
  case IMC_PARAM_SLOWDOWN:
    rsp->value = parameters.slowdown;
    break;
  case IMC_PARAM_SYNC_ERROR:
    rsp->value = parameters.sync_error;
    break;
  default:
    ;
  }
}

void handle_set_parameter(msg_set_param_t* msg){
  // Can't set error info or sync_error
  // Pullups and motor on/off are special, as we actually have to do io
  uint32_t mask;
  switch(msg->param_id){
  case IMC_PARAM_FLIP_AXIS:
  case IMC_PARAM_HOME_DIR:
  case IMC_PARAM_MIN_SOFTWARE_ENDSTOPS:
  case IMC_PARAM_MAX_SOFTWARE_ENDSTOPS:
  case IMC_PARAM_MIN_LIMIT_EN:
  case IMC_PARAM_MAX_LIMIT_EN:
  case IMC_PARAM_MIN_LIMIT_INV:
  case IMC_PARAM_MAX_LIMIT_INV:
    mask = const_to_mask(msg->param_id);
    parameters.homing = ((~mask) & parameters.homing) | (msg->param_value ? mask : 0);
    break;
  case IMC_PARAM_MIN_POS: parameters.min_pos = msg->param_value; break;
  case IMC_PARAM_MAX_POS: parameters.max_pos = msg->param_value; break;
  case IMC_PARAM_HOMING_FEEDRATE: parameters.homing_feedrate = msg->param_value; break;
  case IMC_PARAM_HOME_POS: parameters.home_pos = msg->param_value; break;
  case IMC_PARAM_MOTOR_IDLE_TIMEOUT: parameters.motor_timeout = msg->param_value; break;
  case IMC_PARAM_SLOWDOWN: parameters.slowdown = msg->param_value; break;
  case IMC_PARAM_MIN_LIMIT_PULLUP:
    // Do some IO to set the pullup value directly
    break;
  case IMC_PARAM_MAX_LIMIT_PULLUP:
    // Do some IO to set the pullup value directly
    break;
  case IMC_PARAM_MOTOR_ON:
    // Do some IO to turn the motor on or off
    parameters.motor_on = msg->param_value; 
    break;
  default:
    break;
  }
}
