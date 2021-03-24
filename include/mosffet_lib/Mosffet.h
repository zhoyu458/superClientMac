#include <Arduino.h>
#include "../include/event_lib/IntervalEvent.h"
#include "../include/event_lib/TimeoutEvent.h"
#include "../include/pir_lib/Pir.h"
#define _MINI_PWM_STRENGTH 0
// #define HARD_OFF 1
// #define HARD_ON 2
// #define AUTO 3

// #define LED_OFF_NO_DETECTIOIN_STATUS 1
// #define LED_TURNING_ON_STATUS 2
// #define LED_MAX_AND_WAIT_STATUS 3
// #define LED_TURNING_OFF_STATUS 4
// #define LED_HARD_ON_STATUS 5
// #define LED_HARD_OFF_STATUS 6

#ifndef _MOSFFET_H
#define _MOSFFET_H

class Mosffet
{

public:
    // const int _MINI_PWM_STRENGTH = 0;
    int _maxPwn = 100;
    // const int _PWM_INTERVAL = 20;       // 20ms as the interval
    // const int _LIGHT_ON_PERIOD = 45000; // light ON for 30 seconds once triggered 45000
    int _currentPwm = 0;
    int _change = 0;
    // int _opeartionMode = AUTO;
    unsigned int _pinNum;
    // boolean _locker = false; // the Variable works with
    // boolean _isHardOn = false;
    // boolean _isHardOff = false;
    // boolean _isAuto = false;
    // IntervalEvent *_onOffEvent = NULL;
    // TimeoutEvent *_waitToTurnOffEvent = NULL;
    // byte _status = LED_OFF_NO_DETECTIOIN_STATUS;
    // Pir *_pir;

    //int _prePwn = 0; // debug variable

public:

    Mosffet(){

    }
    Mosffet(unsigned int pinNum) //, Pir *pir
    {
        _pinNum = pinNum;
        pinMode(_pinNum, OUTPUT);
        // turn off mosffet  at the start
        analogWrite(_pinNum, 0);
        // _pir = pir;

        // _onOffEvent = new IntervalEvent(_PWM_INTERVAL, [&]() -> void { ApplyPwmStrength(_currentPwm += _change); });
    }

    void setMaxPwm(int newValue)
    {
        _maxPwn = newValue;
    }

    void ApplyPwmStrength(unsigned int level)
    {

        analogWrite(_pinNum, level);
    }

    void off()
    {
        _currentPwm = _MINI_PWM_STRENGTH;
        analogWrite(_pinNum, _currentPwm);
    }

    void on()
    {
        analogWrite(_pinNum, _maxPwn);
    }

    // void run()
    // {
    //     switch (_status)
    //     {
    //     case LED_OFF_NO_DETECTIOIN_STATUS:
    //         // Serial.println("status 1");
    //         // Serial.print("------------");
    //         // Serial.println(_pir->detect());

    //         _change = 0;
    //         // detect people, shift status
    //         if (_pir->detect() == true)
    //         {
    //             _status = LED_TURNING_ON_STATUS;
    //         }
    //         break;
    //     case LED_TURNING_ON_STATUS:
    //         // Serial.println("status 2");
    //         _change = 1;
    //         // reach max light intensity , shift status
    //         if (_currentPwm >= _maxPwn)
    //         {
    //             _status = LED_MAX_AND_WAIT_STATUS;
    //         }
    //         break;
    //     case LED_MAX_AND_WAIT_STATUS:
    //         // Serial.println("status 3");
    //         _change = 0;

    //         if (_waitToTurnOffEvent == NULL)
    //         {
    //             _waitToTurnOffEvent = new TimeoutEvent(_LIGHT_ON_PERIOD, [&]() -> void { _status = LED_TURNING_OFF_STATUS; });
    //         }

    //         // during waiting, if detect people, _waitToTurnOffEvent clock reset
    //         if (_pir->detect() == true)
    //         {
    //             _waitToTurnOffEvent->resetEventClock();
    //         }

    //         break;
    //     case LED_TURNING_OFF_STATUS:
    //         // Serial.println("status 4");
    //         _change = -1;

