void haha(){
	
}
void softOn()
    {
        if (_onOffEvent == NULL) 
            _onOffEvent = new IntervalEvent(_PWM_INTERVAL,
                                            [&]() -> void { PwmApplyStrength(_currentPwm += _change); });
        if (_change != 1)
            _change = 1;

        if (_onOffEvent != NULL)
        {
            _onOffEvent->start();
            // Serial.println("softON.....");
        }

        if (_currentPwm >= _maxPwn && _onOffEvent != NULL)
        {
            _currentPwm = _maxPwn;
            _onOffEvent = NULL;
            delete _onOffEvent;
            _softOnTrigger = false;
        }
    }

    void softOff()
    {
        // event point to NULL and strength reaches maximum
        if (_onOffEvent == NULL && _currentPwm >= _maxPwn)
            _onOffEvent = new IntervalEvent(_PWM_INTERVAL,
                                            [&]() -> void { PwmApplyStrength(_currentPwm += _change); });

        if (_change != -1)
            _change = -1;

        // start to soft turn off
        if (_onOffEvent != NULL)
            _onOffEvent->start();

        // detect object while turning off
        if (_pir->detect() == true && _currentPwm < _maxPwn && _onOffEvent != NULL)
        {
            _onOffEvent = NULL;
            delete _onOffEvent;
            _softOffTrigger = false;
            _softOnTrigger = false;
        }
        // normally turn off, no dection during turning off
        if (_currentPwm <= _MINI_PWM_STRENGTH && _onOffEvent != NULL)
        {
            _currentPwm = _MINI_PWM_STRENGTH;
            _onOffEvent = NULL;
            delete _onOffEvent;
            _softOffTrigger = false;
        }
    }


    void run()
    {
        // Serial.print("level: ");
        // Serial.println(_currentPwm);
        // Serial.print("---------");
        // Serial.print("_change: ");
        // Serial.println(_change);
        // Serial.print("PIR: ");
        // Serial.println(_pir->detect());
        if (_softOffTrigger == false && _currentPwm < _maxPwn)
        {
            if (_pir->detect() == true)
            {
                // Serial.println("_softOnTrigger = true now");
                _softOnTrigger = true;
            }

            if (_softOnTrigger == true)
            {
                softOn();
            }
        }
        else if (_currentPwm >= _maxPwn)
        {
            if (_pir->detect() == false)
            {
                if (_waitToTurnOffEvent == NULL)
                {

                    _waitToTurnOffEvent = new TimeoutEvent(_LIGHT_ON_PERIOD, [&]() -> void { _softOffTrigger = true;  });
                }

                if (_waitToTurnOffEvent != NULL)
                {
                    _waitToTurnOffEvent->start(&_waitToTurnOffEvent);
                }
            }
            else // during light time, if detect an object, then stop counting down
            {
                if (_waitToTurnOffEvent != NULL)
                {
                    _waitToTurnOffEvent->kill(&_waitToTurnOffEvent);
                }
            }
        }

        if (_softOffTrigger == true)
        {
            softOff();
        }
    }