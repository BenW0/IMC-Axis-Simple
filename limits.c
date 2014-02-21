#include <pin_config.h>
#include "hardware.h"
#include "sync.h"

void portb_isr(void){
  if(SYNC_CTRL & ISF){

    SYNC_CTRL = STANDARD_OUTPUT;
    CONTROL_DDR |= SYNC_BIT;
    CONTROL_PORT(COR) = SYNC_BIT;
    PIT_TCTRL0 |= TEN;

    SYNC_CTRL |= ISF;
      
    //Measure timeout error, and set the timer to trigger for the propagation delay;
    //  uint32_t sync_error = SYNC_TIMEOUT - PIT_CVAL2;
    //  PIT_TCTRL2 &= ~TEN;
    // PIT_LDVAL2 = SYNC_DELAY;
    // PIT_TCTRL2 |= TEN;
    // Enable the stepper interrupt again!
    //  SYNC_CTRL = STANDARD_OUTPUT;
    //CONTROL_DDR |= SYNC_BIT;
    //CONTROL_PORT(COR) = SYNC_BIT;
    //PIT_TCTRL0 |= TEN;
    /*
    while(1){
      int i;
      for(i = 0; i < 48000000; i++){
	STEPPER_PORT(COR) = STEP_BIT;
      }
      for(i = 0; i < 48000000; i++){
	STEPPER_PORT(SOR) = STEP_BIT;
      }
      }*/

  }
  // Otherwise, limits code goes here
}
