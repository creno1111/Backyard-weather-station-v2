/************************************************************
 * project: Weather Station V2.0
 * 2nd iteration of Arduino based, comon parts plus 3d Printed enclosre.
 *  * WSv1: First weather station was sepperated into different sensor packages
 *  and a RaspberryPI was used w/graphana as a dispaly.
 *  * WSv2 [CURRENT], This version is all-in-one enclosure with embedded ESP32
 *  webserver serving up a dynamic evenromental weather page with history
 *  and graphs.
 *  
 * Creator: CReno
 * Date: 2021-01-01 - present
 * [NOTE: im a hobbyist, not a professional programmer, so please excuse my coding style, etc]
 * 
 * esp32 based, common off the shelf parts (arduino based)
 * 
 * FEATURES:
 * humidity, temperature, barometric pressure, wind speed, wind direction
 *  History graphing, OTA updates, config portal, NTP time sync
 *  WiFi, WebSocket, WebServer, JSON, SPI, OTA, NTP, MDNS,
 *  DNS, AP, TCP, UDP, HTTP, SPIFFS, GPIO, PWM, ADC
 * monthly data FS logging file updated every 15 minutes,
 *  minute data display, 24 hour data display
 *  every 0.01-3 seconds sensors are recorded(ram)
 * PWS Upload to WU PWS protocol sites (Wunderground.com, PWSweather.com, etc)
 * NSW weather warnings, weather forecast, hourly data
 * Windy.com built in Lightning, Radar, Temperature, Rain Acc, Wind
 * Built-in FS(FW) utility. OTA(FW,Partition, FS File),
 *  FS List,Size,Upload,Download 
 * 3D printed enclosure 
 * 
 * 
 * TODO:
 * - add wireless ESP32 epaper weather display project (tbd)
 * - add page to display FS historic data
 * - add FS low space detection and auto FS history cleanup(fifo)
 * - misc code cleanup, ugh - im a lazy coder :)
 * 
 * Libraries
 * ├── ArduinoJson @ 6.21.1 (required: bblanchon/ArduinoJson @ ^6.21.1)
 * ├── BME280 @ 3.0.0 (required: finitespace/BME280 @ ^3.0.0)
 * ├── CircularBuffer @ 1.3.3 (required: rlogiacco/CircularBuffer @ ^1.3.3)
 * ├── ESP_WifiManager @ 1.10.2 (required: khoih-prog/ESP_WiFiManager @ ^1.3.0)
 * │   ├── ESP_DoubleResetDetector @ 1.3.1 (required: khoih-prog/ESP_DoubleResetDetector @ >=1.3.1)
 * │   │   └── LittleFS_esp32 @ 1.0.6 (required: lorol/LittleFS_esp32 @ ^1.0.6)
 * ├── ElegantOTA @ 2.2.9 (required: ayushsharma82/ElegantOTA @ ^2.2.7)
 * ├── NTPClient @ 3.2.1 (required: arduino-libraries/NTPClient @ ^3.1.0)
 * ├── WebSockets @ 2.3.7 (required: links2004/WebSockets @ ^2.3.6)
 * ├── ezTime @ 0.8.3 (required: ropg/ezTime @ ^0.8.3)
 * ├── Chiper.cpp Created on: Feb 28, 2019 Author: joseph
 * └── BatteryRead Copyright (c) 2019 Pangodream
 * 
 * MIT License
*/

#include <Arduino.h>
#include "Network.h"        // networking, webserver, mdns, config portal AP 
#include "FileSvc.h"        // parameter save file i/o
#include "WebSvc.h"         // webservices
#include "Wire.h"
#include "timekeeper.h"
#include "sensors.h"

FS* filesystem = &SPIFFS;              // global File system define
RTC_DATA_ATTR bool resetCMD = false;        // DRD & serial cmd to reset credentials, RTC DATA - won't clear on a deepsleeep
bool updEnable=false;                       // OTA update indicator
bool digitOverlay=true;

//Globals
#define minuteDataPoints 60
#define dayDataPoints    96
float minuteGraphTemp[minuteDataPoints];
int minuteGraphHumidity[minuteDataPoints];
float minuteGraphBarometric[minuteDataPoints];
float minuteGraphWindspeed[minuteDataPoints];
float dayGraphTemp[dayDataPoints];
int dayGraphHumidity[dayDataPoints];
float dayGraphBarometric[dayDataPoints];
float dayGraphWindspeed[dayDataPoints];
struct settingsWS settings_WS;   

//init graph buffers
void graphDataInit(void){
  for(int a=0;a<minuteDataPoints;a++){minuteGraphTemp[a]=200;minuteGraphHumidity[a]=200;minuteGraphBarometric[a]=200;minuteGraphWindspeed[a]=200;}
  for(int a=0;a<dayDataPoints;a++){dayGraphTemp[a]=200;dayGraphHumidity[a]=200;dayGraphBarometric[a]=200;dayGraphWindspeed[a]=200;}  
}

void setup() {
  //setCpuFrequencyMhz(80);       //bettery power reduction
  Serial.begin(115200);         
  Serial.println("\n\n______________________________________________");
  Serial.println("Setup started\n"); 
  if(resetCMD){ Serial.println(" *********CLEARING WIFI CREDENTIALS*********\n"); }
  wifiSetup();                  // Wifi setup
  webSetup();                   // setup web services
  Serial.println("\nSetup complete");
  Serial.println("______________________________________________\n\n"); 
  sensorsInit();                //init sensors  
  timeNTPStart();               //init NTP services
  graphDataInit();              //init graph buffers
}

void loop() {
  //Check WiFi and reconnect if disconnected
  WiFiHealthCheck();
  
  //process web functions
  webServerHandle();  // handle webserver function
  webSocketSvc();     // handle websocket function
  
  //process weather functions
  sensorsSvc();       //service sensors
  servicePWS();       //service external PWS updates
  SvcArchivalData();  //service archival data buffers
}




