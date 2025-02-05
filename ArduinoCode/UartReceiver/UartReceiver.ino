char buffer[5]; // array data


void setup() {
  Serial.begin(9600);
}

void loop() {
  int i =0;
while(Serial.available()) {
buffer[i] = Serial.read();
i++;
if(i == 5) {
  Serial.println(buffer);
  break;
  };
}

memset(buffer,0,5);      // clear array
delay(1000);
}