#include <mk20dx128.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <pin_config.h>

#include "protocol/message_structs.h"
#include "sync.h"
#include "queue.h"
#include "hardware.h"
#include "config.h"
#include "stepper.h"

static void set_step_events_per_minute(uint32_t); 

#define TICKS_PER_MICROSECOND (F_CPU/1000000)
#define CYCLES_PER_ACCELERATION_TICK ((TICKS_PER_MICROSECOND*1000000)/ACCELERATION_TICKS_PER_SECOND)

enum pulse_status {PULSE_SET, PULSE_RESET};

typedef struct {
  uint32_t pulse_length;
  volatile enum pulse_status step_interrupt_status;
} pulse_state;

static pulse_state pit1_state;

static stepper_state_t st;
static msg_queue_move_t* current_block;

volatile uint32_t out_step;
volatile uint32_t out_dir;

// Reset all stepper parameters, setup clocks, and make sure
// there is no power to steppers.
void initialize_stepper(void){
  disable_stepper();
  // Clock up all of the PITs
  SIM_SCGC6 |= SIM_SCGC6_PIT;
  PIT_MCR = 0x00;
  // Configure PIT 0 - main interrupt timer - as running at minimum steps/minute, but not
  // interrupting.
  set_step_events_per_minute(MINIMUM_STEPS_PER_MINUTE);
  // Configure PIT 1 - reset timer
  PIT_TCTRL1 = TIE;
  // Start in the idle state, but first wake up to check for keep steppers enabled option.
  NVIC_ENABLE_IRQ(IRQ_PIT_CH0);
  NVIC_ENABLE_IRQ(IRQ_PIT_CH1);
  // Zero all parameters, and go into idle
  memset(&st, 0, sizeof(st));
  current_block = NULL;
  st.state =  STATE_IDLE;
}
// Enable power to steppers - deassert stepper disable pin
void enable_stepper(void){
  STEPPER_PORT(COR) = DISABLE_BIT;
}
// Power down the stepper motor
void disable_stepper(void){
  STEPPER_PORT(SOR) = DISABLE_BIT;
}


inline static uint32_t iterate_trapezoid_cycle_counter() 
{
  st.trapezoid_tick_cycle_counter += st.cycles_per_step_event;  
  if(st.trapezoid_tick_cycle_counter > CYCLES_PER_ACCELERATION_TICK) {
    st.trapezoid_tick_cycle_counter -= CYCLES_PER_ACCELERATION_TICK;
    return(true);
  } else {
    return(false);
  }
}          

static uint32_t config_step_timer(uint32_t cycles)
{

  PIT_TCTRL0 &= ~TEN; // Stop the timer 
  PIT_LDVAL0 = cycles; // Load the new value
  PIT_TCTRL0 |= TEN;
  return(cycles);
}

static void set_step_events_per_minute(uint32_t steps_per_minute) 
{
  if (steps_per_minute < MINIMUM_STEPS_PER_MINUTE){
    steps_per_minute = MINIMUM_STEPS_PER_MINUTE;
  }
  st.cycles_per_step_event = config_step_timer((F_CPU*((uint32_t)60))/steps_per_minute);
}


