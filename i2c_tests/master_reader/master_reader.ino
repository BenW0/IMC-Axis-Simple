// TWI/I2C Bandwidth & Error Rate Test Bench - Master code
// Ben Weiss
// Modified from sample code by Nicholas Zambetti <http://www.zambetti.com>

#include <Wire.h>

#define TEST   2   // 1 - bandwidth test (master to slave)  2 - error rate test (master to slave)
#define SLAVE_ADDR   2
#define BYTES_PER_PACKET  30
#define PACKETS_TO_SEND   10000L

unsigned char data[BYTES_PER_PACKET];

float slave_idle_cpufree = 0;
long bytesRead = 0;

void setup()
{
  Wire.begin();        // join i2c bus (address optional for master)
  Serial.begin(9600);  // start serial for output
  switch(TEST)
  {
  case 1:
    Serial.println("Starting Master-Slave Bandwidth Test...");
    break;
  case 2:
    Serial.println("Staring Master-Slave Bit Error Rate Test...");
    break;
  case 3:
    Serial.println("Staring Slave-Master Bandwidth Test...");
    break;
  case 4:
    Serial.println("Staring Master-Slave Bit Error Rate Test...");
    break;
  };
  
  // deactivate internal pullups for twi.
  //digitalWrite(SDA, 0);
  //digitalWrite(SCL, 0);
  
  for( int i = 0; i < BYTES_PER_PACKET; i++ )
    data[i] = i;
    
#if TEST == 1
  // ....gather initial data for slave and master cpu usage (no load)....
  
  Serial.print("Gathering CPU usage data...");
  
  
  delay(4000);
  
  // Read back the results of the slave - cpu usage
  Wire.requestFrom(SLAVE_ADDR, 8);    // request 8 bytes from slave device #2

  unsigned long foo = 0;
  if(Wire.available() >= 4)    // slave may send less than requested
    Wire.readBytes((char *)&foo, 4); // receive a byte as character
    
  if(Wire.available() >= 4)    // slave may send less than requested
    Wire.readBytes((char *)&slave_idle_cpufree, 4); // receive a byte as character
    
    
  Serial.print("done.");
  Serial.println(slave_idle_cpufree);
#endif
}

