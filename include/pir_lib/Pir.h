#include <Arduino.h>
// #include "../include./event_lib./IntervalEvent.h"
#ifndef _PIR_H
#define _PIR_H

class Pir
{
public:
    int _pinNum;
    Pir(int pinNum)
    {
        _pinNum = pinNum;
        pinMode(_pinNum, INPUT);

        // turn off PIR sensor at the start
        digitalWrite(_pinNum, LOW);
    }

    
    boolean detect(){
        return (digitalRead(_pinNum) == 1);
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