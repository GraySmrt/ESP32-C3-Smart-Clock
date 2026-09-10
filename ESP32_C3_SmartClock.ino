// ============== PARSE WEATHER JSON ==============
void parseWeatherJSON(String json) {
  // Extract temperature from main object
  // OpenWeatherMap format: "main":{"temp":25.5,"feels_like":...,"humidity":44,...}
  
  int mainIndex = json.indexOf("\"main\":{");
  if (mainIndex != -1) {
    // Find the temp value within the main object
    int tempStart = json.indexOf("\"temp\":", mainIndex);
    if (tempStart != -1) {
      // Find the comma or closing brace after temp value
      int tempEnd = json.indexOf(",", tempStart);
      if (tempEnd == -1) {
        tempEnd = json.indexOf("}", tempStart);
      }
      
      String tempStr = json.substring(tempStart + 7, tempEnd);
      tempStr.trim();
      temperature = tempStr.toFloat();
      Serial.print("Extracted temp string: ");
      Serial.println(tempStr);
    }
  }

  // Extract humidity from main object
  int humidityStart = json.indexOf("\"humidity\":");
  if (humidityStart != -1) {
    int humidityEnd = json.indexOf(",", humidityStart);
    if (humidityEnd == -1) {
      humidityEnd = json.indexOf("}", humidityStart);
    }
    
    String humidityStr = json.substring(humidityStart + 11, humidityEnd);
    humidityStr.trim();
    humidity = humidityStr.toInt();
  }

  // Extract weather description from weather array
  // Format: "weather":[{"id":800,"main":"Clear","description":"clear sky",...}]
  int weatherIndex = json.indexOf("\"weather\":[{");
  if (weatherIndex != -1) {
    int mainDescStart = json.indexOf("\"main\":\"", weatherIndex);
    if (mainDescStart != -1) {
      int mainDescEnd = json.indexOf("\"", mainDescStart + 8);
      weatherDescription = json.substring(mainDescStart + 8, mainDescEnd);
    }
  }

  Serial.print("Parsed - Temp: ");
  Serial.print(temperature);
  Serial.print("°C, Humidity: ");
  Serial.print(humidity);
  Serial.print("%, Description: ");
  Serial.println(weatherDescription);
}
