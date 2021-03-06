#include <Arduino.h>
#include "dht.h"

#ifndef _MYDHT_H
#define _MYDHT_H

class MYDHT{
public:
    int _pin;
    dht *DHT = NULL; 

public:
    MYDHT(int pin){
        _pin = pin;
        DHT = new dht();
    }
    double getTemp(){
        return DHT->temperature;
    }
    double getHumidity(){
        return DHT->humidity;
    }


};
#endif

