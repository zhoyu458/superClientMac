#include <Arduino.h>
#include "../include/event_lib/IntervalEvent.h"
#ifndef _LED_H
#define _LED_H

class Led
{
    static const unsigned int BLYNK_INTERVAL = 1000; // 1s as default

public:
    int _pinNum;
    IntervalEvent *_blinkEvent = NULL;

public:
    Led(int pinNum)
    {
        _pinNum = pinNum;
        pinMode(_pinNum, OUTPUT);

        digitalWrite(_pinNum, !digitalRead(_pinNum));
        _blinkEvent = new IntervalEvent(BLYNK_INTERVAL, [&]() -> void { digitalWrite(_pinNum, !digitalRead(_pinNum)); });
    }

    void blink()
    {
        _blinkEvent->start();
    }
};
#endif