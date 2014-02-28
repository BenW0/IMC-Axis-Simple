#include "hardware.h"
#include "i2c_slave.h"
#include <pin_config.h>

void portb_isr(void){
  //  STEPPER_PORT(TOR) = STEP_BIT;
  if(SDA_CTRL & ISF){
    SDA_CTRL |= ISF;
    if (!(I2C0_S & I2C_S_BUSY)){
      SDA_CTRL = (~IRQC_MASK & SDA_CTRL) | IRQC_NONE;
      // We're done right here
      STEPPER_PORT(TOR) = STEP_BIT;
    } else {
      if (++irqcount >= 2) {
	SDA_CTRL = (~IRQC_MASK & SDA_CTRL) | IRQC_NONE;
      }
    }
  }
}


int main(void){
  STEP_CTRL = STANDARD_OUTPUT;
  STEPPER_DDR |= STEP_BIT;
  initialize_i2c(10);
  while(1);
}
