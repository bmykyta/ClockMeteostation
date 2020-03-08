#include "JsonBufferParser.h"

JsonBufferParser::WeatherData JsonBufferParser::getWeatherData(String bufferExternal) {
  if (bufferExternal == "") {
    pweather.empty = true;
    return pweather;
  }
  const size_t capacity = JSON_OBJECT_SIZE(9) + 116;
  DynamicJsonBuffer jsonBuffer(capacity);
  JsonObject& weather = jsonBuffer.parseObject(bufferExternal); // parse response from api
  if (!weather.success()) {
    Serial.println(F("Parsing failed!"));
    pweather.empty = true;
    return pweather;
  }
  pweather.empty = false;
  pweather.city = weather.get<String>("city");
  pweather.temp = weather.get<float>("temp"); // температура текущая
  pweather.feels_like = weather.get<float>("feels_like"); // ощущается температура как градусы °С
  pweather.pressure = weather.get<int>("pressure"); //  атмосферное давление гПа (паскали)
  pweather.humidity = weather.get<float>("humidity"); // влажность %
  pweather.wind_speed = weather.get<float>("wind_speed"); // скорость ветра m/s
  pweather.clouds_all = weather.get<float>("clouds_all"); // облачность %
  pweather.weather_param = weather.get<String>("weather_param");
  pweather.weather_description = weather.get<String>("weather_description");

  return pweather;
}

JsonBufferParser::TimeData JsonBufferParser::getTimeData(String bufferExternal) {
  if (bufferExternal == "") {
    ptime.empty = true;
    return ptime;
  }
  DynamicJsonBuffer jsonBuffer(2000); // prepare buffer for response
  // Parse JSON object
  JsonObject& doc = jsonBuffer.parseObject(bufferExternal); // parse response from api
  if (!doc.success()) {
    Serial.println(F("Parsing failed!"));
    ptime.empty = true;
    return ptime;
  }
  ptime.empty = false;
  ptime.timezone = doc.get<String>("timezone");
  ptime.datetime = doc.get<String>("datetime");
  ptime.date = ptime.datetime.substring(0, 10);
  ptime.hm = ptime.datetime.substring(11, 16);
  ptime.hms = ptime.datetime.substring(11, 19);

  return ptime;
}
