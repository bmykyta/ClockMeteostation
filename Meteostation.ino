/*
    Meteostation with node-mcu esp8266
    Written by dev_iks
*/
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <MQ135.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include "configs.h"
#include "ScrollLcdText.h"
#include "functions.h"
#include "JsonBufferParser.h"

#define DHTPIN D7 // DHT pin
#define LCDADDR 0x27 // address of lcd screen
#define GASPIN A0 // mq135 sensor pin
#define BUFFER_SIZE 100 // buffer size
#define MQTT_MAX_PACKET_SIZE 256
#define SECS_PER_MIN (60000)
#define HOURS_IN_MLSECS(hour) (hour * 60 * SECS_PER_MIN)

const bool _debug = false;
// initialize config variables
const char* ssid = STASSID; // SSID for wi-fi station
const char* password = STAPSK; // password for wi-fi station
const char* time_api = TIMEAPI; // world time api for current time
const char* mqtt_server = MQTTSERVER; // mqtt server addr
const int mqtt_port = MQTTPORT; // mqtt port
const char* mqtt_user = MQTTUSER; // if mqtt anonymous with user and pass
const char* mqtt_pass = MQTTPASS;

const int rLEDPin = D5;
const int button = D4;

bool butt_flag = false;
bool butt;
unsigned long last_press = 0;
unsigned long last_time = 0;

void callbackOnMessage(char* topic, byte* payload, unsigned int length);

// Secure client for telegram
WiFiClientSecure net_ssl; // later we confirm ssl certificates

WiFiClient wfc;
PubSubClient mqttClient(mqtt_server, mqtt_port, callbackOnMessage, wfc);

LiquidCrystal_I2C lcd(LCDADDR, 20, 4);
DHT dht(DHTPIN, DHT11);
MQ135 gasSensor = MQ135(GASPIN);
Scroll_Lcd_Text scrollLcdText;
JsonBufferParser jsonParser;
void getWeather();

String buffer_line_time; // buffer for recieve response from time api
String buffer_line_weather; // buffer for recieve response from weather api
unsigned long updateTimeDHT = 0;
unsigned long updateTimeWeather = 0;
int ledStatus = 0; // led status
int current_mode = 0;
float humidity; // humidity - влажность воздуха получаемая на текущем модуле DHT-11
float temperature; // temperature - температура получаемая на текущем модуле DHT-11
float ppm; // co2 concentration in ppm
bool continueToScroll = false;
JsonBufferParser::WeatherData weather;
JsonBufferParser::TimeData locTime;

void lcdPrint(String str, int col = 0, int row = 0)
{
  lcd.setCursor(col, row);
  lcd.print(str);
}

