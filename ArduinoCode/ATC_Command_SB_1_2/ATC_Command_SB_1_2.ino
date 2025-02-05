/**
 SSS  M   M  AA  RRRR  TTTTTT     BBBB  U   U III L    DDD  III N   N  GGG 
S     MM MM A  A R   R   TT       B   B U   U  I  L    D  D  I  NN  N G    
 SSS  M M M AAAA RRRR    TT       BBBB  U   U  I  L    D  D  I  N N N G  GG
    S M   M A  A R R     TT       B   B U   U  I  L    D  D  I  N  NN G   G
SSSS  M   M A  A R  RR   TT       BBBB   UUU  III LLLL DDD  III N   N  GGG 

   @file ATC_Command_SB_1_2.ino
   @author Duynghk

   @brief This sketch add custom ATC command to RFThings RF210SB board ver 3.0. These commands help controlling on-board sensors,
   For detail description, please visit: https://github.com/RFThings/SMART-BUILDING.git

   @version 0.1.2
   @date 2024-06-27

   @copyright Copyright (c) 2024

*/

#define DATA_INTERVAL 5000 //ms

#include <Arduino.h>
#include <SensirionI2CScd4x.h>
#include "Zanshin_BME680.h"
#include <Wire.h>
#include "kxtj3-1057.h"
#include "Adafruit_LTR329_LTR303.h"
#include "Adafruit_SGP30.h"
#include "ATC_Command_SB_1_2.h"



#define DATA_INTERVAL 500 // ms


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
// #define ABP_BAND     (RAK_REGION_AS923_2)
// #define ABP_DEVADDR  {0x26, 0x0B, 0x24, 0xF4}
// #define ABP_APPSKEY  {0x39, 0x23, 0x1A, 0xFE, 0x93, 0x20, 0x93, 0x99, 0xB4, 0x24, 0x9E, 0x0A, 0x95, 0xE9, 0xE3, 0x92}
// #define ABP_NWKSKEY  {0x9E, 0xAA, 0xED, 0xAD, 0x01, 0x9A, 0x29, 0x05, 0x94, 0x9F, 0x27, 0x5A, 0x06, 0xE7, 0x2C, 0xE0}
#define OTAA_BAND     (RAK_REGION_AS923_2)
#define OTAA_DEVEUI   {0xEF, 0xE0, 0xA9, 0xC9, 0xDE, 0x04, 0x87, 0xC1}
#define OTAA_APPEUI   {0x01,0x01, 0x01, 0x01, 0x01,0x01, 0x01, 0x01}
#define OTAA_APPKEY   {0xC9, 0xF4, 0xCC, 0x74, 0x55, 0x40, 0x4F, 0x4B, 0x3D, 0xF6, 0x73, 0x45, 0x3E, 0xB8, 0x1F, 0xA9}
#define OTAA_PERIOD   (20000)
KXTJ3 myIMU(0x0E); // Address can be 0x0E or 0x0F
volatile bool flag;
Adafruit_LTR303 ltr = Adafruit_LTR303();
SensirionI2CScd4x scd4x;
BME680_Class BME680;  ///< Create an instance of the BME680 class
Adafruit_SGP30 sgp;

uint16_t error;
int pirState = LOW;             // we start, assuming no motion detected
int val = 0;
unsigned char gain = 0;   // Gain setting, values = 0-7
unsigned char integrationTime = 0; // Integration ("shutter") time in milliseconds
uint16_t voltage_adc;
uint16_t voltage;
float kx_x, kx_y, kx_z;
//sensors_event_t hum, temp;
bool valid;
bool ltr_status;
uint16_t visible_plus_ir, infrared;
double lux;
unsigned long currentMillis = 0, getSensorDataPrevMillis = 0;
//bool sht_status;

uint16_t co2 = 0;
float temperature = 0.0f;
float humidity = 0.0f;
int interation = 0; // iteration for PIR
uint8_t PIR_array[12];
uint8_t Sound_array[12];

