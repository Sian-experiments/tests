#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <RTClib.h>
#include <Wire.h>

// WiFi credentials
const char* ssid = "Royal life";
const char* password = "skytech2011";

// Server configuration
const char* serverURL = "http://192.168.0.177:5000/api/rainfall";
const int ledPin=2;
// Rain sensor pin
const int rainSensorPin = 4;

// RTC object
RTC_DS3231 rtc;

// Rain detection variables
bool currentRainState = false;
bool previousRainState = false;
bool stableRainState = false;
int currentEventId = -1;

// Filtering variables
int consecutiveReadings = 0;
const int requiredConsecutiveReadings = 3; // Need 3 consecutive same readings
bool lastRawReading = false;

void setup() {
  Serial.begin(115200);
  
  // Initialize I2C for RTC
  Wire.begin();
  pinMode(ledPin,OUTPUT);
  digitalWrite(ledPin, LOW); // Start with LED off
  
  // Initialize RTC
  if (!rtc.begin()) {
    Serial.println("❌ Couldn't find RTC module!");
    Serial.println("Check wiring: SDA->GPIO21, SCL->GPIO22, VCC->3.3V, GND->GND");
    while (1) delay(1000);
  }
  
  // Setup rain sensor pin
  pinMode(rainSensorPin, INPUT_PULLUP);
  
  // Connect to WiFi
  connectToWiFi();
  
  // Check RTC status
  checkRTCStatus();
  
  // Initialize with current sensor state
  int initialReading = digitalRead(rainSensorPin);
  previousRainState = (initialReading == LOW);
  stableRainState = previousRainState;
  
  Serial.println("=== Rain Sensor System Ready ===");
  Serial.print("Initial sensor state: ");
  Serial.println(stableRainState ? "RAINING" : "DRY");
  Serial.println("Monitoring for ACTUAL state changes...");
}

void loop() {
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
  }
  
  // Display current time every 30 seconds
  static unsigned long lastTimeDisplay = 0;
  if (millis() - lastTimeDisplay > 30000) {
    displayCurrentTime();
    lastTimeDisplay = millis();
  }
  
  // Check rain sensor
  checkRainSensor();
  
  delay(2000); // Check every 2 seconds
}

void checkRTCStatus() {
  Serial.println("🕐 Checking RTC status...");
  
  if (rtc.lostPower()) {
    Serial.println("⚠️  RTC lost power! Time may be incorrect.");
    Serial.println("Please set the correct time using setRTCTime() function");
    Serial.println("Example: setRTCTime(2025, 6, 12, 14, 30, 0); // Year, Month, Day, Hour, Minute, Second");
  } else {
    Serial.println("✅ RTC is running normally");
  }
  
  displayCurrentTime();
}

void displayCurrentTime() {
  // Get time from RTC (treating as local time)
  DateTime rtcTime = rtc.now();
  
  Serial.println("=== Current Time Status ===");
  Serial.println("RTC Local Time: " + formatDateTime(rtcTime));
  Serial.println("Temperature: " + String(rtc.getTemperature()) + "°C");
  Serial.println("Timezone: IST (Local Time)");
}

void checkRainSensor() {
  // Read raw sensor value
  int rawSensorValue = digitalRead(rainSensorPin);
  bool rawRainReading = (rawSensorValue == LOW); // LOW = rain detected
  
  // Print raw reading for debugging
  Serial.print("Raw sensor: ");
  Serial.print(rawSensorValue);
  Serial.print(" | Current stable state: ");
  Serial.print(stableRainState ? "RAINING" : "DRY");
  
  // Check if reading is consistent
  if (rawRainReading == lastRawReading) {
    consecutiveReadings++;
    Serial.print(" | Consecutive: ");
    Serial.print(consecutiveReadings);
  } else {
    consecutiveReadings = 1; // Reset counter
    lastRawReading = rawRainReading;
    Serial.print(" | Reset counter");
  }
  
  Serial.println();
  
  // Only change state if we have enough consecutive readings
  if (consecutiveReadings >= requiredConsecutiveReadings) {
    currentRainState = rawRainReading;
    
    // Check for ACTUAL state change
    if (currentRainState != stableRainState) {
      Serial.println("=== STATE CHANGE DETECTED ===");
      displayCurrentTime();
      Serial.print("Previous state: ");
      Serial.println(stableRainState ? "RAINING" : "DRY");
      Serial.print("New state: ");
      Serial.println(currentRainState ? "RAINING" : "DRY");
      
      // Update stable state
      stableRainState = currentRainState;
      
      if (currentRainState) {
        // Rain started
        Serial.println("🌧️  RAIN STARTED!");
        sendRainEvent("start");
        digitalWrite(ledPin, HIGH); // Turn LED on when raining
      } else {
        // Rain stopped
        Serial.println("☀️  RAIN STOPPED!");
        sendRainEvent("end");
        currentEventId = -1; // Reset event ID
        digitalWrite(ledPin, LOW); // Turn LED off when not raining
      }
      
      // Reset consecutive counter after state change
      consecutiveReadings = 0;
    }
  }
}

