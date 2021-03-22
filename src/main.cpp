#define BLYNK_PRINT Serial
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <BlynkSimpleEsp8266.h>
#include <Ticker.h>
#include "../include/event_lib/TimeoutEvent.h"
#include "../include/event_lib/IntervalEvent.h"
#include "../include/datetime_lib/Datetime.h"
#include "../include/components_lib/Button.h"
#include "../include/watchdog_lib/Watchdog.h"
#include "../include/pir_lib/Pir.h"
#include "../include/mosffet_lib/DeckMosffetSystem.h"
#include "../include/components_lib/Led.h"

const char *ssid = "SPARK-5RUXSX";
const char *pass = "ReliableSloth20VV";
const char *auth = "Ah-lQpqdle6DvDC68M0qhofsS-8fD4KU";
// test auth:-------UPpQ01aooX9O3q3ICgkKEDn_HpJeThPi
// production auth: Ah-lQpqdle6DvDC68M0qhofsS-8fD4KU

const int FETCH_DATETIME_INTERVAL = 60000;                // get current datetime every 1mins
const int HEATBEAT_TO_SUPERCLIENT_INTERVAL = 3000;        // send heartbeat to superclient nodemcu every 3s
const int lightIntensity_TO_SUPERCLIENT_INTERVAL = 15000; // send light intensity to superclient nodemcu every 15s
const int BLINK_INTERVAL = 1000;                          // led blinks every 1s
byte ledStartHour = 19;                                   // led starts working from 7pm
byte ledStopHour = 6;                                     // led stops workinf from 6 am

/************************Pir sensor***************************************/
Pir *pir = new Pir(D2);
/************************Mosffet***************************************/
DeckMosffetSystem *deckMosffetSystem = new DeckMosffetSystem(D1, pir);
// IntervalEvent *blinkEvent = new IntervalEvent(2000, test);

/**************************Datetime***************************************/
Datetime *datetime = new Datetime(ssid, pass);

/**************************BLYNK WRITE*********************************/
BLYNK_WRITE(V1)
{

  // Serial.println(param.asInt());
  deckMosffetSystem->setMaxPwm(param.asInt());
  if ((datetime->getHours() >= ledStartHour || datetime->getHours() <= ledStopHour))
  {
    deckMosffetSystem->_status = LED_TURNING_ON_STATUS;
  }
  else
  {
    if (deckMosffetSystem->_opeartionMode == HARD_ON)
      deckMosffetSystem->ApplyPwmStrength(deckMosffetSystem->_currentPwm = deckMosffetSystem->_maxPwn);
  }
}

BLYNK_WRITE(V0)
{
  int temp = param.asInt();
  // 1 for OFF, 2 for ON, 3 for Auto, but superclient will send 254 instead of 3,
  // so the below code is to convert 254 to 3
  if (temp != 1 && temp != 2)
  {
    temp = 3;
  }
  deckMosffetSystem->_opeartionMode = temp; //
}
/**************************WATCHDOG************************************/
Watchdog *watchdog = NULL;

/**************************BUILT_IN LED***************************************/
Led *led = new Led(D0);
/**************************BRIDGE***************************************/
WidgetBridge bridgeToSuperClient(V13);
BLYNK_CONNECTED()
{
  bridgeToSuperClient.setAuthToken("td2xBB3r_PM3ZnvsSnDUq_ykyLzfvtNB"); // superclient token
}

/**************************INTERVALEVENT***************************************/
IntervalEvent *datetimeEvent = new IntervalEvent(FETCH_DATETIME_INTERVAL,
                                                 [&]() -> void { datetime->updateHours(); });

IntervalEvent *heartBeatToSuperClientEvent = new IntervalEvent(HEATBEAT_TO_SUPERCLIENT_INTERVAL,
                                                               [&]() -> void { bridgeToSuperClient.virtualWrite(V13, ("deckLightStatus_" + String(deckMosffetSystem->_opeartionMode))); });

IntervalEvent *lightIntensityToSuperClientEvent = new IntervalEvent(lightIntensity_TO_SUPERCLIENT_INTERVAL,
                                                                    [&]() -> void { bridgeToSuperClient.virtualWrite(V13, ("deckLightBrightness_" + String(deckMosffetSystem->_currentPwm))); });

IntervalEvent *ledBlinkEvent = new IntervalEvent(BLINK_INTERVAL, [&]() -> void {});
/**************************TEST***************************************/
// IntervalEvent *test = new IntervalEvent(1500, [&]() -> void {  Serial.println(pir->detect()); });

void setup()
{
  Serial.begin(9600);
  Blynk.begin(auth, ssid, pass);
  Serial.println("deck nodemcu starts now");
  datetime->updateHours(); // get current time at the start
  watchdog = new Watchdog();
}

void loop()
{
  Blynk.run();
  watchdog->start();
  datetimeEvent->start();
  led->blink();
  heartBeatToSuperClientEvent->start();      //Do not active this line, unless for production
  lightIntensityToSuperClientEvent->start(); //Do not active this line, unless for production

  if (deckMosffetSystem != NULL)
  {

    // mosffet->run();
    if ((datetime->getHours() >= ledStartHour || datetime->getHours() <= ledStopHour))
    {
      deckMosffetSystem->run();
    }
    else // outside the hours, hardOn and hardOFF should be still working
    {
      deckMosffetSystem->nonRegularHourRun();
    }
  }
}


