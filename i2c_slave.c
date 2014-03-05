#include "hardware.h"
#include "i2c_slave.h"
#include "parser.h"
#include <pin_config.h>
#include <mk20dx128.h>

void initialize_i2c(uint8_t addr){
  SIM_SCGC4 |= SIM_SCGC4_I2C0;
  SDA_CTRL = PORT_PCR_MUX(2) | PORT_PCR_ODE | PORT_PCR_SRE | PORT_PCR_DSE;
  SCL_CTRL = PORT_PCR_MUX(2) | PORT_PCR_ODE | PORT_PCR_SRE | PORT_PCR_DSE;
#if F_BUS == 48000000
  I2C0_F = 0x27;
  I2C0_FLT = 4;
#elif F_BUS == 24000000
  I2C0_F = 0x1F;  
  I2C0_FLT = 2;
#else
#error "F_BUS must be 48 MHz or 24 MHz"
#endif
  I2C0_C2 = I2C_C2_HDRS;
  I2C0_C1 = I2C_C1_IICEN;
  I2C0_A1 = addr << 1;
  I2C0_C1 = I2C_C1_IICEN | I2C_C1_IICIE;
  NVIC_ENABLE_IRQ(IRQ_I2C0);
}

uint8_t txBuffer[BUFFER_LENGTH];

volatile uint32_t txBufferLength;
volatile uint32_t txBufferIndex;
volatile uint32_t irqcount;

void on_request(void){;}

void i2c0_isr(void)
{
  uint8_t status, c1, data;
  static uint8_t receiving=0;

  I2C0_S = I2C_S_IICIF;
  status = I2C0_S;
  //serial_print(".");
  if (status & I2C_S_ARBL) {
    // Arbitration Lost
    I2C0_S = I2C_S_ARBL;
    if (!(status & I2C_S_IAAS)) return;
  }
  if (status & I2C_S_IAAS) {
    //serial_print("\n");
    // Addressed As A Slave
    if (status & I2C_S_SRW) {
      I2C0_C1 = I2C_C1_IICEN | I2C_C1_IICIE | I2C_C1_TX;
      I2C0_D = txBuffer[0];
      txBufferIndex = 1;
    } else {
      // Begin Slave Receive
      receiving = 1;
      I2C0_C1 = I2C_C1_IICEN | I2C_C1_IICIE;
      data = I2C0_D;
    }
    I2C0_S = I2C_S_IICIF;
    return;
  }
  c1 = I2C0_C1;
  if (c1 & I2C_C1_TX) {
    // Continue Slave Transmit
    //serial_print("t");
    if ((status & I2C_S_RXAK) == 0) {
      // Master ACK'd previous byte
      if (txBufferIndex < txBufferLength) {
	I2C0_D = txBuffer[txBufferIndex++];
      } else {
	I2C0_D = 0; // Pad with zeros
      }
      I2C0_C1 = I2C_C1_IICEN | I2C_C1_IICIE | I2C_C1_TX;
    } else {
      //serial_print("*");
      // Master did not ACK previous byte
      I2C0_C1 = I2C_C1_IICEN | I2C_C1_IICIE;
      data = I2C0_D;
    }
  } else {
    // Continue Slave Receive
    irqcount = 0;
    
    SDA_CTRL = (~IRQC_MASK & SDA_CTRL) | IRQC_RISING;

    //attachInterrupt(18, sda_rising_isr, RISING);
    //digitalWriteFast(4, HIGH);
    data = I2C0_D;
    //serial_phex(data);
    if(receiving)
      feed_data(data);
    //digitalWriteFast(4, LOW);
  }
}
 
