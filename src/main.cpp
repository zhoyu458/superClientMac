#define BLYNK_PRINT Serial
#define GARAGE_WARNNING_DISTANCE 200 // less than the value, increase garageWarnningCounter
#define DRIVEWAY_WARNNING_HOUR 22    // less than the value, increase garageWarnningCounter
#define ONE_HOUR 3600000             // an hour in mili seconds
#define ONE_MINITE 60000
#define dht_apin D5 // analog pin
#define ledSwitch D8
#define LIGHT_THRESHOLD_VALUE 1200 // more light, greater value
#define LED_DECK_SYSTEM 1
#define LED_INDOOR_SYSTEM 2
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <Ticker.h>
#include <TM1637Display.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#include "../lib/DHT/dht.h"
#include "../include/sonic_lib/Sonic.h"
#include "../include/event_lib/IntervalEvent.h"
#include "../include/event_lib/TimeoutEvent.h"
#include "../include/datetime_lib/Datetime.h"
#include "../include/components_lib/Led.h"
#include "../include/watchdog_lib/Watchdog.h"
#include "../include/sonic_lib/Sonic.h"
// #include "../include/MYDHT_lib/MYDHT.h" //bugs in the lib.
#include "../include/TM_display_lib/TM_display.h"
#include "../include/mosffet_lib/Mosffet.h"
#include "../include/mosffet_lib/GarageLedSystem.h"
#include "../include/pir_lib/Pir.h"
#include "../include/LDR_lib/LDR.h"
#include "../utilsTool/StringTools.cpp"

/********this line is for test***************/

/*********************************Global**************************/
byte garageWarnningCounter = 0; // if the counter reaches 6,
bool garageNeverReport = true;
bool garageIsClosed = true;
// default delay time is 15 mins after the initial report
unsigned long garageReportDelay = 15;
unsigned long garageReportDelayInMillis = ONE_MINITE * 15;

bool drivewayIsClosed = true;
bool drivewayNodemcuIsOnline = true;
int drivewayWatchdogCount = 0;     // driveway status display "offline" when the variable reaches a certian number
int drivewayNotificationHour = 22; // after 22:00, if driveway is still opened, sends notification to app

byte deckLedOperationModeFromApp = 0; // 0: off, 1: ON 2: auto
int deckLedIntensityFromApp = 0;
bool deckNodeMcuIsOnline = false;
int deckWatchdogCount = 0; // deckWatchdog status display "offline" when the variable reaches a certian number

int ledSystemSelector = 1;

// The selector is used for selecting a certian led system<indoorLed or deckLed or ...> for status updating
// like brightness, opMode and etc.
// 1 for deckLed system, 2 for indoor led, else is reserved.
/*********************************WIFI****************************/
char auth[] = "td2xBB3r_PM3ZnvsSnDUq_ykyLzfvtNB";
char ssid[] = "SPARK-5RUXSX";
char pass[] = "ReliableSloth20VV";

// test:  UPpQ01aooX9O3q3ICgkKEDn_HpJeThPi
// production: td2xBB3r_PM3ZnvsSnDUq_ykyLzfvtNB

/************************Pir sensor***************************************/
Pir *pir = NULL;

/************************LDR sensor***************************************/
LDR *ldr = NULL;

/************************Mosffet***************************************/
// Mosffet *mosffet = NULL;

// GarageLedSystem *garageLedSystem = NULL;

/*****************************pulse pin**************************/
// send pulse to arduino watchdog, prevent power off
Led *pulse = NULL;

/*****************************watch dog**************************/
Watchdog *watchdog = NULL;

/*****************************Sonic Sensor**************************/
SonicSensor *sonic1 = NULL;
SonicSensor *sonic2 = NULL;

/*****************************DHT sensor**************************/
dht *dhtSensor = NULL;

/*****************************TM display**************************/
TM_display *blockDisplay = NULL;

/*****************************Datetime***************************/
Datetime *dt = NULL;

