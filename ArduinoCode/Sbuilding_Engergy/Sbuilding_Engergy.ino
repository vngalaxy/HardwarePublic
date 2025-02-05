
// #define CFG_EU 1
#define CFG_VN 1

/*******************************************************************************/

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
// #include "LowPower.h"
//Sensors librairies
// #include <Wire.h>
#include <LTR303.h>
#include "kxtj3-1057.h"  // http://librarymanager/All#kxtj3-1057
#include "SHTC3.h"
#include <PZEM004Tv30.h>
#include <SoftwareSerial.h>
// Create an LTR303 object, here called "light":

LTR303 lightsensor;
KXTJ3 myIMU(0x0E);  // Address can be 0x0E or 0x0F
SHTC3 s(Wire);

#if !defined(PZEM_RX_PIN) && !defined(PZEM_TX_PIN)
#define PZEM_RX_PIN 7
#define PZEM_TX_PIN 5
#endif


SoftwareSerial pzemSWSerial(PZEM_RX_PIN, PZEM_TX_PIN);
PZEM004Tv30 pzem(pzemSWSerial);
// Global variables:

// LoRaWAN end-device address (DevAddr)

static const u4_t DEVADDR = 0x073c2576;

// LoRaWAN NwkSKey, network session key
// This is the default Semtech key, which is used by the early prototype TTN
// network.
static const PROGMEM u1_t NWKSKEY[16] = {0x1E, 0x09, 0x62, 0x82, 0xA3, 0x4F, 0xA9, 0xCA, 0x08, 0xFA, 0xA6, 0xEB, 0x4D, 0x40, 0xAA, 0xBB};


// LoRaWAN AppSKey, application session key
// This is the default Semtech key, which is used by the early prototype TTN
// network.
static const u1_t PROGMEM APPSKEY[16] = { 0xBB, 0xBC, 0x84, 0x49, 0x55, 0xE2, 0x3F, 0x97, 0xE7, 0x1B, 0xF3, 0x07, 0x28, 0x4E, 0xD2, 0x24 };


// These callbacks are only used in over-the-air activation, so they are
// left empty here (we cannot leave them out completely unless
// DISABLE_JOIN is set in config.h, otherwise the linker will complain).
void os_getArtEui(u1_t* buf) {}
void os_getDevEui(u1_t* buf) {}
void os_getDevKey(u1_t* buf) {}

static osjob_t sendjob;
string c;

// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
const unsigned TX_INTERVAL = 15;

// Pin mapping
const lmic_pinmap lmic_pins = {
  .nss = 10,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = 8,
  .dio = { 6, 6, 6 },
};

// ---------------------------------------------------------------------------------
// Functions
// ---------------------------------------------------------------------------------

extern volatile unsigned long timer0_overflow_count;
extern volatile unsigned long timer0_millis;
void addMillis(unsigned long extra_millis) {
  uint8_t oldSREG = SREG;
  cli();
  timer0_millis += extra_millis;
  SREG = oldSREG;
  sei();
}

// void do_sleep(unsigned int sleepyTime) {
//   unsigned int eights = sleepyTime / 8;
//   unsigned int fours = (sleepyTime % 8) / 4;
//   unsigned int twos = ((sleepyTime % 8) % 4) / 2;
//   unsigned int ones = ((sleepyTime % 8) % 4) % 2;

//   Serial.print("Sleep during ");
//   Serial.print(sleepyTime);
//   Serial.println("sec");
//   delay(50);

//   Serial.end();

//   for (int x = 0; x < eights; x++) {
//     // put the processor to sleep for 8 seconds
//     LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
//     // LMIC uses micros() to keep track of the duty cycle, so
//     // hack timer0_overflow for a rude adjustment:
//     cli();
//     timer0_overflow_count += 8 * 64 * clockCyclesPerMicrosecond();
//     sei();
//   }
//   for (int x = 0; x < fours; x++) {
//     // put the processor to sleep for 4 seconds
//     LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF);
//     // LMIC uses micros() to keep track of the duty cycle, so
//     // hack timer0_overflow for a rude adjustment:
//     cli();
//     timer0_overflow_count += 4 * 64 * clockCyclesPerMicrosecond();
//     sei();
//   }
//   for (int x = 0; x < twos; x++) {
//     // put the processor to sleep for 2 seconds
//     LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_OFF);
//     // LMIC uses micros() to keep track of the duty cycle, so
//     // hack timer0_overflow for a rude adjustment:
//     cli();
//     timer0_overflow_count += 2 * 64 * clockCyclesPerMicrosecond();
//     sei();
//   }
//   for (int x = 0; x < ones; x++) {
//     // put the processor to sleep for 1 seconds
//     LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);
//     // LMIC uses micros() to keep track of the duty cycle, so
//     // hack timer0_overflow for a rude adjustment:
//     cli();
//     timer0_overflow_count += 64 * clockCyclesPerMicrosecond();
//     sei();
//   }
//   addMillis(sleepyTime * 1000);
//   Serial.begin(115200);
// }