float   sampleRate = 6.25;  // HZ - Samples per second - 0.781, 1.563, 3.125, 6.25, 12.5, 25, 50, 100, 200, 400, 800, 1600Hz
uint8_t accelRange = 2;     // Accelerometer range = 2, 4, 8, 16g
bool KXTJ3_status;

static char     buf[16];                        // sprintf text buffer
static float    alt;                            // Temporary variable
static uint16_t loopCounter = 0;                // Display iterations
int counter = 0;


void recvCallback(SERVICE_LORA_RECEIVE_T * data)
{
    if (data->BufferSize > 0) {
        Serial.println("Something received!");
        for (int i = 0; i < data->BufferSize; i++) {
            Serial.printf("%x", data->Buffer[i]);
        }
        Serial.print("\r\n");
    }
}

void joinCallback(int32_t status)
{
    Serial.printf("Join status: %d\r\n", status);
}

void sendCallback(int32_t status)
{
    if (status == RAK_LORAMAC_STATUS_OK) {
        Serial.println("Successfully sent");
    } else {
        Serial.println("Sending failed");
    }
}

void setup_LoRaWan() 
{
  // if(api.lorawan.nwm.get() != 1)
  // {
  //   Serial.printf("Set Node device work mode %s\r\n",
  //   api.lorawan.nwm.set() ? "Success" : "Fail");
  //   api.system.reboot();
  // }

  //   // ABP Device Address MSB first
  // uint8_t node_dev_addr[4] = ABP_DEVADDR;
  //   // ABP Application Session Key
  // uint8_t node_app_skey[16] = ABP_APPSKEY;
  //   // ABP Network Session Key
  // uint8_t node_nwk_skey[16] = ABP_NWKSKEY;
  
  // if (!api.lorawan.njm.set(RAK_LORA_ABP))	// Set the network join mode to ABP
  // {
  //   Serial.printf("LoRaWan ABP - set network join mode is incorrect! \r\n");
  //   return;
  // }
  // if (!api.lorawan.daddr.set(node_dev_addr, 4)) {
  //   Serial.printf("LoRaWan ABP - set device addr is incorrect! \r\n");
  //   return;
  // }
  // if (!api.lorawan.appskey.set(node_app_skey, 16)) {
  //   Serial.printf("LoRaWan ABP - set application session key is incorrect! \r\n");
  //   return;
  // }
  // if (!api.lorawan.nwkskey.set(node_nwk_skey, 16)) {
  //   Serial.printf("LoRaWan ABP - set network session key is incorrect! \r\n");
  //   return;
  //   }
  // if (!api.lorawan.band.set(ABP_BAND)) {
  //   Serial.printf("LoRaWan ABP - set band is incorrect! \r\n");
  //   return;
  // }
  // if (!api.lorawan.deviceClass.set(RAK_LORA_CLASS_C)) {
  //   Serial.printf("LoRaWan ABP - set device class is incorrect! \r\n");
  //   return;
  // }
  
  // if (!api.lorawan.adr.set(true)) {
  //   Serial.printf("LoRaWan ABP - set adaptive data rate is incorrect! \r\n");
  //   return;
  // }
  // if (!api.lorawan.rety.set(1)) {
  //   Serial.printf("LoRaWan ABP - set retry times is incorrect! \r\n");
  //   return;
  // }
  // if (!api.lorawan.cfm.set(1)) {
  //   Serial.printf("LoRaWan ABP - set confirm mode is incorrect! \r\n");
  //   return;
  // }
  // service_lora_set_timereq(1);
  //   /** Check LoRaWan Status*/
  // Serial.printf("Duty cycle is %s\r\n", api.lorawan.dcs.get()? "ON" : "OFF");	// Check Duty Cycle status
  // Serial.printf("Packet is %s\r\n", api.lorawan.cfm.get()? "CONFIRMED" : "UNCONFIRMED");	// Check Confirm status
  // uint8_t assigned_dev_addr[4] = { 0 };
  // api.lorawan.daddr.get(assigned_dev_addr, 4);
  // Serial.printf("Device Address is %02X%02X%02X%02X\r\n", assigned_dev_addr[0], assigned_dev_addr[1], assigned_dev_addr[2], assigned_dev_addr[3]);	// Check Device Address
  // Serial.println("");
  // api.lorawan.registerRecvCallback(recvCallback);
  // api.lorawan.registerSendCallback(sendCallback);


  Serial.begin(115200, RAK_AT_MODE);
    // delay(2000);
    // pinMode(PIN_8, OUTPUT);
    // digitalWrite(PIN_8, LOW);
    Serial.println("RAKwireless LoRaWan OTAA Example");
    Serial.println("------------------------------------------------------");
  
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
  
    /** Check LoRaWan Status*/
    Serial.printf("Duty cycle is %s\r\n", api.lorawan.dcs.get()? "ON" : "OFF");	// Check Duty Cycle status
    Serial.printf("Packet is %s\r\n", api.lorawan.cfm.get()? "CONFIRMED" : "UNCONFIRMED");	// Check Confirm status
    uint8_t assigned_dev_addr[4] = { 0 };
    api.lorawan.daddr.get(assigned_dev_addr, 4);
    // api.lorawan.dr.set(5);
    Serial.printf("Device Address is %02X%02X%02X%02X\r\n", assigned_dev_addr[0], assigned_dev_addr[1], assigned_dev_addr[2], assigned_dev_addr[3]);	// Check Device Address
    Serial.printf("Uplink period is %ums\r\n", OTAA_PERIOD);
    Serial.println("");
    api.lorawan.registerRecvCallback(recvCallback);
    api.lorawan.registerJoinCallback(joinCallback);
    api.lorawan.registerSendCallback(sendCallback);
}

