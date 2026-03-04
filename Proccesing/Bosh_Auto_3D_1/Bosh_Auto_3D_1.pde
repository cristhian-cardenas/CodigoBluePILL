import processing.serial.*;

Serial imuPort;

// Ángulos en radianes
float yaw = 0;
float roll = 0;
float pitch = 0;

void setup() {
  size(900, 600, P3D);
  println(Serial.list());          // Ver puertos disponibles
  imuPort = new Serial(this, Serial.list()[0], 115200);  // Cambia el índice si hace falta
  imuPort.bufferUntil('\n');
}

void draw() {
  background(180, 200, 255);
  lights();

  translate(width/2, height/2, 0);

  // Aplicar rotaciones de la IMU
  rotateY(yaw);    // Yaw → eje X Y
  rotateZ(pitch);  // Pitch → eje Y Z
  rotateX(roll);   // Roll → eje Z X

  drawCar();
}

// ---------------- SERIAL -----------------

void serialEvent(Serial p) {
  String line = p.readStringUntil('\n');
  if (line == null) return;

  line = trim(line);

  // Ejemplo recibido:
  // Yaw: 3.38  Roll: 36.94  Pitch: 25.31

  String[] parts = splitTokens(line, " :");

  if (parts.length >= 6) {
    float newYaw   = float(parts[1]);
    float newRoll  = float(parts[3]);
    float newPitch = float(parts[5]);

    // Convertir grados → radianes
    yaw   = radians(newYaw);
    roll  = radians(newRoll);
    pitch = radians(newPitch);
  }
}

// ---------------- AUTO 3D -----------------

void drawCar() {

  pushMatrix();

  // Cuerpo
  fill(200, 0, 0);
  box(220, 60, 100);

  // Techo
  translate(0, -45, 0);
  fill(180, 0, 0);
  box(120, 50, 80);

  translate(0, 45, 0);

  // Ruedas
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
