#include <Arduino.h>
#include "../include/event_lib/IntervalEvent.h"
#include "../include/event_lib/TimeoutEvent.h"
#include "../include/pir_lib/Pir.h"
#include "../include/mosffet_lib/Mosffet.h"
#include "../include/LDR_lib/LDR.h"
#define HARD_OFF 1
#define HARD_ON 2
#define AUTO 3

#define LED_OFF_NO_DETECTIOIN_STATUS 1
#define LED_TURNING_ON_STATUS 2
#define LED_MAX_AND_WAIT_STATUS 3
#define LED_TURNING_OFF_STATUS 4
#define LED_HARD_ON_STATUS 5
#define LED_HARD_OFF_STATUS 6

#define LIGHT_THRESHOLD 900 // more light, greater value

#ifndef _DECKMOSFFETSYSTEM_H
#define _DECKMOSFFETSYSTEM_H

// : public Mosffet
class GarageLedSystem
{
public:
    Mosffet *_mosffet = NULL;
    Pir *_pir = NULL;
    LDR *_ldr = NULL;

    const int _PWM_INTERVAL = 20;       // 20ms as the interval
    const int _LIGHT_ON_PERIOD = 35000; // light ON for 30 seconds once triggered 45000
    byte _status = LED_OFF_NO_DETECTIOIN_STATUS;

    int _opeartionMode = AUTO;
    boolean _locker = false; // the Variable works with
    boolean _isHardOn = false;
    boolean _isHardOff = false;
    boolean _isAuto = false;

    IntervalEvent *_onOffEvent = NULL;
    TimeoutEvent *_waitToTurnOffEvent = NULL;

public:
    GarageLedSystem(Mosffet *mosffet, Pir *pir, LDR *ldr)
    {

        _mosffet = mosffet;
        // turn off mosffet  at the start
        _ldr = ldr;
        _mosffet->ApplyPwmStrength(_MINI_PWM_STRENGTH);
        _pir = pir;
        _onOffEvent = new IntervalEvent(_PWM_INTERVAL, [&]() -> void { _mosffet->ApplyPwmStrength(_mosffet->_currentPwm += _mosffet->_change); });
    }