void printUint16Hex(uint16_t value) {
  Serial.print(value < 4096 ? "0" : "");
  Serial.print(value < 256 ? "0" : "");
  Serial.print(value < 16 ? "0" : "");
  Serial.print(value, HEX);
}

// // void printSerialNumber(uint16_t serial0, uint16_t serial1, uint16_t serial2) {
//   Serial.print("Serial: 0x");
//   printUint16Hex(serial0);
//   // printUint16Hex(serial1);
//   printUint16Hex(serial2);
//   Serial.println();
// }

void SCD4x_init()
{
  Wire.begin();

  uint16_t error;
  char errorMessage[256];

  scd4x.begin(Wire);


  // stop potentially previously started measurement
  error = scd4x.stopPeriodicMeasurement();
  if (error) {
    // Serial.print("Error trying to execute stopPeriodicMeasurement(): ");
    errorToString(error, errorMessage, 256);
    // Serial.println(errorMessage);
  }

  uint16_t serial0;
  uint16_t serial1;
  uint16_t serial2;
  error = scd4x.getSerialNumber(serial0, serial1, serial2);
  if (error) {
    // Serial.print("Error trying to execute getSerialNumber(): ");
    errorToString(error, errorMessage, 256);
    //Serial.println(errorMessage);
  } 

  // Start Measurement
  error = scd4x.startPeriodicMeasurement();
  if (error) {
    Serial.println("SCD4x error!");
    // errorToString(error, errorMessage, 256);
    // Serial.println(errorMessage);
  }
  else 
  {
    Serial.println("Successfully init SCD4x");
  }

  // Serial.println("Waiting for first measurement... (5 sec)");
}

SCD4xData read_SCD4x() {
  SCD4xData sensorData;
  sensorData.valid = false;

  char errorMessage[256];

  bool isDataReady = false;
  error = scd4x.getDataReadyFlag(isDataReady);
  if (error) {
    errorToString(error, errorMessage, 256);
    return sensorData;
  }
  if (!isDataReady) {
    return sensorData;
  }
  error = scd4x.readMeasurement(co2, temperature, humidity);
  if (error) {
    errorToString(error, errorMessage, 256);
  } else if (co2 != 0) {
    sensorData.co2 = co2;
    sensorData.temperature = temperature;
    sensorData.humidity = humidity;
    sensorData.valid = true;
  }
  return sensorData;
}


