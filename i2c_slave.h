#ifndef i2c_slave_h
#define i2c_slave_h
#define BUFFER_LENGTH 32

void initialize_i2c(uint8_t);

extern uint8_t txBuffer[BUFFER_LENGTH];

extern volatile uint32_t txBufferLength;
extern volatile uint32_t txBufferIndex;
extern volatile uint32_t irqcount;
#endif