void loop()
{
#if TEST == 1 || TEST == 2
  // Read back the results of the slave - this clears its cpu counter
  Wire.requestFrom(SLAVE_ADDR, 8);    // request 6 bytes from slave device #2

  while(Wire.available())    // slave may send less than requested
    Wire.read();
    
    
  long starttime = micros();

  for( long k = 0; k < PACKETS_TO_SEND; k++ )
  {
    for( int i = 0; i < BYTES_PER_PACKET; i++ )
      data[i]++;
      
    Wire.beginTransmission(SLAVE_ADDR);
    Wire.write(data, BYTES_PER_PACKET);
    switch(Wire.endTransmission())
    {
    case 1:
      Serial.print(k);
      Serial.println(" - Data too long to fit in Transmit buffer");
      break;
    case 2:
      Serial.print(k);
      Serial.println(" - NACK for address (address does not exist)");
      break;
    case 3:
      Serial.print(k);
      Serial.println(" - NACK for data");
      break;
    case 4:
      Serial.print(k);
      Serial.println(" - Other error");
      break;
    }
  }
  
  long endtime = micros();
  
  // Read back the results of the slave - number of bytes read.
  Wire.requestFrom(SLAVE_ADDR, 8);    // request 8 bytes from slave device #2

  unsigned long slave_read_bytes = 0;
  float slave_load_cpufree = 0;
  if(Wire.available() >= 4)    // slave may send less than requested
    Wire.readBytes((char *)&slave_read_bytes, 4); // receive a byte as character
    
  if(Wire.available() >= 4)    // slave may send less than requested
    Wire.readBytes((char *)&slave_load_cpufree, 4); // receive a byte as character
    
  
  Serial.println("Finished test! ");
  Serial.print(" Bytes sent: ");
  Serial.println(PACKETS_TO_SEND * BYTES_PER_PACKET);
  Serial.print(" Bytes received: ");
  Serial.print(slave_read_bytes);
  Serial.print(" (");
  Serial.print(100.f * (float)slave_read_bytes / (PACKETS_TO_SEND * BYTES_PER_PACKET));
  Serial.println("%)");
  Serial.print(" Time: ");
  Serial.print((float)(endtime - starttime) / 1000.f);
  Serial.println(" ms ");
  Serial.print(" Bandwidth: ");
  Serial.print((float)PACKETS_TO_SEND * BYTES_PER_PACKET / (float)(endtime - starttime) * 1000000.f);
  Serial.println(" bits/sec");
  Serial.print(" Slave CPU usage: ");
  Serial.print((slave_idle_cpufree - slave_load_cpufree) / slave_idle_cpufree * 100.f);
  Serial.println("%");
  
#elif TEST == 3
  
  long starttime = micros();
  
  bytesRead = 0;
  
  for( long k = 0; k < PACKETS_TO_SEND; k++ )
  {
    bytesRead += Wire.requestFrom(SLAVE_ADDR, BYTES_PER_PACKET);
    while(Wire.available())
      Wire.read();
  }
  
  long totaltime = micros() - starttime;
  
  // end of test. Print results.
  Wire.beginTransmission(SLAVE_ADDR);
  Wire.write((uint8_t *)&bytesRead, sizeof(long));
  Wire.write((uint8_t *)&totaltime, sizeof(long));
  switch(Wire.endTransmission())
  {
  case 1:
    Serial.println(" - Data too long to fit in Transmit buffer");
    break;
  case 2:
    Serial.println(" - NACK for address (address does not exist)");
    break;
  case 3:
    Serial.println(" - NACK for data");
    break;
  case 4:
    Serial.println(" - Other error");
    break;
  }
  
#elif TEST == 4   // slave-master bit error rate test
  boolean datafull = false;
  int extraBytes = 0;
  int missingBytes = 0;
  int incorrectBytes = 0;
  int incorrectBits = 0;
  
  long starttime = micros();
  
  bytesRead = 0;
  
  for( long k = 0; k < PACKETS_TO_SEND; k++ )
  {
    
    unsigned char buf[BYTES_PER_PACKET];
    int i = 0;
    // request a buffer full...
    bytesRead += Wire.requestFrom(SLAVE_ADDR, BYTES_PER_PACKET);
    
    // read all the data the Slave sent us, storing it on the buffer
    for(i = 0; i < BYTES_PER_PACKET && Wire.available(); i++)
      buf[i] = Wire.read();
    while(Wire.available())
    {
      Wire.read();          // receive an extra byte
      extraBytes++;
    }
    
    // did we get enough bytes?
    if( BYTES_PER_PACKET > i )
      missingBytes += BYTES_PER_PACKET - i;
    
    // validate the data we received
    if( datafull )    // first time, there is no previous buffer to compare to
    {
      for( i = 0; i < BYTES_PER_PACKET; i++ )
      {
        if( buf[i] != (unsigned char)(data[i] + 1) )
        {
          incorrectBytes++;
          //Serial.print("Wanted ");
          //Serial.print(data[i]+1);
          //Serial.print(" got ");
          //Serial.println(buf[i]);
          // figure out how many bits were wrong
          // count # of bits set in buf[i].
          unsigned char v = buf[i] ^ (data[i] + 1); // count the number of bits set in v, the difference between the byte received and the byte expected
          unsigned char c; // c accumulates the total bits set in v
          for (c = 0; v; c++)
          {
            v &= v - 1; // clear the least significant bit set
          }
          incorrectBits += c;
          // increment data to what it should be
          data[i]++;
        }
        else
          data[i] = buf[i];
      }
    }
    else
    {
      datafull = true;
      for( i = 0; i < BYTES_PER_PACKET; i++ )
        data[i] = buf[i];
    }
  }
  
  long totaltime = micros() - starttime;
  
  
  // end of test. Print results.
  Wire.beginTransmission(SLAVE_ADDR);
  Wire.write((uint8_t *)&bytesRead, sizeof(long));
  Wire.write((uint8_t *)&totaltime, sizeof(long));
  switch(Wire.endTransmission())
  {
  case 1:
    Serial.println(" - Data too long to fit in Transmit buffer");
    break;
  case 2:
    Serial.println(" - NACK for address (address does not exist)");
    break;
  case 3:
    Serial.println(" - NACK for data");
    break;
  case 4:
    Serial.println(" - Other error");
    break;
  }
  
  
  Serial.print("Bytes read: ");
  Serial.println(bytesRead);
  Serial.print("Bytes missed: ");
  Serial.println(missingBytes);
  Serial.print("Bytes extra: ");
  Serial.println(extraBytes);
  Serial.print("Byte errors: ");
  Serial.println(incorrectBytes);
  Serial.print("Bit errors: ");
  Serial.println(incorrectBits);
#endif
}
