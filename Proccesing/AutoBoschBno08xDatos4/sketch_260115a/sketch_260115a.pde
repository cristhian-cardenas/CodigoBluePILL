import processing.serial.*;

Serial imuPort;

// Ángulos en radianes
float yaw = 0;     // heading
float roll = 0;
float pitch = 0;

void setup() {
  size(900, 600, P3D);
  println(Serial.list());
  imuPort = new Serial(this, Serial.list()[0], 115200);
  imuPort.bufferUntil('\n');
}

void draw() {
  background(180, 200, 255);
  lights();

  translate(width/2, height/2, 0);

  // Aplicar orientación
  rotateY(yaw);
  rotateX(roll);
  rotateZ(pitch);

  drawCar();
}

// ---------------- SERIAL -----------------

void serialEvent(Serial p) {
  String line = p.readStringUntil('\n');
  if (line == null) return;
  line = trim(line);

  if (!line.startsWith("@imu:")) return;

  // Quitar "@imu:"
  line = line.substring(5);

  String[] values = split(line, ';');

  if (values.length < 6) return;

  float newRoll    = float(values[0]);
  float newPitch   = float(values[1]);
  float newHeading = float(values[2]);

  // Convertir a radianes
  roll  = radians(newRoll);
  pitch = radians(newPitch);
  yaw   = radians(newHeading);
}

// ---------------- AUTO 3D -----------------

void drawCar() {

  pushMatrix();

  fill(200, 0, 0);
  box(220, 60, 100);

  translate(0, -45, 0);
  fill(180, 0, 0);
  box(120, 50, 80);
  translate(0, 45, 0);

  fill(30);

  pushMatrix();
  translate(-80, 30, 45);
  sphere(20);
  translate(160, 0, 0);
  sphere(20);
  popMatrix();

  pushMatrix();
  translate(-80, 30, -45);
  sphere(20);
  translate(160, 0, 0);
  sphere(20);
  popMatrix();

  popMatrix();
}
