#include <mk20dx128.h>
#include <stdbool.h>

#include "protocol/message_structs.h"
#include "queue.h"
#include "hardware.h"
#include "config.h"
#include "stepper.h"

#define TICKS_PER_MICROSECOND (F_CPU/1000000)
#define CYCLES_PER_ACCELERATION_TICK ((TICKS_PER_MICROSECOND*1000000)/ACCELERATION_TICKS_PER_SECOND)
#define STATE_CYCLE      3 // Cycle is running
#define STATE_HOLD       4 // Executing feed hold

typedef struct {
  // Used by the bresenham line algorithm
  int32_t counter;        // Counter variables for the bresenham line tracer
  uint32_t event_count;
  uint32_t step_events_completed;  // The number of step events left in current motion

  // Used by the trapezoid generator
  uint32_t cycles_per_step_event;        // The number of machine cycles between each step event
  uint32_t trapezoid_tick_cycle_counter; // The cycles since last trapezoid_tick. Used to generate ticks at a steady
                                              // pace without allocating a separate timer
  uint32_t trapezoid_adjusted_rate;      // The current rate of step_events according to the trapezoid generator
  uint32_t min_safe_rate;  // Minimum safe rate for full deceleration rate reduction step. Otherwise halves step_rate.

  // The following fields are taken from the grbl sys data structure
  int32_t position;
  uint32_t state;

} stepper_t;

// Bits in the PIT register:
#define TIE 2 // Timer interrupt enable
#define TEN 1 // Timer enable
enum pulse_status {PULSE_SET, PULSE_RESET};

typedef struct {
  volatile uint32_t active_bits;
  uint32_t pulse_length;
  volatile enum pulse_status step_interrupt_status;
} pulse_state;

static pulse_state pit1_state;

static stepper_t st;
static msg_queue_move_t* current_block;
volatile uint32_t busy;

volatile uint32_t out_step;
volatile uint32_t out_dir;


void st_go_idle(void){
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
  PIT_TFLG0 = 1;

  if (busy) { return; } // The busy-flag is used to avoid reentering this interrupt
  // Set the direction bits. Todo: only do this at the start of a block?

  STEPPER_PORT(DOR) = (STEPPER_PORT(DOR) & ~DIR_BIT) | (out_dir ? DIR_BIT : 0);
  trigger_pulse(out_step);

  busy = true;
  
  // If there is no current block, attempt to pop one from the buffer
  if (current_block == NULL) {
    // Anything in the buffer? If so, initialize next motion.
    current_block = dequeue_block();
    if (current_block != NULL) {
      if (st.state == STATE_CYCLE) {
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
      st_go_idle();
      // Notify main control flow that we're done
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
      if (st.state == STATE_HOLD) {
        // Check for and execute feed hold by enforcing a steady deceleration from the moment of 
        // execution. The rate of deceleration is limited by rate_delta and will never decelerate
        // faster or slower than in normal operation. If the distance required for the feed hold 
        // deceleration spans more than one block, the initial rate of the following blocks are not
        // updated and deceleration is continued according to their corresponding rate_delta.
        // NOTE: The trapezoid tick cycle counter is not updated intentionally. This ensures that 
        // the deceleration is smooth regardless of where the feed hold is initiated and if the
        // deceleration distance spans multiple blocks.
        if ( iterate_trapezoid_cycle_counter() ) {                    
          // If deceleration complete, set system flags and shutdown steppers.
          if (st.trapezoid_adjusted_rate <= current_block->acceleration) {
            // Just go idle. Do not NULL current block. The bresenham algorithm variables must
            // remain intact to ensure the stepper path is exactly the same. Feed hold is still
            // active and is released after the buffer has been reinitialized.
            st_go_idle();
	    // Also, notify program here
          } else {
            st.trapezoid_adjusted_rate -= current_block->acceleration;
            set_step_events_per_minute(st.trapezoid_adjusted_rate);
          }      
        }
        
      } else {
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
      }            
    } else {   
      // If current block is finished, reset pointer, forcing
      // the next iteration to either pop a new block, or die
      current_block = NULL;
    }
  }
 
  busy = false;
}

inline void trigger_pulse(uint32_t active){
  // Hand PIT1 the bit mask containing the step pins to toggle and enable it. In the case of stepper drivers
  // that require a delay between setting direction pins and step pins (eg. DRV8825 - 650 ns), the PIT1 interrupt
  // triggers once to set the pins active (per invert mask) and then resets itself and fires again to clear them.
  // Otherwise, we toggle the bits here and pit1 fires once, clearing them.
  pit1_state.active_bits = active;  
#ifdef STEP_PULSE_DELAY
  pit1_state.step_interrupt_status = PULSE_SET;
  PIT_LDVAL1 = STEP_PULSE_DELAY;
#else
  STEPPER_PORT(TOR) = active;
  PIT_LDVAL1 = pit1_state.pulse_length;
#endif
  PIT_TCTRL1 |= TEN;
}

void pit1_isr(void){
  PIT_TFLG1 = 1;
  PIT_TCTRL1 &= ~TEN;
  STEPPER_PORT(TOR) = pit1_state.active_bits;
#ifdef STEP_PULSE_DELAY
  if(pit1_state.step_interrupt_status == PULSE_SET){
    pit1.step_interrupt_status = PULSE_RESET;
    PIT_LDVAL1 = pit1_state.pulse_length;
    PIT_TCTRL1 |= TEN;  
  }
  #endif
}
