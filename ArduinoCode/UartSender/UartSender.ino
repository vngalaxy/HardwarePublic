#include <SoftwareSerial.h>

SoftwareSerial mySerial(7, 9); // RX, TX

void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Native USB only
  }

  Serial.println("Goodnight moon!");

  // set the data rate for the SoftwareSerial port
  mySerial.begin(9600);
}

void loop() // run over and over
{
  mySerial.println("Hello, world?"); //send to uart
  delay(1000);
}