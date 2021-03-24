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
#include "../include/mosffet_lib/IndoorLedSystem.h"
#include "../include/components_lib/Led.h"
#include "../include/LDR_lib/LDR.h"

const char *ssid = "SPARK-5RUXSX";
const char *pass = "ReliableSloth20VV";
const char *auth = "UPpQ01aooX9O3q3ICgkKEDn_HpJeThPi";
// test auth:-------UPpQ01aooX9O3q3ICgkKEDn_HpJeThPi
// production auth: Ah-lQpqdle6DvDC68M0qhofsS-8fD4KU

const int FETCH_DATETIME_INTERVAL = 60000;                // get current datetime every 1mins
const int HEATBEAT_TO_SUPERCLIENT_INTERVAL = 3000;        // send heartbeat to superclient nodemcu every 3s
const int lightIntensity_TO_SUPERCLIENT_INTERVAL = 15000; // send light intensity to superclient nodemcu every 15s
const int BLINK_INTERVAL = 1000;                          // led blinks every 1s

byte ledStartHour = 19; // led starts working from 7pm
byte ledStopHour = 6;   // led stops workinf from 6 am

/************************Pir sensor***************************************/
Pir *pirUpstair = new Pir(D2);
Pir *pirDownstair = new Pir(D3);
Pir *pirFaceMasterRoom = new Pir(D4);

/************************LDR sensor***************************************/
LDR *ldr = new LDR(D1);

/************************mosffet***************************************/
Mosffet *mosffet = new Mosffet(D5);

/************************Indoor led system***************************************/
IndoorLedSystem *indoorLedSystem = new IndoorLedSystem(mosffet, pirUpstair, pirDownstair, pirFaceMasterRoom, ldr);

/**************************Datetime***************************************/
Datetime *datetime = new Datetime(ssid, pass);

/**************************BLYNK WRITE*********************************/

/**************************WATCHDOG************************************/
Watchdog *watchdog = NULL;

/**************************BUILT_IN LED***************************************/
Led *led = new Led(D0);
/**************************BRIDGE***************************************/
// WidgetBridge bridgeToSuperClient(V13);
// BLYNK_CONNECTED()
// {
//   bridgeToSuperClient.setAuthToken("td2xBB3r_PM3ZnvsSnDUq_ykyLzfvtNB"); // superclient token
// }

/**************************INTERVALEVENT***************************************/
IntervalEvent *datetimeEvent = new IntervalEvent(FETCH_DATETIME_INTERVAL,
                                                 [&]() -> void { datetime->updateHours(); });

IntervalEvent *heartBeatToSuperClientEvent = new IntervalEvent(HEATBEAT_TO_SUPERCLIENT_INTERVAL,
                                                               [&]() -> void {});

/**************************LED BLINK EVENT***************************************/
IntervalEvent *ledBlinkEvent = new IntervalEvent(BLINK_INTERVAL, [&]() -> void {});
/**************************TEST***************************************/
// IntervalEvent *test = new IntervalEvent(1500, [&]() -> void {  Serial.println(pir->detect()); });

void setup()
{
  Serial.begin(9600);
  Blynk.begin(auth, ssid, pass);
  Serial.println("inddor led nodemcu starts now");
  datetime->updateHours(); // get current time at the start
  watchdog = new Watchdog();
}

void loop()
{
  Blynk.run();
  watchdog->start();
  datetimeEvent->start();
  led->blink();
  //heartBeatToSuperClientEvent->start();      //Do not active this line, unless for production

  if ((datetime->getHours() >= ledStartHour || datetime->getHours() <= ledStopHour))
  {
    indoorLedSystem->run(ledStartHour, datetime->getHours());
  }else{
    indoorLedSystem->_mosffet->off();
  }
}
