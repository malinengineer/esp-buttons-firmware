#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include "WiFiManager.h"          //https://github.com/tzapu/WiFiManager

void configModeCallback (WiFiManager *myWiFiManager) {
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

uint64_t messageSentCount = 0;

WiFiClient client;

void setup() {
  
  // setup GPIO
  pinMode(BUTTON1, INPUT);
  pinMode(BUTTON2, INPUT);
  pinMode(BUTTON3, INPUT);
  pinMode(BUTTON4, INPUT);
  pinMode(BUTTON5, INPUT);
  
  Serial.begin(115200);
  // redirect debug output to Serial1. Serial1 uses UART1, TX pin is GPIO2
  Serial1.setDebugOutput(true);
  
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
    // WiFi.macAddress().c_str()
  }

  if (client.connect("192.168.22.11", 8080))
  {
    client.write("ESP connected!\r\n");
    Serial.println("Connected to the TCP server");
    messageSentCount = 0;
  }
  else
  {
    Serial.println("Couldn't connect to TCP server");
  }
 
}

void loop() {
  // put your main code here, to run repeatedly:
  uint8_t button2_val = digitalRead(BUTTON2);
  if (button2_val == HIGH)
  {
    Serial.println("BUTTON2");
    client.write("BUTTON2\r\n");
  }
  uint8_t button5_val = digitalRead(BUTTON5);
  if (button5_val == HIGH)
  {
    Serial.println("BUTTON5");
    client.write("BUTTON5\r\n");
  }
  delay(180);
}
