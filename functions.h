#pragma once
#ifndef REDPIN
#define REDPIN D5
#endif

void wifiBegin(const char* ssid, const char* password) {
  Serial.print(F("Connecting to "));
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password); // initialize station

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }
  Serial.println();
  Serial.println(F("WiFi connected"));
  Serial.println(WiFi.localIP());
  Serial.println("...ok...");
}