float altitude(const int32_t press, const float seaLevel = 1013.25);
float altitude(const int32_t press, const float seaLevel) {
  /*!
    @brief     This converts a pressure measurement into a height in meters
    @details   The corrected sea-level pressure can be passed into the function if it is known,
             otherwise the standard atmospheric pressure of 1013.25hPa is used (see
             https://en.wikipedia.org/wiki/Atmospheric_pressure) for details.
    @param[in] press    Pressure reading from BME680
    @param[in] seaLevel Sea-Level pressure in millibars
    @return    floating point altitude in meters.
  */
  static float Altitude;
  Altitude =
    44330.0 * (1.0 - pow(((float)press / 100.0) / seaLevel, 0.1903));  // Convert into meters
  return (Altitude);
}  // of method altitude()

void BME680_init() 
{
  if(!BME680.begin(I2C_STANDARD_MODE)) {  // Start BME680 using I2C, use first device found
    Serial.println("BME680 error!");
  }  // of loop until device is located
  else 
  {
    BME680.setOversampling(TemperatureSensor, Oversample16);  // Use enumerated type values
    BME680.setOversampling(HumiditySensor, Oversample16);     // Use enumerated type values
    BME680.setOversampling(PressureSensor, Oversample16);     // Use enumerated type values
    BME680.setIIRFilter(IIR4);  // Use enumerated type values
    BME680.setGas(320, 50);  // 320�c for 150 milliseconds
    Serial.println("Successfully init BME680");
  }
}

BME680Data read_BME680() {
  BME680Data sensorData;
  sensorData.valid = false;

  static int32_t  temp, humidity, pressure, gas;  // BME readings
  BME680.getSensorData(temp, humidity, pressure, gas);  // Get readings

  // Lưu dữ liệu vào struct
  sensorData.temperature = temp/100.0;
  sensorData.humidity = humidity/1000.0;
  sensorData.pressure = pressure / 100.0; // hPa
  sensorData.gas_resistance = gas / 100.0; // kOhms
  sensorData.valid = true;

  return sensorData;
}

void SGP30_init() 
{

  if (!sgp.begin()){
    Serial.println("SGP30 error!");
  }
  else 
  {
    Serial.println("Successfully init SGP30");
  }
}

SGP30Data read_SGP30() {
  SGP30Data sensorData;

  if (! sgp.IAQmeasure()) {
    Serial.println("SGP30 Measurement failed");
    // Trả về giá trị mặc định nếu đo thất bại
    sensorData.TVOC = 0;
    sensorData.eCO2 = 0;
    return sensorData;
  }
  
  // Lưu dữ liệu vào struct
  sensorData.TVOC = sgp.TVOC;
  sensorData.eCO2 = sgp.eCO2;
  if (! sgp.IAQmeasureRaw()) {
    sensorData.rawH2 = 0;
    sensorData.rawEthanol = 0;
    return sensorData;
  }
  sensorData.rawH2 = sgp.rawH2;
  sensorData.rawEthanol = sgp.rawEthanol;
  
  delay(1000);

  counter++;
  if (counter == 30) {
    counter = 0;

    uint16_t TVOC_base, eCO2_base;
    if (! sgp.getIAQBaseline(&eCO2_base, &TVOC_base)) {
      Serial.println("SGP30 Failed to get baseline readings");
      return sensorData;
    }
  }

  return sensorData;
}

void KXTJ3_init()
{
  if( myIMU.begin(sampleRate, accelRange) != 0 )
  {
    Serial.println("KXTJ3 error!");
  }
  else
  {
    Serial.println("Successfully init KXTJ3");
  }
  
  // Detection threshold, movement duration and polarity
  myIMU.intConf(123, 1, 10, HIGH);

  uint8_t readData = 0;

  // Get the ID:
  myIMU.readRegister(&readData, KXTJ3_WHO_AM_I);
}

