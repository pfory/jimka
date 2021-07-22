#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include "Sender.h"
#include <timer.h>
#include <DoubleResetDetector.h>      //https://github.com/khoih-prog/ESP_DoubleResetDetector
#include <Ticker.h>
//#include "MedianFilterLib2.h"


//SW name & version
#define     VERSION                      "0.33"
#define     SW_NAME                      "Jimka"

#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

//const float VERSION = 0.23;
const char* fwUrlBase = "http://192.168.1.56/fota/";

//#define timers
#define verbose
//#define time
#ifdef time
#include <TimeLib.h>
#include <Timezone.h>
#endif

#define AUTOCONNECTPWD    "password"

//#define NODEEPSLEEP

#ifdef NODEEPSLEEP
#define ota
#endif

#ifdef ota
#include <ArduinoOTA.h>
#define HOSTNAMEOTA       SW_NAME VERSION
#define AUTOCONNECTNAME   HOSTNAMEOTA
#else
#define AUTOCONNECTNAME   SW_NAME VERSION
#endif


/*
--------------------------------------------------------------------------------------------------------------------------

Version history:

--------------------------------------------------------------------------------------------------------------------------
HW
ESP8266 Wemos D1
HC-SR04
*/


#define verbose
#ifdef verbose
  #define DEBUG_PRINT(x)         Serial.print (x)
  #define DEBUG_PRINTDEC(x)      Serial.print (x, DEC)
  #define DEBUG_PRINTLN(x)       Serial.println (x)
  #define DEBUG_PRINTF(x, y)     Serial.printf (x, y)
  #define DEBUG_PRINTHEX(x)      Serial.print (x, HEX)
  #define PORTSPEED 115200
  #define SERIAL_BEGIN           Serial.begin(PORTSPEED);
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTDEC(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(x, y)
#endif 

#define SENDSTAT_DELAY                       60000  //poslani statistiky kazdou minutu
#define MEASSURE_DELAY                       20000  //mereni

uint32_t              connectDelay                = 30000; //30s
uint32_t              lastConnectAttempt          = 0;  

// Number of seconds after reset during which a
// subseqent reset will be considered a double reset.
#define DRD_TIMEOUT 2
// RTC Memory Address for the DoubleResetDetector to use
#define DRD_ADDRESS 0

#define CONFIG_PORTAL_TIMEOUT 60 //jak dlouho zustane v rezimu AP nez se cip resetuje
#define CONNECT_TIMEOUT 120 //jak dlouho se ceka na spojeni nez se aktivuje config portal

static const char* const      mqtt_server                    = "192.168.1.56";
static const uint16_t         mqtt_port                      = 1883;
static const char* const      mqtt_username                  = "datel";
static const char* const      mqtt_key                       = "hanka12";
static const char* const      mqtt_base                      = "/home/jimka";
#ifdef NODEEPSLEEP
static const char* const      mqtt_topic_restart             = "restart";
static const char* const      mqtt_topic_netinfo             = "netinfo";
#endif


uint32_t                      lastRun                        = 0;
int                           distance                       = 0;


#define DEEPSLEEPTIMEOUT      600e6    //600sec 

#define MEASCOUNT             100

//All of the IO pins have interrupt/pwm/I2C/one-wire support except D0.
//#define                                   D0 //10k Pull-down, SS          GPIO16
//SCL                                       D1 //                           GPIO5
//SDA                                       D2 //                           GPIO4
//#define                                   D3 // 10k Pull-up               GPIO0
//BUILTIN_LED                               D4 //10k Pull-up, BUILTIN_LED   GPIO2
//#define ONE_WIRE_BUS_OUT                  D5 //SCK                        GPIO14
#define TRIGPIN                             D6 //MISO                       GPIO12
#define ECHOPIN                             D7 //MOSI                       GPIO13
#define LOADPIN                             D2 //                           GPIO4
//#define                                   D8 //                           GPIO15

#endif
