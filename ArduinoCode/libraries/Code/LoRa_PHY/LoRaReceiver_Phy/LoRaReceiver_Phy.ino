#include <SPI.h>
#include <LoRa.h>

int counter = 0;

// Parameters you can play with :

int txPower = 14; // from 0 to 20, default is 14
int spreadingFactor = 12; // from 7 to 12, default is 12
long signalBandwidth = 125E3; // 7.8E3, 10.4E3, 15.6E3, 20.8E3, 31.25E3,41.7E3,62.5E3,125E3,250E3,500e3, default is 125E3
int codingRateDenominator=5; // Numerator is 4, and denominator from 5 to 8, default is 5
int preambleLength=8; // from 2 to 20, default is 8
String payload = "hello"; // you can change the payload
int bat=0;
int tat=0;
#define SS 10
#define RST 8
#define DI0 3
#define BAND 865E6  // Here you define the frequency carrier
#define CFG_VN 1
void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("LoRa Receiver");
  Serial.print("SetFrequency : ");
  Serial.print(BAND);
  Serial.println("Hz");
  Serial.print("SetSpreadingFactor : SF");
  Serial.println(spreadingFactor);
  

  SPI.begin();
  LoRa.setPins(SS,RST,DI0);

  

  if (!LoRa.begin(BAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
 LoRa.setTxPower(txPower,1);
 LoRa.setSpreadingFactor(spreadingFactor);
 LoRa.setSignalBandwidth(signalBandwidth);
 LoRa.setCodingRate4(codingRateDenominator);
 LoRa.setPreambleLength(preambleLength);
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
 attachInterrupt(0, tatled, FALLING); 
 attachInterrupt(1, batled, FALLING); 
}
void tatled()
{
    analogWrite(A2,0); // tắt đèn led
    Serial.print("tat");
    if(tat==1){
      tat=0;
    }else tat =1;
     pinMode(2, INPUT_PULLUP); 
}
void batled()
{
    analogWrite(A2,1023); // bật đèn led
    Serial.print("bat");
    if(bat==1){
      bat = 0;
    }else bat =1;
     pinMode(3, INPUT_PULLUP); 
}
void loop() {
   // try to parse packet
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // received a packet
   
    // Serial.println(LoRa.readString().length());
    Serial.print(digitalRead(3));
    int t = LoRa.readString().length();
     Serial.print("Received packet: ");
     Serial.println(t);
  if(t==3&&bat==0&&tat==0)
  {
  analogWrite(A2,0);
  Serial.print("false");
  }
  else if(t==2&&bat==0&&tat==0)
  {
  analogWrite(A2,1023);
  Serial.print("true");
  }
  Serial.print(bat);
  Serial.print(tat);
    // // read packet
    // while (LoRa.available()) {
    //   Serial.print((char)LoRa.read());
    // }

    // // print RSSI of packet
    // Serial.print("' with RSSI ");
    // Serial.println(LoRa.packetRssi());
  }
}




 
