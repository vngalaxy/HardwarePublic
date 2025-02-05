/***
 *  This example shows LoRaWan protocol joining the network in OTAA mode, class A, region EU868.
 *  Device will send uplink every 20 seconds.
***/

#define OTAA_PERIOD   (60000)
#define BLINK_PERIOD (30000)
/*************************************

   LoRaWAN band setting:
     RAK_REGION_EU433
     RAK_REGION_CN470
     RAK_REGION_RU864
     RAK_REGION_IN865
     RAK_REGION_EU868
     RAK_REGION_US915
     RAK_REGION_AU915
     RAK_REGION_KR920
     RAK_REGION_AS923

 *************************************/
#define OTAA_BAND     (RAK_REGION_AS923)
// #define OTAA_DEVEUI   {0x51, 0x8D, 0x4E, 0xD7, 0x35, 0xC0, 0xED, 0x24}
// #define OTAA_APPEUI   {0x01,0x01, 0x01, 0x01, 0x01,0x01, 0x01, 0x01}
// #define OTAA_APPKEY   {0x88, 0x41, 0x07, 0xC7, 0xBF, 0x1E, 0xB7, 0x2F, 0xC6, 0x1B, 0x09, 0xC4, 0x1D, 0x13, 0x99, 0x74}

#define OTAA_DEVEUI   {0x89, 0x03, 0x14, 0x72, 0xd0, 0x53, 0x16, 0xa3}
#define OTAA_APPEUI   {0x01,0x01, 0x01, 0x01, 0x01,0x01, 0x01, 0x01}
#define OTAA_APPKEY   {0xBF, 0xAA, 0xFF, 0xE6, 0x5A, 0x8E, 0x50, 0x12, 0x46, 0x7F, 0x3D, 0xC5, 0x1A, 0xFC, 0xA0, 0x4F}

#define PIN_9 PA9
#define PIN_1 PA1
volatile bool interruptFlag = false;
/** Packet buffer for sending */
uint8_t collected_data[64] = { 0 };
int status = 0;
void recvCallback(SERVICE_LORA_RECEIVE_T * data)
{
    if (data->BufferSize > 0) {
        Serial.println("Something received!");
            String receivedString = "";
    for (int i = 0; i < data->BufferSize; i++) {
      receivedString += (char)data->Buffer[i];  // Chuyển từng byte thành ký tự
    }
    
    // In chuỗi ký tự nhận được
    Serial.println("Received String: " + receivedString);
    Serial.print("\r\n");
    }
}

void joinCallback(int32_t status)
{
    Serial.printf("Join status: %d\r\n", status);
}

/*************************************
 * enum type for LoRa Event
    RAK_LORAMAC_STATUS_OK = 0,
    RAK_LORAMAC_STATUS_ERROR,
    RAK_LORAMAC_STATUS_TX_TIMEOUT,
    RAK_LORAMAC_STATUS_RX1_TIMEOUT,
    RAK_LORAMAC_STATUS_RX2_TIMEOUT,
    RAK_LORAMAC_STATUS_RX1_ERROR,
    RAK_LORAMAC_STATUS_RX2_ERROR,
    RAK_LORAMAC_STATUS_JOIN_FAIL,
    RAK_LORAMAC_STATUS_DOWNLINK_REPEATED,
    RAK_LORAMAC_STATUS_TX_DR_PAYLOAD_SIZE_ERROR,
    RAK_LORAMAC_STATUS_DOWNLINK_TOO_MANY_FRAMES_LOSS,
    RAK_LORAMAC_STATUS_ADDRESS_FAIL,
    RAK_LORAMAC_STATUS_MIC_FAIL,
    RAK_LORAMAC_STATUS_MULTICAST_FAIL,
    RAK_LORAMAC_STATUS_BEACON_LOCKED,
    RAK_LORAMAC_STATUS_BEACON_LOST,
    RAK_LORAMAC_STATUS_BEACON_NOT_FOUND,
 *************************************/

