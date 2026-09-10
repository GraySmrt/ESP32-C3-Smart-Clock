#include <WiFi.h>
#include <time.h>
#include <HTTPClient.h>
#include <Adafruit_SSD1306.h>
#include <MD_MAX72xx.h>
#include <SPI.h>

// ============== DISPLAY SETUP ==============
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ============== LED MATRIX SETUP ==============
#define MAX_DEVICES 1
#define CS_PIN 3      // Chip Select
#define CLK_PIN 4     // Clock
#define DIN_PIN 5     // Data In
MD_MAX72XX mx = MD_MAX72XX(MD_MAX72XX::FC16_HW, DIN_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

// ============== TOUCH PINS ==============
#define TOUCH_ICON 1      // Pin for cycling matrix icons
#define TOUCH_DISPLAY 2   // Pin for cycling OLED display (clock/weather)

// ============== WiFi CREDENTIALS ==============
const char* ssid = "YOUR_SSID";           // Replace with your WiFi name
const char* password = "YOUR_PASSWORD";   // Replace with your WiFi password

// ============== API KEYS ==============
const char* openWeatherMapApiKey = "YOUR_API_KEY";  // Get from openweathermap.org
const char* city = "Shirvan";
const char* country = "IR";

// ============== TIMEZONE ==============
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3.5 * 3600;              // Iran UTC+3:30
const int daylightOffset_sec = 0;                    // No DST in Iran

// ============== GLOBAL VARIABLES ==============
unsigned long lastWeatherUpdate = 0;
const unsigned long weatherUpdateInterval = 600000; // 10 minutes
float temperature = 0;
String weatherDescription = "";
int humidity = 0;

int currentIconIndex = 0;
int currentDisplayMode = 0; // 0 = Clock, 1 = Weather
unsigned long lastTouchTime = 0;
const unsigned long debounceDelay = 200;

// Animation frame tracking
unsigned long lastAnimationUpdate = 0;
const unsigned long animationInterval = 500; // 500ms per frame (heartbeat rhythm)
int animationFrame = 0;

// ============== LED MATRIX ANIMATIONS ==============

// PULSING HEART - 2 frames (shrunk and expanded like a real heartbeat)
uint8_t pulsingHeartFrames[2][8] = {
  // Frame 1 - Shrunk/relaxed
  {
    0b00000000,
    0b00011000,
    0b00111100,
    0b00111100,
    0b00111100,
    0b00011000,
    0b00001000,
    0b00000000
  },
  // Frame 2 - Expanded/pumping
  {
    0b01100110,
    0b11111111,
    0b11111111,
    0b11111111,
    0b01111110,
    0b00111100,
    0b00011000,
    0b00000000
  }
};

// Smiley face (static)
uint8_t iconSmile[8] = {
  0b00111100,
  0b01000010,
  0b10100101,
  0b10000001,
  0b10100101,
  0b10011001,
  0b01000010,
  0b00111100
};

// Rocket (static)
uint8_t iconRocket[8] = {
  0b00011000,
  0b00111100,
  0b01111110,
  0b01111110,
  0b01111110,
  0b00111100,
  0b01100110,
  0b11000011
};

// Big filled square
uint8_t iconSquare[8] = {
  0b11111111,
  0b11111111,
  0b11111111,
  0b11111111,
  0b11111111,
  0b11111111,
  0b11111111,
  0b11111111
};

// ============== SETUP ==============
void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n\nStarting ESP32 C3 Smart Clock...");

  // Initialize OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (1);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Initializing...");
  display.display();

  // Initialize LED Matrix
  mx.begin();
  mx.clear();

  // Setup touch pins
  pinMode(TOUCH_ICON, INPUT);
  pinMode(TOUCH_DISPLAY, INPUT);

  // Connect to WiFi
  connectToWiFi();

  // Sync time with NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("Waiting for NTP time sync...");
  time_t now = time(nullptr);
  int attempts = 0;
  while (now < 24 * 3600 && attempts < 40) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
    attempts++;
  }
  Serial.println();
  Serial.println("Time synced!");

  // Initial weather fetch
  fetchWeather();

  display.clearDisplay();
}

// ============== MAIN LOOP ==============
void loop() {
  // Handle touch inputs
  handleTouchInputs();

  // Update weather periodically
  if (millis() - lastWeatherUpdate > weatherUpdateInterval) {
    fetchWeather();
  }

  // Update animation frames
  updateAnimationFrame();

  // Update displays
  updateOLED();
  updateLEDMatrix();

  delay(50);
}

