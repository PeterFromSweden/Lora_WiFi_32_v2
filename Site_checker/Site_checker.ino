#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include "ExtendedDisplay.h"

#include "Credentials.h"
// Credentials.h should contain the following:
//const char* ssid = "SSID";
//const char* password = "password";
//const String serverUrl = "http://server/site-check/";

ExtendedDisplay display(ArialMT_Plain_16, 16);
bool ledBlink = false;
bool ledState = false;

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  LEDOutput(false);
  pinMode(Vext, OUTPUT);
  VextOutput(false);
  // Initialize OLED display
  display.init();
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  display.clear();
  display.println("Connect to WiFi..");
  display.display();
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  display.println("Ok");
  delay(1000);
}

void loop() {
  // Your main code here
  fetchAndDisplayWeather();
  int timeout = 30000;
  while( timeout > 0)
  {
    if(ledBlink)
    {
      toggleLED();
      delay(200);
      timeout -= 200;
    }
    else
    {
      LEDOutput(false);
      delay(timeout);
      timeout = 0;
    }
  }
}

void fetchAndDisplayWeather() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverUrl);
    int httpResponseCode = http.GET();
    
    display.clearScreen();
    if (httpResponseCode > 0) {
      String payload = http.getString();
      Serial.println(payload);
      
      StaticJsonDocument<200> doc;
      DeserializationError error = deserializeJson(doc, payload);
      
      if (error) {
        Serial.println("Parsing failed");
        displayError("JSON parse error");
        return;
      }
      
      int warning = doc["warning"];
      JsonArray toDisplay;
      if( warning )
      {
        ledBlink = true;
        toDisplay = doc["site-check"];
      }
      else
      {
        ledBlink = false;
        toDisplay = doc["weather"];
      }
      
      if(toDisplay.size() < 4)
      {
        display.setFont(ArialMT_Plain_24, 21);
      }
      else if(toDisplay.size() < 5)
      {
        display.setFont(ArialMT_Plain_16, 16);
      }
      else
      {
        display.setFont(ArialMT_Plain_10, 9);
      }
      display.clear();
      
      for(const char *item : toDisplay)
      {
        display.println(item);
        Serial.println(item);
      }
      display.display();
    }
    else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
      displayError("HTTP Error " + String(httpResponseCode));
    }
    http.end();
  }
  else {
    Serial.println("WiFi Disconnected");
    displayError("WiFi Disconnected");
  }
}

void displayError(String message) {
  display.clearScreen();
  display.println("Error:");
  display.println(message);
  display.display();
  ledBlink = true;
}

void VextOutput(bool newState)
{
  digitalWrite(Vext, !newState); // Active low
}

void toggleLED() {
  LEDOutput(!ledState);
}

void LEDOutput(bool newLedState) {
  ledState = newLedState;
  digitalWrite(LED, ledState);
}