/*****************************intervalEvent**************************/
IntervalEvent *sonicSensorEvent = NULL;
IntervalEvent *dhtSensorEvent = NULL;
IntervalEvent *getRealtimeEvent = NULL;
IntervalEvent *drivewayEvent = NULL;
IntervalEvent *MsgToAppHandler = NULL;
IntervalEvent *drivewayWatchdogEvent = NULL;
IntervalEvent *deckWatchdogEvent = NULL;
IntervalEvent *updateAppTableEvent = NULL;

IntervalEvent *debugEvent = NULL;
/*****************************timeoutEvent**************************/
TimeoutEvent *garageReportEvent = NULL;
TimeoutEvent *syncBridgeEvent = NULL;
TimeoutEvent *syncGeneralEvent = NULL;
TimeoutEvent *turnOffLedSwitchEvent = NULL; //turn off the pin connects to Arduino

/********************************BRIDGE***************************/
WidgetBridge remote_nodemcu_bridge(V1); // serverMCU is the unit connect to the physical remotes

// WidgetBridge indoor_alarm_nodemcu_bridge(V17);

WidgetBridge deck_nodemcu_bridge(V100);

/*****************************BLYNK WRITE**************************/
// V1 receives data from driveway nodemcu, check if it is online.
BLYNK_WRITE(V1) // V1 is used for bridge which receive data from driveway nodemcu to check if it is online.
{
  drivewayIsClosed = param.asInt();
  // Serial.println("receive from driveway mcu");
  drivewayWatchdogCount = 0; //reset driveway_missing_signal_counter, if
  drivewayNodemcuIsOnline = true;
}

//  V2 is a text input terminal on Blynk App, user can open or close garage or driveway by send a msg
WidgetTerminal terminal(V2);

//  V7 terminal is sending msg about garage door, driveway gate and etc info.
WidgetTerminal terminalV7(V7);

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

// // V4 is update garage warnning delay duration. V4 recieves a number from 0 to 60 as delay minutes
BLYNK_WRITE(V4)
{
  garageReportDelay = param.asInt();                          // delay can vary from 1 mins to 15 mins
  garageReportDelayInMillis = garageReportDelay * ONE_MINITE; // convert to mili-seconds
  if (garageReportEvent)
  {
    garageReportEvent->updateTimeoutDuration(garageReportDelayInMillis);
  }
}

// // V8 send msg to turn ON / OFF driveway door
BLYNK_WRITE(V8)
{
  if (param.asInt())
  {
    remote_nodemcu_bridge.virtualWrite(V0, "1001");
  }
}

// // V9 send msg to turn ON / OFF garage door
BLYNK_WRITE(V9)
{
  if (param.asInt())
  {
    remote_nodemcu_bridge.virtualWrite(V0, "1001g");
  }
}

// // V10 receives an integer represent the hour that trigger the driveway is opened notification
BLYNK_WRITE(V10)
{
  drivewayNotificationHour = param.asInt();
  if (drivewayNotificationHour < 0 || drivewayNotificationHour > 23)
  {
    drivewayNotificationHour = 22;
  }
}

// // V12 received info from app to set deck led operation status
BLYNK_WRITE(V12)
{
  if (ledSystemSelector != 1)
    return; // if selector is not 1, then deckLedSystem is not allow to take update.

  deckLedOperationModeFromApp = param.asInt(); // the blynk segment tab start from 0, So have to minus 1
  if (deckLedOperationModeFromApp < 0)
  {
    deckLedOperationModeFromApp = 0;
  }
  else if (deckLedOperationModeFromApp > 2)
  {
    deckLedOperationModeFromApp = 254;
  }
  /************************Write to deckNodeMCU*****************/
  deck_nodemcu_bridge.virtualWrite(V0, deckLedOperationModeFromApp);
}

