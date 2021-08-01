/*
--------------------------------------------------------------------------------------------------------------------------
JIMKA - mereni hladiny v jimce
Petr Fory pfory@seznam.cz
GIT - https://github.com/pfory/jimka
*/

#include "Configuration.h"

#ifdef NODEEPSLEEP
auto timer = timer_create_default(); // create a timer with default settings
Timer<> default_timer; // save as above
#endif

uint32_t      heartBeat                     = 0;

#ifdef NODEEPSLEEP
//MQTT callback
void callback(char* topic, byte* payload, unsigned int length) {
  char * pEnd;
  String val =  String();
  DEBUG_PRINT("\nMessage arrived [");
  DEBUG_PRINT(topic);
  DEBUG_PRINT("] ");
  for (int i=0;i<length;i++) {
    DEBUG_PRINT((char)payload[i]);
    val += (char)payload[i];
  }
  DEBUG_PRINTLN();
 
  if (strcmp(topic, (String(mqtt_base) + "/" + String(mqtt_topic_restart)).c_str())==0) {
    DEBUG_PRINT("RESTART");
    ESP.restart();
  } else if (strcmp(topic, (String(mqtt_base) + "/" + String(mqtt_topic_netinfo)).c_str())==0) {
    DEBUG_PRINT("NET INFO");
    sendNetInfoMQTT();
  }
}
#endif

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
  pinMode(ECHOPIN, INPUT_PULLUP);
  pinMode(LOADPIN, OUTPUT);
  
  digitalWrite(ECHOPIN, HIGH);

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
  heartBeat = _reset_reason;
  
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
#ifdef NODEEPSLEEP  
  client.setCallback(callback);
#endif

  WiFi.printDiag(Serial);

  if (!wifiManager.autoConnect(AUTOCONNECTNAME, AUTOCONNECTPWD)) { 
    DEBUG_PRINTLN("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.deepSleep(DEEPSLEEPTIMEOUT);
  } 
  
  sendNetInfoMQTT();

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
    if (error == OTA_AUTH_ERROR) DEBUG_PRINTLN("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) DEBUG_PRINTLN("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) DEBUG_PRINTLN("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) DEBUG_PRINTLN("Receive Failed");
    else if (error == OTA_END_ERROR) DEBUG_PRINTLN("End Failed");
  });
  ArduinoOTA.begin();
#endif

  checkForUpdates();

#ifdef NODEEPSLEEP
  timer.every(SENDSTAT_DELAY, sendStatisticMQTT);
  timer.every(MEASSURE_DELAY, sendMeassurement);
#endif

  void * a;
  sendStatisticMQTT(a);
  sendMeassurement(a);
   
  DEBUG_PRINTLN(" Ready");

  ticker.detach();
  //keep LED on
  digitalWrite(BUILTIN_LED, HIGH);

  drd.stop();

  DEBUG_PRINTLN(F("Setup end."));
}
 
/////////////////////////////////////////////   L  O  O  P   ///////////////////////////////////////
void loop(void) {
#ifndef NODEEPSLEEP
  DEBUG_PRINT("Sleep for ");
  DEBUG_PRINT(DEEPSLEEPTIMEOUT/1e6);
  DEBUG_PRINTLN(" sec.");
  
  DEBUG_PRINT("Boot time: ");
  DEBUG_PRINT((uint16_t)(millis() - lastRun));
  DEBUG_PRINTLN(" ms");
  
  ESP.deepSleep(DEEPSLEEPTIMEOUT);
#else
  timer.tick(); // tick the timer
#ifdef ota
  ArduinoOTA.handle();
#endif
  reconnect();
  client.loop();
#endif
}

bool sendMeassurement(void *) {
  meassurement();
  sendDataMQTT();
  return true;
}
  

void meassurement() {
  /* signál (PING) se pouští jako HIGH na 2 mikrosekundy nebo více */
  /* ještě před signálem dáme krátký puls LOW pro čistý následující HIGH */
  // The PING))) is triggered by a HIGH pulse of 2 or more microseconds.
  // Give a short LOW pulse beforehand to ensure a clean HIGH pulse:
  digitalWrite(LOADPIN, LOW);

  pinMode(TRIGPIN, OUTPUT);
  digitalWrite(TRIGPIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGPIN, HIGH);
  delayMicroseconds(15);
  digitalWrite(TRIGPIN, LOW);
   
  long zpozdeni = pulseIn(ECHOPIN, HIGH, 26000);
   
  /* pomocí funkce si překonvertujeme zpoždění na délkové jednotky */
  distance = MikrosekundyNaCentimetry(zpozdeni);
    
  DEBUG_PRINT("Distance: ");
  DEBUG_PRINT(distance);
  DEBUG_PRINTLN(" cm");
}

