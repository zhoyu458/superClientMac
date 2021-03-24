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
    int _lightValue = 0;
    int _lightThreshold = 90;
    int _lightThresholdAdjustment = 100; // stop led flicker while environemnt light hit the original threshold

    const int _PWM_INTERVAL = 20;       // 20ms as the interval
    const int _LIGHT_ON_PERIOD = 30000; // light ON for 30 seconds once triggered 45000

public:
    IndoorLedSystem(Mosffet *mosffet, Pir *pirUpstair, Pir *pirDownstair, Pir *pirFaceMasterRoom, LDR *ldr)
    {
        _mosffet = mosffet;
        _pirUpstair = pirUpstair;
        _pirDownstair = pirDownstair;
        _pirFaceMasterRoom = pirFaceMasterRoom;
        _ldr = ldr;

        _lightChangeEvent = new IntervalEvent(_PWM_INTERVAL, [&]() -> void { mosffet->ApplyPwmStrength(mosffet->_currentPwm += mosffet->_change); });
    }

    void run()
    {

        switch (_status)
        {
        case LED_OFF_NO_DETECTIOIN_STATUS:
            _mosffet->_change = 0;
            if ((_pirUpstair->detect() || _pirDownstair->detect() || _pirFaceMasterRoom->detect()) && (_lightValue < _lightThreshold))
            {
                _status = LED_TURNING_ON_STATUS;
            }
            else if (_lightValue >= _lightThreshold)
            {
                _mosffet->_currentPwm = 0;
                _mosffet->ApplyPwmStrength(_mosffet->_currentPwm);
                _status = LED_OFF_NO_DETECTIOIN_STATUS;
            }
            break;

        case LED_TURNING_ON_STATUS:
            _mosffet->_change = 1;
            if (_mosffet->_currentPwm >= _mosffet->_maxPwn)
            {
                _status = LED_MAX_AND_WAIT_STATUS;
            }
            break;

        case LED_MAX_AND_WAIT_STATUS:
            _mosffet->_change = 0;

            _waitEvent = new TimeoutEvent(_LIGHT_ON_PERIOD, [&]() -> void { _status = LED_TURNING_OFF_STATUS; });

            // during waiting, if detect people, _waitToTurnOffEvent clock reset
            if ((_pirUpstair->detect() || _pirDownstair->detect() || _pirFaceMasterRoom->detect()) && (_lightValue < _lightThreshold))
            {
                _waitEvent->resetEventClock();
            }
            break;

        case LED_TURNING_OFF_STATUS:
           _mosffet->_change = -1;
            if (_mosffet->_currentPwm <= _MINI_PWM_STRENGTH)
            {
                _status = LED_OFF_NO_DETECTIOIN_STATUS;
            }

            // while turnning off, if detect an objec, then led starts to turning on again.
            if ((_pirUpstair->detect() || _pirDownstair->detect() || _pirFaceMasterRoom->detect()) && (_lightValue < _lightThreshold))
            {
                _status = LED_TURNING_ON_STATUS;
            }
            break;

        default:
            break;
        }

        if (_lightChangeEvent)
            _lightChangeEvent->start();

        if (_waitEvent)
            _waitEvent->start(&_waitEvent);
    }
};
#endif