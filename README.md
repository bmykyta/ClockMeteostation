# Meteo station
## This project is done on a nodemcu esp8266 board

Here you can find visual and concept diagrams for the project

#### What electronic devices do you need?

* Definitely board with esp8266 microcontroller
* Temperature and humidity sensor (i.e. I use -> DHT-11, DHT-22 etc.)
* LCD display (1602, 2004 etc.)
* Gas,CO2 level sensor (I use: MQ-135, MQ-7, MQ-2 etc.)
* LED
* Button

Also you need to create configs.h file:
```C++

#ifndef STASSID
#define STASSID "yourwifiname"
#define STAPSK  "wifipassword"
#endif

#define DHTPIN D7 // DHT pin
#define LCDADDR 0x27 // address of lcd screen
#define BUFFER_SIZE 100 // buffer size
#define BOTtoken  "" // telegram bot token 

byte raindrop_symb[8] =      // raindrop symbol / humidity
{
  B00000,
  B00100,
  B01110,
  B11111,
  B11111,
  B11111,
  B01110,
  B00000
};

byte cloud_part1[] = { // first part of cloud for lcd
  B00000,
  B00000,
  B00000,
  B00111,
  B01111,
  B11111,
  B01111,
  B00111
};

byte cloud_part2[] = { // second part of cloud for lcd
  B00000,
  B00000,
  B10000,
  B11100,
  B11110,
  B11111,
  B11111,
  B11110
};
```