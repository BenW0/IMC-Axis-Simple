#include "hardware.h"
#include "control_isr.h"
#include "homing.h"
#include "stepper.h"
#include "parameters.h"
#include "config.h"

#include <util.h>

#define MICROS_PER_MINUTE 60000000L
#define PULSE_LENGTH 2 // General pulse length / propagation delay for all stepper signals
#define MICROS_PER_TICK (1000000L / ACCELERATION_TICKS_PER_SECOND)


static int32_t untrigger_limit(uint32_t mask, uint32_t invert_mask, uint32_t speed){
  int32_t steps = 0;
  while((CONTROL_PORT(DIR) & mask) ^ invert_mask){
    STEPPER_PORT(SOR) = STEP_BIT;
    delay_microseconds(PULSE_LENGTH);
    STEPPER_PORT(COR) = STEP_BIT;
    steps++;
    delay_microseconds(speed);
  }
  return steps;
}

static int32_t trigger_limit(uint32_t mask, uint32_t invert_mask, uint32_t speed){
  int32_t steps = 0;
  while(!((CONTROL_PORT(DIR) & mask) ^ invert_mask)){
    STEPPER_PORT(SOR) = STEP_BIT;
    delay_microseconds(PULSE_LENGTH);
    STEPPER_PORT(COR) = STEP_BIT;
    steps++;
    delay_microseconds(speed);
  }
  return steps;
}
static void take_steps(uint32_t steps, uint32_t speed){
  while(steps-- > 0){
    STEPPER_PORT(SOR) = STEP_BIT;
    delay_microseconds(PULSE_LENGTH);
    STEPPER_PORT(COR) = STEP_BIT;
    delay_microseconds(speed);
  }
}

static void set_direction(uint32_t dir){
  STEPPER_PORT(DOR) = (STEPPER_PORT(DOR) & ~DIR_BIT) | (dir ? DIR_BIT : 0);
  delay_microseconds(PULSE_LENGTH);
}

void enter_homing_routine(void){
  uint32_t homing, direction_bit;
  uint32_t inv_homing_feed, backoff;
  uint32_t mask, invert_mask;
  int32_t steps;
  // If we're not in idle mode, hard error
  if(st.state != STATE_IDLE){
    st.state = STATE_ERROR; // Probably should set a real error code
    return;
  }

  CONTROL_PORT(DDR) = SYNC_BIT;

  homing = parameters.homing;
  // Disable the appropriate hard limit function - choose the side we're homing to, don't change the pull-up
  // state, and pass a homing bit mask that results in either direction disabled.
  configure_limit_gpio(homing & HOME_DIR, PRESERVE_PULLUP, ~ (ENABLE_MIN | ENABLE_MAX));
  // Make sure our motor is enabled 
  enable_stepper();
  // If the flip bit is asserted, we should home to the opposite of the home direction bit
  direction_bit = !((homing & FLIP_AXIS) ^ ((homing & HOME_DIR) >> 1));
  set_direction(direction_bit);

  inv_homing_feed = MICROS_PER_MINUTE / parameters.homing_feedrate;
  backoff = HOMING_BACKOFF * 1000 / inv_homing_feed;

  // Compute the masks to use in determining our end condition
  if(homing & HOME_DIR){
    mask = MAX_LIMIT_BIT;
    invert_mask = (homing & INVERT_MAX) ? mask : 0;
  }else{
    mask = MIN_LIMIT_BIT;
    invert_mask = (homing & INVERT_MIN) ? mask : 0;
  }
  // Execute the first homing pass at the nominal homing speed
  steps = trigger_limit(mask, invert_mask, inv_homing_feed);
  // Back off at minimum velocity until the switch is deasserted, taking into account the negative distance
  set_direction(!direction_bit);
  steps -= untrigger_limit(mask, invert_mask, MICROS_PER_MINUTE / MINIMUM_STEPS_PER_MINUTE);
  // This is the most accurate guess of the pre-homing position we get
  parameters.last_home = homing & HOME_DIR ? parameters.home_pos - steps : parameters.home_pos + steps;
  
  take_steps(backoff, inv_homing_feed);
  // We're now at the home position!
  st.position = parameters.home_pos + backoff;

  // restore hard limits
  delay(10);
  configure_limit_gpio(homing & HOME_DIR, PRESERVE_PULLUP, homing);
  float_sync_line();
}