KXTJ3Data read_KXTJ3() {
  KXTJ3Data sensorData;
  sensorData.valid = false;

  // Đọc giá trị gia tốc từ các trục X, Y, Z
  sensorData.x = myIMU.axisAccel(X);
  sensorData.y = myIMU.axisAccel(Y);
  sensorData.z = myIMU.axisAccel(Z);

  // Giả sử các giá trị được đọc hợp lệ nếu không phải là giá trị mặc định (thay đổi điều kiện nếu cần)
  if (sensorData.x != 0 || sensorData.y != 0 || sensorData.z != 0) {
    sensorData.valid = true;
  }

  return sensorData;
}

void LTR_init()
{
  if ( ! ltr.begin() ) {
    Serial.println("LTR error!");
  }
  else 
  {
    // Set gain of 1 (see advanced demo for all options!
    ltr.setGain(LTR3XX_GAIN_1);
    // Set integration time of 50ms (see advanced demo for all options!
    ltr.setIntegrationTime(LTR3XX_INTEGTIME_50);
    // Set measurement rate of 50ms (see advanced demo for all options!
    ltr.setMeasurementRate(LTR3XX_MEASRATE_50);
    Serial.println("Successfully init LTR303");
  }
}

LTRData read_LTR() {
  LTRData sensorData;
  sensorData.valid = ltr.readBothChannels(sensorData.visible_plus_ir, sensorData.infrared);
  return sensorData;
}

int read_Battery() 
{
  int voltage_adc = (uint16_t)analogRead(BATVOLT_PIN);
  voltage = (uint16_t)((ADC_AREF / 1.024) * (BATVOLT_R1 + BATVOLT_R2) / BATVOLT_R2 * (float)voltage_adc);
  return voltage;
}

bool check_power_status()
{
  int adc_value = analogRead(POWER_STATUS);
  if (adc_value < POWER_THRESHOLD) return false;
  return true;
}

void read_sound(SoundData* sensorData) {
  sensorData->soundArray = new int[sensorData->numSamples];

  unsigned long startTime;
  unsigned long elapsedTime;
  unsigned long intervalMicros = 1000000 / sensorData->sampleRate; // Khoảng thời gian giữa các mẫu (micro giây)

  pinMode(LED, OUTPUT); // Thiết lập chân LED là output

  for (int i = 0; i < sensorData->numSamples; i++) {
    startTime = micros();
    int soundValue = analogRead(MIC);
    sensorData->soundArray[i] = soundValue; // Lưu giá trị vào mảng

    // Điều khiển LED dựa trên giá trị âm thanh
    if (soundValue > 380 && soundValue < 1000) {
      digitalWrite(LED, LOW); // Nếu âm thanh nằm trong ngưỡng, bật LED
    } else {
      digitalWrite(LED, HIGH); // Nếu âm thanh không nằm trong ngưỡng, tắt LED
    }

    // Đợi đủ thời gian giữa các lần lấy mẫu
    do {
      elapsedTime = micros() - startTime;
    } while (elapsedTime < intervalMicros);
  }

  // Giả sử dữ liệu hợp lệ nếu có bất kỳ mẫu nào trong dải giá trị hợp lệ
  sensorData->valid = true;

  digitalWrite(LED, HIGH); // Tắt LED khi kết thúc lấy mẫu
}


int read_PIR(){
  return digitalRead(PIR);
}

int get_fan_status() 
{
  return digitalRead(FAN);
}

void turn_on_fan() 
{
  digitalWrite(FAN, HIGH);
}

void turn_off_fan() 
{
  digitalWrite(FAN, LOW);
}

void read_Sensors(SensorsData *sensorsData) 
{
  sensorsData->ltr = read_LTR();
  sensorsData->scd4x = read_SCD4x();
  sensorsData->bme680 = read_BME680();
  sensorsData->sgp30 = read_SGP30();
  sensorsData->kxtj3 = read_KXTJ3();
  read_sound(&(sensorsData->sound));
  sensorsData->pir = read_PIR();
  sensorsData->battery_voltage = read_Battery();
  sensorsData->power_status = check_power_status();
}

