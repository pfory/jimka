/*
--------------------------------------------------------------------------------------------------------------------------
ZUMPA - mereni hladiny v jimce
Petr Fory pfory@seznam.cz
GIT - https://github.com/pfory/zumpa
*/

#include "Configuration.h"

//for LED status
Ticker ticker;

void tick() {
  //toggle state
  int state = digitalRead(BUILTIN_LED);  // get the current state of GPIO1 pin
  digitalWrite(BUILTIN_LED, !state);     // set pin to the opposite state
}

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  DEBUG_PRINTLN("Entered config mode");
  DEBUG_PRINTLN(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  DEBUG_PRINTLN(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  ticker.attach(0.2, tick);
}

DoubleResetDetector drd(DRD_TIMEOUT, DRD_ADDRESS);

ADC_MODE(ADC_VCC); //vcc read

bool isDebugEnabled() {
#ifdef verbose
  return true;
#endif // verbose
  return false;
}

WiFiClient espClient;
PubSubClient client(espClient);

WiFiManager wifiManager;

//MedianFilter2<int> medianFilter2(MEASCOUNT);

/////////////////////////////////////////////   S  E  T  U  P   ////////////////////////////////////
void setup(void) {
  lastRun = millis();
  SERIAL_BEGIN;
  DEBUG_PRINT(F(SW_NAME));
  DEBUG_PRINT(F(" "));
  DEBUG_PRINTLN(F(VERSION));

  pinMode(TRIGPIN, OUTPUT);
  pinMode(ECHOPIN, INPUT);

  pinMode(BUILTIN_LED, OUTPUT);
  ticker.attach(1, tick);

  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setConfigPortalTimeout(CONFIG_PORTAL_TIMEOUT);
  wifiManager.setConnectTimeout(CONNECT_TIMEOUT);

  if (drd.detectDoubleReset()) {
    DEBUG_PRINTLN("Double reset detected, starting config portal...");
    ticker.attach(0.2, tick);
    if (!wifiManager.startConfigPortal(AUTOCONNECTNAME)) {
      DEBUG_PRINTLN("failed to connect and hit timeout");
      delay(3000);
      //reset and try again, or maybe put it to deep sleep
      ESP.reset();
      delay(5000);
    }
  }

  rst_info *_reset_info = ESP.getResetInfoPtr();
  uint8_t _reset_reason = _reset_info->reason;
  DEBUG_PRINT("Boot-Mode: ");
  DEBUG_PRINTLN(_reset_reason);
  
  /*
 REASON_DEFAULT_RST             = 0      normal startup by power on 
 REASON_WDT_RST                 = 1      hardware watch dog reset 
 REASON_EXCEPTION_RST           = 2      exception reset, GPIO status won't change 
 REASON_SOFT_WDT_RST            = 3      software watch dog reset, GPIO status won't change 
 REASON_SOFT_RESTART            = 4      software restart ,system_restart , GPIO status won't change 
 REASON_DEEP_SLEEP_AWAKE        = 5      wake up from deep-sleep 
 REASON_EXT_SYS_RST             = 6      external system reset 
  */
  client.setServer(mqtt_server, mqtt_port);
  //client.setCallback(callback);

  WiFi.printDiag(Serial);

  if (!wifiManager.autoConnect(AUTOCONNECTNAME, AUTOCONNECTPWD)) { 
    DEBUG_PRINTLN("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.deepSleep(DEEPSLEEPTIMEOUT);
  } 

#ifdef ota
  ArduinoOTA.setHostname(HOSTNAMEOTA);

  ArduinoOTA.onStart([]() {
    DEBUG_PRINTLN("Start updating ");
  });
  ArduinoOTA.onEnd([]() {
   DEBUG_PRINTLN("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    DEBUG_PRINTF("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    DEBUG_PRINTF("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
#endif

  ticker.detach();
  //keep LED on
  digitalWrite(BUILTIN_LED, HIGH);

  drd.stop();

  DEBUG_PRINTLN(F("Setup end."));
}
 
/////////////////////////////////////////////   L  O  O  P   ///////////////////////////////////////
void loop(void) {
  /* signál (PING) se pouští jako HIGH na 2 mikrosekundy nebo více */
  /* ještě před signálem dáme krátký puls LOW pro čistý následující HIGH */
  // The PING))) is triggered by a HIGH pulse of 2 or more microseconds.
  // Give a short LOW pulse beforehand to ensure a clean HIGH pulse:
  pinMode(TRIGPIN, OUTPUT);
  digitalWrite(TRIGPIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGPIN, HIGH);
  delayMicroseconds(5);
  digitalWrite(TRIGPIN, LOW);
   
  long zpozdeni = pulseIn(ECHOPIN, HIGH);
   
  /* pomocí funkce si překonvertujeme zpoždění na délkové jednotky */
  distance = MikrosekundyNaCentimetry(zpozdeni);
    
  DEBUG_PRINT("Distance: ");
  DEBUG_PRINT(distance);
  DEBUG_PRINTLN(" cm");
  sendDataMQTT();

  DEBUG_PRINT("Sleep for ");
  DEBUG_PRINT(DEEPSLEEPTIMEOUT/1e6);
  DEBUG_PRINTLN(" sec.");
  
  DEBUG_PRINT("Boot time: ");
  DEBUG_PRINT((uint16_t)(millis() - lastRun));
  DEBUG_PRINTLN(" ms");
  
  ESP.deepSleep(DEEPSLEEPTIMEOUT);
}

void sendDataMQTT(void) {
  digitalWrite(BUILTIN_LED, LOW);
  //printSystemTime();
  DEBUG_PRINTLN(F("Data"));

  SenderClass sender;
  sender.add("Version",         VERSION);
  sender.add("Napeti",          ESP.getVcc());
  sender.add("IP",              WiFi.localIP().toString().c_str());
  sender.add("MAC",             WiFi.macAddress());
  sender.add("distance",        distance);
  sender.add("bootTime",        (uint16_t)(millis() - lastRun));
  
  
  DEBUG_PRINTLN(F("Calling MQTT"));
  
  sender.sendMQTT(mqtt_server, mqtt_port, mqtt_username, mqtt_key, mqtt_base);
  digitalWrite(BUILTIN_LED, HIGH);
  return;
}

// long MikrosekundyNaPalce(long microseconds) {
  // /* rychlost zvuku je cca 73.746 mikrosekund na palec (1130 stop za sekundu) */
  // /* nezapomeňte, že signál musí urazit cestu k překážce a zpět, tedy ještě vydělit dvěma! */
  // return microseconds / 74 / 2;
// }
 
int MikrosekundyNaCentimetry(long microseconds) {
  /* rychlost zvuku je cca 340 m/s nebo 29 mikrosekund na centimetr */
  /* nezapomeňte, že signál musí urazit cestu k překážce a zpět, tedy ještě vydělit dvěma! */
  return microseconds / 29 / 2;
}