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

#define LED_GARAGE_SYSTEM 3

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <Ticker.h>
#include <TM1637Display.h>
#include <NTPClient.h>
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
// #include "../include/LDR_lib/LDR.h"
#include "../utilsTool/StringTools.cpp"

/********CONST ***************/
const String INDOOR_SYSTEM_ID = "indoorLed";
const String DECK_SYSTEM_ID = "deckLed";
const String GARAGE_SYSTEM_ID = "garageLed";

const char *superClientToken = "oreBMF8O88yybTR2uWLrLly_cGqRt1DE";
// public token: td2xBB3r_PM3ZnvsSnDUq_ykyLzfvtNB
// local  token: oreBMF8O88yybTR2uWLrLly_cGqRt1DE
const char *RemoteToken = "OgFV_WD9Z0LCz2Ih66BXxoDN6qAtm9u4";
// public token: 20e0a85a41fa4fc7b7c93a53576bb2c3
// local  token: OgFV_WD9Z0LCz2Ih66BXxoDN6qAtm9u4
const char *deckToken = "R604IZ48t4elfnb0CovtxXgpgipN69FN";
// public token:  Ah-lQpqdle6DvDC68M0qhofsS-8fD4KU
// local  token:  R604IZ48t4elfnb0CovtxXgpgipN69FN
const char *indoorToken = "bFM0IonFSGovu6XXkS0eFhixztHxktsY";
// public token: s7u2S42iA0LXum1WqX36Vjwu7QE2OP1v
// local  token:  bFM0IonFSGovu6XXkS0eFhixztHxktsY

/*********************************Global**************************/
// byte garageWarnningCounter = 0; // if the counter reaches 6,
byte garageOpenCounter = 0;  // count plus 1 if measured distance is less then the value
byte garageCloseCounter = 0; // count plus 1 if measured distance is less then the value
byte sonicCounter = 0;       // count plus 1 every time sonic sensor take measurement. planning to sample 10 measurement then check if garage is open or closed
// this is due to unstable meansurement from the sonic sensor.

bool garageNeverReport = true;
bool garageIsClosed = true;
// default delay time is 15 mins after the initial report
unsigned long garageReportDelay = 15;
unsigned long garageReportDelayInMillis = ONE_MINITE * 15;

bool drivewayIsClosed = true;
bool drivewayNodemcuIsOnline = true;
int drivewayWatchdogCount = 0;     // driveway status display "offline" when the variable reaches a certian number
int drivewayNotificationHour = 22; // after 22:00, if driveway is still opened, sends notification to app

int deckLedOpMode = 3; // 1: off, 2: ON 3: auto
int deckLedLight = 500;
bool deckMcuOnline = false;
int deckWatchdogCount = 0; // deckWatchdog status display "offline" when the variable reaches a certian number
int deckLedStartHour = 17;
int deckLedStopHour = 8;

int indoorLedOpMode = 3;
int indoorLedLight = 100;
bool indoorMcuOnline = false;
int indoorWatchdogCount = 0; // deckWatchdog status display "offline" when the variable reaches a certian number
int indoorLedStartHour = 17;
int indoorLedStopHour = 8;

// 0: off, 1: ON 2: auto
// led in the garage only take operation mode, no other paramter
// set auto as the default mode
int garageLedOpMode = 3;
int garageLedLight = 0; // the value does not mean anything
int garageLedStartHour = 16; // the value does not mean anything
int garageLedStopHour = 4; // the value does not mean anything
// the var is for solving garage led does not turn off from hard_on to auto
bool garageLedOpModeFromHardOnToAutoTransition = false;  

// The selector is used for selecting a certian led system<indoorLed or deckLed or ...> for status updating
// like brightness, opMode and etc.
// 1 for deckLed system, 2 for indoor led, else is reserved.
int ledSystemSelector = 1;

String deckLedInfo = "";
String indoorLedInfo = "";
String garageLedInfo = "";

/***************************FUNCTION PROTOTYPE**************************/
void syncLedDeckSystemBySending();
void syncAllMcuBySending();
void syncLedIndoorSystemBySending();
void syncAppLedSystemSelector();
void syncLedGarageSystem();

void sonicSensorEventWrapper();
void updateAppTableEventWrapper();
void dhtSensorEventWrapper();
void drivewayEventWrapper();
void drivewayWatchdogEventWrapper();
void MsgToAppHandlerWrapper();
void deckWatchdogEventWrapper();
void debugEventWrapper();
void indoorWatchdogEventWrapper();
void updateV127Data();
void debugTerminalPrintEventWrapper();

