#include <Arduino.h>
#include "../include/event_lib/TimeoutEvent.h"
#ifndef _BUTTON_H
#define _BUTTON_H

class Button
{
    static const unsigned long ONTIME = 1000;

public:
    int _pinNum;
    const char *_msg; // receive msg from blynk app
    TimeoutEvent *_pressEvent = NULL;
    TimeoutEvent *_releaseEvent = NULL;

public:
    Button(int pinNum)
    {
        _pinNum = pinNum;
        pinMode(_pinNum, OUTPUT);
        _msg = "";
    }

    void press()
    {
        digitalWrite(_pinNum, HIGH);
        // Serial.println("HIGH");
    }

    void release()
    {
        digitalWrite(_pinNum, LOW);
        // Serial.println("LOW\n\n");
    }

    void virtualWriteConfig(const char *newMsg, String pMsg[], unsigned int arraySize)
    {

        for (unsigned int i = 0; i < arraySize; i++)
        {
            // Serial.println(pMsg[i]);
            if (strcmp(newMsg, pMsg[i].c_str()) == 0)
            {
                this->_pressEvent = new TimeoutEvent(0, [&]() -> void { this->press(); });
                this->_releaseEvent = new TimeoutEvent(ONTIME, [&]() -> void { this->release(); });
                return;
            }
        }
    }

    void run()
    {
        if (this != NULL && this->_pressEvent != NULL)
        {

            this->_pressEvent->start(&this->_pressEvent);
        }

        if (this != NULL && this->_releaseEvent != NULL)
        {

            this->_releaseEvent->start(&this->_releaseEvent);
        }
    }
};
#endif