// // v13 is using to receive data from deck nodemcu <------or more nodemcu----> to check if the system if online or not.
BLYNK_WRITE(V13)
{
  // String msg = param.asStr();
  // received msg from deckMcu is in a format of string: <operation mode>,<intensity>

  // param.asStr() has a format of <opmode>,<intensity>, <opStartHour>,<opStopHour>,.....
  // function: LedSystemInfoParser, return an array contains all info, the interger 4 means there are three pieces of info
  
  String rec = param.asStr();
  // Serial.print("recevie data is ");
  // Serial.println(rec);

  String *ledSystemInfo = LedSystemInfoParser(rec, 4);
  if(ledSystemInfo == NULL){
    Serial.println("ledSystemInfo is NULL at line 223");
    return;
  }
  // String test = String(ledSystemInfo[0]+ ledSystemInfo[1]+ ledSystemInfo[2]+ledSystemInfo[3]);
  // Serial.println(test);
  int ledOpMode = ledSystemInfo[0].toInt();
  int LedIntensity = ledSystemInfo[1].toInt();
  String opHours = String(ledSystemInfo[2] + "-" + ledSystemInfo[3]);
  Blynk.virtualWrite(V12, ledOpMode);
  Blynk.virtualWrite(V14, LedIntensity);
  Blynk.virtualWrite(V20, opHours);

  // int index = msg.indexOf(',');
  // int ledOpMode = msg.substring(0, index + 1).toInt();
  // int LedIntensity = msg.substring(index + 1).toInt();

  if (ledOpMode != deckLedOperationModeFromApp)
  {
    deck_nodemcu_bridge.virtualWrite(V0, deckLedOperationModeFromApp);
  }

  if (LedIntensity != deckLedIntensityFromApp)
  {
    deck_nodemcu_bridge.virtualWrite(V1, deckLedIntensityFromApp); // original was V0 which looks like a bug, update to V1 wait for test.
  }
}

// // V14 is using to receive led intensity from app
BLYNK_WRITE(V14)
{
  if (ledSystemSelector != 1)
    return; // if selector is not 1, then deckLedSystem is not allow to take update.
  deckLedIntensityFromApp = param.asInt();
  deck_nodemcu_bridge.virtualWrite(V1, deckLedIntensityFromApp);
}

// // V15 is using to receive heatbeat from deck nodemcu, rest deckWathdog
BLYNK_WRITE(V15)
{
  param.asInt();
  deckWatchdogCount = 0;
  // receives heatbeat from deckNodeMcu
  deckNodeMcuIsOnline = true;
}

// // V16 taking msg from user App and send to indoor nodemcu to turn ON or OFF the alarm.
// // this part is not coded yet
// BLYNK_WRITE(V16)
// {
//   // turn_on_indoor_alarm = param.asInt();
//   // indoor_alarm_nodemcu_bridge.virtualWrite(V0, turn_on_indoor_alarm);
//   // indoor_nodeMcu_report_theft = false; // anyway, you reset the variable, assuming no theft

//   // if (turn_on_indoor_alarm == HIGH)
//   // {
//   //   //Serial.println("v18 can notify is true");
//   //   v18_can_notify = true;
//   // }
// }

// // V18 works with v16, this part is not ready
// BLYNK_WRITE(V18)
// {
//   // indoor_nodeMcu_report_theft = param.asInt();
//   // if (v18_can_notify)
//   // {
//   //   if (indoor_nodeMcu_report_theft && turn_on_indoor_alarm)
//   //   {
//   //     Blynk.notify("请注意， 屋内检测到有人走动。");
//   //     v18_can_notify = false;
//   //   }
//   // }
// }

// V19 is a selector, select either deckNodeMcu or indoorNodemcu
BLYNK_WRITE(V19)
{
  ledSystemSelector = param.asInt();
  if (ledSystemSelector == LED_DECK_SYSTEM)
  { // 1 for deckLedSystem;
    // pull all from deck LED nodeMcu, include (1) opMode (2) lightBrightnes (3) opearting hour
    // just send a signal to deck nodeMcu to pull all data, the value does not matter
    // then V13 should handle all incoming data and reflect to the App.
    deck_nodemcu_bridge.virtualWrite(V2, 1);
  }
  else if (ledSystemSelector == LED_INDOOR_SYSTEM)
  {
    // pull all from inddor LED nodeMcu
  }
}

