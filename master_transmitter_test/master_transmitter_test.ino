/* I2C Master Test Code
   ME 498/599 - Matthew Sorenson & Ben Weiss
   Winter 2014
   University of Washington
   
   This code runs on the I2C Master device for testing purposes. It simply re-transmits packets from
   the virtual serial port to the i2c bus. Packet length is defined according to the packet type, specified
   in the first byte of each packet.
 */
 
 #include <Wire.h>
 #include "constants.h"
 #include "message_structs.h"
 
 // constants
#define MAX_PACKET_BYTES  36
#define SLAVE_ADDR   2


void setup() {
  Serial.begin(9600);  // start serial for PC I/O
  
  // init I2C bus 
  Wire.begin();        // join i2c bus (address optional for master)
  digitalWrite(SDA, 0);
  digitalWrite(SCL, 0);
  
}

void loop() {
  uint8_t data[MAX_PACKET_BYTES];
  uint8_t i;
  
  // read and forward a packet from the host
  if( Serial.available() > 0 )
  {
    // check the first byte for a packet ID.
    uint8_t header = data[0] = Serial.read();
    // sanity check
    if( header > 0 && header <= imc_message_type_count )
    {
      // read the rest of the message
      uint8_t gotbytes = (uint8_t)Serial.readBytes((char*)data + 1, imc_message_length[header]);
      Serial.print("Got ");
      Serial.print(gotbytes);
      Serial.print(" bytes from serial. Need ");
      Serial.print(imc_message_length[header]);
      Serial.println(" bytes for packet.");
      if( gotbytes >= imc_message_length[header] )
      {
        // forward the message to the slave
        Wire.beginTransmission(SLAVE_ADDR);
        Wire.write(data, imc_message_length[header] + 1);
        switch(Wire.endTransmission(true))    // done transmitting, but don't want to release the bus.
        {
        case 1:
          Serial.println("Data too long to fit in Transmit buffer");
          break;
        case 2:
          Serial.println("NACK for address (address does not exist)");
          break;
        case 3:
          Serial.println("NACK for data");
          break;
        case 4:
          Serial.println("Other error");
          break;
        }
        
        Serial.println("Packet Sent. Requesting reply...");
        
        // read back the response - we will need imc_resp_length[header] + 1 because of the added response byte.
        Wire.requestFrom((uint8_t)SLAVE_ADDR, (uint8_t)(imc_resp_length[header] + 1));
        
        for( i = 0; i <= imc_resp_length[header] && Wire.available(); i++ )
          data[i] = Wire.read();
        if( i < imc_resp_length[header] + 1 )
          Serial.println("Slave did not return enough bytes");
        else
          Serial.println("Slave responded correctly.");
        
        // write the response back to the PC
        Serial.write(data, imc_resp_length[header] + 1);
      }
      else    // gotbytes < imc_message_length[header]
      {
        Serial.print("Not enough bytes in stream to fill ");
        Serial.print(header);
        Serial.println(" packet!");
      }
    }
    else
    {
      Serial.print("Invalid header: ");
      Serial.println(header);
    }
  }
}
