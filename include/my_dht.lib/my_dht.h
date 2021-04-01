#include <Arduino.h>
#include "dht.h"

#ifndef _MYDHT_H
#define _MYDHT_H

class MyDHT{
public:
    int _pin;
    dht *DHT = NULL; 

public:
    MyDHT(int pin){
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

