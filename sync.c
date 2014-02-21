#include <mk20dx128.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <pin_config.h>

#include "hardware.h"
#include "sync.h"

void enable_sync_isr(void){
  NVIC_ENABLE_IRQ(IRQ_PIT_CH2);
  PIT_TCTRL2 = TIE;
}



void pit2_isr(void){
  PIT_TFLG2 = 1;
  // Stop the timer...
  PIT_TCTRL2 &= ~TEN; 
  if(PIT_LDVAL2 == SYNC_DELAY){
    // Configure this as output pulled low
    CONTROL_DDR |= SYNC_BIT;
  }else{ // We've timed out and bad things are happening
    // For testing purposes, just keep executing
    while(1);
  }
}
