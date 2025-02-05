/***
 *  This example shows LoRaWan protocol joining the network in ABP mode, class A, region US915.
 *  Device will send uplink every 5 seconds.
***/

#define ABP_PERIOD   (30000)
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
#define ABP_BAND     (RAK_REGION_AS923_2)
// #define ABP_DEVADDR  {0x00, 0xbf, 0x18, 0xf7}
// #define ABP_APPSKEY  {0xB4, 0xC9, 0x99, 0x8B, 0x0A, 0x45, 0x6F, 0x5C, 0xB8, 0xC1, 0xF3, 0x2A, 0x5F, 0x68, 0xCA, 0x53}
// #define ABP_NWKSKEY  {0x14, 0xC8, 0x2A, 0xAB, 0x71, 0x00, 0x60, 0xA3, 0x4C, 0x50, 0x3E, 0x03, 0x65, 0x7C, 0xC4, 0x74}

#define ABP_DEVADDR  {0x01, 0xc9, 0xe1, 0x2a}
#define ABP_APPSKEY  {0x1B, 0x59, 0xF2, 0xD2, 0x50, 0x7B, 0x47, 0x01, 0xF9, 0x58, 0xA5, 0xC1, 0x6A, 0x65, 0x79, 0xA2}
#define ABP_NWKSKEY  {0x8C, 0x8F, 0x2B, 0xBA, 0x61, 0xB0, 0x45, 0x1C, 0x0D, 0xEC, 0x24, 0xBA, 0x7B, 0x52, 0x2D, 0xBC}

#define PIN_8 PA8

/** Packet buffer for sending */
uint8_t collected_data[64] = { 0 };

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
    if(receivedString=="on"){
      Serial.println("Status is: On");
       digitalWrite(PIN_8, HIGH);
    }else if(receivedString=="off"){
       Serial.println("Status is: Off");
        digitalWrite(PIN_8, LOW);
    }else{
      Serial.println(receivedString);
    }
    Serial.print("\r\n");
  }
}

/*************************************
   enum type for LoRa Event
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
  }
}

void setup()
{
  Serial.begin(115200, RAK_AT_MODE);
  delay(2000);

  Serial.println("RAKwireless LoRaWan ABP Example");
  Serial.println("------------------------------------------------------");
  if (api.lorawan.nwm.get() != 1)
  {
    Serial.printf("Set Node device work mode %s\r\n",
                  api.lorawan.nwm.set() ? "Success" : "Fail");
    api.system.reboot();
  }else if(api.lorawan.nwm.get()==1){
    Serial.printf("Node device work mode LoRaWAN %s\r\n");
  };

  // ABP Device Address MSB first
  uint8_t node_dev_addr[4] = ABP_DEVADDR;
  // ABP Application Session Key
  uint8_t node_app_skey[16] = ABP_APPSKEY;
  // ABP Network Session Key
  uint8_t node_nwk_skey[16] = ABP_NWKSKEY;

  if (!api.lorawan.njm.set(0))	// Set the network join mode to ABP
  {
    Serial.printf("LoRaWan ABP - set network join mode is incorrect! \r\n");
    Serial.printf("Network join mode is %s\n\r", api.lorawan.njm.get() ? "OTAA" : "ABP");
    return;
  }
  
  if (!api.lorawan.daddr.set(node_dev_addr, 4)) {
    Serial.printf("LoRaWan ABP - set device addr is incorrect! \r\n");
    return;
  }
  if (!api.lorawan.appskey.set(node_app_skey, 16)) {
    Serial.printf("LoRaWan ABP - set application session key is incorrect! \r\n");
    return;
  }
  if (!api.lorawan.nwkskey.set(node_nwk_skey, 16)) {
    Serial.printf("LoRaWan ABP - set network session key is incorrect! \r\n");
    return;
  }
  if (!api.lorawan.band.set(ABP_BAND)) {
    Serial.printf("LoRaWan ABP - set band is incorrect! \r\n");
    return;
  }
  if (!api.lorawan.deviceClass.set(RAK_LORA_CLASS_C)) {
    Serial.printf("LoRaWan ABP - set device class is incorrect! \r\n");
    return;
  }
  if (!api.lorawan.adr.set(0)) {
    Serial.printf("LoRaWan ABP - set adaptive data rate is incorrect! \r\n");
    return;
  }
  if (!api.lorawan.rety.set(0)) {
    Serial.printf("LoRaWan ABP - set retry times is incorrect! \r\n");
    return;
  }

  // uint16_t maskBuff = 0x0002;
  // Serial.printf("Set channel mask %s\r\n", api.lorawan.mask.set(&maskBuff) ? "Success" : "Fail");

  service_lora_set_timereq(1);

  /** Check LoRaWan Status*/
  Serial.printf("Duty cycle is %s\r\n", api.lorawan.dcs.get() ? "ON" : "OFF");	// Check Duty Cycle status
  Serial.printf("Packet is %s\r\n", api.lorawan.cfm.get() ? "CONFIRMED" : "UNCONFIRMED");	// Check Confirm status
  uint8_t assigned_dev_addr[4] = { 0 };
  Serial.printf("Device Address is %02X%02X%02X%02X\r\n", assigned_dev_addr[0], assigned_dev_addr[1], assigned_dev_addr[2], assigned_dev_addr[3]);	// Check Device Address
  Serial.printf("Uplink period is %ums\r\n", ABP_PERIOD);
  Serial.println("");
  pinMode(PIN_8, OUTPUT);
  digitalWrite(PIN_8, LOW);
  api.lorawan.registerRecvCallback(recvCallback);
  api.lorawan.registerSendCallback(sendCallback);
}

void uplink_routine()
{
  /** Payload of Uplink */
  uint8_t data_len = 0;
  collected_data[data_len++] = (uint8_t) 't';
  collected_data[data_len++] = (uint8_t) 'e';
  collected_data[data_len++] = (uint8_t) 's';
  collected_data[data_len++] = (uint8_t) 't';

  Serial.println("Data Packet:");
  for (int i = 0; i < data_len; i++) {
    Serial.printf("0x%02X ", collected_data[i]);
  }
  Serial.println("");

  /** Send the data package */
  if (api.lorawan.send(data_len, (uint8_t *) & collected_data, 2,true, 1)) {
    Serial.println("Sending is requested");
  } else {
    Serial.println("Sending failed");
  }
}

void loop()
{
  static uint64_t last = 0;
  static uint64_t elapsed;

  if ((elapsed = millis() - last) > ABP_PERIOD)
  {
    uplink_routine();
    last = millis();
    // Serial.printf("The local time(UTC) is %s\r\n", api.lorawan.timereq.get().c_str());
  }
  
  //Serial.printf("Try sleep %ums..", ABP_PERIOD);
  // api.system.sleep.all(ABP_PERIOD);
  //Serial.println("Wakeup..");
}