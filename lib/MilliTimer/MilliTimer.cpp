/*
  MilliTimer.cpp - lib for setting periodic timers
*/

#include <Arduino.h>
#include <MilliTimer.h>

MilliTimer::MilliTimer(unsigned long period)
{
  _period = period;
}

bool MilliTimer::check()
{
  unsigned long currentMillis = millis();
  if (currentMillis - _currentTimer >= _period)
  {
    _currentTimer = currentMillis;
    return true;
  }
  return false;
}

void MilliTimer::reset()
{
  _currentTimer = millis();
}