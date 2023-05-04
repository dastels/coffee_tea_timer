// -*- mode: C++;-*-

#include <Bounce2.h>
#include <Adafruit_CircuitPlayground.h>

#define LEFT_BUTTON_PIN (4)
#define RIGHT_BUTTON_PIN (19)
#define NEOPIXEL_PIN (17)
#define SPEAKER_ENABLE_PIN (11)
#define LED_PIN (13)
#define DONE_PIN (1)

#define IDLE_STATE (0)
#define WAIT_STATE (1)
#define ALARM_STATE (2)
#define CLEANUP_STATE (3)

int state = IDLE_STATE;

#define ALARM_TOGGLE_INTERVAL (500)

#define ONE_MINUTE (60000)        // 60 * 1000
#define COFFEE_INTERVAL (240000)  // 4 minutes
#define TEA_INTERVAL (180000)     // 3 minutes
#define BEEP_DURATION (5000)

unsigned long timer_duration = 0;
unsigned long alarm_advance_interval = 0;
unsigned long alarm_time = 0;
unsigned long advance_time = 0;
unsigned long toggle_time = 0;
bool toggle_state = false;
int light_count = 0;
int beep_count = 0;
unsigned long beep_time = 0;
unsigned long timeout = 0;

Bounce left_debouncer = Bounce();
Bounce right_debouncer = Bounce();

bool slide_switch;


void setup() {
  CircuitPlayground.begin();
  CircuitPlayground.speaker.enable(false);
  CircuitPlayground.clearPixels();

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, false);

  pinMode(LEFT_BUTTON_PIN, INPUT_PULLUP);
  left_debouncer.attach(LEFT_BUTTON_PIN);
  left_debouncer.interval(5);  // interval in ms

  pinMode(RIGHT_BUTTON_PIN, INPUT_PULLUP);
  right_debouncer.attach(RIGHT_BUTTON_PIN);
  right_debouncer.interval(5);  // interval in ms

  pinMode(DONE_PIN, OUTPUT);
  digitalWrite(DONE_PIN, LOW);

  slide_switch = CircuitPlayground.slideSwitch();
  timer_duration = 0;

  timeout = millis() + 30000;
}


void alarm_in(unsigned long interval) {
  alarm_time = millis() + interval;
  alarm_advance_interval = interval / 11;
  advance_time = 0;
  toggle_time = 0;
  CircuitPlayground.clearPixels();
  state = WAIT_STATE;
}


void loop() {
  unsigned long now = millis();

  left_debouncer.update();
  right_debouncer.update();

  switch (state) {
    case IDLE_STATE:
      if (now > timeout) {
        state = CLEANUP_STATE;
        return;
      } else if (slide_switch) {
        if (left_debouncer.rose()) {
          if (timer_duration == 10) {
            timer_duration = 0;
            CircuitPlayground.clearPixels();
          }
          CircuitPlayground.setPixelColor(timer_duration, 0x0000FF);
          timer_duration++;
        } else if (right_debouncer.rose()) {
          alarm_in(timer_duration * ONE_MINUTE);
        }
      } else {
        if (left_debouncer.rose()) {
          alarm_in(COFFEE_INTERVAL);
          state = WAIT_STATE;
        } else if (right_debouncer.rose()) {
          alarm_in(TEA_INTERVAL);
          state = WAIT_STATE;
        }
      }
      return;
    case WAIT_STATE:
      if (now >= alarm_time) {
        state = ALARM_STATE;
        return;
      }
      if (advance_time == 0) {  // first time through
        digitalWrite(LED_PIN, true);
        CircuitPlayground.speaker.enable(true);
        advance_time = now + alarm_advance_interval;
        CircuitPlayground.strip.fill(0x000000);
      } else {
        if (now >= beep_time) {
          beep_time = now + BEEP_DURATION;
          CircuitPlayground.playTone(4000, 20);
        }
        if (now >= advance_time) {
          advance_time = now + alarm_advance_interval;
          CircuitPlayground.setPixelColor(light_count++, 0x00FF00);
        }
      }
      return;
    case ALARM_STATE:
      if (now >= toggle_time) {
        if (beep_count++ == 10) {
          CircuitPlayground.strip.fill(0x0000FF);
          state = CLEANUP_STATE;
          return;
        }
        toggle_time = now + ALARM_TOGGLE_INTERVAL;
        toggle_state = !toggle_state;
        if (toggle_state) {
          // BEEP
          CircuitPlayground.strip.fill(0xFF0000);
          CircuitPlayground.strip.show();
          CircuitPlayground.playTone(3000, 50);
          delay(50);
          CircuitPlayground.playTone(3000, 50);
        } else {
          // SILENCE
          CircuitPlayground.strip.fill(0x000000);
          CircuitPlayground.strip.show();
        }
      }
      return;
    case CLEANUP_STATE:
      while (1) {
        digitalWrite(DONE_PIN, HIGH);
        delay(5);
        digitalWrite(DONE_PIN, LOW);
        delay(5);
      }
  }
}
