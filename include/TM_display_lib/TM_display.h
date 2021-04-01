#include <Arduino.h>
#include <TM1637Display.h>

#ifndef _TM_DISPLAY_H
#define _TM_DISPLAY_H

class TM_display
{
    TM1637Display *display = NULL;

    TM_display(int CLK, int DIO)
    {
        display = new TM1637Display(CLK, DIO);

        // initiate the dispaly and present 9999 on the screen
        display->setBrightness(0xf);           // setup brightness, can be replace by decimal
        uint8_t data[] = {0x0, 0x0, 0x0, 0x0}; // setup brightness, can be replace by decimal
        display->setSegments(data);
        display->showNumberDec(9999, false, 4, 0);
    }

    void showDeciNumber(double number)
    {
        display->setBrightness(0xf);           // setup brightness, can be replace by decimal
        uint8_t data[] = {0x0, 0x0, 0x0, 0x0}; // setup brightness, can be replace by decimal
        display->setSegments(data);
        display->showNumberDec(number, false, 4, 0);
    }
};
#endif