void sendRainEvent(String eventType) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ Cannot send: WiFi not connected");
    return;
  }

  Serial.println("📡 Sending rain event: " + eventType);

  HTTPClient http;
  http.begin(serverURL);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(10000);
  
  // Get current time from RTC (local time)
  DateTime rtcTime = rtc.now();
  String timestamp = formatISODateTime(rtcTime);
  
  Serial.println("Using RTC local time: " + timestamp);
  
  // Create JSON payload
  StaticJsonDocument<400> doc;
  doc["event"] = eventType;
  doc["timestamp"] = timestamp;
  doc["sensor_pin"] = rainSensorPin;
  doc["sensor_state"] = stableRainState;
  doc["rtc_temperature"] = rtc.getTemperature();
  doc["time_source"] = "RTC";
  doc["timezone_offset"] = "+05:30"; // IST timezone
  
  // Add event ID for end events
  if (eventType == "end" && currentEventId > 0) {
    doc["id"] = currentEventId;
  }
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  Serial.println("JSON: " + jsonString);
  
  // Send POST request
  int httpCode = http.POST(jsonString);
  
  if (httpCode > 0) {
    String response = http.getString();
    Serial.printf("✅ HTTP %d: %s\n", httpCode, response.c_str());
    
    // Extract event ID from response for start events
    if (eventType == "start" && httpCode == 200) {
      StaticJsonDocument<200> responseDoc; // Increased size
      DeserializationError error = deserializeJson(responseDoc, response);
      if (error == DeserializationError::Ok) {
        if (responseDoc.containsKey("id")) {
          currentEventId = responseDoc["id"];
          Serial.println("✅ Event ID stored: " + String(currentEventId));
        } else {
          Serial.println("⚠️ No ID field in response");
        }
      } else {
        Serial.println("❌ Failed to parse JSON response: " + String(error.c_str()));
        Serial.println("Response was: " + response);
      }
    }
  } else {
    Serial.printf("❌ HTTP Error %d: %s\n", httpCode, http.errorToString(httpCode).c_str());
  }
  
  http.end();
}

// Format DateTime for display
String formatDateTime(DateTime dt) {
  char buffer[25];
  snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d",
           dt.year(), dt.month(), dt.day(),
           dt.hour(), dt.minute(), dt.second());
  return String(buffer);
}

// Format DateTime as ISO 8601 with IST timezone
String formatISODateTime(DateTime dt) {
  char buffer[30];
  snprintf(buffer, sizeof(buffer), "%04d-%02d-%02dT%02d:%02d:%02d+05:30",
           dt.year(), dt.month(), dt.day(),
           dt.hour(), dt.minute(), dt.second());
  return String(buffer);
}

// Get date string (YYYY-MM-DD)
String getDateString(DateTime dt) {
  char buffer[12];
  snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d",
           dt.year(), dt.month(), dt.day());
  return String(buffer);
}

// Get time string (HH:MM:SS)
String getTimeString(DateTime dt) {
  char buffer[10];
  snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d",
           dt.hour(), dt.minute(), dt.second());
  return String(buffer);
}

void connectToWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n❌ WiFi Connection Failed!");
  }
}

// Manual time setting function (call from Serial Monitor)
// Set the RTC to current local time (IST)
void setRTCTime(int year, int month, int day, int hour, int minute, int second) {
  rtc.adjust(DateTime(year, month, day, hour, minute, second));
  Serial.println("✅ RTC time set to: " + formatDateTime(rtc.now()));
  Serial.println("This time is treated as local IST time");
}

// Function to display comprehensive time info
void displayTimeInfo() {
  Serial.println("=== RTC Time Information ===");
  
  DateTime rtcTime = rtc.now();
  Serial.println("RTC Local Time: " + formatDateTime(rtcTime));
  Serial.println("RTC ISO Format: " + formatISODateTime(rtcTime));
  Serial.println("RTC Temperature: " + String(rtc.getTemperature()) + "°C");
  Serial.println("Timezone: IST (UTC+05:30)");
  Serial.println("WiFi Status: " + String(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected"));
  
  if (rtc.lostPower()) {
    Serial.println("⚠️  Warning: RTC lost power - time may be incorrect");
  } else {
    Serial.println("✅ RTC is running normally");
  }
}

// Helper function to set time easily from serial monitor
void setCurrentTime() {
  Serial.println("=== Manual Time Setting ===");
  Serial.println("To set the current time, use: setRTCTime(year, month, day, hour, minute, second)");
  Serial.println("Example for June 12, 2025, 2:30:45 PM IST:");
  Serial.println("setRTCTime(2025, 6, 12, 14, 30, 45);");
  Serial.println();
  Serial.println("Current RTC time: " + formatDateTime(rtc.now()));
}

// Function to get current unix timestamp from RTC
unsigned long getRTCUnixTime() {
  DateTime rtcTime = rtc.now();
  return rtcTime.unixtime();
}