/*********************************WIFI****************************/
const char *auth = superClientToken;
const char *ssid = "SPARK-5RUXSX";
const char *pass = "ReliableSloth20VV";
const char *blynkserver = "192.168.1.132";
int port = 8080;
// test:  UPpQ01aooX9O3q3ICgkKEDn_HpJeThPi
// production: td2xBB3r_PM3ZnvsSnDUq_ykyLzfvtNB

/************************Pir sensor***************************************/
Pir *pir = NULL;

/************************LDR sensor***************************************/
// LDR *ldr = NULL;

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
IntervalEvent *indoorwatchDogEvent = NULL;
/*****************************timeoutEvent**************************/
TimeoutEvent *garageReportEvent = NULL;
TimeoutEvent *syncBridgeEvent = NULL;
TimeoutEvent *syncGeneralEvent = NULL;
TimeoutEvent *turnOffLedSwitchEvent = NULL; //turn off the pin connects to Arduino

/********************************BRIDGE***************************/
WidgetBridge remote_nodemcu_bridge(V1); // serverMCU is the unit connect to the physical remotes

WidgetBridge deck_nodemcu_bridge(V100);

WidgetBridge indoor_nodemcu_bridge(V101);

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

// // V12 received info from app to set led system to an operation mode
BLYNK_WRITE(V12)
{
  // 1 for deck led. 2 for indoor led
  if (ledSystemSelector != 1 && ledSystemSelector != 2 && ledSystemSelector != 3)
    return;

  int opMode = param.asInt();
  if (opMode != 1 && opMode != 2)
  {
    opMode = 3;
  }

  if (ledSystemSelector == 1)
  {
    deckLedOpMode = opMode;
    deck_nodemcu_bridge.virtualWrite(V0, deckLedOpMode);
  }
  else if (ledSystemSelector == 2)
  {
    indoorLedOpMode = opMode;
    indoor_nodemcu_bridge.virtualWrite(V0, indoorLedOpMode);
  }
  else if (ledSystemSelector == 3)
  {
    garageLedOpMode = opMode;
  }
  updateV127Data();
}

// // v13 is using to receive data from deck nodemcu <------or more nodemcu----> to check if the system if online or not.
BLYNK_WRITE(V13)
{
  // param.asStr() has a format of <systemId><opmode>,<intensity>, <opStartHour>,<opStopHour>,.....
  // function: LedSystemInfoParser, return an array contains all info, the interger 4 means there are three pieces of info

  String rec = param.asStr();
  // Serial.print("recevie data is ");
  // Serial.println(rec);

  String *ledSystemInfo = LedSystemInfoParser(rec, 5);
  if (ledSystemInfo == NULL)
  {
    Serial.println("ledSystemInfo is NULL at line 223");
    return;
  }
  // String test = String(ledSystemInfo[0]+ ledSystemInfo[1]+ ledSystemInfo[2]+ledSystemInfo[3]);
  // Serial.println(test);
  // senderId can be deckLed OR indoorLed
  // String senderId = ledSystemInfo[0]; // currently not useful but keep it inside the code
  int ledOpMode = ledSystemInfo[1].toInt();
  int ledIntensity = ledSystemInfo[2].toInt();
  int ledStartHour = ledSystemInfo[3].toInt();
  int ledStopHour = ledSystemInfo[4].toInt();

  Blynk.virtualWrite(V12, ledOpMode);
  Blynk.virtualWrite(V14, ledIntensity);
  Blynk.virtualWrite(V20, ledStartHour);
  Blynk.virtualWrite(V21, ledStopHour);
}

// // V14 is using to receive led intensity from app
BLYNK_WRITE(V14)
{

  if (ledSystemSelector == LED_DECK_SYSTEM)
  {
    deckLedLight = param.asInt();
    deck_nodemcu_bridge.virtualWrite(V1, deckLedLight);
    updateV127Data();
    return;
  }

  if (ledSystemSelector == LED_INDOOR_SYSTEM)
  {
    indoorLedLight = param.asInt();
    indoor_nodemcu_bridge.virtualWrite(V1, indoorLedLight);
    updateV127Data();
    return;
  }
}

