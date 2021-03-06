#include "pins.h"
#include "keyboard.h"
#include "ISRProfiler.h"

#include "arduino_midi_library-5.0.2/src/MIDI.h"

#define MIDI_CH 1
#define MIDI_VEL 127

//// Fn declarations
void setupISR();
void startupLedBlink();

//// Global scope
MIDI_CREATE_DEFAULT_INSTANCE();
Keyboard keyboard;

const float isr_millis_total = 0.3;
ISRProfiler isrProfiler(isr_millis_total);

////
void setup() {
  KeyboardHardware::setupPins();
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(115200);
  setupISR();

  Serial.println("Startup.... ");
  startupLedBlink();
}

unsigned int isrCount = 0;
ISR(TIMER1_COMPA_vect) {
  isrProfiler.onISREnter();

  keyboard.scanBank(isrCount % NUM_BANKS);
  isrCount++;
  if(isrCount > 30000) {
    isrCount = 0;
  }

  isrProfiler.onISRExit();
}

void loop() {
  while(keyboard.hasEvent()) {
    const KeyboardEvent event = keyboard.popEvent();
    
    Serial.print("Note ");
    Serial.print(event.key);
    if (event.type == KEY_PRESS) {
      // Serial.println("   ON");
      MIDI.sendNoteOn(keyToMidiNote(event.key), MIDI_VEL, MIDI_CH);
    } else if(event.type == KEY_RELEASE) {
      // Serial.println("   OFF");
      MIDI.sendNoteOff(keyToMidiNote(event.key), MIDI_VEL, MIDI_CH);
    }

    delayMicroseconds(50);
  }

  delayMicroseconds(200);
  // isrProfiler.printReport();
}

void setupISR() {
  cli(); //stop interrupts

  //set timer1 interrupt at 1Hz
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0; //initialize counter value to 0
  // set compare match register
  // Formula is:
  // OCR1A = (T_int [ms] * 1/1000 * (16*10^6)/(1024)) - 1      [must be <65536]
  OCR1A = (isr_millis_total * 15.625) - 1;
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS10 and CS12 bits for 1024 prescaler
  TCCR1B |= (1 << CS12) | (1 << CS10);  
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);

  sei(); //allow interrupts
}

void startupLedBlink() {
  const int t_on = 100;
  const int t_off = 100;

  for(int i = 0; i < 3; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(t_on);
    digitalWrite(LED_BUILTIN, LOW);
    delay(t_off);
  }
}