void serializeSensorData(const SensorsData& sensorsData, unsigned char*& buffer, size_t& size) {
  // Calculate total size needed for the buffer
  size = sizeof(sensorsData.scd4x) + sizeof(sensorsData.bme680) + sizeof(sensorsData.sgp30) +
         sizeof(sensorsData.kxtj3) + sizeof(sensorsData.ltr) + sizeof(sensorsData.sound.sampleRate) +
         sizeof(sensorsData.sound.sampleDuration) + sizeof(sensorsData.sound.numSamples) +
         sizeof(sensorsData.sound.valid) + sensorsData.sound.numSamples * sizeof(int) +
         sizeof(sensorsData.pir) + sizeof(sensorsData.battery_voltage) + sizeof(sensorsData.power_status);

  // Allocate memory for the buffer
  Serial.print(size);
  buffer = new unsigned char[size];
  unsigned char* ptr = buffer;

  // Copy data to buffer
  std::memcpy(ptr, &sensorsData.scd4x, sizeof(sensorsData.scd4x));
  ptr += sizeof(sensorsData.scd4x);

  std::memcpy(ptr, &sensorsData.bme680, sizeof(sensorsData.bme680));
  ptr += sizeof(sensorsData.bme680);

  std::memcpy(ptr, &sensorsData.sgp30, sizeof(sensorsData.sgp30));
  ptr += sizeof(sensorsData.sgp30);

  std::memcpy(ptr, &sensorsData.kxtj3, sizeof(sensorsData.kxtj3));
  ptr += sizeof(sensorsData.kxtj3);

  std::memcpy(ptr, &sensorsData.ltr, sizeof(sensorsData.ltr));
  ptr += sizeof(sensorsData.ltr);

  // std::memcpy(ptr, &sensorsData.sound.sampleRate, sizeof(sensorsData.sound.sampleRate));
  // ptr += sizeof(sensorsData.sound.sampleRate);

  // std::memcpy(ptr, &sensorsData.sound.sampleDuration, sizeof(sensorsData.sound.sampleDuration));
  // ptr += sizeof(sensorsData.sound.sampleDuration);

  // std::memcpy(ptr, &sensorsData.sound.numSamples, sizeof(sensorsData.sound.numSamples));
  // ptr += sizeof(sensorsData.sound.numSamples);

  // std::memcpy(ptr, &sensorsData.sound.valid, sizeof(sensorsData.sound.valid));
  // ptr += sizeof(sensorsData.sound.valid);

  // std::memcpy(ptr, sensorsData.sound.soundArray, sensorsData.sound.numSamples * sizeof(int));
  // ptr += sensorsData.sound.numSamples * sizeof(int);

  // std::memcpy(ptr, &sensorsData.pir, sizeof(sensorsData.pir));
  // ptr += sizeof(sensorsData.pir);

  // std::memcpy(ptr, &sensorsData.battery_voltage, sizeof(sensorsData.battery_voltage));
  // ptr += sizeof(sensorsData.battery_voltage);

  // std::memcpy(ptr, &sensorsData.power_status, sizeof(sensorsData.power_status));
}

