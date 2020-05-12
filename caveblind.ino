#include <Stepper.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// your WiFi settings
#define WIFI_SSID     "your WiFi SSID"
#define WIFI_PWD      "your WiFi password"

#define BAUD_RATE             57600
#define DELAY_IN_LOOP         200
#define DELAY_IN_WIFI         500

// Joystick Configuration
#define MAX_JOYSTICK          1024 
#define PIN_JOYSTICK_X        A0
#define PIN_JOYSTICK_SW       2   // D4 = GPIO_2
#define SW_OFF                1
#define SW_ON                 0
#define JOYSTICK_THRESOLD     150
#define JOYSTICK_CENTER       (MAX_JOYSTICK / 2)
#define JOYSTICK_RANGE_MIN    200
#define JOYSTICK_RANGE_MAX    900

#define MOTOR_ON_MIN          (JOYSTICK_CENTER - 400)
#define MOTOR_ON_MAX          (JOYSTICK_CENTER + 400)
#define MOTOR_ONOFF_DELAY     100

#define BLIND_OPEN_TIME_MIN   (8 * 60 + 30)  // open time 08:30
#define BLIND_OPEN_TIME_MAX   (16 * 60 + 30) // close time 16:30
#define BLIND_CLOSE           false
#define BLIND_OPEN            true
#define BLIND_CYCLE           200 // it depends on blind types

#define TIME_UPDATE_INTERVAL  0x0000FFFF
#define TIME_ZONE             -5  // your timezone

// Stepper Motor (28YBJ-48)
#define STEPS_PER_RESOLUTION  64
#define MOTOR_SPEED           220

#define DEBUG_BEGIN(baudRate) Serial.begin(baudRate)
#define DEBUG_PRINT(_name, _value) Serial.print(_name); Serial.println(_value)


unsigned long timeUpdate = TIME_UPDATE_INTERVAL;
bool motor_on = false; 
bool blind_open = BLIND_CLOSE;

WiFiUDP udp;
NTPClient ntp(udp, "pool.ntp.org", (TIME_ZONE * 60 * 60), TIME_UPDATE_INTERVAL * 60 * 1000);
Stepper stepper(STEPS_PER_RESOLUTION, D5, D7, D6, D8);


void setup() {
  DEBUG_BEGIN(BAUD_RATE);
  connectWiFi();
  stepper.setSpeed(MOTOR_SPEED);
  motorOn(false);

  runBlind(BLIND_CLOSE);
  ntp.update();
  DEBUG_PRINT("time=", ntp.getFormattedTime());
}



void loop() {
  if (checkJoystick()) {
    // do nothing
  }
  else if (checkTimeOfDay()) {
    // do nothing
  }
}



bool checkJoystick() {
  int x = analogRead(PIN_JOYSTICK_X);
  int should_motor_on = ( MOTOR_ON_MIN > x || x > MOTOR_ON_MAX );
  bool motorUpdated = (should_motor_on != motor_on);

  if (motorUpdated) {
    motor_on = should_motor_on;
    motorOn(motor_on);
  }
  
  if (motor_on) {
    if (x < JOYSTICK_RANGE_MIN) {
      stepper.step(1);  // CW 1 step
    }
    else if (x > JOYSTICK_RANGE_MAX) {
      stepper.step(-1); // CCW 1 step
    }
  }

  return motorUpdated;
}



void motorOn(bool turnOn) {
  int onoff = turnOn ? HIGH : LOW;
  digitalWrite(D5, onoff);
  digitalWrite(D6, onoff);
  digitalWrite(D7, onoff);
  digitalWrite(D8, onoff);
  delay(MOTOR_ONOFF_DELAY);
}



void runBlind(bool openBlind) {
  DEBUG_PRINT("runBlind, open=", openBlind);
  motorOn(true);
  for (int i=0; i<BLIND_CYCLE; i++) {
    stepper.step(openBlind ? 1 : -1);  // CW : CCW
  }
  motorOn(false);
}



bool checkTimeOfDay() {
  bool blindUpdated = false;  
  if (timeUpdate <= 0) {
    if (WiFi.status() == WL_CONNECTED) {
      ntp.update();
    }
    
    DEBUG_PRINT("time=", ntp.getFormattedTime());
    
    int hours = ntp.getHours();
    int minutes = ntp.getMinutes();
    int dailyTime = hours * 60 + minutes;
    bool shouldBlindOpen = ( BLIND_OPEN_TIME_MIN < dailyTime && dailyTime < BLIND_OPEN_TIME_MAX );
    
    blindUpdated = (shouldBlindOpen != blind_open);
    if (blindUpdated) {
      runBlind(shouldBlindOpen);
      blind_open = shouldBlindOpen;
    }
    timeUpdate = TIME_UPDATE_INTERVAL;
  }
  timeUpdate--;
  
  return blindUpdated;
}



void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(DELAY_IN_LOOP);

  Serial.print("device mac: ");
  Serial.println(WiFi.macAddress());

  WiFi.begin(WIFI_SSID, WIFI_PWD);
  Serial.print("Connecting to ");
  Serial.print(WIFI_SSID); 
  Serial.println(" ...");
  
  Serial.println("Waiting for WiFi");
  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(DELAY_IN_WIFI);
    Serial.print(".");
    if (i >= 9) {
      i = 0;
      Serial.println("");
    }
    else {
      i++;
    }
  }
  Serial.println("connected");

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.println("network is ready!\n");
}
