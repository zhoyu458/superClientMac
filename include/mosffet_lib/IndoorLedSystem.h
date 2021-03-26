#include <Arduino.h>
#include "../include/LDR_lib/LDR.h"
#include "../include/mosffet_lib/Mosffet.h"
#include "../include/pir_lib/Pir.h"
#include "../include/event_lib/IntervalEvent.h"
#include "../include/event_lib/TimeoutEvent.h"

#define LED_OFF_NO_DETECTIOIN_STATUS 1
#define LED_TURNING_ON_STATUS 2
#define LED_MAX_AND_WAIT_STATUS 3
#define LED_TURNING_OFF_STATUS 4

#ifndef _INDOORLEDSYSTEM_H
#define _INDOORLEDSYSTEM_H

class IndoorLedSystem
{
public:
    Mosffet *_mosffet;
    Pir *_pirUpstair;
    Pir *_pirDownstair;
    Pir *_pirFaceMasterRoom;
    LDR *_ldr;
    byte _status = LED_OFF_NO_DETECTIOIN_STATUS;
    IntervalEvent *_lightChangeEvent = NULL;
    TimeoutEvent *_waitEvent = NULL;
    // int _lightValue = 0;
    int _lightThreshold = 0;
    int _originalLightThreshold = 900;
    int _adjustlightThreshold = 700;    // stop led flicker while environemnt light hit the original threshold, higher value more sensitive
    const int _PWM_INTERVAL = 20;       // 20ms as the interval
    const int _LIGHT_ON_PERIOD = 35000; // light ON for 30 seconds once triggered 
    const byte _adjustPeriod = 2;       // set adjust period as 2 hours

    bool _alarmIsOn = false;
    bool _theft = false;

public:
    IndoorLedSystem(Mosffet *mosffet, Pir *pirUpstair, Pir *pirDownstair, Pir *pirFaceMasterRoom, LDR *ldr)
    {
        _mosffet = mosffet;
        _pirUpstair = pirUpstair;
        _pirDownstair = pirDownstair;
        _pirFaceMasterRoom = pirFaceMasterRoom;
        _ldr = ldr;

        _lightChangeEvent = new IntervalEvent(_PWM_INTERVAL, [&]() -> void { _mosffet->ApplyPwmStrength(_mosffet->_currentPwm += _mosffet->_change); });
    }

    void run(byte startHour, byte currentHour)
    {
        /******************DO NOT DELTE THE PRINT, OTHERWISE WIFI UNSTABLE CONNECTION*********/
        Serial.println(_ldr->getValue());
        // Serial.print("------");
        // Serial.println(_lightThreshold);
    //    Serial.println();

        if (currentHour <= startHour + _adjustPeriod)
        {
            _lightThreshold = _adjustlightThreshold;
        }
        else
        {
            _lightThreshold = _originalLightThreshold;
        }
        // Serial.println("222");
        switch (_status)
        {
        case LED_OFF_NO_DETECTIOIN_STATUS:
/*******************************************************************/
            if(_ldr->getValue() < _lightThreshold && _mosffet->_currentPwm > _MINI_PWM_STRENGTH)
            {
                _mosffet->_currentPwm = _MINI_PWM_STRENGTH;
                _mosffet->ApplyPwmStrength(_mosffet->_currentPwm);
                break;
            }
/*******************************************************************/

            _mosffet->_change = 0;
            if ((_pirUpstair->detect() || _pirDownstair->detect() || _pirFaceMasterRoom->detect()) && (_ldr->getValue() > _lightThreshold))
            {
                // Serial.println("333");

                _status = LED_TURNING_ON_STATUS;
            }
          
            break;

        case LED_TURNING_ON_STATUS:
            // Serial.println("444");
/*******************************************************************/
            if(_ldr->getValue() < _lightThreshold)
            {
                _status = LED_OFF_NO_DETECTIOIN_STATUS;
                break;
            }
/*******************************************************************/

            _mosffet->_change = 1;
            if (_mosffet->_currentPwm >= _mosffet->_maxPwn)
            {
                _status = LED_MAX_AND_WAIT_STATUS;
            }
            break;

        case LED_MAX_AND_WAIT_STATUS:
            // Serial.println("555");
/*******************************************************************/
            if(_ldr->getValue() < _lightThreshold)
            {
                _status = LED_OFF_NO_DETECTIOIN_STATUS;
                break;
            }
/*******************************************************************/

            _mosffet->_change = 0;

            if(_waitEvent == NULL)
            _waitEvent = new TimeoutEvent(_LIGHT_ON_PERIOD, [&]() -> void { _status = LED_TURNING_OFF_STATUS; });

            // during waiting, if detect people, _waitToTurnOffEvent clock reset
            if ((_pirUpstair->detect() || _pirDownstair->detect() || _pirFaceMasterRoom->detect()) && (_ldr->getValue() > _lightThreshold))
            {
                if(_waitEvent)
                _waitEvent->resetEventClock();
            }
            break;

        case LED_TURNING_OFF_STATUS:
            // Serial.println("666");

            _mosffet->_change = -1;
            if (_mosffet->_currentPwm <= _MINI_PWM_STRENGTH)
            {
                _status = LED_OFF_NO_DETECTIOIN_STATUS;
            }

            // while turnning off, if detect an objec, then led starts to turning on again.
            if ((_pirUpstair->detect() || _pirDownstair->detect() || _pirFaceMasterRoom->detect()) && (_ldr->getValue() > _lightThreshold))
            {
                _status = LED_TURNING_ON_STATUS;
            }
            break;

        default:
            break;
        }

        // Serial.println("777");

        if (_lightChangeEvent)
            _lightChangeEvent->start();

        // Serial.println("888");
        if (_waitEvent)
            _waitEvent->start(&_waitEvent);

        //  Serial.print(_pirUpstair->detect());
        //  Serial.print(_pirDownstair->detect());
        //  Serial.print(_pirFaceMasterRoom->detect());
        //  Serial.print(_ldr->getValue());
        //  Serial.println();

    }

    // bool bugularSystemRun()
    // {
    //     return _pirUpstair->detect() || _pirDownstair->detect() || _pirFaceMasterRoom->detect();
    // }
};
#endif