#include <Arduino.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <avr/power.h>

// see: https://gist.github.com/SeeJayDee/caa9b5cc29246df44e45b8e7d1b1cdc5
#include <tiny_IRremote.h>

#include <MilliTimer.h>

extern const uint8_t gamma8[];

#define MILLIS_PER_SECOND 1000
#define MILLIS_PER_MINUTE 60000
#define MILLIS_PER_HOUR 3600000

#define STATUS_LED 0

#define MAX_HOURS_ON 1

#define BUTTON 2
#define SHORT_BUTTON_PRESS 30
#define LONG_BUTTON_PRESS 1000
#define NOT_PRESSED 1
#define INITAL_PRESS 10
#define PRESSED_SHORT 20
#define PRESSED_SHORT_HAPPENED 25
#define PRESSED_SHORT_UP 28
#define PRESSED_LONG 30
#define PRESSED_LONG_HAPPENED 35
#define PRESSED_LONG_UP 38

// system status...
#define BEDTIME 1
#define NORMAL 10

MilliTimer pulsarTimer(3);
MilliTimer secondTimer(MILLIS_PER_SECOND);
MilliTimer fiveSecondTimer(MILLIS_PER_SECOND * 5);
MilliTimer minuteTimer(MILLIS_PER_MINUTE);
MilliTimer hourTimer(MILLIS_PER_HOUR);

IRsend irsend;

uint8_t ledLevel = 0;
bool ledUp = true;
uint8_t hoursOn = 0;
uint8_t buttonState = NOT_PRESSED;
uint16_t buttonPressCounter = 0;
uint8_t systemState = NORMAL;

void resetSystem()
{
  pulsarTimer.reset();
  secondTimer.reset();
  fiveSecondTimer.reset();
  minuteTimer.reset();
  hourTimer.reset();
  ledLevel = 0;
  ledUp = true;
  hoursOn = 0;
  buttonState = NOT_PRESSED;
  buttonPressCounter = 0;
  systemState = NORMAL;
}

void flashSTATUS_LED(uint8_t times = 3, uint16_t onTime = 100, uint16_t offTime = 100)
{
  for (uint8_t x = 0; x < times; x++)
  {
    digitalWrite(STATUS_LED, HIGH);
    delay(onTime);
    digitalWrite(STATUS_LED, LOW);
    delay(offTime);
  }
}

void setupInterrupts()
{
  // set PCINT2 (PB2) as interrupt (ATTiny85 pin 7)
  bitSet(PCMSK, PCINT2);
  // set the interrupt flag to one - will clear when interrupt routine executed
  bitSet(GIFR, PCIF);
  // enable pin change interrupt
  bitSet(GIMSK, PCIE);
}

void setup()
{
  delay(500);
  setupInterrupts();
  pinMode(STATUS_LED, OUTPUT);
  pinMode(BUTTON, INPUT);
  // enablr pullup resistor
  digitalWrite(BUTTON, HIGH);
  irsend.enableIROut(38);
  flashSTATUS_LED();
  systemState = BEDTIME;
  delay(500);
}

void doPulsar()
{
  if (pulsarTimer.check())
  {
    ledUp ? ledLevel++ : ledLevel--;
    if (ledLevel > 180)
      ledUp = false;
    if (ledLevel < 1)
      ledUp = true;
  }
  analogWrite(STATUS_LED, pgm_read_byte(&gamma8[ledLevel]));
}

uint8_t checkButton()
{
  bool currentlyDown = !digitalRead(BUTTON);
  if (currentlyDown)
  {
    buttonState = INITAL_PRESS;
    if (buttonPressCounter == SHORT_BUTTON_PRESS)
    {
      buttonState = PRESSED_SHORT;
    }
    if (buttonPressCounter > SHORT_BUTTON_PRESS)
    {
      buttonState = PRESSED_SHORT_HAPPENED;
    }
    if (buttonPressCounter == LONG_BUTTON_PRESS)
    {
      buttonState = PRESSED_LONG;
    }
    if (buttonPressCounter > LONG_BUTTON_PRESS)
    {
      buttonState = PRESSED_LONG_HAPPENED;
    }
    buttonPressCounter++;
  }
  else
  {
    switch (buttonState)
    {
    case PRESSED_LONG_HAPPENED:
      buttonState = PRESSED_LONG_UP;
    case PRESSED_SHORT_HAPPENED:
      buttonState = PRESSED_SHORT_UP;
    default:
      buttonState = NOT_PRESSED;
    }
    if (buttonPressCounter == 0)
    {
      buttonState = NOT_PRESSED;
    }
    buttonPressCounter = 0;
  }
  return buttonState;
}

void goToSleep()
{
  // listen for pin change interrupts...
  bitSet(GIMSK, PCIE);
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  ADCSRA = 0;
  power_all_disable();
  sleep_enable();
  sleep_cpu();
  sleep_disable();
  power_all_enable();
  if (digitalRead(BUTTON))
  {
    goToSleep();
  }
}

void doButtonCheck()
{
  uint8_t buttonState = checkButton();
  switch (buttonState)
  {
  case PRESSED_SHORT:
    break;
  case PRESSED_LONG:
    flashSTATUS_LED(3, 10, 100);
    systemState = BEDTIME;
    break;
  }
}

void doHourCheck()
{
  if (hourTimer.check())
  {
    hoursOn++;
  }
  if (hoursOn > MAX_HOURS_ON)
  {
    flashSTATUS_LED(100, 10, 100);
    systemState = BEDTIME;
  }
}

void doNormalState()
{
  // ignore pin change interrupts...
  bitClear(GIMSK, PCIE);
  doPulsar();
  doButtonCheck();
  doHourCheck();
  irsend.mark(1000);
  irsend.space(1000);
}

void loop()
{
  switch (systemState)
  {
  case BEDTIME:
    goToSleep();
    break;
  case NORMAL:
    doNormalState();
  default:
    doNormalState();
  }
}

ISR(PCINT0_vect)
{
  resetSystem();
}

// from https://learn.adafruit.com/led-tricks-gamma-correction?view=all
const uint8_t PROGMEM gamma8[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2,
    2, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5,
    5, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10,
    10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
    17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
    25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
    37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
    51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
    69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
    90, 92, 93, 95, 96, 98, 99, 101, 102, 104, 105, 107, 109, 110, 112, 114,
    115, 117, 119, 120, 122, 124, 126, 127, 129, 131, 133, 135, 137, 138, 140, 142,
    144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 167, 169, 171, 173, 175,
    177, 180, 182, 184, 186, 189, 191, 193, 196, 198, 200, 203, 205, 208, 210, 213,
    215, 218, 220, 223, 225, 228, 231, 233, 236, 239, 241, 244, 247, 249, 252, 255};
