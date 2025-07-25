#include <WiFi.h>
#include <HTTPClient.h>
#include <RTClib.h>
#include <Wire.h>

// WiFi credentials
const char* ssid = "TrEST Park WiFi";
const char* password = "4712598555";

// Google Apps Script Web App URL
const char* googleScriptURL = "https://script.google.com/macros/s/AKfycbzZs1zkHVffQO2PHhwXTqGanZFGfVqRkNuF3j8L7bqpfKjqYX7nyxkJU56lyBYNs3LJ/exec";

// Rain sensor pin
const int rainSensorPin = 4;
const int ledPin = 2;

// RTC object
RTC_DS3231 rtc;

// Data structure for sensor data
struct SensorData {
  String date;
  String time;
  String event;
  String sensor_state;
  String temperature;
  String device_id;
  String timezone;
  String unix_timestamp;
  String duration;
  String rain_start_time;
  String rain_end_time;
};

// Rain detection variables
bool currentRainState = false;
bool stableRainState = false;
bool rainEventActive = false;

// Duration tracking variables
unsigned long rainStartTime = 0;
unsigned long rainEndTime = 0;

// Filtering variables
int consecutiveReadings = 0;
const int requiredConsecutiveReadings = 3;
bool lastRawReading = false;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  
  // Wait for serial to initialize
  delay(2000);
  Serial.println("\n=== ESP32 Rain Sensor Starting ===");
  pinMode(ledPin, OUTPUT);
  // Scan for available networks first
  scanWiFiNetworks();
  
  // Initialize RTC
  if (!rtc.begin()) {
    Serial.println("❌ Couldn't find RTC module!");
    while (1) delay(1000);
  }
  
  pinMode(rainSensorPin, INPUT_PULLUP);
  connectToWiFi();
  
  // Initialize sensor state
  int initialReading = digitalRead(rainSensorPin);
  stableRainState = (initialReading == LOW);
  rainEventActive = false;
  
  Serial.println("=== Rain Sensor System Ready ===");
  Serial.print("Initial state: ");
  Serial.println(stableRainState ? "RAINING" : "DRY");
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
  }
  
  static unsigned long lastTimeDisplay = 0;
  if (millis() - lastTimeDisplay > 30000) {
    displayCurrentTime();
    lastTimeDisplay = millis();
  }
  
  checkRainSensor();
  delay(2000);
}

void checkRainSensor() {
  int rawSensorValue = digitalRead(rainSensorPin);
  bool rawRainReading = (rawSensorValue == LOW);
  
  Serial.print("Raw: ");
  Serial.print(rawSensorValue);
  Serial.print(" | State: ");
  Serial.print(stableRainState ? "RAINING" : "DRY");
  
  if (rawRainReading == lastRawReading) {
    consecutiveReadings++;
    Serial.print(" | Consecutive: ");
    Serial.print(consecutiveReadings);
  } else {
    consecutiveReadings = 1;
    lastRawReading = rawRainReading;
    Serial.print(" | Reset");
  }
  Serial.println();
  
  if (consecutiveReadings >= requiredConsecutiveReadings) {
    currentRainState = rawRainReading;
    
    if (currentRainState != stableRainState) {
      Serial.println("=== STATE CHANGE ===");
      displayCurrentTime();
      Serial.print("Previous: ");
      Serial.println(stableRainState ? "RAINING" : "DRY");
      Serial.print("New: ");
      Serial.println(currentRainState ? "RAINING" : "DRY");
      
      stableRainState = currentRainState;
      
      if (currentRainState) {
        rainStartTime = rtc.now().unixtime();
        Serial.println("🌧️  RAIN STARTED!");
        sendToGoogleSheets("Rain Started");
        rainEventActive = true;
        digitalWrite(ledPin, HIGH);   // turn the LED on (HIGH is the voltage level)
                            // wait for a second
      } else {
        rainEndTime = rtc.now().unixtime();
        Serial.println("☀️  RAIN STOPPED!");
        if (rainEventActive) {
          sendToGoogleSheets("Rain Stopped");
          rainEventActive = false;
          digitalWrite(ledPin, LOW);    // turn the LED off by making the voltage LOW
          
        }
      }
      consecutiveReadings = 0;
    }
  }
}