    //         if (_currentPwm <= _MINI_PWM_STRENGTH)
    //         {
    //             _status = LED_OFF_NO_DETECTIOIN_STATUS;
    //         }

    //         // while turnning off, if detect an objec, then led starts to turning on again.
    //         if (_pir->detect() == true)
    //         {
    //             _status = LED_TURNING_ON_STATUS;
    //         }

    //         break;

    //     case LED_HARD_ON_STATUS:
    //         _change = 0;
    //         _currentPwm = _maxPwn;
    //         ApplyPwmStrength(_currentPwm);
    //         break;

    //     case LED_HARD_OFF_STATUS:
    //         _change = 0;
    //         _currentPwm = _MINI_PWM_STRENGTH;
    //         ApplyPwmStrength(_currentPwm);

    //         break;

    //     default:
    //         break;
    //     }

    //     if (_onOffEvent)
    //         _onOffEvent->start();

    //     if (_waitToTurnOffEvent)
    //         _waitToTurnOffEvent->start(&_waitToTurnOffEvent);

    //     // handle operation mode change from Blynk App
    //     if (_opeartionMode == HARD_ON)
    //     {
    //         _status = LED_HARD_ON_STATUS;
    //         _locker = true;
    //         if (_waitToTurnOffEvent)
    //         {
    //             _waitToTurnOffEvent->kill(&_waitToTurnOffEvent);
    //         }
    //     }
    //     else if (_opeartionMode == HARD_OFF)
    //     {
    //         _status = LED_HARD_OFF_STATUS;
    //         _locker = true;
    //         if (_waitToTurnOffEvent)
    //         {
    //             _waitToTurnOffEvent->kill(&_waitToTurnOffEvent);
    //         }
    //     }
    //     else if (_opeartionMode == AUTO && _locker == true)
    //     {
    //         _status = LED_TURNING_ON_STATUS;
    //         _locker = false;
    //     }

    //     // debug
    //     // Serial.println("test");
    //     //  Serial.println(_currentPwm);
    //     // if (_prePwn != _currentPwm)
    //     // {
    //     //     Serial.println(_currentPwm);
    //     //     _prePwn = _currentPwm;
    //     // }
    // }

    // void nonRegularHourRun()
    // {

        // Serial.print(_opeartionMode == AUTO );
        // Serial.print("-----------mode and isAuto------------");
        // Serial.println(_isAuto == false );
    //     if (_opeartionMode == HARD_ON && _isHardOn == false)
    //     {
    //         _currentPwm = _maxPwn;
    //         ApplyPwmStrength(_currentPwm);
    //         _status = LED_HARD_ON_STATUS;
    //         _locker = true;
    //         // toggle the status to improve efficiency
    //         _isHardOn = true;
    //         _isHardOff = false;
    //         _isAuto = false;

    //         if (_waitToTurnOffEvent)
    //         {
    //             _waitToTurnOffEvent->kill(&(_waitToTurnOffEvent));
    //         }
    //     }
    //     else if (_opeartionMode == HARD_OFF && _isHardOff == false)
    //     {
    //         _currentPwm = _MINI_PWM_STRENGTH;
    //         ApplyPwmStrength(_currentPwm);
    //         _status = LED_HARD_OFF_STATUS;
    //         _locker = true;
    //         // toggle the status to improve efficiency
    //         _isHardOff = true;
    //         _isHardOn = false;
    //         _isAuto = false;

    //         if (_waitToTurnOffEvent)
    //         {
    //             _waitToTurnOffEvent->kill(&(_waitToTurnOffEvent));
    //         }
    //     }



    //     else if (_opeartionMode == AUTO && _isAuto == false)
    //     {
    //         _currentPwm = _MINI_PWM_STRENGTH;
    //         // Serial.print("_currentPwm----");
    //         // Serial.println(_currentPwm);
    //         ApplyPwmStrength(_currentPwm);
    //         _status = LED_OFF_NO_DETECTIOIN_STATUS;
    //         // toggle the status to improve efficiency
    //         _isHardOff = false;
    //         _isHardOn = false;
    //         _isAuto = true;

    //         _locker = true;
    //     }
    // }
};
#endif