// ReadVcc function to read MCU Voltage
long readVcc() {
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2);             // Wait for Vref to settle
  ADCSRA |= _BV(ADSC);  // Convert
  while (bit_is_set(ADCSRA, ADSC))
    ;
  result = ADCL;
  result |= ADCH << 8;
  result = 1126400L / result;  // Back-calculate AVcc in mV
  return result;
}

// Read Light function for
double readLight() {
  double result;
  unsigned int data0, data1;
  lightsensor.getData(data0, data1);
  // Perform lux calculation:
  lightsensor.getLux(0, 1, data0, data1, result);
  return result;
}



void onEvent(ev_t ev) {
  switch (ev) {
    case EV_SCAN_TIMEOUT:
      Serial.println(F("EV_SCAN_TIMEOUT"));
      break;
    case EV_BEACON_FOUND:
      Serial.println(F("EV_BEACON_FOUND"));
      break;
    case EV_BEACON_MISSED:
      Serial.println(F("EV_BEACON_MISSED"));
      break;
    case EV_BEACON_TRACKED:
      Serial.println(F("EV_BEACON_TRACKED"));
      break;
    case EV_JOINING:
      Serial.println(F("EV_JOINING"));
      break;
    case EV_JOINED:
      Serial.println(F("EV_JOINED"));
      break;
    case EV_RFU1:
      Serial.println(F("EV_RFU1"));
      break;
    case EV_JOIN_FAILED:
      Serial.println(F("EV_JOIN_FAILED"));
      break;
    case EV_REJOIN_FAILED:
      Serial.println(F("EV_REJOIN_FAILED"));
      break;
    case EV_TXCOMPLETE:
      Serial.println(F("EV_TXCOMPLETE"));
      if (LMIC.txrxFlags & TXRX_ACK)
        Serial.println(F("Received ack"));
      if (LMIC.dataLen) {
        Serial.print(F("Received "));
        Serial.print(LMIC.dataLen);
        Serial.println(F(" bytes of payload"));
        for (int i = 0; i < LMIC.dataLen; i++) {
          if (LMIC.frame[LMIC.dataBeg + i] < 0x10) {
            Serial.print(F("0"));
          }
          Serial.print(LMIC.frame[LMIC.dataBeg + i], HEX);
        }
        Serial.println("");
      }
      // Schedule next transmission

      os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TX_INTERVAL), do_send);

      // do_sleep(TX_INTERVAL);

      break;
    case EV_LOST_TSYNC:
      Serial.println(F("EV_LOST_TSYNC"));
      break;
    case EV_RESET:
      Serial.println(F("EV_RESET"));
      break;
    case EV_RXCOMPLETE:
      // data received in ping slot
      Serial.println(F("EV_RXCOMPLETE"));
      break;
    case EV_LINK_DEAD:
      Serial.println(F("EV_LINK_DEAD"));
      break;
    case EV_LINK_ALIVE:
      Serial.println(F("EV_LINK_ALIVE"));
      break;
    default:
      Serial.println(F("Unknown event"));
      break;
  }
}

