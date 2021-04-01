#include <Arduino.h>
#include <Ticker.h>
#include <ESP8266WiFi.h>
#include "../include/event_lib/IntervalEvent.h"
#ifndef _WATCHDOGWITHOUTBLYNK_H
#define _WATCHDOGWITHOUTBLYNK_H

class WatchdogWithoutBlynk
{
public:
    byte _count = 0;
    Ticker *_tickBitePtr = NULL;
    IntervalEvent *_monitorEvent = NULL;

    WatchdogWithoutBlynk()
    {
        _tickBitePtr = new Ticker();
        _tickBitePtr->attach(1, [&]() -> void { this->bite(); });
        _monitorEvent = new IntervalEvent(5000, [&]() -> void { this->pad(); });
    }

    void pad()
    {
        // Serial.println("pad and check WIFI function called");
        _count = 0;

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
    - WatchdogWithoutBlynk *WatchdogWithoutBlynk = NULL;
    void setup()
    {
     WatchdogWithoutBlynk = new WatchdogWithoutBlynk(); // initiation must be in the setup 
    }
    void loop()
    {
     WatchdogWithoutBlynk->start();  // start must be in the loop
    }
*/