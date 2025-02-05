// ESP32 C3 SERIAL1 (second UART)
HardwareSerial mySerial1(1);

#define rxPin  20
#define txPin  21
#define EN_RAK 10
#define LED_R 8
#define LED_G 9
#define LED_B 2

void setup() {
    Serial.begin(115200);

    // Set up GPIO pins and initialize Serial1
    pinMode(txPin, OUTPUT);
    pinMode(rxPin, INPUT);
    pinMode(EN_RAK, OUTPUT); 

    pinMode(LED_R, OUTPUT); // LED Red
    pinMode(LED_G, OUTPUT); // LED Green
    pinMode(LED_B, OUTPUT); // LED Blue

    digitalWrite(LED_R, HIGH); // turn the LED off (HIGH is the voltage level)
    digitalWrite(LED_G, HIGH); // turn the LED off (HIGH is the voltage level)
    digitalWrite(LED_B, HIGH); // turn the LED off (HIGH is the voltage level)
    delay(1000);

    digitalWrite(EN_RAK, HIGH); // Switch on RAK
}

void loop() {

}
