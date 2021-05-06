#include <Arduino.h>
#include "../include/event_lib/IntervalEvent.h"
#include "../include/event_lib/TimeoutEvent.h"
#ifndef _NODEMCUASWATCHDOG_H
#define _NODEMCUASWATCHDOG_H

class NodeMcuAsWatchdog
{
public:
    int _inputPin;
    bool _status; // status should change at a certian interval, usually is 1 second
    TimeoutEvent *_monitorEvent = NULL;
    int _resetPin;

    NodeMcuAsWatchdog(int inputPinNum, int resetPinNum)
    {
        _status = false;
        _inputPin = inputPinNum;
        _resetPin = resetPinNum;
        pinMode(_inputPin, INPUT);

        if (_monitorEvent == NULL)
            _monitorEvent = new TimeoutEvent(30000, [&]() -> void { this->bite(); });
    }

    void pad()
    {
        // if sense pulse, reset or pad the watch dog
        if (_status != analogRead(_inputPin))
        {
            if (_monitorEvent)
            {
                _monitorEvent->resetEventClock();
            }

            _status = analogRead(_inputPin);
        }
    }

    void start()
    {
        if (_monitorEvent)
        {
            _monitorEvent->start(&_monitorEvent);
        }
    }

    void bite()
    {
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