    void run()
    {
      
        switch (_status)
        {
        case LED_OFF_NO_DETECTIOIN_STATUS:
            _mosffet->_currentPwm = _MINI_PWM_STRENGTH;
            _mosffet->ApplyPwmStrength(_mosffet->_currentPwm);

            _mosffet->_change = 0;
            // if light condition is good, then led turn off immedietaly
            if (_ldr->getValue() > LIGHT_THRESHOLD)
            {
                _status = LED_OFF_NO_DETECTIOIN_STATUS;
                // if there is an ongoing _waitToTurnOffEvent, then kill the timeout event
                if(_waitToTurnOffEvent)
                _waitToTurnOffEvent->kill(&_waitToTurnOffEvent);

                break;
            }

            // detect people, shift status
            if (_pir->detect() == true)
            {
                _status = LED_TURNING_ON_STATUS;
            }
            break;
        case LED_TURNING_ON_STATUS:
            _mosffet->_change = 1;
            // if light condition is good, then led turn off immedietaly
            if (_ldr->getValue() > LIGHT_THRESHOLD)
            {
                _status = LED_OFF_NO_DETECTIOIN_STATUS;
                break;
            }

            // reach max light intensity , shift status
            if (_mosffet->_currentPwm >= _mosffet->_maxPwn)
            {
                _status = LED_MAX_AND_WAIT_STATUS;
            }
            break;
        case LED_MAX_AND_WAIT_STATUS:
            _mosffet->_change = 0;

            // if light condition is good, then led turn off immedietaly
            if (_ldr->getValue() > LIGHT_THRESHOLD)
            {
                _status = LED_OFF_NO_DETECTIOIN_STATUS;
                break;
            }

            if (_waitToTurnOffEvent == NULL)
            {
                _waitToTurnOffEvent = new TimeoutEvent(_LIGHT_ON_PERIOD, [&]() -> void { _status = LED_TURNING_OFF_STATUS; });
            }

            // during waiting, if detect people, _waitToTurnOffEvent clock reset
            if (_pir->detect() == true)
            {
                _waitToTurnOffEvent->resetEventClock();
            }

            break;
        case LED_TURNING_OFF_STATUS:
            _mosffet->_change = -1;

            // if light condition is good, then led turn off immedietaly
            if (_ldr->getValue() > LIGHT_THRESHOLD)
            {
                _status = LED_OFF_NO_DETECTIOIN_STATUS;
                break;
            }

            if (_mosffet->_currentPwm <= _MINI_PWM_STRENGTH)
            {
                _status = LED_OFF_NO_DETECTIOIN_STATUS;
            }

            // while turnning off, if detect an objec, then led starts to turning on again.
            if (_pir->detect() == true)
            {
                _status = LED_TURNING_ON_STATUS;
            }

            break;

        case LED_HARD_ON_STATUS:
            _mosffet->_change = 0;
            _mosffet->_currentPwm = _mosffet->_maxPwn;
            _mosffet->ApplyPwmStrength(_mosffet->_currentPwm);
            break;

        case LED_HARD_OFF_STATUS:
            _mosffet->_change = 0;
            _mosffet->_currentPwm = _MINI_PWM_STRENGTH;
            _mosffet->ApplyPwmStrength(_mosffet->_currentPwm);

            break;

        default:
            break;
        }

        if (_onOffEvent)
            _onOffEvent->start();

        if (_waitToTurnOffEvent)
            _waitToTurnOffEvent->start(&_waitToTurnOffEvent);

        // handle operation mode change from Blynk App
        if (_opeartionMode == HARD_ON)
        {
            _status = LED_HARD_ON_STATUS;
            _locker = false;
            if (_waitToTurnOffEvent)
            {
                _waitToTurnOffEvent->kill(&_waitToTurnOffEvent);
            }
        }
        else if (_opeartionMode == HARD_OFF)
        {
            _status = LED_HARD_OFF_STATUS;
            _locker = false;
            if (_waitToTurnOffEvent)
            {
                _waitToTurnOffEvent->kill(&_waitToTurnOffEvent);
            }
        }
        else if (_opeartionMode == AUTO && _locker == false)
        {
            _status = LED_TURNING_ON_STATUS;
            _locker = true;
        }

        // debug
        // Serial.println("test");
        //  Serial.println(_currentPwm);
        // if (_prePwn != _currentPwm)
        // {
        //     Serial.println(_currentPwm);
        //     _prePwn = _currentPwm;
        // }
    }

    void nonRegularHourRun()
    {

        // Serial.print(_opeartionMode == AUTO );
        // Serial.print("-----------mode and isAuto------------");
        // Serial.println(_isAuto == false );
        if (_opeartionMode == HARD_ON && _isHardOn == false)
        {
            _mosffet->_currentPwm = _mosffet->_maxPwn;
            _mosffet->ApplyPwmStrength(_mosffet->_currentPwm);
            _status = LED_HARD_ON_STATUS;
            _locker = false;
            // toggle the status to improve efficiency
            _isHardOn = true;
            _isHardOff = false;
            _isAuto = false;

            if (_waitToTurnOffEvent)
            {
                _waitToTurnOffEvent->kill(&(_waitToTurnOffEvent));
            }
        }
        else if (_opeartionMode == HARD_OFF && _isHardOff == false)
        {
            _mosffet->_currentPwm = _MINI_PWM_STRENGTH;
            _mosffet->ApplyPwmStrength(_mosffet->_currentPwm);
            _status = LED_HARD_OFF_STATUS;
            _locker = false;
            // toggle the status to improve efficiency
            _isHardOff = true;
            _isHardOn = false;
            _isAuto = false;

            if (_waitToTurnOffEvent)
            {
                _waitToTurnOffEvent->kill(&(_waitToTurnOffEvent));
            }
        }

        else if (_opeartionMode == AUTO && _isAuto == false)
        {
            _mosffet->_currentPwm = _MINI_PWM_STRENGTH;
            // Serial.print("_currentPwm----");
            // Serial.println(_currentPwm);
            _mosffet->ApplyPwmStrength(_mosffet->_currentPwm);
            _status = LED_OFF_NO_DETECTIOIN_STATUS;
            // toggle the status to improve efficiency
            _isHardOff = false;
            _isHardOn = false;
            _isAuto = true;

            _locker = false;
        }
    }
};
#endif