bool sendStatisticMQTT(void *) {
  digitalWrite(BUILTIN_LED, LOW);
  //printSystemTime();
  DEBUG_PRINTLN(F("Statistic"));

  SenderClass sender;
  sender.add("VersionSW",                     VERSION);
  sender.add("Napeti",                        ESP.getVcc());
#ifdef NODEEPSLEEP
  sender.add("HeartBeat",                     heartBeat++);
  if (heartBeat % 10 == 0) sender.add("RSSI", WiFi.RSSI());
#else
  sender.add("RSSI", WiFi.RSSI());
#endif
  
  DEBUG_PRINTLN(F("Calling MQTT"));
  
  sender.sendMQTT(mqtt_server, mqtt_port, mqtt_username, mqtt_key, mqtt_base);
  digitalWrite(BUILTIN_LED, HIGH);
  return true;
}


void sendDataMQTT(void) {
  digitalWrite(BUILTIN_LED, LOW);
  //printSystemTime();
  DEBUG_PRINTLN(F("Data"));

  SenderClass sender;
  sender.add("distance",        distance);
#ifndef NODEEPSLEEP  
  sender.add("bootTime",        (uint16_t)(millis() - lastRun));
#endif
  
  DEBUG_PRINTLN(F("Calling MQTT"));
  
  sender.sendMQTT(mqtt_server, mqtt_port, mqtt_username, mqtt_key, mqtt_base);
  digitalWrite(BUILTIN_LED, HIGH);
  return;
}

void sendNetInfoMQTT() {
  //printSystemTime();
  DEBUG_PRINTLN(F("Net info"));

  SenderClass sender;
  sender.add("IP",                            WiFi.localIP().toString().c_str());
  sender.add("MAC",                           WiFi.macAddress());
  
  DEBUG_PRINTLN(F("Calling MQTT"));
  
  sender.sendMQTT(mqtt_server, mqtt_port, mqtt_username, mqtt_key, mqtt_base);
  return;
}

void update_started() {
  DEBUG_PRINTLN("CALLBACK:  HTTP update process started");
}

void update_finished() {
  DEBUG_PRINTLN("CALLBACK:  HTTP update process finished, REBOOT...");
}

void update_progress(int cur, int total) {
  Serial.printf("CALLBACK:  HTTP update process at %d of %d bytes...\n", cur, total);
}

void update_error(int err) {
  Serial.printf("CALLBACK:  HTTP update fatal error code %d\n", err);
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


void checkForUpdates() {
  String fwURL = String( fwUrlBase );
  fwURL.concat("jimka");
  String fwVersionURL = fwURL;
  fwVersionURL.concat( ".version" );
  //DEBUG_PRINT("Cesta k souboru s verzi:");
  //DEBUG_PRINTLN(fwVersionURL);
  
  HTTPClient httpClient;
  httpClient.begin(espClient, fwVersionURL );
  int httpCode = httpClient.GET();
  if (httpCode == 200) {
    String newFWVersion = httpClient.getString();

    DEBUG_PRINT( "Current firmware version: " );
    DEBUG_PRINTLN(VERSION);
    DEBUG_PRINT( "Available firmware version: " );
    DEBUG_PRINTLN(newFWVersion);

    float newVersion = newFWVersion.toFloat();
    String oldVersionS = VERSION;
    float oldVersion = ((String)VERSION).toFloat();
    
    if (newVersion > oldVersion) {
      DEBUG_PRINTLN( "Preparing to update." );

      String fwImageURL = fwURL;
      DEBUG_PRINTLN(fwImageURL);
      fwImageURL.concat( ".bin" );
      DEBUG_PRINTLN(fwImageURL);
      
      // The line below is optional. It can be used to blink the LED on the board during flashing
      // The LED will be on during download of one buffer of data from the network. The LED will
      // be off during writing that buffer to flash
      // On a good connection the LED should flash regularly. On a bad connection the LED will be
      // on much longer than it will be off. Other pins than LED_BUILTIN may be used. The second
      // value is used to put the LED on. If the LED is on with HIGH, that value should be passed
      ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);

      // Add optional callback notifiers
      ESPhttpUpdate.onStart(update_started);
      ESPhttpUpdate.onEnd(update_finished);
      ESPhttpUpdate.onProgress(update_progress);
      ESPhttpUpdate.onError(update_error);
      
      t_httpUpdate_return ret = ESPhttpUpdate.update(espClient, fwImageURL);
      DEBUG_PRINTLN(ret);
      switch(ret) {
        case HTTP_UPDATE_FAILED:
          Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
          break;

        case HTTP_UPDATE_NO_UPDATES:
          DEBUG_PRINTLN("HTTP_UPDATE_NO_UPDATES");
          break;
      }
    } else {
      DEBUG_PRINTLN("Already on latest version");
    }
  } else {
    DEBUG_PRINT("Firmware version check failed, got HTTP response code" );
    DEBUG_PRINTLN( httpCode );
  }
  httpClient.end();
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    if (lastConnectAttempt == 0 || lastConnectAttempt + connectDelay < millis()) {
      DEBUG_PRINT("Attempting MQTT connection...");
      // Attempt to connect
      if (client.connect(mqtt_base, mqtt_username, mqtt_key)) {
        DEBUG_PRINTLN("connected");
        client.subscribe((String(mqtt_base) + "/#").c_str());
      } else {
        lastConnectAttempt = millis();
        DEBUG_PRINT("failed, rc=");
        DEBUG_PRINTLN(client.state());
      }
    }
  }
}