//  20 terminal is for updating led system operationg hours, the format is 16:7
WidgetTerminal terminalV20(V20);
BLYNK_WRITE(V20)
{
  // the parse retruen a string array, [0] is start hour and [1] is stop hour
  if(ledSystemSelector != LED_DECK_SYSTEM || ledSystemSelector != LED_INDOOR_SYSTEM){
    return;
  }

  String strHour = param.asStr();
  if (strHour.indexOf("-") == -1)
  {
    terminalV20.print("格式错误，标准模板 18-7");
    terminalV20.flush();
    return;
  }

  String *newHours = ledOpHourParser(strHour, 2);

  if(newHours == NULL){
    Serial.println("newHours ptr is NULL at line 325");
    return;
  }

  // update rules are below
  if (!isNumericString(newHours[0]) || !isNumericString(newHours[1]))
  {
    terminalV20.print("格式错误，标准模板 18-7");
    terminalV20.flush();
    return;
  }

  int startHour = newHours[0].toInt();
  if (startHour < 16)
    startHour = 16;
  else if (startHour > 23)
    startHour = 23;

  int endHour = newHours[1].toInt();
  if (endHour < 5)
    endHour = 5;
  else if (endHour > 9)
    endHour = 9;

  String msg = String(startHour) + "-" + String(endHour);
  terminalV20.print(msg);
  terminalV20.flush();

  if(ledSystemSelector == LED_DECK_SYSTEM){
    deck_nodemcu_bridge.virtualWrite(V3, msg); // V3 is the receving pin on Deck_node_mcu
  }

}
/**********************FUNCTION***************************************************************************/
void sync_bridges()
{
  // sync deck light operation mode and intensity after superclient reboots
  deck_nodemcu_bridge.virtualWrite(V0, deckLedOperationModeFromApp);
  deck_nodemcu_bridge.virtualWrite(V1, deckLedIntensityFromApp);
  // Serial.println("sync all bridges get called");
}
void sync_general()
{
  Blynk.virtualWrite(V19, ledSystemSelector); // send the value back to App for sync
}

void sonicSensorEventWrapper()
{
  if (garageReportEvent)
    garageReportEvent->start(&garageReportEvent);

  sonic1->getDistance() < GARAGE_WARNNING_DISTANCE ? garageWarnningCounter++ : garageWarnningCounter = 0;
  sonic2->getDistance() < GARAGE_WARNNING_DISTANCE ? garageWarnningCounter++ : garageWarnningCounter = 0;

  // Serial.printf("garageWarnningCounter is %d \n", garageWarnningCounter);

  // send notification to app user
  if (garageWarnningCounter >= 10)
  {
    garageIsClosed = false;
    garageWarnningCounter = 10;
    if (garageNeverReport)
    {
      Blynk.notify("车库门被打开，请注意！"); //comment out for debugging
      garageNeverReport = false;
    }
    else // garage has reported before
    {
      if (garageReportEvent == NULL)
      {
        garageReportEvent = new TimeoutEvent(garageReportDelayInMillis, []() -> void { Blynk.notify("车库门被打开，请注意！"); }); //Blynk.notify("车库门被打开，请注意！");
      }
    }
  }
  // garageWarnningCounter <= 0, garage door is fully closed
  else if (garageWarnningCounter <= 0)
  {
    garageWarnningCounter = 0;
    garageIsClosed = true;
    garageNeverReport = true;
    if (garageReportEvent)
    {
      garageReportEvent->kill(&garageReportEvent);
    }
  }
}

void updateAppTableEventWrapper()
{
  // update value to blynk table
  Blynk.virtualWrite(V11, "update", 1, "感应距离1", sonic1->getDistance());
  Blynk.virtualWrite(V11, "update", 2, "感应距离2", sonic2->getDistance());

  String msg = deckNodeMcuIsOnline ? "在线" : "离线";
  Blynk.virtualWrite(V11, "update", 3, "甲板led", msg);
}

