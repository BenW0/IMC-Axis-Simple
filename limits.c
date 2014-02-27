#include <pin_config.h>
#include "hardware.h"
#include "stepper.h"
#include "sync.h"

void portb_isr(void){
  if(SYNC_CTRL & ISF){
    SYNC_CTRL = MUX_GPIO | IRQC_NONE;

    PIT_TCTRL2 &= ~TEN;
    PIT_LDVAL2 = SYNC_DELAY;
    PIT_TCTRL2 |= TEN;

    //Next time this ISR is triggered, we've gone through the sync sequence, and can just start executing
    PIT_TCTRL0 |= TEN;
    SYNC_CTRL |= ISF;
  }
}
