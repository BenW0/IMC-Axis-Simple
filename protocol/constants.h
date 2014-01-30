/* 
This file contains definitions for all constants in the specification of the IMCC protocol.

(C) 2014, Ben Weiss & Matthew Sorensen
 */
#ifndef imc_protocol_constants_h
#define imc_protocol_constants_h

typedef enum __attribute__ ((__packed__)) {
  IMC_MSG_INITIALIZE,
  IMC_MSG_STATUS,
  IMC_MSG_HOME,
  IMC_MSG_GETPARAM,
  IMC_MSG_QUEUEMOVE,
  IMC_MSG_SETPARAM,
} imc_message_type;

typedef enum __attribute__ ((__packed__)) {
  IMC_RSP_OK,
  IMC_RSP_UNKNOWN,
  IMC_RSP_ERROR,
  IMC_RSP_QUEUEFULL
} imc_response_type;

typedef enum __attribute__ ((__packed__)) {
  IMC_PARAM_ERROR_INFO1,
  IMC_PARAM_ERROR_INFO2,
  IMC_PARAM_FLIP_AXIS,
  IMC_PARAM_ENFORCE_LIMITS,
  IMC_PARAM_MIN_LIMIT_EN,
  IMC_PARAM_MIN_LIMIT_INV,
  IMC_PARAM_MIN_LIMIT_PULLUP,
  IMC_PARAM_MAX_LIMIT_EN,
  IMC_PARAM_MAX_LIMIT_INV,
  IMC_PARAM_MAX_LIMIT_PULLUP,
  IMC_PARAM_MOTOR_ON,
  IMC_PARAM_MOTOR_IDLE_TIMEOUT,
  IMC_PARAM_SLOWDOWN
} imc_axis_parameter;

typedef enum __attribute__ ((__packed__)) {
  IMC_ERR_CONTROL,
  IMC_ERR_ELECTRICAL,
  IMC_ERR_MECHANICAL,
  IMC_ERR_TIMEOUT
} imc_axis_error;

#endif
