#define BLYNK_PRINT Serial
#define GARAGE_WARNNING_DISTANCE 200 // less than the value, increase garageWarnningCounter
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <Ticker.h>
#include <TM1637Display.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#include "../include/sonic_lib/Sonic.h"
#include "../include/event_lib/IntervalEvent.h"
#include "../include/event_lib/TimeoutEvent.h"
#include "../include/datetime_lib/Datetime.h"
#include "../include/components_lib/Led.h"
#include "../include/watchdog_lib/Watchdog.h"
#include "../include/sonic_lib/Sonic.h"
#include "../include/MYDHT_lib/MYDHT.h"
#include "../include/TM_display_lib/TM_display.h"

/*********************************Global**************************/
byte garageWarnningCounter = 0; // if the counter reaches 6,
bool garageNeverReport = true;
unsigned long garageReportDelay = 60000 * 15; // default delay time is 15 mins after the initial report
/*********************************WIFI****************************/
char auth[] = "td2xBB3r_PM3ZnvsSnDUq_ykyLzfvtNB";
char ssid[] = "SPARK-5RUXSX";
char pass[] = "ReliableSloth20VV";

/********************************BRIDGE***************************/
WidgetBridge remote_nodemcu_bridge(V1); // serverMCU is the unit connect to the physical remotes

WidgetBridge indoor_alarm_nodemcu_bridge(V17);

WidgetBridge deck_nodemcu_bridge(V100);
/*****************************BLYNK CONNECT**************************/
BLYNK_CONNECTED()
{
  remote_nodemcu_bridge.setAuthToken("20e0a85a41fa4fc7b7c93a53576bb2c3"); // Place the AuthToken of the second hardware here
  deck_nodemcu_bridge.setAuthToken("Ah-lQpqdle6DvDC68M0qhofsS-8fD4KU");   // Place the AuthToken of the second hardware here
  indoor_alarm_nodemcu_bridge.setAuthToken("s7u2S42iA0LXum1WqX36Vjwu7QE2OP1v");
  Blynk.syncAll();
  sync_bridges(); // send values to relevant bridges
}

/*****************************BLYNK WRITE**************************/
// V2 is a text input terminal on Blynk App, user can open or close garage or driveway by send a msg
WidgetTerminal terminal(V2);
BLYNK_WRITE(V2)
{
  String msg = param.asStr();
  if (msg == "reset" || msg == "Reset")
  {
    delay(10000); // wait for 10 seconds and reset the NodeMcu
    ESP.reset();
  }
  // send message to the nodemcu with the real remote.
  remote_nodemcu_bridge.virtualWrite(V0, msg);

  //terminal.clear();
  terminal.print("ok");
  terminal.flush();
}

// V4 is update garage warnning delay duration. V4 recieves a number from 0 to 60 as delay minutes
BLYNK_WRITE(V4)
{
  garageReportDelay = param.asInt(); // delay can vary from 1 mins to 15 mins
  if (garageReportEvent)
  {
    garageReportEvent->updateTimeoutDuration(garageReportDelay);
  }
}

// V8 send msg to turn ON / OFF driveway door
BLYNK_WRITE(V8)
{
  if (param.asInt())
  {
    remote_nodemcu_bridge.virtualWrite(V0, "1001");
  }
}

// V8 send msg to turn ON / OFF garage door
BLYNK_WRITE(V9)
{
  if (param.asInt())
  {
    remote_nodemcu_bridge.virtualWrite(V0, "1001g");
  }
}

// V16 taking msg from user App and send to indoor nodemcu to turn ON or OFF the alarm.
// this part is not coded yet
BLYNK_WRITE(V16)
{
  // turn_on_indoor_alarm = param.asInt();
  // indoor_alarm_nodemcu_bridge.virtualWrite(V0, turn_on_indoor_alarm);
  // indoor_nodeMcu_report_theft = false; // anyway, you reset the variable, assuming no theft

  // if (turn_on_indoor_alarm == HIGH)
  // {
  //   //Serial.println("v18 can notify is true");
  //   v18_can_notify = true;
  // }
}
// V18 works with v16, this part is not ready
BLYNK_WRITE(V18)
{
  // indoor_nodeMcu_report_theft = param.asInt();
  // if (v18_can_notify)
  // {
  //   if (indoor_nodeMcu_report_theft && turn_on_indoor_alarm)
  //   {
  //     Blynk.notify("请注意， 屋内检测到有人走动。");
  //     v18_can_notify = false;
  //   }
  // }
}

