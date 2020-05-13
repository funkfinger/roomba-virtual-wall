/*
    MilliTimer.h - lib for setting periodic timers
*/

#ifndef MilliTimer_h
#define MilliTimer_h

#include <Arduino.h>

class MilliTimer
{
public:
  MilliTimer(unsigned long period);
  bool check();
  void reset();

private:
  unsigned long _period;
  unsigned long _currentTimer;
};

#endif