void pit0_isr(void) {
  // Set the direction bits. Todo: only do this at the start of a block.
  STEPPER_PORT(DOR) = (STEPPER_PORT(DOR) & ~DIR_BIT) | (out_dir ? DIR_BIT : 0);
  if(out_step)
    trigger_pulse();
  
  if(st.state == STATE_SYNC){ // Done with a block, and done with outputting the last pulse
    // Disable this timer, and configure it to execute as soon as possible
    PIT_TCTRL0 &= ~TEN;
    PIT_TCTRL0 &= ~TIE;

    PIT_LDVAL0 = 1; // Can this be 0?
    //Next time this ISR is triggered, we've gone through the sync sequence, and can just start executing
    st.state = STATE_EXECUTE; 
    // Configure the sync line as high-z input with an interrupt on rising edge
    CONTROL_DDR &= ~SYNC_BIT;
    SYNC_CTRL = MUX_GPIO | IRQC_RISING;
    // Start counting down on timer 2
    PIT_LDVAL2 = SYNC_TIMEOUT;
    PIT_TCTRL2 |= TEN;
    // Allow this to retrigger
    PIT_TFLG0 = 1;
    return;
  }

  // If there is no current block, attempt to pop one from the buffer
  if (current_block == NULL){
    // Anything in the buffer? If so, initialize next motion.
    current_block = dequeue_block();
    if (current_block != NULL) {
      if (st.state == STATE_EXECUTE) {
        // During feed hold, do not update rate and trap counter. Keep decelerating.
        st.trapezoid_adjusted_rate = current_block->initial_rate;
        set_step_events_per_minute(st.trapezoid_adjusted_rate); // Initialize cycles_per_step_event
        st.trapezoid_tick_cycle_counter = CYCLES_PER_ACCELERATION_TICK/2; // Start halfway for midpoint rule.
      }
      st.min_safe_rate = (3 * current_block->acceleration) >> 1; // 1.5 x rate_delta
      st.counter = -(current_block->total_length >> 1);
      st.event_count = current_block->total_length;
      st.step_events_completed = 0;     
    } else {
      // We have no more moves in the queue, so go idle but keep steppers enabled
      st.state = STATE_IDLE; 
    }    
  } 

  if (current_block != NULL) {
    // Execute step displacement profile by bresenham line algorithm
    out_dir = out_step = 0;
    if(current_block->length < 0){
      out_dir = 1;
      st.counter -= current_block->length;
    }else{
      st.counter += current_block->length;
    }

    if (st.counter > 0) {
      out_step = 1;
      st.counter -= st.event_count;
      if (out_dir) { st.position--; }
      else { st.position++; }
    }

    st.step_events_completed++; // Iterate step events

    // While in block steps, check for de/ac-celeration events and execute them accordingly.
    if (st.step_events_completed < current_block->total_length) {
      // The trapezoid generator always checks step event location to ensure de/ac-celerations are 
      // executed and terminated at exactly the right time. This helps prevent over/under-shooting
      // the target position and speed. 
      // NOTE: By increasing the ACCELERATION_TICKS_PER_SECOND in config.h, the resolution of the 
      // discrete velocity changes increase and accuracy can increase as well to a point. Numerical 
      // round-off errors can effect this, if set too high. This is important to note if a user has 
      // very high acceleration and/or feedrate requirements for their machine.
      if (st.step_events_completed < current_block->stop_accelerating) {
	// Iterate cycle counter and check if speeds need to be increased.
	if ( iterate_trapezoid_cycle_counter() ) {
	  st.trapezoid_adjusted_rate += current_block->acceleration;
	  if (st.trapezoid_adjusted_rate >= current_block->nominal_rate) {
	    // Reached nominal rate a little early. Cruise at nominal rate until decelerate_after.
	    st.trapezoid_adjusted_rate = current_block->nominal_rate;
	  }
	  set_step_events_per_minute(st.trapezoid_adjusted_rate);
	}
      } else if (st.step_events_completed >= current_block->start_decelerating) {
	// Reset trapezoid tick cycle counter to make sure that the deceleration is performed the
	// same every time. Reset to CYCLES_PER_ACCELERATION_TICK/2 to follow the midpoint rule for
	// an accurate approximation of the deceleration curve. For triangle profiles, down count
	// from current cycle counter to ensure exact deceleration curve.
	if (st.step_events_completed == current_block->start_decelerating) {
	  if (st.trapezoid_adjusted_rate == current_block->nominal_rate) {
	    st.trapezoid_tick_cycle_counter = CYCLES_PER_ACCELERATION_TICK/2; // Trapezoid profile
	  } else {  
	    st.trapezoid_tick_cycle_counter = CYCLES_PER_ACCELERATION_TICK-st.trapezoid_tick_cycle_counter; // Triangle profile
	  }
	} else {
	  // Iterate cycle counter and check if speeds need to be reduced.
	  if ( iterate_trapezoid_cycle_counter() ) {  
	    // NOTE: We will only do a full speed reduction if the result is more than the minimum safe 
	    // rate, initialized in trapezoid reset as 1.5 x rate_delta. Otherwise, reduce the speed by
	    // half increments until finished. The half increments are guaranteed not to exceed the 
	    // CNC acceleration limits, because they will never be greater than rate_delta. This catches
	    // small errors that might leave steps hanging after the last trapezoid tick or a very slow
	    // step rate at the end of a full stop deceleration in certain situations. The half rate 
	    // reductions should only be called once or twice per block and create a nice smooth 
	    // end deceleration.
	    if (st.trapezoid_adjusted_rate > st.min_safe_rate) {
	      st.trapezoid_adjusted_rate -= current_block->acceleration;
	    } else {
	      st.trapezoid_adjusted_rate >>= 1; // Bit shift divide by 2
	    }
	    if (st.trapezoid_adjusted_rate < current_block->final_rate) {
	      // Reached final rate a little early. Cruise to end of block at final rate.
	      st.trapezoid_adjusted_rate = current_block->final_rate;
	    }
	    set_step_events_per_minute(st.trapezoid_adjusted_rate);
	  }
	}
      } else {
	// No accelerations. Make sure we cruise exactly at the nominal rate.
	if (st.trapezoid_adjusted_rate != current_block->nominal_rate) {
	  st.trapezoid_adjusted_rate = current_block->nominal_rate;
	  set_step_events_per_minute(st.trapezoid_adjusted_rate);
	}
      }            
    } else {
      // If current block is finished, reset pointer, forcing
      // the next iteration to either pop a new block, or die
      // This is where we deal with our sync protocol:
      // 1) switch sync pin from pulling up sync line to high-z input. Maybe wait for line to stablize a bit.
      // 2) switch to sync state, disable the stepper interrupt, start a PIT counter, and enable an interrupt on sync line falling edge.
      // 3) When interrupt fires, immediately restart the stepper interrupt. Value of PIT is the sync error.
      current_block = NULL;
      st.state = STATE_SYNC;
    }
  }
  PIT_TFLG0 = 1;
}

inline void trigger_pulse(void){
#ifdef STEP_PULSE_DELAY
  pit1_state.step_interrupt_status = PULSE_SET;
  PIT_LDVAL1 = STEP_PULSE_DELAY;
#else
  STEPPER_PORT(TOR) = STEP_BIT;
  PIT_LDVAL1 = pit1_state.pulse_length;
#endif
  PIT_TCTRL1 |= TEN;
}

void pit1_isr(void){
  PIT_TFLG1 = 1;
  PIT_TCTRL1 &= ~TEN;
  STEPPER_PORT(TOR) = STEP_BIT;
#ifdef STEP_PULSE_DELAY
  if(pit1_state.step_interrupt_status == PULSE_SET){
    pit1.step_interrupt_status = PULSE_RESET;
    PIT_LDVAL1 = pit1_state.pulse_length;
    PIT_TCTRL1 |= TEN;  
  }
  #endif
}


void enable_stepper_int(void){
  PIT_TCTRL0 |= TEN | TIE;
}
