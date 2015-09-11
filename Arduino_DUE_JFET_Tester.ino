/*
 * F_RELAY_SEEED_V2 is for a directly attached SEEED 4 way relay board
 * F_RELAY_CUSTOM is for some SainSmart 8-way boards
 *
 * This was a quick hack/proof of concept, going to majorly revamp it and
 * add support for many other things (ADCs, digital pot control to control
 * op-amp gain (w/ attentuator) for low or high Idss/Vpp devices -- will
 * only work on my custom board or a big breadboard with lots o' wires
 * at that point though!
 *
 * Changes needed as I can't currently measure the Idss of the 2N5638,
 * least of all with 3.3v devices, and a resistor attentuator would need
 * to be very high impedance in order to not interefere directly with the
 * measurement.. I may also add temperature sensors near the JFETs for
 * devices like the 2N5638 and 2N4391 as even with 150mW the TO-92 package
 * has poor heat dissipation.
 *
 * I should hopefully have a bunch of ADC samples coming in from multiple
 * vendors to write code for :)
 *
 * Frank DiMitri <frankd@frank-d.info>
 */
void readArduinoADCs();
void activateRelays(unsigned char);
void configureDigitalPins(unsigned char);
void setPins_seeedV2(unsigned char);
void hintChangeModes();
void activateRelays_seeedV2(unsigned char val);
void activateRelays_custom(unsigned char val);
void configureDigitalPins(unsigned char pt);
void setPins_custom(unsigned char pt);

// Configuration
#define ARDUINO_ATMEGA2560

#define F_RELAY_SEEED_V2    0x00000001
#define F_RELAY_CUSTOM      0x00000002
#define F_RELAY_2CUSTOM     0x00000004
#define F_ADC_MASK_DUE_ALL  0x0000000C
#define F_ADC_RES_8         0x00010000
#define F_ADC_RES_10        0x00020000
#define F_ADC_RES_12        0x00040000
#define PIN_MODE_VPP        23          // The switch should be debounced, I took out the interrupt code though
#define S_MEASURE_IDSS      0x00000001
#define S_MEASURE_VPP       0x00000002
#define ADC_RESOLUTION      0x0A
#define SCALE_FACTOR        (float) (1 << ADC_RESOLUTION)
#ifdef ARDUINO_ATMEGA2560
  #define F_ADC_RES           F_ADC_RES_8 | F_ADC_RES_10
  #define NUM_ADCS            0x10  // I dream in hexadecimal
#elif ARDUINO_DUE
  #define F_ADC_RES           F_ADC_RES_8 | F_ADC_RES_10 | F_ADC_RES_12
  #define NUM_ADCS            0x0C
#elif ARDUINO_UNO
  #define NUM_ADCS            0x06
#elif ARDUINO_PRO_MINI5
  #define NUM_ADCS            0x08  // This varies, my knock-offs do have 8
#elif ARDUINO_PRO_MINI33
  #define NUM_ADCS            0x08  // This varies
#elif AVR_ATMEGA2561
  #define NUM_ADCS            0x08
#else
  #define NUM_ADCS            0x01  // Gotta have atleast 1, right?
#endif
#define NUM_SAMPLES           0x1F  // Sample 31 times!
#define CUSTOM_RELAY_IO_START 38  // Start IO pin for relay toggles
#define CUSTOM_RELAY_IO_END   54  // End IO pin for relay toggles

int flags = F_RELAY_CUSTOM;
int iADCReadings[NUM_ADCS];
volatile int state = S_MEASURE_IDSS;

/*
 *  int adc_flags = F_ADC_RES;
 *
 *  Currently unused
 */


void setup() {
#ifdef ARDUINO_DUE
  analogReadResolution(12);
#endif
  Serial.begin(9600);
  delay(2000);

  configureDigitalPins(OUTPUT);
  pinMode(PIN_MODE_VPP, INPUT_PULLUP);
  Serial.println("Exitting setup()");
  return;
}

