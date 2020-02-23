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

//handle arrived messages from mqtt broker
void callbackOnMessage(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  if (String(topic) == "room/led") {
    int ledState = String((char*)payload).toInt();
    digitalWrite(REDPIN, ledState);
  }
}
