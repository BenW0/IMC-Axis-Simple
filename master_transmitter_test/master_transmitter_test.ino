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
 
#define MAX_PACKET_BYTES  36
#define SLAVE_ADDR   2

void setup() {
  Serial.begin(9600);  // start serial for PC I/O  
  // init I2C bus 
  Wire.begin();        // join i2c bus (address optional for master)
  digitalWrite(SDA, 0);
  digitalWrite(SCL, 0);
  pinMode(13,OUTPUT);
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
    if( header > 0 && header <= IMC_MESSAGE_TYPE_COUNT)
    {
      // read the rest of the message
      uint8_t gotbytes = (uint8_t)Serial.readBytes((char*)data + 1, imc_message_length[header]);
      if( gotbytes >= imc_message_length[header] ){
        digitalWrite(13,HIGH);
        delay(100);
        digitalWrite(13,LOW);
        
        // forward the message to the slave
        Wire.beginTransmission(SLAVE_ADDR);
        Wire.write(data, imc_message_length[header] + 1);
        Wire.endTransmission(true);
	// read back the response - we will need imc_resp_length[header] + 1 because of the added response byte.
        Wire.requestFrom((uint8_t)SLAVE_ADDR, (uint8_t)(imc_resp_length[header] + 1));
        
        for( i = 0; i <= imc_resp_length[header] && Wire.available(); i++ )
          data[i] = Serial.write(Wire.read());
      }
    }else{
      Serial.print("Invalid header: ");
      Serial.println(header);
    }
  }
}