// Function to create sensor data struct
SensorData createSensorData(String event) {
  SensorData data;
  DateTime rtcTime = rtc.now();
  unsigned long currentUnixTime = rtcTime.unixtime();
  
  // Populate all fields as strings
  data.date = getDateString(rtcTime);
  data.time = getTimeString(rtcTime);
  data.event = event;
  data.sensor_state = stableRainState ? "RAINING" : "DRY";
  data.temperature = String(rtc.getTemperature(), 1);
  data.device_id = "ESP32_Rain_Sensor";
  data.timezone = "IST";
  data.unix_timestamp = String(currentUnixTime);
  data.rain_start_time = String(rainStartTime);
  data.rain_end_time = String(rainEndTime);
  
  // Calculate duration for rain stopped events
  if (event == "Rain Stopped" && rainStartTime > 0) {
    unsigned long durationSeconds = currentUnixTime - rainStartTime;
    data.duration = formatDuration(durationSeconds);
  } else {
    data.duration = "0";
  }
  
  return data;
}

void sendToGoogleSheets(String event) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ Cannot send: WiFi not connected");
    return;
  }

  Serial.println("📡 Sending: " + event);

  // Create sensor data struct
  SensorData sensorData = createSensorData(event);
  
  // Print struct data for debugging
  printSensorData(sensorData);

  HTTPClient http;
  http.begin(googleScriptURL);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.setTimeout(15000);
  
  // Build POST data from struct
  String postData = buildPostData(sensorData);
  
  int httpCode = http.POST(postData);
  
  if (httpCode > 0) {
    String response = http.getString();
    Serial.printf("✅ HTTP %d: %s\n", httpCode, response.c_str());
  } else {
    Serial.printf("❌ Error %d: %s\n", httpCode, http.errorToString(httpCode).c_str());
  }
  
  http.end();
}

// Function to build POST data from struct
String buildPostData(const SensorData& data) {
  String postData = "";
  postData += "date=" + urlEncode(data.date);
  postData += "&time=" + urlEncode(data.time);
  postData += "&event=" + urlEncode(data.event);
  postData += "&sensor_state=" + urlEncode(data.sensor_state);
  postData += "&temperature=" + urlEncode(data.temperature);
  postData += "&device_id=" + urlEncode(data.device_id);
  postData += "&timezone=" + urlEncode(data.timezone);
  postData += "&unix_timestamp=" + urlEncode(data.unix_timestamp);
  postData += "&duration=" + urlEncode(data.duration);
  postData += "&rain_start_time=" + urlEncode(data.rain_start_time);
  postData += "&rain_end_time=" + urlEncode(data.rain_end_time);
  
  return postData;
}

// Function to print struct data for debugging
void printSensorData(const SensorData& data) {
  Serial.println("=== Sensor Data Struct ===");
  Serial.println("Date: " + data.date);
  Serial.println("Time: " + data.time);
  Serial.println("Event: " + data.event);
  Serial.println("Sensor State: " + data.sensor_state);
  Serial.println("Temperature: " + data.temperature + "°C");
  Serial.println("Device ID: " + data.device_id);
  Serial.println("Timezone: " + data.timezone);
  Serial.println("Unix Timestamp: " + data.unix_timestamp);
  Serial.println("Duration: " + data.duration);
  Serial.println("Rain Start Time: " + data.rain_start_time);
  Serial.println("Rain End Time: " + data.rain_end_time);
  Serial.println("========================");
}

String formatDuration(unsigned long durationSeconds) {
  if (durationSeconds == 0) return "0s";
  
  unsigned long hours = durationSeconds / 3600;
  unsigned long minutes = (durationSeconds % 3600) / 60;
  unsigned long seconds = durationSeconds % 60;
  
  String result = "";
  if (hours > 0) result += String(hours) + "h ";
  if (minutes > 0) result += String(minutes) + "m ";
  if (seconds > 0 || result == "") result += String(seconds) + "s";
  
  // Remove trailing space instead of using trim()
  if (result.endsWith(" ")) {
    result = result.substring(0, result.length() - 1);
  }
  
  return result;
}

void displayCurrentTime() {
  DateTime rtcTime = rtc.now();
  Serial.println("Time: " + formatDateTime(rtcTime) + " IST");
  Serial.println("Temp: " + String(rtc.getTemperature()) + "°C");
}

String formatDateTime(DateTime dt) {
  char buffer[25];
  snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d",
           dt.year(), dt.month(), dt.day(),
           dt.hour(), dt.minute(), dt.second());
  return String(buffer);
}

String getDateString(DateTime dt) {
  char buffer[12];
  snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d",
           dt.year(), dt.month(), dt.day());
  return String(buffer);
}

String getTimeString(DateTime dt) {
  char buffer[10];
  snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d",
           dt.hour(), dt.minute(), dt.second());
  return String(buffer);
}

