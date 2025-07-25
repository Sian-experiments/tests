#include <WiFi.h>
#include <HTTPClient.h>
#include <RTClib.h>
#include <Wire.h>

// WiFi credentials
const char* ssid = "Royal life";
const char* password = "skytech2011";

// Google Apps Script Web App URL
const char* googleScriptURL = "https://script.google.com/macros/s/AKfycby54dDQjWaRJvoXIu7bjxMkuYS5DNb-26Es3UhyfvMDCh9DSkPCL5Ax7SpKpJ0DJRM4Ow/exec";

// Rain sensor pin
const int rainSensorPin = 4;

// RTC object
RTC_DS3231 rtc;

// Rain detection variables
bool currentRainState = false;
bool stableRainState = false;
bool rainEventActive = false;

// Timestamp tracking variables
unsigned long rainStartTime = 0;
unsigned long rainEndTime = 0;

// Filtering variables
int consecutiveReadings = 0;
const int requiredConsecutiveReadings = 3;
bool lastRawReading = false;

void setup() {
 Serial.begin(115200);
 Wire.begin();

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
 Serial.println("🌧️ RAIN STARTED!");
 Serial.println("Rain start time: " + String(rainStartTime));
 sendToGoogleSheets("Rain Started");
 rainEventActive = true;
 } else {
 if (rainEventActive) {
 rainEndTime = rtc.now().unixtime();
 Serial.println("☀️ RAIN STOPPED!");
 Serial.println("Rain start time: " + String(rainStartTime));
 Serial.println("Rain end time: " + String(rainEndTime));

 sendToGoogleSheets("Rain Stopped");
 rainEventActive = false;
 }
 }
 consecutiveReadings = 0;
 }
 }
}

void sendToGoogleSheets(String event) {
 if (WiFi.status() != WL_CONNECTED) {
 Serial.println("❌ Cannot send: WiFi not connected");
 return;
 }

 Serial.println("📡 Sending: " + event);

 HTTPClient http;
 http.begin(googleScriptURL);
 http.addHeader("Content-Type", "application/x-www-form-urlencoded");
 http.setTimeout(15000);

 DateTime rtcTime = rtc.now();
 String date = getDateString(rtcTime);
 String time = getTimeString(rtcTime);
 unsigned long currentUnixTime = rtcTime.unixtime();

 // Determine the correct sensor state for the event
 String sensorState;
 if (event == "Rain Started") {
 sensorState = "RAINING";
 } else if (event == "Rain Stopped") {
 sensorState = "DRY";
 } else {
 sensorState = stableRainState ? "RAINING" : "DRY";
 }

 // Send timestamps for tracking
 String rainStartTimeStr = "0";
 String rainEndTimeStr = "0";
 String rainStopTime = "";
 String rainStopDate = "";

 if (event == "Rain Started") {
 rainStartTimeStr = String(currentUnixTime);
 } else if (event == "Rain Stopped") {
 rainStartTimeStr = String(rainStartTime);
 rainEndTimeStr = String(rainEndTime);
 rainStopTime = time;
 rainStopDate = date;

 Serial.println("Rain started at: " + String(rainStartTime));
 Serial.println("Rain ended at: " + String(rainEndTime));
 }

 String postData = "date=" + urlEncode(date) +
 "&time=" + urlEncode(time) +
 "&event=" + urlEncode(event) +
 "&sensor_state=" + urlEncode(sensorState) +
 "&temperature=" + urlEncode(String(rtc.getTemperature(), 1)) +
 "&sensor_pin=" + urlEncode(String(rainSensorPin)) +
 "&device_id=" + urlEncode("ESP32_Rain_Sensor") +
 "&timezone=" + urlEncode("IST") +
 "&unix_timestamp=" + urlEncode(String(currentUnixTime)) +
 "&rain_start_time=" + urlEncode(rainStartTimeStr) +
 "&rain_end_time=" + urlEncode(rainEndTimeStr) +
 "&rain_stop_time=" + urlEncode(rainStopTime) +
 "&rain_stop_date=" + urlEncode(rainStopDate);

 Serial.println("POST Data: " + postData);

 int httpCode = http.POST(postData);

 if (httpCode > 0) {
 String response = http.getString();
 Serial.printf("✅ HTTP %d: %s\n", httpCode, response.c_str());
 } else {
 Serial.printf("❌ Error %d: %s\n", httpCode, http.errorToString(httpCode).c_str());
 }

 http.end();
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
 Serial.print("IP: ");
 Serial.println(WiFi.localIP());
 } else {
 Serial.println("\n❌ WiFi Failed!");
 }
}

// Manual time setting function
void setRTCTime(int year, int month, int day, int hour, int minute, int second) {
 rtc.adjust(DateTime(year, month, day, hour, minute, second));
 Serial.println("✅ RTC time set to: " + formatDateTime(rtc.now()));
}