// // V15 is using to receive heatbeat from deck nodemcu, rest deckWathdog
BLYNK_WRITE(V15)
{
  String sysId = param.asStr();
  if (sysId == DECK_SYSTEM_ID)
  {
    deckWatchdogCount = 0;
    // receives heatbeat from deckNodeMcu
    deckMcuOnline = true;
    return;
  }

  if (sysId == INDOOR_SYSTEM_ID)
  {
    indoorWatchdogCount = 0;
    indoorMcuOnline = true;
    return;
  }
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
    return;
  }

  if (ledSystemSelector == LED_INDOOR_SYSTEM)
  {
    indoor_nodemcu_bridge.virtualWrite(V2, 1);
    return;
  }

  // The ledSystemSelect will be 254 if reach to this line, we force select = LED_GARAGE_SYSTEM
  ledSystemSelector = LED_GARAGE_SYSTEM;
  syncLedGarageSystem();
  // pull the operation status the LED_GARAGE_SYSTEM
}

// V20 defines the led start hour in the afernoon. It is a slide bar on the app
BLYNK_WRITE(V20)
{
  if (ledSystemSelector != LED_DECK_SYSTEM && ledSystemSelector != LED_INDOOR_SYSTEM)
  {
    return;
  }

  if (ledSystemSelector == LED_DECK_SYSTEM)
  {
    deckLedStartHour = param.asInt();
    // Serial.println(deckLedStartHour);
    deck_nodemcu_bridge.virtualWrite(V3, deckLedStartHour);
    updateV127Data();
    return;
  }

  if (ledSystemSelector == LED_INDOOR_SYSTEM)
  {
    indoorLedStartHour = param.asInt();
    Serial.println(indoorLedStartHour);
    indoor_nodemcu_bridge.virtualWrite(V3, indoorLedStartHour);
    updateV127Data();
    return;
  }
}

// V21 defines the led stop hour in the afernoon.  It is a slide bar on the app
BLYNK_WRITE(V21)
{
  if (ledSystemSelector == LED_DECK_SYSTEM)
  {
    deckLedStopHour = param.asInt();
    deck_nodemcu_bridge.virtualWrite(V4, deckLedStopHour);
    updateV127Data();
    return;
  }

  if (ledSystemSelector == LED_INDOOR_SYSTEM)
  {
    indoorLedStopHour = param.asInt();
    indoor_nodemcu_bridge.virtualWrite(V4, indoorLedStopHour);
    updateV127Data();
    return;
  }
}

// V22 for other mcu to pull data from superClient
BLYNK_WRITE(V22)
{
  String msg = param.asStr();
  if (msg == "deckLed")
  {
    // send all data related to deck led system
    syncLedDeckSystemBySending();
    return;
  }

  if (msg == "indoorLed")
  {
    syncLedIndoorSystemBySending();
    return;
  }
}
WidgetTerminal debugTerminal(V102);
IntervalEvent *debugTerminalPrintEvent = NULL;
// V127 stores info of deckLed with indexof 0 and indoorled with index of 1
BLYNK_WRITE(V127)
{
  deckLedInfo = param[0].asStr();
  indoorLedInfo = param[1].asStr();
  garageLedInfo = param[2].asStr();

  // info structure
  // (1) system_id   [0]
  // (2) opMode      [1]
  // (3) light       [2]
  // (4) startHour   [3]
  // (5) stopHour    [4]
  String *info = LedSystemInfoParser(deckLedInfo, 5);
  // update info on deck led system
  deckLedOpMode = info[1].toInt();
  deckLedLight = info[2].toInt();
  deckLedStartHour = info[3].toInt();
  deckLedStopHour = info[4].toInt();
  syncLedDeckSystemBySending();

  // update info on indoor led system
  info = LedSystemInfoParser(indoorLedInfo, 5);
  indoorLedOpMode = info[1].toInt();
  indoorLedLight = info[2].toInt();
  indoorLedStartHour = info[3].toInt();
  indoorLedStopHour = info[4].toInt();
  syncLedIndoorSystemBySending();

  // update info on garage led system
  info = LedSystemInfoParser(garageLedInfo, 5);
  // led garage system is direct link to the nodemcu
  garageLedOpMode = info[1].toInt();
  garageLedLight = info[2].toInt();
  garageLedStartHour = info[3].toInt();
  garageLedStopHour = info[4].toInt();
  syncLedGarageSystem();
}

// the syncEvent trigger 30 sec after the system start to make sure sync is done properly to other nodeMcu

