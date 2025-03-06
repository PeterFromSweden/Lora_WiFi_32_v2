#include "esp_sleep.h"
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

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("---------------");
  pinMode(LED, OUTPUT);
  //pinMode(Vext, OUTPUT);
  //VextOutput(true);
  // Initialize OLED display
  display.init();
}

void loop() {
  // Your main code here
  fetchAndDisplayWeather();
  int timeToGo = 10 * 60 * 1000; // 10 minutes
  while(timeToGo > 0) {
    if( ledBlink ) {
      digitalWrite(LED, 1); // 1 = ON, 0 = OFF
      sleepDelay(20);
      digitalWrite(LED, 0); // 1 = ON, 0 = OFF
      sleepDelay(1000 - 20);
      timeToGo -= 1000;
    }
    else
    {
      sleepDelay(timeToGo);
      timeToGo = 0;
    }
  }
}

void sleepDelay(int millisec)
{
  esp_sleep_enable_timer_wakeup(millisec * 1000);
  esp_light_sleep_start();
}

void connectToWifi() {
  if (WiFi.status() != WL_CONNECTED) {
    // Connect to Wi-Fi
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    //display.clear();
    //display.println("Connect to WiFi..");
    //display.display();
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      //Serial.println(".");
    }
    //Serial.println("Connected to WiFi");
    //display.println("Ok");
    //display.display();
    delay(1000);
  }
}

void disconnectWifi()
{
  if (WiFi.status() == WL_CONNECTED) {
    WiFi.disconnect(true);
  }
  
  WiFi.mode(WIFI_OFF);
}

void fetchAndDisplayWeather() {
  static int displayCount = 0;

  connectToWifi();
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverUrl);
    int httpResponseCode = http.GET();
    
    display.clearScreen();
    if (httpResponseCode > 0) {
      String payload = http.getString();
      //Serial.println(payload);
      
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
        //Serial.println(item);
      }
      Serial.printf("Display #%i\n", displayCount++);
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
  disconnectWifi();
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