String urlEncode(String str) {
  String encodedString = "";
  char c;
  
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (c == ' ') {
      encodedString += '+';
    } else if (isalnum(c)) {
      encodedString += c;
    } else {
      char code0, code1;
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9) code1 = (c & 0xf) - 10 + 'A';
      c = (c >> 4) & 0xf;
      code0 = c + '0';
      if (c > 9) code0 = c - 10 + 'A';
      encodedString += '%';
      encodedString += code0;
      encodedString += code1;
    }
  }
  return encodedString;
}


void connectToWiFi() {
  // Disconnect any previous connection
  WiFi.disconnect();
  delay(1000);
  
  // Set WiFi mode to station
  WiFi.mode(WIFI_STA);
  
  Serial.println("=== WiFi Connection Attempt ===");
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("Password Length: ");
  Serial.println(strlen(password));
  
  // Start connection
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(1000);
    Serial.print(".");
    
    // Print connection status every 5 attempts
    if (attempts % 5 == 0) {
      Serial.print("\nWiFi Status: ");
      printWiFiStatus();
      Serial.println(); // Ensure status prints on a new line
    }
    
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi Connected Successfully!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Gateway: ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("Subnet Mask: ");
    Serial.println(WiFi.subnetMask());
    Serial.print("DNS: ");
    Serial.println(WiFi.dnsIP());
    Serial.print("RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    Serial.print("MAC Address: ");
    Serial.println(WiFi.macAddress());
  } else {
    Serial.println("\n❌ WiFi Connection Failed!");
    Serial.print("Final Status: ");
    printWiFiStatus();
    Serial.println("\nTroubleshooting tips:");
    Serial.println("1. Check SSID and password");
    Serial.println("2. Move closer to router");
    Serial.println("3. Check if 2.4GHz is enabled");
    Serial.println("4. Try restarting the ESP32");
  }
}

void printWiFiStatus() {
  switch(WiFi.status()) {
    case WL_IDLE_STATUS:
      Serial.print("IDLE");
      break;
    case WL_NO_SSID_AVAIL:
      Serial.print("NO_SSID_AVAILABLE");
      break;
    case WL_SCAN_COMPLETED:
      Serial.print("SCAN_COMPLETED");
      break;
    case WL_CONNECTED:
      Serial.print("CONNECTED");
      break;
    case WL_CONNECT_FAILED:
      Serial.print("CONNECT_FAILED");
      break;
    case WL_CONNECTION_LOST:
      Serial.print("CONNECTION_LOST");
      break;
    case WL_DISCONNECTED:
      Serial.print("DISCONNECTED");
      break;
    default:
      Serial.print("UNKNOWN");
      break;
  }
}

// Manual time setting function
void setRTCTime(int year, int month, int day, int hour, int minute, int second) {
  rtc.adjust(DateTime(year, month, day, hour, minute, second));
  Serial.println("✅ RTC time set to: " + formatDateTime(rtc.now()));
}

// WiFi network scanning function
void scanWiFiNetworks() {
  Serial.println("=== Scanning WiFi Networks ===");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  int n = WiFi.scanNetworks();
  Serial.println("Scan completed");
  
  if (n == 0) {
    Serial.println("No networks found");
  } else {
    Serial.print(n);
    Serial.println(" networks found:");
    Serial.println("Nr | SSID                             | RSSI | CH | Encryption");
    Serial.println("---|----------------------------------|------|----|-----------");
    
    for (int i = 0; i < n; ++i) {
      Serial.printf("%2d", i + 1);
      Serial.print(" | ");
      Serial.printf("%-32.32s", WiFi.SSID(i).c_str());
      Serial.print(" | ");
      Serial.printf("%4d", WiFi.RSSI(i));
      Serial.print(" | ");
      Serial.printf("%2d", WiFi.channel(i));
      Serial.print(" | ");
      
      switch (WiFi.encryptionType(i)) {
        case WIFI_AUTH_OPEN:
          Serial.print("Open");
          break;
        case WIFI_AUTH_WEP:
          Serial.print("WEP");
          break;
        case WIFI_AUTH_WPA_PSK:
          Serial.print("WPA");
          break;
        case WIFI_AUTH_WPA2_PSK:
          Serial.print("WPA2");
          break;
        case WIFI_AUTH_WPA_WPA2_PSK:
          Serial.print("WPA/WPA2");
          break;
        case WIFI_AUTH_WPA2_ENTERPRISE:
          Serial.print("WPA2-EAP");
          break;
        default:
          Serial.print("Unknown");
      }
      Serial.println();
      
      // Check if this is our target network
      if (WiFi.SSID(i) == ssid) {
        Serial.println(">>> Found target network!");
      } 
    }
  }
  Serial.println("=== End WiFi Scan ===\n");
}
