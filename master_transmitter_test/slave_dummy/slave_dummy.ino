// TWI/I2C Bandwidth & Error Rate Test Bench - Slave code
// Ben Weiss
// Modified from sample code by Nicholas Zambetti <http://www.zambetti.com>
// additional source from http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetNaive

#include <Wire.h>

#define SLAVE_ADDR   2
#define MAX_BYTES_PER_PACKET  30

typedef enum __attribute__ ((__packed__)) {
  IMC_RSP_OK,
  IMC_RSP_UNKNOWN,
  IMC_RSP_ERROR,
  IMC_RSP_QUEUEFULL
} imc_response_type;

uint8_t resp = 0;
imc_response_type mode = IMC_RSP_OK;

void setup()
{
  Wire.begin(SLAVE_ADDR);                // join i2c bus with address #4
  Wire.onReceive(receiveEvent); // register event
  Wire.onRequest(requestEvent); // register event
  Serial.begin(9600);           // start serial for output
}

void loop()
{
  if( Serial.available() > 0 )
  {
    char c = Serial.read();
    switch(c)
    {
      case 'o': case 'O':
        Serial.println("Engering OK response mode");
        mode = IMC_RSP_OK;
        break;
      case 'u': case 'U':
        Serial.println("Entering Unknown response mode");
        mode = IMC_RSP_UNKNOWN;
        break;
      case 'e': case 'E':
        Serial.println("Entering Error response mode");
        mode = IMC_RSP_ERROR;
        break;
      case 'q': case 'Q':
        Serial.println("Entering Queuefull response mode");
        mode = IMC_RSP_QUEUEFULL;
        break;
    }
  }
}

// function that executes whenever data is received from master
// this function is registered as an event, see setup()
void receiveEvent(int howMany)
{
  Serial.print("Got Packet! ");
  Serial.print(howMany);
  Serial.println(" bytes.");
  
  while(Wire.available())
  {
    Serial.print((int)Wire.read(),HEX);
    Serial.print(" ");
  }
  Serial.println("");
  //resp = 1;
}

// function that executes whenever data is requested by master
// this function is registered as an event, see setup()
void requestEvent()
{
  Serial.print("Requested byte: Sending ");
  Serial.print("\\");
  Serial.print(mode, HEX);
  Wire.write(mode);
  if( IMC_RSP_OK == mode )
  {
    for(int i = 0; i < 10; i++)
    {
      Serial.print("\\");
      Serial.print(resp, HEX);
      Wire.write(resp++);
    }
  }
  Serial.println("");
  
}