TimeoutEvent *syncEvent = NULL;
/*****************************BLYNK CONNECT**************************/
BLYNK_CONNECTED()
{
  remote_nodemcu_bridge.setAuthToken(RemoteToken); // Place the AuthToken of the second hardware here
  deck_nodemcu_bridge.setAuthToken(deckToken);     // Place the AuthToken of the second hardware here
  indoor_nodemcu_bridge.setAuthToken(indoorToken);
}

void setup()
{
  // setup Serial port
  Serial.begin(9600);
  // setup blynk
  Blynk.begin(auth, ssid, pass, blynkserver, port);
  pulse = new Led(D0);
  watchdog = new Watchdog();
  sonic1 = new SonicSensor(D6, D7);
  sonic2 = new SonicSensor(D1, D2);
  dhtSensor = new dht();
  blockDisplay = new TM_display(D3, D4);
  dt = new Datetime(ssid, pass);
  pir = new Pir(10);
  // ldr = new LDR(A0);
  pinMode(ledSwitch, OUTPUT);
  digitalWrite(ledSwitch, LOW);
  // garageLedSystem = new GarageLedSystem(mosffet, pir, ldr);

  getRealtimeEvent = new IntervalEvent(60000, [&]() -> void
                                       { dt->updateHours(); });
  sonicSensorEvent = new IntervalEvent(2000, sonicSensorEventWrapper);
  dhtSensorEvent = new IntervalEvent(5000, dhtSensorEventWrapper);
  drivewayEvent = new IntervalEvent(ONE_HOUR, drivewayEventWrapper);
  MsgToAppHandler = new IntervalEvent(2000, MsgToAppHandlerWrapper);
  drivewayWatchdogEvent = new IntervalEvent(1500, drivewayWatchdogEventWrapper);
  deckWatchdogEvent = new IntervalEvent(1500, deckWatchdogEventWrapper);

  indoorwatchDogEvent = new IntervalEvent(1500, deckWatchdogEventWrapper);

  updateAppTableEvent = new IntervalEvent(3000, updateAppTableEventWrapper);

  syncBridgeEvent = new TimeoutEvent(10000, [&]() -> void
                                     { syncAllMcuBySending(); });
  syncGeneralEvent = new TimeoutEvent(10000, [&]() -> void
                                      { syncAppLedSystemSelector(); });
  debugEvent = new IntervalEvent(1500, debugEventWrapper);

  debugTerminalPrintEvent = new IntervalEvent(5000, debugTerminalPrintEventWrapper);
  syncEvent = new TimeoutEvent(30000, [&]() -> void
                               { Blynk.syncAll(); });

  // initiate the table, V11 is a table on Blynk App
  Blynk.virtualWrite(V11, "clr");
  Blynk.virtualWrite(V11, "add", 1, "感应距离1", "无数据");
  Blynk.virtualWrite(V11, "add", 2, "感应距离2", "无数据");
  Blynk.virtualWrite(V11, "add", 3, "甲板led", "无数据");
  Blynk.virtualWrite(V11, "add", 4, "室内led", "无数据");
  // if nodemcu reboots, check the driveway and send msg if needs.
  drivewayEventWrapper();
  // send values to relevant bridges, sync_bridge causes internet disconnect,
  // a timeout event will handle this properly
  //  ----------syncAllMcuBySending();---------------

  // update time at the begining
  // print a start message
  if (dt)
  {
    dt->updateHours();
  }
  Serial.print("Current hour is: ");
  Serial.println(dt->getHours());
  Serial.println("Table is configured, ready to start");

  Blynk.syncAll();
  // Blynk.syncVirtual(V127);
} // setup ends here

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

  if (debugTerminalPrintEvent)
    debugTerminalPrintEvent->start();
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

  if (indoorwatchDogEvent)
    indoorwatchDogEvent->start();

  if (syncEvent)
    syncEvent->start(&syncEvent);

  // if (ldr->getValue() < LIGHT_THRESHOLD_VALUE)

  if (garageLedOpMode == 1)
  {
    if(turnOffLedSwitchEvent) turnOffLedSwitchEvent->kill(&turnOffLedSwitchEvent);
    digitalWrite(ledSwitch, LOW);
  }
  else if (garageLedOpMode == 2)
  {
    if(turnOffLedSwitchEvent) turnOffLedSwitchEvent->kill(&turnOffLedSwitchEvent);
    digitalWrite(ledSwitch, HIGH);
    garageLedOpModeFromHardOnToAutoTransition = true;
    
  }
  else if (garageLedOpMode == 3)
  {
    if(garageLedOpModeFromHardOnToAutoTransition == true){
      digitalWrite(ledSwitch, LOW);
      garageLedOpModeFromHardOnToAutoTransition = false;
    }
    Serial.printf("pir is: %d\n", pir->detect()); // do not delete this line, otherwise led does not turn off
    if (pir->detect() == HIGH)
    {
      if (turnOffLedSwitchEvent == NULL)
      {
        digitalWrite(ledSwitch, HIGH);
        turnOffLedSwitchEvent = new TimeoutEvent(20000, [&]() -> void
                                                 { digitalWrite(ledSwitch, LOW); });
      }
      else
      {
        turnOffLedSwitchEvent->resetEventClock();
      }
    }
  }
  // {

  // }
  // else
  // {
  //   digitalWrite(ledSwitch, LOW);
  //   if (turnOffLedSwitchEvent)
  //     turnOffLedSwitchEvent->kill(&turnOffLedSwitchEvent);
  // }
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

