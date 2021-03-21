#include <Arduino.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#ifndef _DATETIME_H
#define _DATETIME_H

class Datetime
{
    static const long utcOffsetInSeconds = 46800;

public:
    unsigned int _year;
    byte _month;
    byte _day;
    byte _hour;
    byte _minute;
    WiFiUDP *_ntpUDP;
    NTPClient *_timeClient;

public:
    Datetime(const char *ssid, const char *password)
    {
        _ntpUDP = new WiFiUDP();
        _timeClient = new NTPClient(*_ntpUDP, "nz.pool.ntp.org");
        WiFi.begin(ssid, password);
        (*_timeClient).begin();
        (*_timeClient).setTimeOffset(utcOffsetInSeconds);
    }

    void updateHours()
    {
        _timeClient->update();
        // Serial.println(_timeClient->getHours());
        _hour = _timeClient->getHours();
    } 

    byte getHours(){
        return _hour;
    }
};
#endif

/*
EXAMPLE: 
  (1) Datetime *datetime = new Datetime();  // declaration
  (2) use "datetime" instance to get the info,  wrap the instance in an intervalEvent for continuous update.

*/