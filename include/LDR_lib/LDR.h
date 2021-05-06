#include <Arduino.h>
#ifndef _LDR_H
#define _LDR_H

class LDR
{
public:
    int _pinNum;

public:
    LDR(int pinNum)
    {
        _pinNum = pinNum;
        pinMode(_pinNum, INPUT);
    }

    int getValue(){
        return analogRead(_pinNum);
    }

    
 
};
#endif