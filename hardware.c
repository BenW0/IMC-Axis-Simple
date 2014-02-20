#include "hardware.h"
#include <mk20dx128.h>
#include <pin_config.h>

// Set the data-direction, multiplexing, and pullup/down/high-z for a limit
// pin. Also enables interrupts.
void configure_limit_gpio(imc_axis_parameter axis, uint32_t invert, uint32_t pullup){
  uint32_t config = MUX_GPIO;
  if(invert)
    config |= IRQC_FALLING;
  else
    config |= IRQC_RISING;

  if(pullup)
    config |= invert ? PULL_DOWN : PULL_UP;

  if(axis == IMC_PARAM_MAX_LIMIT_EN){
    // Set the pin as output in the port ddr
    CONTROL_DDR &= ~MAX_LIMIT_BIT;
    // Set the correct pin config register
    MAX_LIMIT_CTRL = config;
  }else if(axis == IMC_PARAM_MIN_LIMIT_EN){
    CONTROL_DDR &= ~MIN_LIMIT_BIT;
    MIN_LIMIT_CTRL = config;
  }else return;
}

void configure_stepper_gpio(void){
  STEPPER_DDR |= DISABLE_BIT | DIR_BIT | STEP_BIT;
  DISABLE_CTRL = STANDARD_OUTPUT;
  DIR_CTRL = STANDARD_OUTPUT;
  STEP_CTRL = STANDARD_OUTPUT;
  // May want to initialize pin states as well
}
