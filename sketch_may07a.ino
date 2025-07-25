
// Pin configuration for IR sensors
int sensorA = 3;  // Initialize Pin 3 for IR sensor A
int sensorB = 2;  // Initialize Pin 2 for IR sensor B

// Timing and flag variables
int timer1;  // Define timer1 variable
int timer2;  // Define timer2 variable
int flag1 = 0;  // Flag for sensor A
int flag2 = 0;  // Flag for sensor B

// Speed calculation variables
float speed;
float Time;
float distance = 5.0;  // Set the distance between sensors

void setup() {
  Serial.begin(9600);
  // Define IR Sensor pins as Input
  pinMode(sensorA, INPUT);
  pinMode(sensorB, INPUT);
}

void loop() {
  // Check if sensor A is triggered
  if (digitalRead(sensorA) == LOW && flag1 == 0) {
    timer1 = millis();
    flag1 = 1;
  }

  // Check if sensor B is triggered
  if (digitalRead(sensorB) == LOW && flag2 == 0) {
    timer2 = millis();
    flag2 = 1;
  }

  // Calculate speed if both sensors are triggered
  if (flag1 == 1 && flag2 == 1) {
    if (timer1 > timer2) {
      Time = (timer1 - timer2) / 1000.0;  // Convert milliseconds to seconds
    } else {
      Time = (timer2 - timer1) / 1000.0;  // Convert milliseconds to seconds
    }
    speed = (distance / Time) * 3.6;  // Convert m/s to km/h
    Serial.println("Speed:");
    Serial.println(speed, 1);
    Serial.println("km/h");
    if (speed>40){
      Serial.println("Overspeeding");
    }
     
    if (speed==0){
      Serial.println("No Vehicle Detected");
    }
    else{
      Serial.println("Normal Speed");
    }
    // Reset flags and timers
    flag1 = 0;
    flag2 = 0;
    delay(3000);  // Display speed for 3 seconds before next measurement
  }
}
