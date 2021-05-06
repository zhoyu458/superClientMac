#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

#ifndef _COMMUNICATOR_H
#define _COMMUNICATOR_H
class Comunicator
{
public:
    WidgetBridge *_bridge;
    Comunicator(int vPinNumber, const char *token)
    {
        _bridge = new WidgetBridge(vPinNumber);
    }
};
#endif