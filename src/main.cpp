#include <Arduino.h>

// see: https://gist.github.com/SeeJayDee/caa9b5cc29246df44e45b8e7d1b1cdc5
#include <tiny_IRremote.h>

IRsend irsend;

void setup()
{
  irsend.enableIROut(38);
  pinMode(PIN1, OUTPUT);
}

void loop()
{
  irsend.mark(1000);
  irsend.space(1000);
}
