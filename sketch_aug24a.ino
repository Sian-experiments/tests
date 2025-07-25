/*----------------Server: render by Hari, LED ok, Buzzer ok, rtos, interrupt handled, Audio file uploaded
  ------------------i2s mic integrated, sening 10chunks of raw audio.voice break fixed; SD card added(Removed hardware
  ------------------, bcz restart issue need to be fixed, LED pins changed, adding web socket, web socket working
  ------------------,  audio uploaded played it back with audacity, audio quality issue solved */

#include <WiFi.h>
#include <HTTPClient.h>
#include <WebSocketsClient.h>
// #include <FreeRTOS.h>
// #include <freertos/task.h>
#include <driver/i2s.h>
#define SLOW_BREATH 1
#define FAST_BREATH 15
#define INTERRUPT_PIN 4 // GPIO4 (D4)
#define bufferLen 1024
#define AUDIO_CHUNK 16
#define fullBuffLen bufferLen * AUDIO_CHUNK

// Connections to INMP441 I2S microphone
#define I2S_WS 32
#define I2S_SD 26
#define I2S_SCK 25

// Use I2S Processor 0
#define I2S_PORT I2S_NUM_0

int16_t sBuffer[fullBuffLen];

const char* ssid = "POCO M2 Pro";
const char* password = "thebe@tles";
bool fPushButton = false;
const unsigned long debounceDelay = 1000;

const char* webSocketHost = "report-button-render.onrender.com"; // Node.js WebSocket server
const int webSocketPort = 443; // Use port 443 for secure WebSocket (wss://)

WebSocketsClient webSocket;

bool isWebSocketConnected = false;
bool isButtonPressed = false;
bool isButtonPressedPrevious = false;


unsigned long startMillis;//for debug
unsigned long endMillis;
unsigned long elapsedMillis;


// Task handles
// TaskHandle_t BuzzerTaskHandle = NULL;

unsigned long lastDebounceTime = 0;
void IRAM_ATTR handleInterrupt() {
  unsigned long currentMillis = millis();

  // Check if enough time has passed since the last interrupt
  if (currentMillis - lastDebounceTime >= debounceDelay) {
    // Update the last debounce time
    lastDebounceTime = currentMillis;

    Serial.println("Interrupt occurred!");
    isButtonPressed = true;
  }
}

const byte BUZZER = 33;

void i2s_install() {
  // Set up I2S Processor configuration
  const i2s_config_t i2s_config = {
    .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 16000,
    .bits_per_sample = i2s_bits_per_sample_t(16),
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S ),
    .intr_alloc_flags = 0,
    .dma_buf_count = 32,
    .dma_buf_len = bufferLen,
    .use_apll = false
  };

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
}

void i2s_setpin() {
  // Set I2S pin configuration
  const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = -1,
    .data_in_num = I2S_SD
  };

  i2s_set_pin(I2S_PORT, &pin_config);
}

void setup() {

  Serial.begin(115200);

  // Set DNS servers
  IPAddress dns1(8, 8, 8, 8); // Google DNS
  IPAddress dns2(8, 8, 4, 4); // Google DNS
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, dns1, dns2);

  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected!");

  Serial.printf("Connecting to WebSocket server at %s:%d\n", webSocketHost, webSocketPort);
  webSocket.beginSSL(webSocketHost, webSocketPort, "/");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000); // 5 seconds

  pinMode(INTERRUPT_PIN, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), handleInterrupt, FALLING);

  // Set up I2S
  i2s_install();
  i2s_setpin();
  i2s_start(I2S_PORT);
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  int16_t dataArray[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

  switch (type) {
    case WStype_DISCONNECTED:
      Serial.println("WebSocket Disconnected");
      isWebSocketConnected = false;
      break;
    case WStype_CONNECTED:
      Serial.printf("WebSocket Connected to %s\n", webSocketHost);
      isWebSocketConnected = true;
      // webSocket.sendBIN((uint8_t*)dataArray, sizeof(dataArray));
      break;
    case WStype_TEXT:
      Serial.printf("WebSocket Received message: %s\n", payload);
      break;
    case WStype_BIN:
      Serial.printf("WebSocket Received binary message\n");
      break;
    case WStype_ERROR:
      Serial.println("WebSocket Error");
      break;
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
      Serial.println("WebSocket Fragment");
      break;
  }
}

int count = 0;
bool buttonState = true;
unsigned long int SentDataCount = 0;

void loop() {
  webSocket.loop();
  if (true == isButtonPressed) {

    if (isButtonPressedPrevious == false) { //if isButtonPressed is true and isButtonPressedPrevious is false, it will be updated in next cycle
      webSocket.sendTXT("START_OF_AUDIO_DATA");
      webSocket.sendTXT("TEAM4");
      Serial.println("START_OF_AUDIO_DATA & DEVICE ID Sent!");
    }
    size_t bytesIn = 0;
    esp_err_t result;

    for (int i = 0; i < AUDIO_CHUNK; i++) {
      result = i2s_read(I2S_PORT, (void*)&sBuffer[bufferLen * i], bufferLen * sizeof(int16_t), &bytesIn, portMAX_DELAY);
    }
    SentDataCount++;
    webSocket.sendBIN((uint8_t*)sBuffer, sizeof(sBuffer));
    Serial.print("Data Chunks Sent: ");
    Serial.println(SentDataCount);

    buttonState = digitalRead(INTERRUPT_PIN);
    if (true == buttonState) { //true means not pressed; flag will be resetted.
      Serial.println("Flag Cleard");
      isButtonPressed = false;
      for (int i = 0; i < AUDIO_CHUNK; i++) {
        result = i2s_read(I2S_PORT, (void*)&sBuffer[bufferLen * i], bufferLen * sizeof(int16_t), &bytesIn, portMAX_DELAY);
      }
      SentDataCount++;
      webSocket.sendBIN((uint8_t*)sBuffer, sizeof(sBuffer));
      Serial.print("Last Data Chunks Sent: ");
      Serial.println(SentDataCount);

      webSocket.sendTXT("END_OF_AUDIO_STREAM"); //when button is released
      Serial.println("END_OF_AUDIO_STREAM Sent!");
    }
  }
  isButtonPressedPrevious = isButtonPressed;//used for detecting first itration after button press
}
