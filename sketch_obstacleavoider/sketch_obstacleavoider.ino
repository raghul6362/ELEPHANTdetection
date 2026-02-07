// -------- MOTOR PINS --------
#define IN1 26
#define IN2 27
#define IN3 14
#define IN4 12
#define ENA 25
#define ENB 33

// -------- ULTRASONIC SENSOR --------
#define TRIG 5
#define ECHO 18

// -------- SPEED --------
#define SPEED_50 120 // 50% speed (PWM)

// -------- DISTANCE SETTINGS --------
#define DETECT_DISTANCE 25   // cm (obstacle detect)
#define BACK_DISTANCE   30   // cm (reverse target) ✅ CHANGED

long duration;
int distance;

void setup() {
  Serial.begin(115200);

  // Motor pins
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // Ultrasonic pins
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  // -------- PWM (ESP32 Core 3.x) --------
  ledcAttach(ENA, 1000, 8);
  ledcAttach(ENB, 1000, 8);

  ledcWrite(ENA, SPEED_50);
  ledcWrite(ENB, SPEED_50);

  randomSeed(millis());

  Serial.println("Obstacle Avoiding Car Started (50% Speed, 30cm Reverse)");
}

void loop() {
  distance = getDistance();

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  if (distance > DETECT_DISTANCE) {
    moveForward();
  } else {
    handleObstacle();
  }

  delay(30);  // smooth loop
}

// -------- OBSTACLE HANDLING --------
void handleObstacle() {
  Serial.println("Obstacle detected → reversing FIRST");

  // STEP 1: FORCE REVERSE (guaranteed movement)
  reverseCar();
  delay(500);   // always reverse visibly

  // STEP 2: CONTINUE REVERSE until 30 cm or timeout
  unsigned long startTime = millis();
  while (getDistance() < BACK_DISTANCE && millis() - startTime < 1500) {
    delay(40);
  }

  stopCar();
  delay(150);

  // STEP 3: TURN LEFT or RIGHT
  if (random(0, 2) == 0) {
    Serial.println("Turning Left");
    turnLeft();
  } else {
    Serial.println("Turning Right");
    turnRight();
  }

  delay(450);
  stopCar();
  delay(120);
}

// -------- DISTANCE FUNCTION --------
int getDistance() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  duration = pulseIn(ECHO, HIGH, 30000);
  int d = duration * 0.034 / 2;

  if (d == 0) d = 100;  // safety fallback
  return d;
}

// -------- MOTOR FUNCTIONS --------
void moveForward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void reverseCar() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void turnLeft() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void turnRight() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void stopCar() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}
