/*
    Meteostation with node-mcu esp8266
    Written by dev_iks
*/
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <UniversalTelegramBot.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include "configs.h"
#include "ScrollLcdText.h"
#include "functions.h"
#include "JsonBufferParser.h"

const char* ssid = STASSID; // SSID for wi-fi station
const char* password = STAPSK; // password for wi-fi station
const char* time_api = "worldtimeapi.org"; // world time api for current time
const char* weather_api = "api.openweathermap.org/data/2.5"; // open weather map for current weather and forecasts
const String owm_token = "APPID=955b3280174bbe6155b80a34ad117a0d"; // token for communication with open weather map api
String location = "Kharkiv,UA"; // location for the forecast
const char* mqtt_server = "192.168.1.6"; // mqtt server addr
const int mqtt_port = 1883; // mqtt port
const char* mqtt_user = ""; // if mqtt anonymous with user and pass
const char* mqtt_pass = "";

const int rLEDPin = D5;
const int button = D4;

bool butt_flag = false;
bool butt;
unsigned long last_press = 0;
unsigned long last_time = 0;

// Secure client for telegram
WiFiClientSecure net_ssl; // later we confirm ssl certificates
UniversalTelegramBot bot(BOTtoken, net_ssl);

WiFiClient wifiMqttClient;
PubSubClient mqttClient(mqtt_server, mqtt_port, callbackOnMessage, wifiMqttClient);

LiquidCrystal_I2C lcd(LCDADDR, 20, 4);
DHT dht(DHTPIN, DHT11);
Scroll_Lcd_Text scrollLcdText;
JsonBufferParser jsonParser;
void getWeather();

String buffer_line_time; // buffer for recieve response from time api
String buffer_line_weather; // buffer for recieve response from weather api
long checkTelegramDueTime;
int checkTelegramDelay = 1000; // check new messages in telegram
long Bot_lasttime = 0; // bot last time checking new messages
unsigned long updateTimeDHT;
int Bot_mtbs = 1000;
bool Start = false; // if new user starting bot
int ledStatus = 0; // led status
float humidity; // humidity - влажность воздуха получаемая на текущем модуле DHT-11
float temperature; // temperature - температура получаемая на текущем модуле DHT-11
int current_mode = 0;

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
      Serial.print("connected  state =");
      Serial.println(mqttClient.state());
      mqttClient.subscribe("room/+");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void switch_led_command(String chat_id, String text) {
  if (text == "On") {
    digitalWrite(rLEDPin, HIGH); // turn the LED ON (HIGH is the voltage level)
    bot.sendMessage(chat_id, "LED is ON", ""); // sending response to bot
    ledStatus = 1;
  }

  if (text == "Off") {
    ledStatus = 0;
    digitalWrite(rLEDPin, LOW);    // turn the LED off (LOW is the voltage level)
    bot.sendMessage(chat_id, "Led is OFF", "");  // sending response to bot
  }
}

void check_led_status_command(String chat_id, String text) {
  if (text == "/status") {
    if (ledStatus) {
      bot.sendMessage(chat_id, "Led is ON 💡", ""); // \xF0\x9F\x92\xA1
    } else {
      bot.sendMessage(chat_id, "Led is OFF 💡", "");
    }
  }
}

void get_meteodata_command(String chat_id, String text) {
  if (text == "/getmeteodata") {
    String meteostation_data = "Сейчас в комнате " + String(temperature) + " °С . \n";
    meteostation_data += "💧 Влажность: " + String(humidity) + " %.\n"; // droplet :droplet: or U+1F4A7 or \xF0\x9F\x92\xA7
    bot.sendMessage(chat_id, meteostation_data, "Markdown");
  }
}

void printWeatherLcd(JsonBufferParser::WeatherData weather) {
  lcdPrint("City:" + weather.city + " " + weather.weather_param_en);
  lcdPrint("T:" + String(weather.temp) + "\337C F:" + String(weather.feels_like) + "\337C", 0, 1);
  lcdPrint("Wind:" + String(weather.wind_speed) + "m/s \1" + String(weather.humidity) + "%", 0, 2);
  lcdPrint("\2\3" + String(weather.clouds_all) + " %", 0, 3);
}

void get_weather_command(String chat_id, String text) {
  if (text == "/getweather") { // get weather in our location from weather api
    getWeather();
    auto weather = jsonParser.getWeatherData(buffer_line_weather);
    if (weather.empty != true) {
      String meteostation_data = "Погода в *" + weather.city + "*.\n";
      meteostation_data += "🌡 Температура:" + String(weather.temp) + " °С, ощущается как " + String(weather.feels_like) + " °С. ";
      meteostation_data += weather.weather_param_ru + "\n";
      meteostation_data += "💨 Ветер: " + String(weather.wind_speed) + " м/с\n";
      meteostation_data += "💧 Влажность: " + String(weather.humidity) + " %\n";
      meteostation_data += "🌥 Облачность: " + String(weather.clouds_all) + " %\n"; // droplet :droplet: or U+1F4A7 or \xF0\x9F\x92\xA7

      bot.sendMessage(chat_id, meteostation_data, "Markdown"); // send message
      lcd.clear();
      printWeatherLcd(weather); // printing all info to lcd screen
      delay(10000);
      //      lcd.clear();
    } else {
      bot.sendMessage(chat_id, "Something go wrong...", "");
      getWeather();
      return;
    }
  }
}

