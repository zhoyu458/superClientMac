#include <Arduino.h>
#ifndef _INTERVALEVENT_H
#define _INTERVALEVENT_H

class IntervalEvent
{
    // typedef void (*func)();
    typedef std::function<void(void)> func;

public:
    unsigned long long _initTime = 0; // _interval unit is in miliseconds, the default value is
    unsigned long long _interval = 0; // _interval unit is in miliseconds, the default value is
    boolean _holdEvent = false;
    func _callback;

public:
    // parameter explain
    // interval: take an integer as milliseconds, how often the callback function getted called
    IntervalEvent(unsigned long long interval, func callback)
    {
        _interval = interval;
        _callback = callback;
    }
    void start()
    {
        if (millis() - _initTime >= _interval && _holdEvent == false)
        {
            _callback();
            _initTime = millis();
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

    void kill(IntervalEvent *(*ptr))
    {

        *ptr = NULL;
        this->_callback = NULL;
        delete this;
    }
};
#endif

// EXAMPLE
/*
 //(1) HOW TO START AN EVENT
    TimeoutEvent *printEvent = new TimeoutEvent(5000, print); // declaration
    if (blinkEvent)   blinkEvent is an instance of the class
        blinkEvent->start();

   (2) HOW TO KILL AN EVENT
   // !!!!!!!!!!!!!!---blinkEvent != NULL is important---!!!!!!!!!!!!!!!!!!!!, without this condition, crash occurs
    if (condition 8 && blinkEvent != NULL)  
    {
        blinkEvent = NULL;
        delete blinkEvent;
    }
*/