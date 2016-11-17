#include <ESP8266WiFi.h>          // https://github.com/esp8266/Arduino
#include <ESP8266HTTPClient.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include "WiFiManager.h"          // https://github.com/tzapu/WiFiManager

void configModeCallback (WiFiManager *myWiFiManager)
{
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

const uint8_t BUTTON1 = 16;
const uint8_t BUTTON2 = 14;
const uint8_t BUTTON3 = 5;
const uint8_t BUTTON4 = 13;
const uint8_t BUTTON5 = 12;

const uint8_t STATUS_LED = 4;
const uint8_t BTN_PRESSED = HIGH;
const uint8_t BTN_NOT_PRESSED = LOW;
const uint8_t BUTTONS_NUM = 5;
const uint8_t BUTTONS[BUTTONS_NUM] = {16, 14, 5, 13, 12};
uint8_t pressed[BUTTONS_NUM] = {0};
uint8_t justpressed[BUTTONS_NUM] = {0};
uint8_t justreleased[BUTTONS_NUM] = {0};

const uint32_t BUTTON_DEBOUNCE_TIME = 200;
const uint32_t POLL_BUTTON_TIME = 500;
const uint32_t REQUEST_TIME = 30000;

const char* serverButtonsEndpoint = "http://esp-buttons.herokuapp.com/buttons";

HTTPClient http;
uint16_t buttonsPresses[BUTTONS_NUM] = {0};
unsigned long lastTime = 0;
unsigned long lastTimeRequest = 0;
unsigned long lastTimeDebounce = 0;

void setup()
{
    // setup GPIO
//  pinMode(BUTTON1, INPUT);
//  pinMode(BUTTON2, INPUT);
//  pinMode(BUTTON3, INPUT);
//  pinMode(BUTTON4, INPUT);
//  pinMode(BUTTON5, INPUT);
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, LOW);
  for (size_t i = 0; i < BUTTONS_NUM; ++i) {
    pinMode(BUTTONS[i], INPUT);
  }

  // redirect debug output to Serial1. Serial1 uses UART1, TX pin is GPIO2
  Serial1.setDebugOutput(true);
  Serial1.begin(115200);
  Serial.begin(115200);
  
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset settings - for testing
//  wifiManager.resetSettings();

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if(!wifiManager.autoConnect()) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  } 

  //if you get here you have connected to the WiFi
  Serial.println("Connected to WiFi");

  uint8_t macAddr[6] = {0};  
  if (WiFi.status() == WL_CONNECTED)
  {
    WiFi.macAddress(macAddr);
    Serial.printf("mac address: %02x:%02x:%02x:%02x:%02x:%02x\n", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
  }
  digitalWrite(STATUS_LED, HIGH);
}

void checkButtons()
{
  static byte previousstate[BUTTONS_NUM];
  static byte currentstate[BUTTONS_NUM];
  static unsigned long lasttime = 0;
  byte index = 0;

  if (millis() < lasttime) {
     lasttime = millis(); // we wrapped around, lets just try again
  }
  
  if ((lasttime + BUTTON_DEBOUNCE_TIME) > millis()) {
    // not enough time has passed to debounce
    return; 
  }
  // ok we have waited BUTTON_DEBOUNCE_TIME milliseconds, lets reset the timer
  lasttime = millis();
  
  for (index = 0; index < BUTTONS_NUM; index++) {
    justreleased[index] = 0;  // when we start, we clear out the "just" indicators
    justpressed[index] = 0;
     
    currentstate[index] = !digitalRead(BUTTONS[index]);   // read the button
    
//    Serial.print(index, DEC);
//    Serial.print(": cstate=");
//    Serial.print(currentstate[index], DEC);
//    Serial.print(", pstate=");
//    Serial.print(previousstate[index], DEC);
//    Serial.print(", press=");
    
    if (currentstate[index] == previousstate[index]) {
      if ((pressed[index] == BTN_PRESSED) && (currentstate[index] == BTN_PRESSED)) {
          // just pressed
          justpressed[index] = 1;
      }
      else if ((pressed[index] == BTN_NOT_PRESSED) && (currentstate[index] == BTN_NOT_PRESSED)) {
          // just released
          justreleased[index] = 1;
      }
      pressed[index] = !currentstate[index];
    }
//    Serial.println(pressed[index], DEC);
    previousstate[index] = currentstate[index];   // keep a running tally of the buttons
  }
}

void loop()
{
  bool clearButtons = false;

//  checkButtons();
  if(millis() < lastTimeDebounce) {
    lastTimeDebounce = millis();
  }
  if (millis() >= (lastTime + BUTTON_DEBOUNCE_TIME)) {
    for (size_t i = 0; i < BUTTONS_NUM; ++i) {
      if (digitalRead(BUTTONS[i]) == HIGH) {
//        Serial.printf("BTN%u\n", i + 1);
        justpressed[i] = 1;
        break;
      }
    }
    lastTimeDebounce = millis();
  }
  

  if (millis() < lastTime) {
    lastTime = millis();
  }  
  if (millis() >= (lastTime + POLL_BUTTON_TIME)) {
    for (size_t i = 0; i < BUTTONS_NUM; ++i) {
      if (justpressed[i]) {
        Serial.printf("BTN%u\n", i + 1);
        ++buttonsPresses[i];
      }
      Serial.printf("BTN presses: %u\t%u\t%u\t%u\t%u\n", buttonsPresses[0], buttonsPresses[1], buttonsPresses[2], buttonsPresses[3], buttonsPresses[4]);
      justpressed[i] = 0;
    }
    lastTime = millis();    
  }
  

  if (millis() < lastTimeRequest) {
    lastTimeRequest = millis();
  }
  if (millis() >= (lastTimeRequest + REQUEST_TIME)) {
    if (WiFi.status() == WL_CONNECTED) {
      String httpButtonsRequest = String(WiFi.macAddress() + "\r\n");
      for (size_t i = 0; i < BUTTONS_NUM; ++i) {
        httpButtonsRequest += "BTN";
        httpButtonsRequest += String(i + 1);
        httpButtonsRequest += ":";
        httpButtonsRequest += String(buttonsPresses[i]);
        httpButtonsRequest += "\r\n";
      }
      Serial.println(httpButtonsRequest);
    
      http.begin(serverButtonsEndpoint);
      http.addHeader("Content-Type", "text/plain");
      http.addHeader("Connection", "close");
      int httpCode = http.POST(httpButtonsRequest);
      if (httpCode > 0) {
        Serial.printf("HTTP code: %d\n", httpCode);
        if (httpCode == HTTP_CODE_OK) {
          clearButtons = true;
        }
        else {
          Serial.println("ERROR!");  
        }
        String response = http.getString();
        Serial.println(response);
      } else {
        Serial.println("ERROR!" + http.errorToString(httpCode));
      }
      http.end();
    } else {
      Serial.println("ERROR! Not connected to WiFi!");
    }
    lastTimeRequest = millis();
  }

  if (clearButtons) {
    for (size_t i = 0; i < BUTTONS_NUM; ++i) {
      buttonsPresses[i] = 0;
    }
    clearButtons = false;
  }
  
}
