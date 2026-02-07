import processing.serial.*;

// ================= USER SETTINGS =================
int PORT_INDEX = 0;        // <<< CHANGE THIS TO YOUR ESP32 PORT
int BAUD_RATE = 115200;
int MAX_DISTANCE = 200;   // cm
// =================================================

Serial myPort;
String data = "";

float angle = 0;
float distance = 0;

float radarRadius = 320;
ArrayList<PVector> points = new ArrayList<PVector>();

void setup() {
  size(1200, 750, P3D);
  smooth(8);
  frameRate(60);

  // ---- PRINT PORTS CLEARLY ----
  String[] ports = Serial.list();
  println("Available Serial Ports:");
  for (int i = 0; i < ports.length; i++) {
    println(i + " : " + ports[i]);
  }

  // ---- OPEN USER-SELECTED PORT ----
  myPort = new Serial(this, ports[PORT_INDEX], BAUD_RATE);
  myPort.bufferUntil('\n');
}

void draw() {
  background(5, 8, 5);   // dark radar-green background

  // ---- LIGHTING ----
  ambientLight(30, 80, 30);
  directionalLight(0, 255, 0, 0, -1, -1);
  lights();

  // ---- CAMERA ----
  translate(width/2, height*0.8, -420);
  rotateX(PI/3.3);
  rotateY(map(mouseX, 0, width, -0.4, 0.4)); // mouse interaction

  drawRadarFloor();
  drawRadarHUD();
  drawScanCone();
  drawObstacles();
}

// ================= FLOOR MESH =================
void drawRadarFloor() {
  stroke(0, 120, 0, 120);
  strokeWeight(1);

  int gridSize = 40;
  int gridCount = 18;

  for (int i = -gridCount; i <= gridCount; i++) {
    line(i * gridSize, 0, -gridCount * gridSize,
         i * gridSize, 0,  gridCount * gridSize);
    line(-gridCount * gridSize, 0, i * gridSize,
          gridCount * gridSize, 0, i * gridSize);
  }
}

// ================= HUD RINGS =================
void drawRadarHUD() {
  stroke(0, 200, 0, 160);
  noFill();

  for (int i = 1; i <= 4; i++) {
    float r = radarRadius * i / 4;
    pushMatrix();
    rotateX(HALF_PI);
    ellipse(0, 0, r * 2, r * 2);
    popMatrix();
  }
}

// ================= SCAN CONE =================
void drawScanCone() {
  pushMatrix();
  rotateY(radians(angle));

  noStroke();
  fill(0, 255, 0, 45);

  beginShape(TRIANGLE_FAN);
  vertex(0, 0, 0);

  for (int i = -10; i <= 10; i++) {
    float a = radians(i * 2);
    float x = radarRadius * cos(a);
    float z = radarRadius * sin(a);
    vertex(x, 0, -z);
  }
  endShape();

  popMatrix();
}

// ================= OBSTACLES =================
void drawObstacles() {

  // add new detection
  if (distance > 0 && distance <= MAX_DISTANCE) {
    float r = map(distance, 0, MAX_DISTANCE, 0, radarRadius);
    float x = r * cos(radians(angle));
    float z = r * sin(radians(angle));

    points.add(new PVector(x, random(-12, 12), -z));
  }

  // draw blobs
  for (int i = points.size() - 1; i >= 0; i--) {
    PVector p = points.get(i);
    float alpha = map(i, 0, points.size(), 50, 255);

    pushMatrix();
    translate(p.x, p.y, p.z);
    noStroke();
    fill(0, 255, 120, alpha);
    sphere(10);
    popMatrix();

    if (points.size() > 200) {
      points.remove(0);
    }
  }
}

// ================= SERIAL EVENT =================
void serialEvent(Serial p) {
  data = trim(p.readStringUntil('\n'));
  String[] v = split(data, ',');

  if (v.length == 2) {
    angle = float(v[0]);
    distance = float(v[1]);
  }
}
