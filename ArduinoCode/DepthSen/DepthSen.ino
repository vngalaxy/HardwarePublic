 #define ANALOG_PIN A3
    #define RANGE 5000 // Depth measuring range 5000mm (for water)
    #define VREF 3300 // ADC's reference voltage on your Arduino,typical value:5000mV
    #define CURRENT_INIT 4.00 // Current @ 0mm (uint: mA)
    #define DENSITY_WATER 1  // Pure water density normalized to 1
    #define DENSITY_GASOLINE 0.74  // Gasoline density
    #define PRINT_INTERVAL 1000

    int16_t dataVoltage;
    float dataCurrent, depth; //unit:mA
    unsigned long timepoint_measure;
    int i = 0;
    float count;
    void setup()
    {
      Serial.begin(9600);
      pinMode(ANALOG_PIN, INPUT);
      timepoint_measure = millis();
    }
long readVcc() {
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1125300L / result; // Back-calculate AVcc in mV
  return result;
}
    void loop()
    {
      
      if (millis() - timepoint_measure > PRINT_INTERVAL) {
        
        timepoint_measure = millis();
        dataVoltage = analogRead(ANALOG_PIN)/ 1024.0 * readVcc();
        dataCurrent = dataVoltage / 120.0; //Sense Resistor:120ohm
        depth = (dataCurrent - CURRENT_INIT) * (RANGE/ DENSITY_WATER / 16.0); //Calculate depth from current readings

        if (depth < 0) 
        {
          depth = 0.0;
        }

        // Serial print results
        Serial.print("depth:");
        Serial.print(depth);
        Serial.println("mm");
        count += depth;
        i++;
        // Serial.print(i);
        // Serial.println( analogRead(ANALOG_PIN) );
        if(i>=10){
          i=0;
          Serial.print("Value after 10s:");
          Serial.print(count/10);
          Serial.println("mm");
          count = 0;
        }
      }
    }