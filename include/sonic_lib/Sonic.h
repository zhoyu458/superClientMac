#include <Arduino.h>
#ifndef _SONIC_H
#define _SONIC_H

class SonicSensor{
  public:
    byte trigPin;
    byte echoPin;
    // long duration;

  SonicSensor(byte tp, byte ep){
    trigPin = tp;
    echoPin = ep;

    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
  }

  int getDistance(){
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    // duration = pulseIn(echoPin, HIGH);   
    return pulseIn(echoPin, HIGH) * 0.034 / 2;
  }

  
};
#endif