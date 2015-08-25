void readADCs();
void activateRelays(unsigned char);
void configureDigitalPins(unsigned char);
void setPins_seeedV2(unsigned char);
#define F_RELAY_SEEED_V2    0x00000001
#define F_RELAY_CUSTOM      0x00000002
#define F_RELAY_2CUSTOM     0x00000004
#define F_ADC_MASK_DUE_ALL  0x0000000C
#define F_ADC_RES_8         0x00010000
#define F_ADC_RES_10        0x00020000
#define F_ADC_RES_12        0x00040000
#define F_ADC_RES_DUE       F_ADC_RES_8 | F_ADC_RES_10 | F_ADC_RES_12
#define PIN_MODE_IDSS       23
#define PIN_MODE_VPP        23
#define S_MEASURE_IDSS      0x00000001
#define S_MEASURE_VPP       0x00000002

int flags = F_RELAY_SEEED_V2;
int adc_flags = F_ADC_MASK_DUE_ALL;
int iADCReadings[12];
volatile int state = S_MEASURE_IDSS;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  delay(2000);
  Serial.println("Hello!");
  delay(2000);
  Serial.println("Hello Again!");
  analogReadResolution(16);
  configureDigitalPins(OUTPUT);
//  pinMode(PIN_MODE_IDSS, INPUT_PULLUP);
  pinMode(PIN_MODE_VPP, INPUT_PULLUP);
//  attachInterrupt(PIN_MODE_IDSS, hint_IDSS, FALLING);
  attachInterrupt(PIN_MODE_VPP, hint_VPP, FALLING);
  
  Serial.println("Exitting setup()");
  return;
}

void hint_IDSS() {
  noInterrupts();
  Serial.print("hint_IDSS(): ");
  state &= ~S_MEASURE_VPP;
  state |= S_MEASURE_IDSS;  
  Serial.println(state);
  interrupts();
  return;
}

void hint_VPP() {
  static int lastChange = 0;
  if (millis() - lastChange < 64) {
    return;
  }
  lastChange = millis();

  if (state & S_MEASURE_IDSS) {
    state &= ~S_MEASURE_IDSS;
    state |= S_MEASURE_VPP;
  }
  else {
    state &= ~S_MEASURE_VPP;
    state |= S_MEASURE_IDSS;  
  }
  Serial.println(state);

  return;
}

void loop() {
  char inByte = 0x0;
  if (Serial.available() > 0) {
    inByte = Serial.read();
  }
  if (inByte == 'R') {
    hint_VPP();
  }
  // put your main code here, to run repeatedly:
  static int localState = 0;
  if (localState == state) {
    return;
    delay(500);
  }
  unsigned char i;
  if (state & S_MEASURE_VPP) {
    activateRelays(1);
  }
  if (state & S_MEASURE_IDSS) {
    activateRelays(0);
  }
  Serial.println("loop()!");
  readADCs();
  for (i = 0; i < 12; i++) {
    if (state & S_MEASURE_VPP) {
      Serial.print("Vpp for port ");
      Serial.print(i);
      Serial.print(" value: ");
      Serial.print((float) ((float) iADCReadings[i] * 3.30 / 65536.0),4);
      Serial.println("V");

    }
    if (state & S_MEASURE_IDSS) {
      Serial.print("Idss for port ");
      Serial.print(i);
      Serial.print(" value: ");
      Serial.print((float) ((float) iADCReadings[i] * 33.0 / 65536.0),4);
      Serial.println("ma");

    }
  }
  delay(2000);
}


void activateRelays(unsigned char val) {
  if (flags & F_RELAY_SEEED_V2) {
    activateRelays_seeedV2(val);
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

void configureDigitalPins(unsigned char pt) {
  if (flags & F_RELAY_SEEED_V2) {
    setPins_seeedV2(pt);
  }
}

void setPins_seeedV2(unsigned char pt) {
  unsigned char i;
  for (i = 4; i < 8; i++) {
    pinMode(i, pt);
  }
  return;
}

void readADCs() {
  char i,y;
  unsigned long Readings[12][16];
  unsigned long tempReading = 0;
  for (y = 0; y < 16; y++) {
    for (i = 0; i < 12; i++) {
      Readings[i][y] = analogRead(i);
      delay(50);
    }
  }
  for (i = 0; i < 12; i++) {
    for (y = 0; y < 16; y++) {
      tempReading += Readings[i][y];
    }
    iADCReadings[i] = (int) tempReading / 16;
    tempReading = 0;
  }
}