void SendSensorData() 
{
  SensorsData sensorsData;
    read_Sensors(&sensorsData);
    // Print to Serial
    Serial.println("LTR Data:");
    Serial.print("  Visible + IR: "); Serial.println(sensorsData.ltr.visible_plus_ir);
    Serial.print("  Infrared: "); Serial.println(sensorsData.ltr.infrared);
    Serial.print("  Valid: "); Serial.println(sensorsData.ltr.valid);
    
    Serial.println("BME680 Data:");
    Serial.print("  Temperature: "); Serial.println(sensorsData.bme680.temperature);
    Serial.print("  Humidity: "); Serial.println(sensorsData.bme680.humidity);
    Serial.print("  Pressure: "); Serial.println(sensorsData.bme680.pressure);
    Serial.print("  Gas Resistance: "); Serial.println(sensorsData.bme680.gas_resistance);
    Serial.print("  Valid: "); Serial.println(sensorsData.bme680.valid);

    Serial.println("SGP30 Data:");
    Serial.print("  TVOC: "); Serial.println(sensorsData.sgp30.TVOC);
    Serial.print("  eCO2: "); Serial.println(sensorsData.sgp30.eCO2);
    Serial.print("  Raw H2: "); Serial.println(sensorsData.sgp30.rawH2);
    Serial.print("  Raw Ethanol: "); Serial.println(sensorsData.sgp30.rawEthanol);

    Serial.println("KXTJ3 Data:");
    Serial.print("  X: "); Serial.println(sensorsData.kxtj3.x);
    Serial.print("  Y: "); Serial.println(sensorsData.kxtj3.y);
    Serial.print("  Z: "); Serial.println(sensorsData.kxtj3.z);
    Serial.print("  Valid: "); Serial.println(sensorsData.kxtj3.valid);

    Serial.println("Sound Data:");
    Serial.print("  Sample Rate: "); Serial.println(sensorsData.sound.sampleRate);
    Serial.print("  Sample Duration: "); Serial.println(sensorsData.sound.sampleDuration);
    Serial.print("  Number of Samples: "); Serial.println(sensorsData.sound.numSamples);
    Serial.print("  Valid: "); Serial.println(sensorsData.sound.valid);
    Serial.print("  Sound Array: ");
    
    for (int i = 0; i < sensorsData.sound.numSamples; i++) {
      Serial.print(sensorsData.sound.soundArray[i]);
      Serial.print(" ");
      delay(1);
    }
    Serial.println();

    Serial.println("SCD4x Data:");
    Serial.print("  CO2: "); Serial.println(sensorsData.scd4x.co2);
    Serial.print("  Temperature: "); Serial.println(sensorsData.scd4x.temperature);
    Serial.print("  Humidity: "); Serial.println(sensorsData.scd4x.humidity);
    Serial.print("  Valid: "); Serial.println(sensorsData.scd4x.valid);

    Serial.println("Additional Data:");
    Serial.print("  PIR: "); Serial.println(sensorsData.pir);
    Serial.print("  Battery Voltage: "); Serial.println(sensorsData.battery_voltage);
    Serial.print("  Power Status: "); Serial.println(sensorsData.power_status);

    // Serialize sensor data

    /// LTR-light cam bien anh sang visiable (mat nhin duoc) infrared (hong ngoai)
    int LTRViir = sensorsData.ltr.visible_plus_ir;
    int LTRInf = sensorsData.ltr.infrared;

    ///// BME680 cam bien moi truong (nhiet do do am, ap suat khi quyen ...)
    int BMETemp = sensorsData.bme680.temperature*100;
    int BMEHum = sensorsData.bme680.humidity*100;
    int BMEPressure = sensorsData.bme680.pressure*100;

    //// SGP30  cam bien chat luong khong khi eCO2 va TVOC (formaldehyde, benzene, toluene, xylenes,....)
    int SGPTvoc = sensorsData.sgp30.TVOC;
    int SGPEco2 = sensorsData.sgp30.eCO2;

    //// SCD40 cam bien CO2 nhiet do, do am
    int SCDCo2 = sensorsData.scd4x.co2;
    int SCDTemp = sensorsData.scd4x.temperature*100;
    int SCDHum = sensorsData.scd4x.humidity*100;

    //// KXTJ3 cam bien gia toc (chua can)

    //// Cac gia tri khac
    int bat = sensorsData.battery_voltage;

    // unsigned char buffer[21];
    // // size_t bufferSize = 0;

    // buffer[0] = LTRViir >> 8;
    // buffer[1] = LTRViir & 0xFF;
    // buffer[2] = LTRInf >>8;
    // buffer[3] = LTRInf & 0xFF;

    // buffer[4] = BMETemp >> 8;
    // buffer[5] = BMETemp & 0xFF;
    // buffer[6] = BMEHum >>8;
    // buffer[7] = BMEHum & 0xFF;
    // buffer[8] = BMEPressure >>16;
    // buffer[9] = BMEPressure >>8;
    // buffer[10] = BMEPressure & 0xFF;

    // buffer[11] = SGPTvoc >> 8;
    // buffer[12] = SGPTvoc & 0xFF;
    // buffer[13] = SGPEco2 >>8;
    // buffer[14] = SGPEco2 & 0xFF;

    // buffer[15] = SCDCo2 >> 8;
    // buffer[16] = SCDCo2 & 0xFF;
    // buffer[17] = SCDTemp >>8;
    // buffer[18] = SCDTemp & 0xFF;
    // buffer[19] = SCDHum >>8;
    // buffer[20] = SCDHum & 0xFF;
    // serializeSensorData(sensorsData, buffer, bufferSize);

    // Print the buffer
    Serial.println("Serialized Sensor Data:");
    // Serial.println(bufferSize);
    uint8_t data_len = 0;
    uint8_t collected_data[64] = { 0 };
    // collected_data[data_len++] = (uint8_t) 't';
    collected_data[data_len++] = LTRViir >> 8;
     collected_data[data_len++]= LTRViir & 0xFF;
     collected_data[data_len++]= LTRInf >>8;
     collected_data[data_len++]= LTRInf & 0xFF;

     collected_data[data_len++]= BMETemp >> 8;
     collected_data[data_len++]= BMETemp & 0xFF;
     collected_data[data_len++]= BMEHum >>8;
     collected_data[data_len++]= BMEHum & 0xFF;
     collected_data[data_len++]= BMEPressure >>16;
     collected_data[data_len++]= BMEPressure >>8;
     collected_data[data_len++] = BMEPressure & 0xFF;

     collected_data[data_len++] = SGPTvoc >> 8;
     collected_data[data_len++] = SGPTvoc & 0xFF;
     collected_data[data_len++] = SGPEco2 >>8;
     collected_data[data_len++] = SGPEco2 & 0xFF;

     collected_data[data_len++] = SCDCo2 >> 8;
     collected_data[data_len++] = SCDCo2 & 0xFF;
     collected_data[data_len++] = SCDTemp >>8;
     collected_data[data_len++] = SCDTemp & 0xFF;
     collected_data[data_len++] = SCDHum >>8;
     collected_data[data_len++] = SCDHum & 0xFF;
  
    Serial.println("Data Packet:");
    for (int i = 0; i < data_len; i++) {
        Serial.printf("0x%02X ", collected_data[i]);
    }
    Serial.println("");
  
    /** Send the data package */
    if (api.lorawan.send(data_len, (uint8_t *) & collected_data, 10, true, 1)) {
        Serial.println("Sending is requested");
    } else {
        Serial.println("Sending failed");
    }
    // Serial.print(buffer);
    // delete[] buffer;
    // delay(2000);
    // Serial.println("Sent LoRa");
  
}

void setup()
{
  Serial.begin(115200,RAK_AT_MODE);  
  pinMode(POWER_STATUS, INPUT);
  pinMode(EN_SENSOR, OUTPUT);
  digitalWrite(EN_SENSOR, HIGH);
  pinMode(PIR, INPUT);
  pinMode(MIC, INPUT);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
  pinMode(FAN, OUTPUT);
  digitalWrite(FAN, LOW);
  
  setup_LoRaWan();
  SCD4x_init();
  BME680_init();
  SGP30_init();
  LTR_init();
  KXTJ3_init();

  digitalWrite(LED, HIGH);
  delay(200);
  digitalWrite(LED, LOW);
  delay(200);

}


void loop()
{
  
  static uint64_t last = 0;
  static uint64_t elapsed;

  if ((elapsed = millis() - last) > INTERVAL_TIME)
  {
    SendSensorData();
    last = millis();
    // Serial.printf("The local time(UTC) is %s\r\n", api.lorawan.timereq.get().c_str());
  }
  // api.system.sleep.all(500);
}