void dhtSensorEventWrapper()
{
  dhtSensor->read11(dht_apin);
  double t = dhtSensor->temperature;
  double h = dhtSensor->humidity;

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

void drivewayEventWrapper()
{
  byte hour = dt->getHours();
  // initially hour is getting 255, trigger an unexpected notification.
  if (hour <= 23 && hour >= 21)
  {
    if (hour >= drivewayNotificationHour)
    {
      if (drivewayIsClosed == false && drivewayNodemcuIsOnline)
      {
        Blynk.notify("入口打开，晚上请关闭");
      }
    }
  }
}

void drivewayWatchdogEventWrapper()
{
  if (drivewayWatchdogCount >= 10)
  {
    drivewayNodemcuIsOnline = false;
    drivewayIsClosed = true; // driveway nodemcu is offline, we assume the door is closed
    drivewayWatchdogCount = 10;
  }
  drivewayWatchdogCount++;
}

void MsgToAppHandlerWrapper()
{
  //attach garage door status
  String reply = "车库:";
  reply += (!garageIsClosed) ? "开" : "关";
  // attach the notification interval
  reply += "(" + String(garageReportDelay) + "分)";

  //attach driveway door status
  reply += "--入口:";
  reply += (!drivewayIsClosed) ? "开" : "关";

  // attache drivewayNodeMcu online status
  if (drivewayNodemcuIsOnline)
  {
    reply += "(在线,";
    reply += "门禁:";
    reply += String(drivewayNotificationHour) + ":00)";
  }
  else
  {
    reply += "(离线)";
  }

  terminalV7.print(reply);
  terminalV7.flush();
}

void deckWatchdogEventWrapper()
{
  deckWatchdogCount++;
  if (deckWatchdogCount >= 10)
  {
    deckWatchdogCount = 10;
    deckNodeMcuIsOnline = false;
  }
}

void debugEventWrapper()
{
  // if(ldr)
  // Serial.printf("ldr value is %d\n", ldr->getValue());
  // if(pir)
  // Serial.printf("pir sensor  %d\n", pir->detect());
  // // if(mosffet)
  // //  Serial.printf("mosffet value  %d\n\n", mosffet->_currentPwm);
  //    Serial.printf("D8 value  %d\n\n", digitalRead(D8));
}
/**********************************************************************************************************/

/*****************************BLYNK CONNECT**************************/
BLYNK_CONNECTED()
{
  remote_nodemcu_bridge.setAuthToken("20e0a85a41fa4fc7b7c93a53576bb2c3"); // Place the AuthToken of the second hardware here
  deck_nodemcu_bridge.setAuthToken("Ah-lQpqdle6DvDC68M0qhofsS-8fD4KU");   // Place the AuthToken of the second hardware here
  //indoor_alarm_nodemcu_bridge.setAuthToken("s7u2S42iA0LXum1WqX36Vjwu7QE2OP1v");
  Blynk.syncAll();
}

void setup()
{
  pulse = new Led(D0);
  watchdog = new Watchdog();
  sonic1 = new SonicSensor(D6, D7);
  sonic2 = new SonicSensor(D1, D2);
  dhtSensor = new dht();
  blockDisplay = new TM_display(D3, D4);
  dt = new Datetime(ssid, pass);
  pir = new Pir(10);
  ldr = new LDR(A0);
  pinMode(ledSwitch, OUTPUT);
  digitalWrite(ledSwitch, LOW);
  // garageLedSystem = new GarageLedSystem(mosffet, pir, ldr);

  getRealtimeEvent = new IntervalEvent(60000, [&]() -> void { dt->updateHours(); });
  sonicSensorEvent = new IntervalEvent(2000, sonicSensorEventWrapper);
  dhtSensorEvent = new IntervalEvent(5000, dhtSensorEventWrapper);
  drivewayEvent = new IntervalEvent(ONE_HOUR, drivewayEventWrapper);
  MsgToAppHandler = new IntervalEvent(2000, MsgToAppHandlerWrapper);
  drivewayWatchdogEvent = new IntervalEvent(1500, drivewayWatchdogEventWrapper);
  deckWatchdogEvent = new IntervalEvent(1500, deckWatchdogEventWrapper);
  updateAppTableEvent = new IntervalEvent(5000, updateAppTableEventWrapper);

  syncBridgeEvent = new TimeoutEvent(15000, [&]() -> void { sync_bridges(); });
  syncGeneralEvent = new TimeoutEvent(15000, [&]() -> void { sync_general(); });

  debugEvent = new IntervalEvent(1500, debugEventWrapper);
  // setup Serial port
  Serial.begin(9600);

  // setup blynk
  Blynk.begin(auth, ssid, pass);
  // initiate the table, V11 is a table on Blynk App
  Blynk.virtualWrite(V11, "clr");
  Blynk.virtualWrite(V11, "add", 1, "感应距离1", "test");
  Blynk.virtualWrite(V11, "add", 2, "感应距离2", "test");
  Blynk.virtualWrite(V11, "add", 3, "甲板控制系统", "test");
  // if nodemcu reboots, check the driveway and send msg if needs.
  drivewayEventWrapper();
  // send values to relevant bridges, sync_bridge causes internet disconnect,
  // a timeout event will handle this properly
  //  ----------sync_bridges();---------------

  // update time at the begining
  // print a start message
  if (dt)
  {
    dt->updateHours();
  }
  Serial.print("Current hour is: ");
  Serial.println(dt->getHours());
  Serial.println("Table is configured, ready to start");
}

void loop()
{
  Blynk.run();
  if (syncBridgeEvent)
    syncBridgeEvent->start(&syncBridgeEvent);

  if (syncGeneralEvent)
  {
    syncGeneralEvent->start(&syncGeneralEvent);
  }

  if (debugEvent) // working
    debugEvent->start();

  if (pulse) // working
    pulse->blink();

  if (watchdog) // working
    watchdog->start();

  if (getRealtimeEvent) // working
    getRealtimeEvent->start();

  if (sonicSensorEvent) // working
    sonicSensorEvent->start();

  if (dhtSensorEvent) //  working 
    dhtSensorEvent->start();

  if (deckWatchdogEvent) // working
    deckWatchdogEvent->start();

  if (drivewayWatchdogEvent)
    drivewayWatchdogEvent->start();

  if (MsgToAppHandler) // working
    MsgToAppHandler->start();

  if (drivewayEvent) //working
    drivewayEvent->start();

  if (updateAppTableEvent) // working
    updateAppTableEvent->start();

  if (turnOffLedSwitchEvent)
    turnOffLedSwitchEvent->start(&turnOffLedSwitchEvent);

  if (ldr->getValue() < LIGHT_THRESHOLD_VALUE)
  {
     Serial.printf("1111.pir is: %d\n", pir->detect()); // do not delete this line, otherwise led does not turn off
    if (pir->detect() == HIGH)
    {
      if (turnOffLedSwitchEvent == NULL)
      {
        digitalWrite(ledSwitch, HIGH);
        turnOffLedSwitchEvent = new TimeoutEvent(20000, [&]() -> void { digitalWrite(ledSwitch, LOW); });
      }
      else
      {
        turnOffLedSwitchEvent->resetEventClock();
      }
    }
  }
  else
  {
    digitalWrite(ledSwitch, LOW);
    if (turnOffLedSwitchEvent)
      turnOffLedSwitchEvent->kill(&turnOffLedSwitchEvent);
  }
  // if(garageLedSystem)
  // garageLedSystem->run();
}

/************************ part have not done yet ************************/
/*
(1) V1 receive data from driveway NodeMcu, V1 handles two things
  - the outside nodemcu online or offline
  - driveway is open or close(Done)

(2) need to introduce real-time(Done)

(3) code the part that driveway door needs to be closed within the time setup

(4) code the msg replay to app about    Garage/driveway status, etc
*/