void sendCallback(int32_t status)
{
    if (status == RAK_LORAMAC_STATUS_OK) {
        Serial.println("Successfully sent");
    } else {
        Serial.println("Sending failed");
         digitalWrite(PIN_1, HIGH);
        delay(200);
        digitalWrite(PIN_1, LOW);
        delay(200);
        digitalWrite(PIN_1, HIGH);
        delay(200);
        digitalWrite(PIN_1, LOW);
    }
}

void setup()
{
    Serial.begin(115200, RAK_AT_MODE);
    pinMode(PIN_9, INPUT_PULLDOWN);
    pinMode(PIN_1, OUTPUT);
    // digitalWrite(PIN_1, HIGH);
    Serial.println("RAKwireless LoRaWan OTAA Example");
    Serial.println("------------------------------------------------------");
    attachInterrupt(PIN_9, turnOn, RISING);
    attachInterrupt(PIN_9, turnOff, FALLING);
    if(api.lorawan.nwm.get() != 1)
    {
        Serial.printf("Set Node device work mode %s\r\n",
            api.lorawan.nwm.set() ? "Success" : "Fail");
        api.system.reboot();
    }

    // OTAA Device EUI MSB first
    uint8_t node_device_eui[8] = OTAA_DEVEUI;
    // OTAA Application EUI MSB first
    uint8_t node_app_eui[8] = OTAA_APPEUI;
    // OTAA Application Key MSB first
    uint8_t node_app_key[16] = OTAA_APPKEY;
  
    if (!api.lorawan.appeui.set(node_app_eui, 8)) {
        Serial.printf("LoRaWan OTAA - set application EUI is incorrect! \r\n");
        return;
    }
    if (!api.lorawan.appkey.set(node_app_key, 16)) {
        Serial.printf("LoRaWan OTAA - set application key is incorrect! \r\n");
        return;
    }
    if (!api.lorawan.deui.set(node_device_eui, 8)) {
        Serial.printf("LoRaWan OTAA - set device EUI is incorrect! \r\n");
        return;
    }
  
    if (!api.lorawan.band.set(OTAA_BAND)) {
        Serial.printf("LoRaWan OTAA - set band is incorrect! \r\n");
        return;
    }
    if (!api.lorawan.deviceClass.set(RAK_LORA_CLASS_C)) {
        Serial.printf("LoRaWan OTAA - set device class is incorrect! \r\n");
        return;
    }
    if (!api.lorawan.njm.set(RAK_LORA_OTAA))	// Set the network join mode to OTAA
    {
        Serial.printf("LoRaWan OTAA - set network join mode is incorrect! \r\n");
        return;
    }
    if (!api.lorawan.join())	// Join to Gateway
    {
        Serial.printf("LoRaWan OTAA - join fail! \r\n");
        return;
    }
  
    /** Wait for Join success */
    while (api.lorawan.njs.get() == 0) {
        Serial.print("Wait for LoRaWAN join...");
         digitalWrite(PIN_1, HIGH);
        delay(200);
        digitalWrite(PIN_1, LOW);
        delay(200);
        digitalWrite(PIN_1, HIGH);
        delay(200);
        digitalWrite(PIN_1, LOW);
        delay(200);
        digitalWrite(PIN_1, HIGH);
        delay(200);
        digitalWrite(PIN_1, LOW);
        api.lorawan.join();
        delay(10000);
    }
  
    if (!api.lorawan.adr.set(true)) {
        Serial.printf("LoRaWan OTAA - set adaptive data rate is incorrect! \r\n");
        return;
    }
    if (!api.lorawan.rety.set(1)) {
        Serial.printf("LoRaWan OTAA - set retry times is incorrect! \r\n");
        return;
    }
    if (!api.lorawan.cfm.set(1)) {
        Serial.printf("LoRaWan OTAA - set confirm mode is incorrect! \r\n");
        return;
    }
    if (!api.lorawan.dr.set(5)) {
        Serial.printf("LoRaWan OTAA - set datarate is incorrect! \r\n");
        return;
    }
  
    /** Check LoRaWan Status*/
    Serial.printf("Duty cycle is %s\r\n", api.lorawan.dcs.get()? "ON" : "OFF");	// Check Duty Cycle status
    Serial.printf("Packet is %s\r\n", api.lorawan.cfm.get()? "CONFIRMED" : "UNCONFIRMED");	// Check Confirm status
    uint8_t assigned_dev_addr[4] = { 0 };
    api.lorawan.daddr.get(assigned_dev_addr, 4);
    Serial.printf("Device Address is %02X%02X%02X%02X\r\n", assigned_dev_addr[0], assigned_dev_addr[1], assigned_dev_addr[2], assigned_dev_addr[3]);	// Check Device Address
    Serial.printf("Uplink period is %ums\r\n", OTAA_PERIOD);
    Serial.println("");
    // api.system.restoreDefault();
    // api.system.sleep.setup(RUI_WAKEUP_RISING_EDGE,PA8);
    //  attachInterrupt(PA8, uplink_interrupt, RISING);
    api.lorawan.registerRecvCallback(recvCallback);
    api.lorawan.registerJoinCallback(joinCallback);
    api.lorawan.registerSendCallback(sendCallback);
}