void reconnect() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-1";
    // Attempt to connect
    if (mqttClient.connect(clientId.c_str())) {
      Serial.print("connected  state = ");
      Serial.println(mqttClient.state());
      mqttClient.subscribe("room/+");
    } else {
      Serial.print("failed, rc = ");
      Serial.print(mqttClient.state());
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void lcdPrintWeather(JsonBufferParser::WeatherData weather) {
  if (weather.empty) {
    lcdPrint("Failed to print");
//    getWeather();
    return;
  }
  lcdPrint("City:" + weather.city + " " + weather.weather_param);
  lcdPrint("T:" + String(weather.temp) + "\337C F:" + String(weather.feels_like) + "\337C", 0, 1);
  lcdPrint("Wind:" + String(weather.wind_speed) + "m/s \1" + String(weather.humidity) + "%", 0, 2);
  lcdPrint("\2\3" + String(weather.clouds_all) + " %", 0, 3);
}

void mqttReconnection() {
  if (!mqttClient.connected()) {
    Serial.println("reconnect!....");
    reconnect();
  }
}

void mqttLoop() {
  mqttReconnection();
  mqttClient.loop();
}

void lcdPrintLocalTime() {
  if (millis() - last_time > 5000) {
    last_time = millis();
    lcdPrint(String("TZ:" + locTime.timezone));
    lcdPrint(locTime.date + " " + locTime.hm, 0, 1);
    getTime();
  }
}

void switchMode() {
  if (butt == HIGH && butt_flag == false && millis() - last_press > 200) {
    butt_flag = true;
    current_mode = !current_mode;
    lcd.clear();
    last_press = millis();
  }

  if (butt == LOW && butt_flag == true) {
    butt_flag = false;
  }
}

void readMeteodata() {
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  if (isnan(humidity) || isnan(temperature)) {  // Check. If we can't get values, outputing «Error», and program exits
    lcdPrint("Error!", 0, 2);
    return;
  }
  ppm = gasSensor.getPPM();
  if(isnan(ppm)){
   lcdPrint("PPM:500", 0, 3);
  }
}

void displayedMode() {
  if (current_mode == 0) {
    lcdPrintLocalTime();
    readMeteodata();
    lcdPrint("\1" + String(humidity) + "% Temp:" + String(temperature) + "\337C", 0, 2);
    lcdPrint("CO2:" + String(ppm), 0, 3);
  } else {
    if (millis() - updateTimeWeather >= HOURS_IN_MLSECS(3)) {
      updateTimeWeather = millis();
      getWeather();
    }
    lcdPrintWeather(weather);
  }

  if (millis() - updateTimeDHT > 30000) {
    updateTimeDHT = millis();
    publishDhtData(temperature, humidity, ppm);
  }
}

void setup() {
  Serial.begin(115200);
  //  gdbstub_init();
  pinMode(rLEDPin, OUTPUT);
  pinMode(button, INPUT);
  // Connect to WiFi network
  wifiBegin(ssid, password);

  dht.begin();
  lcd.init();
  lcd.createChar(1, raindrop_symb);
  lcd.createChar(2, cloud_part1);
  lcd.createChar(3, cloud_part2);
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Server started");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  delay(1500);
  lcd.clear();
  net_ssl.setInsecure(); // confirm certs
  mqttReconnection();
  getTime();
  getWeather(); // initialize weather
  gasSensorSetup();
}

void gasSensorSetup() {
  float rzero = gasSensor.getRZero();
  delay(3000);
  Serial.print("MQ135 RZERO Calibration Value : ");
  Serial.println(rzero);
}

void loop() {
  butt = !digitalRead(button);
  mqttLoop();
  switchMode();
  displayedMode();
  mqttLoop();
}

void getWeather() {
  mqttClient.publish("room/get-weather/telegram", "1");
}

void getTime() {
  mqttClient.publish("room/get-time/telegram", "1");
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
  String _topic = String(topic);
  if (_topic == "room/led") {
    int ledState = String((char*)payload).toInt();
    ledStatus = ledState;
    digitalWrite(REDPIN, ledState);
  } else if (_topic == "room/led-status") {
    char _ledStat[2];
    itoa(ledStatus, _ledStat, 10);
    mqttClient.publish("room/led-status/telegram", _ledStat);
  } else if (_topic == "room/getmeteodata") {
    readMeteodata();
    publishMeteodataTelegram();
  } else if(_topic == "room/getweather") {
    buffer_line_weather = String((char*)payload);
    weather = jsonParser.getWeatherData(buffer_line_weather);
  } else if(_topic == "room/gettime") {
    buffer_line_time = String((char*)payload);
    locTime = jsonParser.getTimeData(buffer_line_time);
  }
}

void publishMeteodataTelegram() {
  String rez = "{";
  rez += "\"temperature\":"+String(temperature)+",";
  rez += "\"humidity\":"+String(humidity)+",";
  rez += "\"ppm\":"+String(ppm);
  rez += "}";
  int rezLen = 2*rez.length();
  char msgBuffer[rezLen];
  rez.toCharArray(msgBuffer, rezLen);
  Serial.println(msgBuffer);
  mqttClient.publish("room/meteodata/telegram", msgBuffer);
}

void publishDhtData(float temperature, float humidity, float ppm) {
  char msgBuffer[20];
  mqttClient.publish("room/temp", dtostrf(temperature, 5, 2, msgBuffer));
  mqttClient.publish("room/humidity", dtostrf(humidity, 5, 2, msgBuffer));
  mqttClient.publish("room/co2", dtostrf(ppm, 7, 0, msgBuffer));
}