// ============== ANIMATION FRAME UPDATE ==============
void updateAnimationFrame() {
  if (millis() - lastAnimationUpdate > animationInterval) {
    lastAnimationUpdate = millis();
    
    // Update frame based on current icon
    if (currentIconIndex == 0) {
      // Pulsing heart - 2 frames
      animationFrame = (animationFrame + 1) % 2;
    }
  }
}

// ============== TOUCH INPUT HANDLER ==============
void handleTouchInputs() {
  if (millis() - lastTouchTime < debounceDelay) return;

  // Check ICON touch button
  if (digitalRead(TOUCH_ICON) == HIGH) {
    currentIconIndex = (currentIconIndex + 1) % 4;
    animationFrame = 0; // Reset animation when changing icon
    lastTouchTime = millis();
    Serial.print("Icon changed to: ");
    Serial.println(currentIconIndex);
  }

  // Check DISPLAY touch button
  if (digitalRead(TOUCH_DISPLAY) == HIGH) {
    currentDisplayMode = (currentDisplayMode + 1) % 2;
    lastTouchTime = millis();
    Serial.print("Display mode changed to: ");
    Serial.println(currentDisplayMode);
  }
}

// ============== OLED UPDATE ==============
void updateOLED() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  if (currentDisplayMode == 0) {
    // Display Clock
    displayClock();
  } else {
    // Display Weather
    displayWeather();
  }

  display.display();
}

// ============== DISPLAY CLOCK ON OLED ==============
void displayClock() {
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);

  // Large time display
  display.setTextSize(2);
  display.setCursor(20, 10);
  char timeStr[9];
  strftime(timeStr, sizeof(timeStr), "%H:%M:%S", timeinfo);
  display.println(timeStr);

  // Date display
  display.setTextSize(1);
  display.setCursor(10, 35);
  char dateStr[30];
  strftime(dateStr, sizeof(dateStr), "%A, %d %B", timeinfo);
  display.println(dateStr);

  // Time zone info
  display.setCursor(10, 50);
  display.println("Iran Standard Time");

  // WiFi status
  display.setCursor(110, 56);
  if (WiFi.status() == WL_CONNECTED) {
    display.println("W");
  } else {
    display.println("X");
  }
}

// ============== DISPLAY WEATHER ON OLED ==============
void displayWeather() {
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Weather - Shirvan");
  
  display.setCursor(0, 15);
  display.print("Temp: ");
  display.print(temperature);
  display.println("C");

  display.setCursor(0, 25);
  display.print("Humidity: ");
  display.print(humidity);
  display.println("%");

  display.setCursor(0, 35);
  display.println("Condition:");
  display.setCursor(0, 45);
  display.println(weatherDescription.substring(0, 20));

  // Last update time
  display.setTextSize(1);
  display.setCursor(0, 56);
  unsigned long minutesAgo = (millis() - lastWeatherUpdate) / 60000;
  display.print("Updated ");
  display.print(minutesAgo);
  display.println("m ago");
}

// ============== LED MATRIX UPDATE ==============
void updateLEDMatrix() {
  mx.clear();

  uint8_t* currentFrame;

  // Select which icon/animation to display
  if (currentIconIndex == 0) {
    // Pulsing heart - animated (2 frames)
    currentFrame = pulsingHeartFrames[animationFrame];
  } else if (currentIconIndex == 1) {
    // Smiley face - static
    currentFrame = iconSmile;
  } else if (currentIconIndex == 2) {
    // Square - static
    currentFrame = iconSquare;
  } else {
    // Rocket - static
    currentFrame = iconRocket;
  }

  // Display the frame on the matrix
  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      if (currentFrame[row] & (1 << (7 - col))) {
        mx.setPoint(row, col, true);
      }
    }
  }
}

// ============== WiFi CONNECTION ==============
void connectToWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to WiFi");
  }
}

// ============== FETCH WEATHER FROM API ==============
void fetchWeather() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, skipping weather update");
    return;
  }

  String url = "https://api.openweathermap.org/data/2.5/weather?q=" + String(city) + "," + String(country) + "&appid=" + String(openWeatherMapApiKey) + "&units=metric";

  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode == 200) {
    String payload = http.getString();
    parseWeatherJSON(payload);
    lastWeatherUpdate = millis();
    Serial.println("Weather updated successfully!");
  } else {
    Serial.print("HTTP Error: ");
    Serial.println(httpCode);
  }

  http.end();
}

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