void start_command(String chat_id, String text, String from_name) {
  from_name = (from_name == "") ? "Guest" : from_name;
  if (text == "/start") {
    String welcome = "Welcome to BoniBot, " + from_name + ".\n";
    welcome += "This is *Meteostation* example.\n\n";
    welcome += "On : to switch the Led *ON* 💡\n";
    welcome += "Off : to switch the Led *OFF* 💡.\n";
    welcome += "/status : Returns current status of LED.\n";
    welcome += "/getmeteodata : Returns current status of humidity, temperature and CO2 concentration in the room.\n";
    welcome += "/getweather : Returns weather in current location.\n";
    bot.sendMessage(chat_id, welcome, "Markdown");
  }
}

// handle new messages in telegram
void handleNewMessages(int numNewMessages) {
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));

  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;

    String from_name = bot.messages[i].from_name; // name of user

    start_command(chat_id, text, from_name);
    bot.sendChatAction(chat_id, "typing");
    switch_led_command(chat_id, text);
    check_led_status_command(chat_id, text); // sends current led status
    get_meteodata_command(chat_id, text); // send current meteodata from our sensors
    get_weather_command(chat_id, text); // send forecast in the city
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
  getLocalTime(); // get world time
}

int continueToScroll = false;

void loop() {
  butt = !digitalRead(button);
  DynamicJsonBuffer jsonBuffer(2000); // prepare buffer for response
  // Parse JSON object
  JsonObject& doc = jsonBuffer.parseObject(buffer_line_time); // parse response from api
  if (!doc.success()) {
    Serial.println(F("Parsing failed!"));
    getLocalTime();
    return;
  }
  if (!mqttClient.connected()) {
    Serial.println("reconnect!....");
    reconnect();
  }
  mqttClient.loop();

  if (millis() - updateTimeDHT > 30000) {
    updateTimeDHT = millis();
    publishDhtData(temperature, humidity);
  }
  if (millis() - Bot_lasttime > checkTelegramDelay)  { // checking new messages for delay
    Bot_lasttime = millis();
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1); // get updates from bot

    while (numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
  }

  if (butt == HIGH && butt_flag == false && millis() - last_press > 200) {
    butt_flag = true;
    current_mode = !current_mode;
    lcd.clear();
    last_press = millis();
  }

  if (butt == LOW && butt_flag == true) {
    butt_flag = false;
  }
  if (current_mode == 0) {
    if (millis() - last_time > 5000) {
      getLocalTime();
      lcd.setCursor(0, 0);
      String timezone = doc.get<String>("timezone");
      lcd.print(String("TZ:" + timezone));
      lcd.setCursor(0, 1);
      String datetime = doc.get<String>("datetime");
      String date = datetime.substring(0, 10);
      String tm = datetime.substring(11, 16); // h:m:s - from 11 to 19 symbols h:m - from 11 to 16

      lcd.print(date);
      lcd.print(" ");
      lcd.print(tm);
      last_time = millis();
    }

    float h = dht.readHumidity();
    float t = dht.readTemperature();
    humidity = h;
    temperature = t;
    if (isnan(h) || isnan(t)) {  // Check. If we can't get values, outputing «Error», and program exits
      lcd.print("Error!");
      return;
    }
    lcd.setCursor(0, 2);
    lcd.print("\1");
    lcd.print(h);
    lcd.print("% Temp:");
    lcd.print(t);
    lcd.print("\337C"); // symbol of gradus
  } else {
    if (buffer_line_weather == "") {
      getWeather();
    }
    auto weather = jsonParser.getWeatherData(buffer_line_weather);
    printWeatherLcd(weather);
  }

  mqttClient.loop();
}

void getLocalTime() {
  WiFiClient client; // initiate Wi-Fi client
  client.setTimeout(1000);
  if (!client.connect("worldtimeapi.org", 80)) { // connect to api
    Serial.println(F("Connection failed"));
    return;
  }

  Serial.println(F("Connected to api!"));

  // Send HTTP(GET) request to api
  client.println(F("GET /api/ip HTTP/1.0"));
  client.println(F("Host: worldtimeapi.org"));
  client.println(F("Connection: close"));
  if (client.println() == 0) {
    Serial.println(F("Failed to send request"));
    return;
  }

  delay(1500);
  while (client.available())
  {
    buffer_line_time = client.readStringUntil('\r'); // put response to buffer line for later parse
  }
  Serial.println("Closing connection");
  client.stop();
}

void getWeather() {
  WiFiClient client;
  client.setTimeout(1000);
  if (!client.connect("api.openweathermap.org", 80)) { // connect to api
    Serial.println(F("Connection failed"));
    return;
  }

  Serial.println(F("Connected to weather api!"));

  // Send HTTP request to api
  String get_url = "GET ";
  get_url += "/data/2.5/weather?q=" + location;
  get_url += "&units=metric&" + owm_token;
  get_url += " HTTP/1.0";
  client.println(get_url); // make GET request
  client.println( F("Host: api.openweathermap.org"));
  client.println(F("Connection: close"));
  if (client.println() == 0) {
    Serial.println(F("Failed to send request"));
    return;
  }

  delay(1500);
  while (client.available())
  {
    buffer_line_weather = client.readStringUntil('\r'); // put response to buffer line for later parse
  }
  Serial.println("Closing connection");
  client.stop();
}

void publishDhtData(float temperature, float humidity) {
  char msgBuffer[20];
  mqttClient.publish("room/temp", dtostrf(temperature, 5, 2, msgBuffer));
  mqttClient.publish("room/humidity", dtostrf(humidity, 5, 2, msgBuffer));
}