/*********************************************************************************************************/
/**********************FUNCTION***************************************************************************/
void syncLedDeckSystemBySending()
{
  deck_nodemcu_bridge.virtualWrite(V0, deckLedOpMode);
  deck_nodemcu_bridge.virtualWrite(V1, deckLedLight);
  deck_nodemcu_bridge.virtualWrite(V3, deckLedStartHour);
  deck_nodemcu_bridge.virtualWrite(V4, deckLedStopHour);
}

void syncLedIndoorSystemBySending()
{
  indoor_nodemcu_bridge.virtualWrite(V0, indoorLedOpMode);
  indoor_nodemcu_bridge.virtualWrite(V1, indoorLedLight);
  indoor_nodemcu_bridge.virtualWrite(V3, indoorLedStartHour);
  indoor_nodemcu_bridge.virtualWrite(V4, indoorLedStopHour);
}

void syncLedDeckSystemBySending(int op, int light, int startHour, int stopHour)
{
  deck_nodemcu_bridge.virtualWrite(V0, op);
  deck_nodemcu_bridge.virtualWrite(V1, light);
  deck_nodemcu_bridge.virtualWrite(V3, startHour);
  deck_nodemcu_bridge.virtualWrite(V4, stopHour);
}

void syncLedIndoorSystemBySending(int op, int light, int startHour, int stopHour)
{
  indoor_nodemcu_bridge.virtualWrite(V0, op);
  indoor_nodemcu_bridge.virtualWrite(V1, light);
  indoor_nodemcu_bridge.virtualWrite(V3, startHour);
  indoor_nodemcu_bridge.virtualWrite(V4, stopHour);
}

void syncLedGarageSystem()
{
  // garage led is direct connect to the nodeMcu, so Virtual pin number is different from above
  Blynk.virtualWrite(V12, garageLedOpMode);
  Blynk.virtualWrite(V14, garageLedLight);
  Blynk.virtualWrite(V20, garageLedStartHour);
  Blynk.virtualWrite(V21, garageLedStopHour);
}

void syncAllMcuBySending()
{
  syncLedDeckSystemBySending();
  syncLedIndoorSystemBySending();
  syncLedGarageSystem();

}

// send ledSystemSelector value to the App if the superclient restart
void syncAppLedSystemSelector()
{
  Blynk.virtualWrite(V19, ledSystemSelector); // send the value back to App for sync
}

// send all info related to deck led system

