#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP32Servo.h>
#include <TinyGPSPlus.h>

// ---------------- WIFI ----------------
const char* ssid = "";
const char* password = "";

// ---------------- TELEGRAM ----------------
#define BOT_TOKEN ""
#define CHAT_ID   ""

// ---------------- PINS ----------------
#define SERVO_PIN 23
#define TRIG_PIN  5
#define ECHO_PIN  18
#define PIR_PIN   22

#define GPS_RX 16
#define GPS_TX 17

// ---------------- SETTINGS ----------------
#define ALERT_DISTANCE    100     // cm
#define SERVO_STEP_DELAY  50      // ms (slow smooth scan)
#define TELEGRAM_DELAY   10000   // ms anti-spam

// ---------------- OBJECTS ----------------
Servo scanServo;
WiFiClientSecure client;
TinyGPSPlus gps;
HardwareSerial gpsSerial(2);

// ---------------- SERVO STATE ----------------
int servoAngle = 0;
int servoDir = 1;
unsigned long lastServoMove = 0;

// ---------------- ALERT STATE ----------------
unsigned long lastTelegramTime = 0;
bool alertTriggered = false;
int alertAngle = 0;
long alertDistance = 0;
bool alertMotion = false;

// ---------------- URL ENCODE ----------------
String urlEncode(const String &msg) {
  String encoded = "";
  char c;
  char buf[5];
  for (int i = 0; i < msg.length(); i++) {
    c = msg.charAt(i);
    if (isalnum(c)) encoded += c;
    else {
      sprintf(buf, "%%%02X", c);
      encoded += buf;
    }
  }
  return encoded;
}

// ---------------- TELEGRAM SEND ----------------
void sendTelegram(String message) {
  client.setInsecure();
  if (!client.connect("api.telegram.org", 443)) return;

  String url = "/bot" + String(BOT_TOKEN) +
               "/sendMessage?chat_id=" + String(CHAT_ID) +
               "&text=" + urlEncode(message);

  client.print(
    String("GET ") + url + " HTTP/1.1\r\n" +
    "Host: api.telegram.org\r\n" +
    "Connection: close\r\n\r\n"
  );

  delay(600);
  client.stop();
}

// ---------------- ULTRASONIC ----------------
long getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 25000);
  if (duration == 0) return -1;
  return duration * 0.034 / 2;
}

// ---------------- GPS LINK ----------------
String getLocationLink() {
  if (gps.location.isValid()) {
    return "https://www.google.com/maps?q=" +
           String(gps.location.lat(), 6) + "," +
           String(gps.location.lng(), 6);
  }
  return "waiting for sateelite response";
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);

  scanServo.attach(SERVO_PIN);
  scanServo.write(servoAngle);

  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  Serial.println("ESP32 radar system running");
}

// ---------------- LOOP ----------------
void loop() {

  // ---- GPS FEED ----
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  // ---- SERVO STEP (NON-BLOCKING) ----
  if (millis() - lastServoMove >= SERVO_STEP_DELAY) {
    lastServoMove = millis();

    servoAngle += servoDir;
    if (servoAngle >= 180) { servoAngle = 180; servoDir = -1; }
    if (servoAngle <= 0)   { servoAngle = 0;   servoDir = 1;  }

    scanServo.write(servoAngle);

    long dist = getDistance();
    bool motion = digitalRead(PIR_PIN);

    // -------- DEBUG OUTPUT --------
    Serial.printf(
      "Angle: %d | Distance: %ld cm | PIR: %s\n",
      servoAngle, dist, motion ? "MOTION" : "NO"
    );

    // -------- PROCESSING OUTPUT (CSV) --------
    Serial.print(servoAngle);
    Serial.print(",");
    Serial.println(dist);

    // -------- DETECTION LOGIC --------
    if ((dist > 0 && dist <= ALERT_DISTANCE) || motion) {
      alertTriggered = true;
      alertAngle = servoAngle;
      alertDistance = dist;
      alertMotion = motion;
    }
  }

  // ---- TELEGRAM ALERT ----
  if (alertTriggered && millis() - lastTelegramTime > TELEGRAM_DELAY) {

    String msg =
      "🚨 ALERT DETECTED\n"
      "Angle: " + String(alertAngle) + "°\n"
      "Distance: " + String(alertDistance) + " cm\n"
      "Motion: " + String(alertMotion ? "YES" : "NO") + "\n"
      "📍 Location:\n" + getLocationLink();

    sendTelegram(msg);
    lastTelegramTime = millis();
    alertTriggered = false;
  }
}