void turnOn()
{
    static uint64_t last = 0;
    static uint64_t elapsed;
    interruptFlag = true;
    int t = digitalRead(PIN_9);
    
    if(t==1){
    if ((elapsed = millis() - last) > 5000) {
        uplink_routine();
        Serial.println(t);
        last = millis();
        };
    };
    
}
void turnOff()
{
    static uint64_t last = 0;
    static uint64_t elapsed;
    interruptFlag = false;
    int t = digitalRead(PIN_9);
    
    if(t==0){
    if ((elapsed = millis() - last) > 5000) {
        uplink_routine();
        Serial.println(t);
        last = millis();
        };
    };
    
}

void uplink_routine()
{
    /** Payload of Uplink */
    uint8_t data_len = 0;
     int t = digitalRead(PIN_9);
    collected_data[data_len++] = t;
  
    Serial.println("Data Packet:");
    for (int i = 0; i < data_len; i++) {
        Serial.printf("0x%02X ", collected_data[i]);
    }
    Serial.println("");
  
    /** Send the data package */
    if (api.lorawan.send(data_len, (uint8_t *) & collected_data, 2, true, 5)) {
        Serial.println("Sending is requested");
    } else {
        Serial.println("Sending failed");
    }
}

void loop()
{
    static uint64_t last = 0;
    static uint64_t lastBlink = 0;
    
    static uint64_t current;
    // int t = digitalRead(PIN_9);
    // Serial.print(t);
    // if (t==HIGH) {
        if ((current = millis() - last) > OTAA_PERIOD) {
          /*
          Nếu thời gian hiện tại (current) - lần cuối kích hoạt (last) > thời gian mong muốn: Sẽ kích hoạt code bên trong
          Sau đó gán thời gian hiện tại cho last
          */
        uplink_routine();
        // detachInterrupt(PIN_9);
        last = millis();
    }
    if ((current = millis() - lastBlink) > BLINK_PERIOD) {
        // detachInterrupt(PIN_9);
        digitalWrite(PIN_1, HIGH);
        delay(200);
        digitalWrite(PIN_1, LOW);
        
        lastBlink = millis();
    }
  //   if (interruptFlag) {
  //   interruptFlag = false; // Xóa cờ ngắt sau khi xử lý

  //   // Thực hiện các tác vụ cần thiết khi có ngắt
  //   // Ví dụ: In ra Serial hoặc thay đổi trạng thái của một biến nào đó
  //   Serial.println("Ngắt được xử lý!");
  // }
  // Serial.println(digitalRead(PIN_9));
    // }
    // Serial.print("Try sleep ");
    // Serial.print(OTAA_PERIOD);
    // Serial.println("ms");
    // api.system.sleep.lora(OTAA_PERIOD);
    // Serial.println(digitalRead(PIN_9));
    // Serial.println("Wakeup..");
}