void do_send(osjob_t* j) {
  // Check if there is not a current TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND) {
    //Serial.println(F("OP_TXRXPEND"));
  }

  else {
    s.begin(true);
    int t = s.readTempC() * 10;
    int h = s.readHumidity() * 2;
    int bat = (int)(readVcc() / 10);  // multiply by 10 for V in Cayenne
    int l = readLight();              // light sensor in Lx

    /* int16_t dataHighres = 0;
            if( myIMU.readRegisterInt16( &dataHighres, KXTJ3_OUT_X_L ) == 0 ){}
            int16_t x = dataHighres/16.384;
            if( myIMU.readRegisterInt16( &dataHighres, KXTJ3_OUT_Y_L ) == 0 ){}
            int16_t y = dataHighres/16.384;
            if( myIMU.readRegisterInt16( &dataHighres, KXTJ3_OUT_Z_L ) == 0 ){}
            int16_t z = dataHighres/16.384; */

    int16_t voltage = pzem.voltage();
    int16_t current = pzem.current();
    int16_t power = pzem.power();
    int16_t energy = pzem.energy();
    int16_t frequency = pzem.frequency();
    int16_t pf = pzem.pf();

    Serial.print("Sensors values : temp = ");
    Serial.print(t / 10);
    Serial.print("deg, hum= ");
    Serial.print(h / 2);
    Serial.print("%, lum = ");
    Serial.print(l);
    Serial.println("");
    Serial.print("Voltage: ");
    Serial.print(voltage);
    Serial.println("V");
    Serial.print("Current: ");
    Serial.print(current);
    Serial.println("A");
    Serial.print("Power: ");
    Serial.print(power);
    Serial.println("W");
    Serial.print("Energy: ");
    Serial.print(energy);
    Serial.println("Wh");
    Serial.print("Frequency: ");
    Serial.print(frequency);
    Serial.println("Hz");
    Serial.print("PF: ");
    Serial.println(pf);
    Serial.println("");

    /*voltage = voltage*10000;
            current = current*10000;
            power = power*10000; */
    // energy = energy * 10000;
    // frequency = frequency * 10000;
    pf = pf * 10000;

    unsigned char mydata[31];
    mydata[0] = 0x1;   // CH1
    mydata[1] = 0x67;  // Temp
    mydata[2] = t >> 8;
    mydata[3] = t & 0xFF;
    mydata[4] = 0x2;   // CH2
    mydata[5] = 0x68;  // Humidity
    mydata[6] = h & 0xFF;
    mydata[7] = 0x3;  // CH3
    mydata[8] = 0x2;  // Analog output
    mydata[9] = bat >> 8;
    mydata[10] = bat & 0xFF;
    mydata[11] = 0x4;   // CH4
    mydata[12] = 0x65;  // Luminosity
    mydata[13] = l >> 8;
    mydata[14] = l & 0xFF;
    mydata[15] = 0x4;   // CH4
    mydata[16] = 0x66;  // Accelerometer
    mydata[17] = voltage >> 8;
    mydata[18] = voltage & 0xFF;
    mydata[19] = current >> 8;
    mydata[20] = current & 0xFF;
    mydata[21] = power >> 8;
    mydata[22] = power & 0xFF;
    mydata[23] = 0x5;   // CH5
    mydata[24] = 0x71;  // Accelerometer
    mydata[25] = energy >> 8;
    mydata[26] = energy & 0xFF;
    mydata[27] = frequency >> 8;
    mydata[28] = frequency & 0xFF;
    mydata[29] = pf >> 8;
    mydata[30] = pf & 0xFF;
    LMIC_setTxData2(1, mydata, sizeof(mydata), 0);
  }
  // Next TX is scheduled after TX_COMPLETE event.
}

void setup() {

  Serial.begin(9600);
  Serial.println("Starting");

  Wire.begin();

  s.begin(true);

  // Set-up sensors
  lightsensor.begin();
  lightsensor.setPowerUp();

  // LMIC init
  os_init();
  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();


  /* This function is intended to compensate for clock inaccuracy (up to ±10% in this example), 
    but that also works to compensate for inaccuracies due to software delays. 
    The downside of this compensation is a longer receive window, which means a higher battery drain. 
    So if this helps, you might want to try to lower the percentage (i.e. lower the 10 in the above call), 
    often 1% works well already. */

  LMIC_setClockError(MAX_CLOCK_ERROR * 2 / 100);

// Set static session parameters. Instead of dynamically establishing a session
// by joining the network, precomputed session parameters are be provided.
#ifdef PROGMEM
  // On AVR, these values are stored in flash and only copied to RAM
  // once. Copy them to a temporary buffer here, LMIC_setSession will
  // copy them into a buffer of its own again.
  uint8_t appskey[sizeof(APPSKEY)];
  uint8_t nwkskey[sizeof(NWKSKEY)];
  memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
  memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
  LMIC_setSession(0x1, DEVADDR, nwkskey, appskey);
#else
  // If not running an AVR with PROGMEM, just use the arrays directly
  LMIC_setSession(0x1, DEVADDR, NWKSKEY, APPSKEY);
#endif

#if defined(CFG_EU)
  // Set up the 8 channels used
  LMIC_setupChannel(0, 922100000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);   // g-band
  LMIC_setupChannel(1, 922300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);  // g-band
  LMIC_setupChannel(2, 922500000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);   // g-band
  LMIC_setupChannel(3, 927700000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);   // g-band
  LMIC_setupChannel(4, 923100000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);   // g-band
  LMIC_setupChannel(5, 923400000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);   // g-band
  LMIC_setupChannel(6, 922700000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);

#elif defined(CFG_VN)
  // Set up the 8 channels used
  LMIC_setupChannel(0, 921400000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);   // g-band
  LMIC_setupChannel(1, 921600000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);  // g-band
  LMIC_setupChannel(2, 921800000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);   // g-band
  LMIC_setupChannel(3, 922000000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);   // g-band
  LMIC_setupChannel(4, 922200000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);   // g-band
  LMIC_setupChannel(5, 922400000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);   // g-band
  LMIC_setupChannel(6, 922600000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);   // g-band
  LMIC_setupChannel(7, 922800000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);   // g-band
  LMIC_setupChannel(8, 922700000, DR_RANGE_MAP(DR_FSK, DR_FSK), BAND_MILLI);    // g2-band
#endif

  // Disable link check validation
  LMIC_setLinkCheckMode(0);

  // TTN uses SF9 for its RX2 window.
  LMIC.dn2Dr = DR_SF9;

  // Set data rate and transmit power for uplink (note: txpow seems to be ignored by the library)
  LMIC_setDrTxpow(DR_SF7, 14);

  // Start job
  do_send(&sendjob);
}

void loop() {
  os_runloop_once();
}