#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

class JsonBufferParser {
  public:
    struct WeatherData {
      String city, weather_param_en, weather_param_ru, weather_description;
      float temp, feels_like, humidity;
      int pressure, wind_speed, clouds_all;
      bool empty;
    };
    struct TimeData {
      String timezone, datetime, date, hm, hms; // hm - hours and minutes hms - <=> and seconds
      bool empty;
    };
    WeatherData getWeatherData(String bufferExternal);
    TimeData getTimeData(String bufferExternal);
    
  private:
    WeatherData pweather;
    TimeData ptime;
    String getWeatherParam(String weatherParam);
};
