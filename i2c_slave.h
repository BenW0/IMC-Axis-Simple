#ifndef i2c_slave_h
#define i2c_slave_h
#define BUFFER_LENGTH 16

extern uint8_t rxBuffer[BUFFER_LENGTH];
extern uint8_t txBuffer[BUFFER_LENGTH];

extern volatile uint32_t rxBufferLength;
extern volatile uint32_t txBufferLength;
extern volatile uint32_t rxBufferIndex;
extern volatile uint32_t txBufferIndex;
extern volatile uint32_t irqcount;
#endif
