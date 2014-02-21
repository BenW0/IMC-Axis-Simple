#ifndef hardware_h
#include <mk20dx128.h>
#include "protocol/constants.h"
#define hardware_h
// All stepper control is placed on port 
#define STEPPER_PORT(reg) GPIOC_P##reg
#define STEPPER_DDR GPIOC_PDDR
// Pin 9  is stepper disable
// Pin 10 is direction
// Pin 11 is step
#define DISABLE_CTRL PORTC_PCR3
#define DISABLE_BIT  (1<<3)
#define DIR_CTRL PORTC_PCR4
#define DIR_BIT  (1<<4)
#define STEP_CTRL PORTC_PCR5
#define STEP_BIT  (1<<5)

// All input is placed on port b
// Pin 16 is the global sync line
// Pin 17 is the axis minimum limit switch
// Pin 18 is the axis maximum limit switch
#define CONTROL_PORT(reg) GPIOB_P##reg
#define CONTROL_DDR       GPIOB_PDDR

#define SYNC_CTRL PORTB_PCR0
#define SYNC_BIT  1

#define MIN_LIMIT_CTRL PORTB_PCR1
#define MIN_LIMIT_BIT  2
#define MAX_LIMIT_CTRL PORTB_PCR3
#define MAX_LIMIT_BIT  (1<<3)
#define LIMIT_MASK (MAX_LIMIT_BIT | MIN_LIMIT_BIT)


// Bits in the PIT register:
#define TIE 2 // Timer interrupt enable
#define TEN 1 // Timer enable

// Set the data-direction, multiplexing, and pullup/down/high-z for a limit
// pin. Also enables interrupts.
// Intended to be passed IMC_PARAM_MAX_LIMIT_EN or IMC_PARAM_MIN_LIMIT_EN,
// and the value of IMC_PARAM_*_LIMIT_INV, and IMC_PARAM_*_LIMIT_PULLUP.
void configure_limit_gpio(imc_axis_parameter, uint32_t, uint32_t);
// Much less interesting - configure them as standard output pins.
void configure_stepper_gpio(void);
#endif
