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
  delay(100);
  Serial.println("---------------");
  pinMode(LED, OUTPUT);
  //pinMode(Vext, OUTPUT);
  //VextOutput(true);
  // Initialize OLED display
  display.init();
  display.println("FW 1.0.1");
  delay(1000);
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

bool waitForWifi(int timeoutMs = 8000) {
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > timeoutMs) return false;
    delay(200);
  }
  return true;
}

void connectToWifi() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.mode(WIFI_OFF);
    delay(50);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    if (!waitForWifi()) {
      displayError("WiFi Timeout");
    }
    delay(300); // allow DHCP + DNS settle
  }
}

void disconnectWifi()
{
  if (WiFi.status() == WL_CONNECTED) {
    WiFi.disconnect(true);
  }
  
  WiFi.mode(WIFI_OFF);
}

int httpGetWithRetry(HTTPClient &http, int retries = 3) {
  int code;
  for (int i = 0; i < retries; i++) {
    code = http.GET();
    if (code > 0) return code;
    Serial.printf("HTTP GET failed (%d), retry %d\n", code, i+1);
    delay(300);
  }
  return code;
}

void fetchAndDisplayWeather() {
  static int displayCount = 0;

  // Etablish WiFi
  connectToWifi();
  if (WiFi.status() != WL_CONNECTED) {
    displayError("WiFi Disconnected");
    return;
  }

  // Get HTTP response
  HTTPClient http;
  http.setTimeout(6000);       // default is too low
  http.setReuse(false);        // prevents stale sockets
  http.begin(serverUrl);
  
  int httpResponseCode = httpGetWithRetry(http);
  if (httpResponseCode <= 0) {
    displayError("HTTP Fail " + String(httpResponseCode));
    return;
  }

  // Deserialize
  display.clearScreen();
  String payload = http.getString();
  
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, payload);
  
  if (error) {
    displayError("JSON parse error");
    return;
  }
  
  // Warning or not?
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
  
  // Now we want to display 'toDisplay'
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
  }
  display.display();

  http.end();

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
