#define MIC PB3 // Chân A3 nối với tín hiệu analog từ MAX9814
#define EN_SENSOR PA8
int analogValue = 0;

void setup() {
  Serial.begin(115200); // Khởi động giao tiếp serial với baudrate 9600
  pinMode(EN_SENSOR,OUTPUT);
  digitalWrite(EN_SENSOR,HIGH);
  pinMode(MIC,INPUT);
}

void loop() {
  analogValue = analogRead(MIC); // Đọc giá trị analog từ chân A3
  Serial.println(analogValue); // Gửi giá trị này qua cổng serial
  delayMicroseconds(20);
}