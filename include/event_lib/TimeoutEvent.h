#include <Arduino.h>
#ifndef _TIMEOUTEVENT_H
#define _TIMEOUTEVENT_H

class TimeoutEvent
{
    // typedef void (*func)();
    typedef std::function<void(void)> func;

public:
    unsigned long _initTime = 0; // _interval unit is in miliseconds, the default value is
    unsigned long _waitTime = 0; // _interval unit is in miliseconds, the default value is
    bool _holdEvent = false;
    func _callback;

public:
    // parameter explain
    // interval: take an integer as milliseconds, how often the callback function getted called
    TimeoutEvent(unsigned long long waitTime, func callback)
    {
        _waitTime = waitTime;
        _callback = callback;
        _initTime = millis();
    }

    void start(TimeoutEvent *(*ptr))
    {
        // Serial.println(millis() - _initTime);

        if (_holdEvent == false && (millis() - _initTime >= _waitTime))
        {
            // Serial.println("call back");
            _callback();
            this->kill(ptr);
        }
    }

    void holdTimeoutEvent()
    {
        _holdEvent = true;
    }

    void resumeTimeoutEvent()
    {
        _holdEvent = false;
    }

    void resetEventClock()
    {
        _initTime = millis();
    }

    void kill(TimeoutEvent *(*ptr))
    {
        *ptr = NULL;
        this->_callback = NULL;
        delete this;
    }

    void updateTimeoutDuration(unsigned long newWaitTime)
    {
        _waitTime = newWaitTime;
        resetEventClock();
    }
};
#endif

/*
EXAMPLE
    timeoutEvent instance will be released automatically, after callback run.
    TimeoutEvent *printEvent = new TimeoutEvent(5000, print);  // declaration.
    if (printEvent != NULL)   // !!!!!!!!!!!!!!---printEvent != NULL is important---!!!!!!!!!!!!!!!!!!!!, without this condition, crash occurs
    {
        printEvent->start(&printEvent);
    }
*/