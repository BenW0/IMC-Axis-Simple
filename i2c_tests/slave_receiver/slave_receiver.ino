// TWI/I2C Bandwidth & Error Rate Test Bench - Slave code
// Ben Weiss
// Modified from sample code by Nicholas Zambetti <http://www.zambetti.com>
// additional source from http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetNaive

#include <Wire.h>

#define TEST     2      // 1 - bandwidth test (master to slave)   2 - bit error test (master to slave)   3 - bandwidth test (slave to master)   4 - bit error test (slave to master)
#define SLAVE_ADDR   2
#define BYTES_PER_PACKET  30

volatile unsigned char data[BYTES_PER_PACKET];
volatile boolean datafull = false;
volatile int extraBytes = 0;
volatile int missingBytes = 0;
volatile int incorrectBytes = 0;
volatile int incorrectBits = 0;
volatile unsigned long bytesRead = 0;
volatile unsigned long bytesSent = 0;

volatile unsigned long idletask = 0;
float idleCpufree = 0;
float cpufree = 0.f;    // this is a number indicating the number of sums/second we can accomplish in our idle loop.
long cpucyclestarttime = 0;

void setup()
{
  Wire.begin(SLAVE_ADDR);                // join i2c bus with address #4
  Wire.onReceive(receiveEvent); // register event
  Wire.onRequest(requestEvent); // register event
  Serial.begin(9600);           // start serial for output
  cpucyclestarttime = micros();
  // deactivate internal pullups for twi.
  digitalWrite(SDA, 0);
  digitalWrite(SCL, 0);
  
  for(unsigned char i = 0; i < BYTES_PER_PACKET; i++)
  {
    data[i] = i;
  }
  
  // get an initial cpu usage counter...
#if TEST == 3
  noInterrupts();
  long startTime = micros();
  for(long i = 0; i < 1000000; i++)
    ;
  idleCpufree = (float)1000000 / (float)(micros() - startTime);
  interrupts();
#endif
  
  
}

void loop()
{
  // when we don't have anything better to do, add up cpu cycles so we can calculate how "busy" the cpu is.
  idletask++;
  if(idletask > 1000000)
  {
    cpufree = (float)idletask / (float)(micros() - cpucyclestarttime);
    idletask = 0; 
    cpucyclestarttime = micros();
  }
    
}

// function that executes whenever data is received from master
// this function is registered as an event, see setup()
void receiveEvent(int howMany)
{
#if TEST == 1
  while(Wire.available())
  {
    Wire.read();
    bytesRead++;
  }
  
#elif TEST == 2
  unsigned char buf[BYTES_PER_PACKET];
  int i = 0;
  // read all the data the Master sent us, storing the first BYTES_PER_PACKET on the buffer
  if(Wire.available() >= BYTES_PER_PACKET)
    i = Wire.readBytes((char *)buf, BYTES_PER_PACKET);
  while(Wire.available())
  {
    Wire.read();          // receive an extra byte
    extraBytes++;
    bytesRead++;
  }
  
  bytesRead += i;
  
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
        unsigned long v = buf[i] ^ (data[i] + 1); // count the number of bits set in v, the difference between the byte received and the byte expected
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
#elif TEST == 3 || TEST == 4      // slave-master bandwidth tests
  // this signals the end of the test. Read off the master's packet and report the results.
  long bytesReadMaster = 0, totalTimeMaster = 0;
  
  if(Wire.available() >= sizeof(long))
    Wire.readBytes((char *)&bytesReadMaster, sizeof(long));
  if(Wire.available() >= sizeof(long))
    Wire.readBytes((char *)&totalTimeMaster, sizeof(long));
  
  Serial.println("Finished test! ");
  Serial.print(" Bytes sent: ");
  Serial.println(bytesSent);
  Serial.print(" Bytes received: ");
  Serial.print(bytesReadMaster);
  Serial.print(" (");
  Serial.print(100.f * (float)bytesReadMaster / (bytesSent));
  Serial.println("%)");
  Serial.print(" Time: ");
  Serial.print((float)totalTimeMaster / 1000.f);
  Serial.println(" ms ");
  Serial.print(" Bandwidth: ");
  Serial.print((float)bytesSent / (float)totalTimeMaster * 1000000.f);
  Serial.println(" bytes/sec");
  Serial.print(" Slave CPU usage: ");
  Serial.print((idleCpufree - cpufree) / idleCpufree * 100.f);
  Serial.println("%");
  
  bytesSent = 0;
#endif
}

// function that executes whenever data is requested by master
// this function is registered as an event, see setup()
void requestEvent()
{
#if TEST == 1
  Wire.write((uint8_t *)&bytesRead, sizeof(long)); 
  Wire.write((uint8_t *)&cpufree, sizeof(float));
  bytesRead = 0;
#elif TEST == 2
  Wire.write((uint8_t *)&bytesRead, sizeof(long)); 
  Wire.write((uint8_t *)&cpufree, sizeof(float));
  
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
  bytesRead = 0;
  missingBytes = 0;
  extraBytes = 0;
  incorrectBytes = 0;
  incorrectBits = 0;
  datafull = false;
#elif TEST == 3 || TEST == 4      // slave-master bandwidth test
  Wire.write((uint8_t *)data, BYTES_PER_PACKET);
  for(unsigned char i = 0; i < BYTES_PER_PACKET; i++)
    data[i]++;
  bytesSent += BYTES_PER_PACKET;
#endif
}
