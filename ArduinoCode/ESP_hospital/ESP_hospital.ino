/**
 * \file ESP_hospital.c
 * \brief Send data form the pulse sensor to the hospital ESP.
 * \author Eliott DANIELLO and Cedric LE BEAUDOUR
 * \date February 2022
 *
 * 
 * For this program, you need to install:
 * ESP8266 NodeMCU LittleFS Filesystem Uploader (https://randomnerdtutorials.com/install-esp8266-nodemcu-littlefs-arduino/)
 * Arduino_JSON (Arduino library Manager)
 * ESPAsyncWebServer (.zip in lib folder)
 * ESPAsyncTCP (.zip in lib folder)
 * 
 * Using this tutorial for the website : https://randomnerdtutorials.com/esp8266-web-server-gauges/
 * 
*/

#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "LittleFS.h"
#include <Arduino_JSON.h>

const char* ssid = "ssid";
const char* password = "pwd";

AsyncWebServer server(80);          // AsyncWebServer on port 80
WiFiServer receptionServer(90);     // Receiving server on port 90

AsyncEventSource events("/events"); // Create an Event Source on /events
JSONVar readings; // Json Variable to Hold Sensor Readings


void sendToWebsite(String heartrate){
  readings["clb_hear_rate"] = heartrate;
  String jsonString = JSON.stringify(readings);
  events.send("ping" ,NULL, millis());
  events.send(jsonString.c_str(), "new_readings", millis());
}


void initFS(){
  if (!LittleFS.begin()){
    Serial.println("An error has occurred while mounting LittleFS");
  }
  else{
    Serial.println("LittleFS mounted successfully");
  }
}


void initWiFi(){
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) { // While wifi not connected
    Serial.print('.');
    delay(500);
  }
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  initWiFi();
  initFS();

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){  // Web Server Root URL
    request->send(LittleFS, "/index.html", "text/html");
  });

  server.serveStatic("/", LittleFS, "/");
  
  server.on("/readings", HTTP_GET, [](AsyncWebServerRequest *request){ // Request for the latest sensor readings
    String json = "0";
    request->send(200, "application/json", json);
    json = String();
  });

  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);
  
  server.begin(); 
  receptionServer.begin();
  delay(3000);
}


void loop() {
  WiFiClient client = receptionServer.available();
  if (client){ // When a new measure is received
    if (client.connected()){ // If the server is connected
      String request = client.readStringUntil('\r'); // Read data from the client
      client.flush();
      Serial.println("new heart rate"); // Print for debug
      Serial.println(request);
      sendToWebsite(request);
    }
  }
}
