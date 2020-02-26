#include "JsonBufferParser.h"

JsonBufferParser::WeatherData JsonBufferParser::getWeatherData(String bufferExternal) {
  if (bufferExternal == "") {
    pweather.empty = true;
    return pweather;
  }
  const size_t capacity = JSON_ARRAY_SIZE(3) + 2 * JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(2) + 3 * JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(6) + JSON_OBJECT_SIZE(12) + 1169;
  DynamicJsonBuffer jsonBuffer(capacity);
  JsonObject& weather = jsonBuffer.parseObject(bufferExternal); // parse response from api
  if (!weather.success()) {
    Serial.println(F("Parsing failed!"));
    //          getWeather();
    pweather.empty = true;
    return pweather;
  }
  pweather.empty = false;
  pweather.city = weather.get<String>("name");
  JsonObject& main = weather["main"];
  pweather.temp = main.get<float>("temp"); // температура текущая
  pweather.feels_like = main.get<float>("feels_like"); // ощущается температура как градусы °С
  pweather.pressure = main.get<int>("pressure"); //  атмосферное давление гПа (паскали)
  pweather.humidity = main.get<float>("humidity"); // влажность %
  pweather.wind_speed = weather["wind"]["speed"]; // скорость ветра m/s
  pweather.clouds_all = weather["clouds"]["all"]; // облачность %
  JsonObject& weather_0 = weather["weather"][0];
  String weather_0_main = weather_0.get<String>("main"); // eng: weather params - Clear, Clouds, Rain, Mist
  pweather.weather_description = weather_0.get<String>("description");
  pweather.weather_param_en = weather_0_main;
  pweather.weather_param_ru = getWeatherParam(weather_0_main); // ru: параметры погоды - солнечно, дождливо, пасмурно

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

String JsonBufferParser::getWeatherParam(String weatherParam) {
  String weather_param;
  if (weatherParam == "Mist") {
    weather_param = "Пасмурно";
  } else if (weatherParam == "Thunderstorm") {
    weather_param = "Гроза";
  } else if (weatherParam == "Drizzle") {
    weather_param = "Небольшой дождь";
  } else if (weatherParam == "Rain") {
    weather_param = "Дождь";
  } else if (weatherParam == "Snow") {
    weather_param = "Снег";
  } else if (weatherParam == "Fog") {
    weather_param = "Туман";
  } else if (weatherParam == "Clear") {
    weather_param = "Чистое небо";
  } else if (weatherParam == "Clouds") {
    weather_param = "Облачно";
  } else {
    weather_param = weatherParam;
  }

  return weather_param;
}
