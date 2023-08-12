#include <avr/sleep.h>
#include <avr/wdt.h>

#define LED_G PB1
#define LED_R PB0
#define MOTOR PB3

volatile uint8_t ctr = 0;
volatile boolean bootup = true;

void setup() {
  // put your setup code here, to run once:
  enable();
  digitalWrite(LED_G,HIGH);
  delay(10);
  digitalWrite(LED_G,LOW);
  delay(10);
  digitalWrite(LED_G,HIGH);
  delay(10);
  digitalWrite(LED_G,LOW);
  disable();
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  setup_watchdog(9);//Aufwachen in 8 Sekunden
  disable();
  sleep_mode();//Einschlafen
}

void enable() {
  if (readVcc()<2200){
    pinMode(LED_R, OUTPUT);
  }
  pinMode(LED_G, OUTPUT);
  pinMode(MOTOR, OUTPUT);
  ADCSRA |= (1 << ADEN); //ADC EIN
}

void disable() {
  digitalWrite(LED_G, LOW);
  digitalWrite(LED_R, LOW);
  digitalWrite(MOTOR, LOW);
  pinMode(LED_G, INPUT);
  pinMode(MOTOR, INPUT);
  pinMode(LED_R, INPUT);
  ADCSRA &= ~(1 << ADEN); //ADC AUS
}

//Watchdog einstellen
void setup_watchdog(uint8_t pre) {
  if (pre > 9) pre = 9;
  byte bb = pre & 7;
  if (pre > 7) bb |= (1 << 5);

  //setzen
  MCUSR &= ~(1 << WDRF); //Watchdog zurücksetzen
  WDTCR |= (1 << WDCE) | (1 << WDE); //Watchdog ein
  WDTCR = bb;//Zeit setzen
  WDTCR |= _BV(WDIE);//Watchdog Interrupt ein
}

//Aus Internet kopiert
long readVcc() {
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
  ADMUX = _BV(MUX3) | _BV(MUX2);

  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA, ADSC)); // measuring

  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH; // unlocks both

  long result = (high << 8) | low;

  result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return result; // Vcc in millivolts
}

void loop() {
  setup_watchdog(9);//Aufwachen in 8 Sekunden
  enable();

  long vt=readVcc();
  if(vt > 2600){
    bootup=false;
  }else if(vt < 2100){
    bootup=true;
  }
  
  //Regelmäßig anschupsen
  if ((ctr%3==0) && (readVcc() > 2300) && (bootup==false)) { 
    uint8_t dreh_max=20-floor((readVcc()/1000)*3);
    digitalWrite(MOTOR, HIGH);
    for (uint8_t i = 0; (i < dreh_max) && (readVcc() > 2000); i++) {
      delay(5);
    }
    digitalWrite(MOTOR, LOW);
  }

  //Überspannungsschutz
  while(readVcc()>5000){
    wdt_reset();
    digitalWrite(MOTOR, HIGH);
    digitalWrite(LED_G, HIGH);
    delay(100);
  }
  digitalWrite(MOTOR, LOW);
  digitalWrite(LED_G, LOW);

  //Ausgabe der Spannung
  uint16_t v = readVcc();
  if ((ctr % 3) == 0 && v>=2200) {
    uint16_t v = readVcc();
    while (v > 1000) {
      digitalWrite(LED_G, HIGH);
      v = v - 1000;
      delay(300);
      digitalWrite(LED_G, LOW);
      delay(300);
    }
    while (v > 100) {
      digitalWrite(LED_G, HIGH);
      v = v - 100;
      delay(10);
      digitalWrite(LED_G, LOW);
      delay(400);
    }
  }

  //Roter Blitz bei Unterspannung
  if (readVcc() <=2200){
    digitalWrite(LED_R, HIGH);
    delay(10);
    digitalWrite(LED_R, LOW);
  }

  /*
  //Kondensator voll
  if (readVcc() > 5000) {
    digitalWrite(MOTOR, HIGH);
    while (readVcc() > 4800) {
      wdt_reset();//Zeit erkaufen--> neue 8 Sekunden
      delay(100);
    }
  }*/

  disable();
  sleep_mode();//Einschlafen
}

ISR(WDT_vect) {
  //Wird vom Watchdog aufgerufen, um zu starten
  ctr++;
}
