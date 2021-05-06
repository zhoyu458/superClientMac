#include <Arduino.h>
#include <Ticker.h>
#include <ESP8266WiFi.h>
#include "../include/event_lib/IntervalEvent.h"
#ifndef _WATCHDOG_H
#define _WATCHDOG_H

class Watchdog
{
public:
    byte _count = 0;
    Ticker *_tickBitePtr = NULL;
    IntervalEvent *_monitorEvent = NULL;

    Watchdog()
    {
        _tickBitePtr = new Ticker();
        _tickBitePtr->attach(1, [&]() -> void { this->bite(); });
        _monitorEvent = new IntervalEvent(5000, [&]() -> void { this->pad_and_check_WIFI(); });
    }

    void pad_and_check_WIFI()
    {
        // Serial.println("pad and check WIFI function called");
        _count = 0;
        if (!Blynk.connected())
        {
            ESP.reset();
        }
    }

    void start()
    {
        _monitorEvent->start();
    }

    void bite()
    {
        // Serial.println("watdog bite function called");
        _count += 1;
        if (_count >= 30)
            ESP.reset();
    }
};
#endif

/*
EXAMPLE
    - Watchdog *watchdog = NULL;
    void setup()
    {
     watchdog = new Watchdog(); // initiation must be in the setup 
    }
    void loop()
    {
     watchdog->start();  // start must be in the loop
    }
*/