/*****************************pulse pin**************************/
// send pulse to arduino watchdog, prevent power off
Led *pulse = NULL;

/*****************************watch dog**************************/
Watchdog *watchdog = NULL;

/*****************************Sonic Sensor**************************/
SonicSensor *sonic1 = NULL;
SonicSensor *sonic2 = NULL;

/*****************************DHT sensor**************************/
MYDHT *dhtSensor = NULL;

/*****************************TM display**************************/
TM_display *blockDisplay = NULL;

/*****************************intervalEvent**************************/
IntervalEvent *sonicSensorEvent = NULL;
IntervalEvent *dhtSensorEvent = NULL;

/*****************************timeoutEvent**************************/
TimeoutEvent *garageReportEvent = NULL;

void setup()
{
  pulse = new Led(D0);
  watchdog = new Watchdog();
  sonic1 = new SonicSensor(D6, D7);
  sonic2 = new SonicSensor(D1, D2);
  dhtSensor = new MYDHT(D5);
  blockDisplay = new TM_display(D3, D4);

  sonicSensorEvent = new IntervalEvent(2000, sonicSensorEventWrapper);
  dhtSensorEvent = new IntervalEvent(2000, dhtSensorEventWrapper);
  // initiate the table, V11 is a table on Blynk App
  Blynk.virtualWrite(V11, "clr");
  Blynk.virtualWrite(V11, "add", 1, "感应距离1", "test");
  Blynk.virtualWrite(V11, "add", 2, "感应距离2", "test");
  Blynk.virtualWrite(V11, "add", 3, "甲板控制系统", "test");
  Serial.println("Table is configured, ready to start");
}

void loop()
{
  Blynk.run();

  if (pulse)
    pulse->blink();

  if (watchdog)
    watchdog->start();
}

/**********************FUNCTION********************************/
void sync_bridges()
{
  // sink deck light status to the deck_nodemcu
  // deck_nodemcu_bridge.virtualWrite(V0, deck_light_system_status_to_send);
}

void sonicSensorEventWrapper()
{
  if (garageReportEvent)
    garageReportEvent->start(&garageReportEvent);

  int d1 = sonic1->getDistance();
  int d2 = sonic2->getDistance();
  d1 < GARAGE_WARNNING_DISTANCE ? garageWarnningCounter++ : garageWarnningCounter--;
  d2 < GARAGE_WARNNING_DISTANCE ? garageWarnningCounter++ : garageWarnningCounter--;

  if (garageWarnningCounter > 10)
    garageWarnningCounter = 10;
  else if (garageWarnningCounter < 0)
    garageWarnningCounter = 0;

  // update value to blynk table
  Blynk.virtualWrite(V11, "update", 1, "感应距离1", d1); // 2 for driveway gate msg
  Blynk.virtualWrite(V11, "update", 2, "感应距离2", d2); // 2 for driveway gate msg

  // send notification to app user
  if (garageWarnningCounter >= 10)
  {
    if (garageNeverReport)
    {
      Blynk.notify("车库门被打开，请注意！");
      garageNeverReport = false;
    }
    else // garage has reported before
    {
      if (garageReportEvent == NULL)
      {
        garageReportEvent = new TimeoutEvent(garageReportDelay, []() { Blynk.notify("车库门被打开，请注意！"); });
      }
    }
  }
  // garageWarnningCounter <= 0, garage door is fully closed
  else if (garageWarnningCounter <= 0)
  {
    garageNeverReport = true;
    if (garageReportEvent)
    {
      garageReportEvent->kill(&garageReportEvent);
    }
  }
}

void dhtSensorEventWrapper()
{
  double t = dhtSensor->getTemp();
  double h = dhtSensor->getHumidity();
  // send data to blynk app
  Blynk.virtualWrite(V5, t);
  Blynk.virtualWrite(V6, h);

  // display temp or humidity, the display is too small, so only display one thing at a time
  // The field "toggler" is toggle between temperature and humidity
  if (blockDisplay)
  {
    if (blockDisplay->toggler)
    {
      blockDisplay->showTemp(t);
    }
    else
    {
      blockDisplay->showHumidity(h);
    }

    blockDisplay->toggler = !blockDisplay->toggler;
  }
}




/************************ part have not done yet ************************/
/*
(1) V1 receive data from driveway NodeMcu, V1 handles two things
  - the outside nodemcu online or offline
  - driveway is open or close

(2) need to introduce real-time

(3) code the part that driveway door needs to be closed within the time setup

(4) code the msg replay to app about    Garage/driveway status, etc
*/