void hintChangeModes() {
  static int lastChange = 0;
  if (millis() - lastChange < 256) {
    Serial.println("Asked to change modes within 1/4 of a second, denied");
    return;
  }
  Serial.print("Changing modes to ");
  lastChange = millis();

  if (state & S_MEASURE_IDSS) {
    Serial.println("VPP");
    state &= ~S_MEASURE_IDSS;
    state |= S_MEASURE_VPP;
  }
  else {
    Serial.println("IDSS");
    state &= ~S_MEASURE_VPP;
    state |= S_MEASURE_IDSS;
  }
  return;
}

void loop() {
  char inByte = 0x0;
  unsigned char i;
  if (Serial.available() > 0) {
    inByte = Serial.read();
  }
  if (inByte == 'R') {
    hintChangeModes();
  }
  // put your main code here, to run repeatedly:
  if (state & S_MEASURE_VPP) {
    activateRelays(1);
  }
  if (state & S_MEASURE_IDSS) {
    activateRelays(0);
  }
  Serial.println("loop()!");
  readArduinoADCs();
  for (i = 0; i < NUM_ADCS; i++) {
    if (state & S_MEASURE_VPP) {
      Serial.print("Vpp for port ");
      Serial.print(i);
      Serial.print(" value: ");
      Serial.print((float) ((float) iADCReadings[i] * 5 / SCALE_FACTOR),4);
      Serial.println("V");

    }
    if (state & S_MEASURE_IDSS) {
      Serial.print("Idss for port ");
      Serial.print(i);
      Serial.print(" value: ");
      Serial.print((float) ((float) iADCReadings[i] * 50 / SCALE_FACTOR),4);
      Serial.println("ma");

    }
  }
  delay(2000);
}


void activateRelays(unsigned char val) {
  if (flags & F_RELAY_SEEED_V2) {
    activateRelays_seeedV2(val);
  }
  if (flags & F_RELAY_CUSTOM) {
    activateRelays_custom(val);
  }
  return;
}

void activateRelays_seeedV2(unsigned char val) {
  unsigned char i;

  Serial.println("Entering activeRelays_seedV2()");
  for (i = 4; i < 8; i++) {
    if (val) {
      digitalWrite(i, HIGH);
    }
    else {
      digitalWrite(i, LOW);
    }
  }
  return;
}

void activateRelays_custom(unsigned char val) {
  unsigned char i;

  Serial.println("Entering activeRelays_custom()");
  for (i = CUSTOM_RELAY_IO_START; i < CUSTOM_RELAY_IO_END; i++) {
    if (val) {
      digitalWrite(i, HIGH);
    }
    else {
      digitalWrite(i, LOW);
    }
  }
  return;
}


void configureDigitalPins(unsigned char pt) {
  if (flags & F_RELAY_SEEED_V2) {
    setPins_seeedV2(pt);
  }
  if (flags & F_RELAY_CUSTOM) {
    setPins_custom(pt);
  }
}

void setPins_seeedV2(unsigned char pt) {
  unsigned char i;
  for (i = 4; i < 8; i++) {
    pinMode(i, pt);
  }
  return;
}

void setPins_custom(unsigned char pt) {
  unsigned char i;
  for (i = CUSTOM_RELAY_IO_START; i < CUSTOM_RELAY_IO_END; i++) {
    pinMode(i, pt);
  }
  return;
}

void readArduinoADCs() {
  char i,y;
  uint32_t Readings[NUM_ADCS][NUM_SAMPLES];
  uint32_t tempReading = 0;
  /* I wasn't sure if unsigned long was the same between AVR and Cortex-M3,
   *  so I explicitly set uint32_t .. this needs to be uint_64t
   */
  for (y = 0; y < NUM_SAMPLES; y++) {
    for (i = 0; i < NUM_ADCS; i++) {
      Readings[i][y] = analogRead(i);
      delay(50);
    }
  }
  for (i = 0; i < NUM_ADCS; i++) {
    for (y = 0; y < NUM_SAMPLES; y++) {
      tempReading += Readings[i][y];
    }
    iADCReadings[i] = (int) tempReading / NUM_SAMPLES;
    tempReading = 0;
  }
}