void sonicSensorEventWrapper()
{
  if (garageReportEvent)
    garageReportEvent->start(&garageReportEvent);

  sonic1->getDistance() < GARAGE_WARNNING_DISTANCE ? garageOpenCounter++ : garageCloseCounter++;
  sonic2->getDistance() < GARAGE_WARNNING_DISTANCE ? garageOpenCounter++ : garageCloseCounter++;
  sonicCounter++;
  if (sonicCounter == 12)
  {
    if (garageCloseCounter >= 8)
    {
      // garageWarnningCounter = 0;
      garageIsClosed = true;
      garageNeverReport = true;
      if (garageReportEvent)
      {
        garageReportEvent->kill(&garageReportEvent);
      }
    }
    else
    {
      garageIsClosed = false;
      // garageWarnningCounter = 10;
      if (garageNeverReport)
      {
        Blynk.notify("车库门被打开，请注意！"); //comment out for debugging
        garageNeverReport = false;
      }
      else // garage has reported before
      {
        if (garageReportEvent == NULL)
        {
          garageReportEvent = new TimeoutEvent(garageReportDelayInMillis, []() -> void
                                               { Blynk.notify("车库门被打开，请注意！"); }); //Blynk.notify("车库门被打开，请注意！");
        }
      }
    }
    sonicCounter = 0;
    garageOpenCounter = 0;
    garageCloseCounter = 0;
  }

  // Serial.printf("garageWarnningCounter is %d \n", garageWarnningCounter);

  // send notification to app user
  //   if (garageWarnningCounter >= 10)
  //   {
  //     garageIsClosed = false;
  //     garageWarnningCounter = 10;
  //     if (garageNeverReport)
  //     {
  //       Blynk.notify("车库门被打开，请注意！"); //comment out for debugging
  //       garageNeverReport = false;
  //     }
  //     else // garage has reported before
  //     {
  //       if (garageReportEvent == NULL)
  //       {
  //         garageReportEvent = new TimeoutEvent(garageReportDelayInMillis, []() -> void
  //                                              { Blynk.notify("车库门被打开，请注意！"); }); //Blynk.notify("车库门被打开，请注意！");
  //       }
  //     }
  //   }
  //   // garageWarnningCounter <= 0, garage door is fully closed
  //   else if (garageWarnningCounter <= 0)
  //   {
  //     garageWarnningCounter = 0;
  //     garageIsClosed = true;
  //     garageNeverReport = true;
  //     if (garageReportEvent)
  //     {
  //       garageReportEvent->kill(&garageReportEvent);
  //     }
  //   }
} // this is the end of the function

void updateAppTableEventWrapper()
{
  // update value to blynk table
  Blynk.virtualWrite(V11, "update", 1, "感应距离1", sonic1->getDistance());
  Blynk.virtualWrite(V11, "update", 2, "感应距离2", sonic2->getDistance());

  if (!deckMcuOnline)
  {
    Blynk.virtualWrite(V11, "update", 3, "甲板led", "离线");
  }
  else
  {
    String msg = String(deckLedStartHour - 12) + "pm->" + String(deckLedStopHour) + "am";
    Blynk.virtualWrite(V11, "update", 3, "甲板led", msg);
  }

  if (!indoorMcuOnline)
  {
    Blynk.virtualWrite(V11, "update", 4, "室内led", "离线");
  }
  else
  {
    String msg = String(indoorLedStartHour - 12) + "pm->" + String(indoorLedStopHour) + "am";
    Blynk.virtualWrite(V11, "update", 4, "室内led", msg);
  }
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
    deckMcuOnline = false;
  }
}

void indoorWatchdogEventWrapper()
{
  indoorWatchdogCount++;
  if (indoorWatchdogCount >= 10)
  {
    indoorWatchdogCount = 10;
    indoorMcuOnline = false;
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

void updateV127Data()
{
  deckLedInfo = DECK_SYSTEM_ID +
                "," +
                String(deckLedOpMode) +
                "," +
                String(deckLedLight) +
                "," +
                String(deckLedStartHour) +
                "," +
                String(deckLedStopHour);

  indoorLedInfo = INDOOR_SYSTEM_ID +
                  "," +
                  String(indoorLedOpMode) +
                  "," +
                  String(indoorLedLight) +
                  "," +
                  String(indoorLedStartHour) +
                  "," +
                  String(indoorLedStopHour);

  garageLedInfo = GARAGE_SYSTEM_ID +
                  "," +
                  String(garageLedOpMode) +
                  "," +
                  String(garageLedLight) + // 240 is just a random value, the led is not allow to change light
                  "," +
                  String(garageLedStartHour) + // the value does not matter
                  "," +
                  String(garageLedStopHour); // the value does not matter
  // Blynk.virtualWrite(V127, "", ""); do not uncomment this line
  Blynk.virtualWrite(V127, deckLedInfo, indoorLedInfo, garageLedInfo);
}

void debugTerminalPrintEventWrapper()
{
  debugTerminal.println(deckLedInfo);
  debugTerminal.println(indoorLedInfo);
  debugTerminal.println(garageLedInfo);

  debugTerminal.println("-----------");
}
/**********************************************************************************************************/
