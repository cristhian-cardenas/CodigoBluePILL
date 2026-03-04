import processing.serial.*;

Serial imuPort;

// Ángulos en radianes
float roll = 0;   // inclinación lateral
float pitch = 0;  // inclinación frontal
float yaw = 0;    // orientación respecto al norte

void setup() {
  size(900, 600, P3D);
  println("Puertos seriales disponibles:");
  println(Serial.list());
  
  imuPort = new Serial(this, Serial.list()[0], 115200);
  imuPort.bufferUntil('\n');
}

void draw() {
  background(180, 200, 255);
  lights();

  // Dibujar ejes fijos del mundo
  drawWorldAxes();

  // Centrar y rotar el carro según IMU
  pushMatrix();
  translate(width/2, height/2, 0);
  
  rotateY(yaw);
  rotateX(roll);
  rotateZ(pitch);

  drawCar();
  popMatrix();

  // Mostrar valores en pantalla
  displayIMUValues();
}

// ---------------- SERIAL -----------------
void serialEvent(Serial p) {
  String line = p.readStringUntil('\n');
  if (line == null) return;
  line = trim(line);

  if (!line.startsWith("@imu:")) return;
  line = line.substring(5);

  String[] values = split(line, ';');
  if (values.length < 6) return;

  roll  = radians(parseIMU(values[0]));
  pitch = radians(parseIMU(values[1]));
  yaw   = radians(parseIMU(values[2]));
}

// Convierte "entero.fraccion" a float
float parseIMU(String s) {
  String[] parts = split(s, '.');
  if (parts.length != 2) return 0;
  float i = float(parts[0]);
  float f = float(parts[1]) / 1000.0; // tu fracción tiene 3 dígitos
  return i + f;
}

// ---------------- CARRO -----------------
void drawCar() {
  pushMatrix();

  fill(200, 0, 0);
  box(220, 60, 100); // cuerpo

  translate(0, -45, 0);
  fill(180, 0, 0);
  box(120, 50, 80); // cabina
  translate(0, 45, 0);

  fill(30); // ruedas

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

// ---------------- EJES DEL MUNDO -----------------
void drawWorldAxes() {
  pushMatrix();
  translate(width/2, height/2, 0);

  strokeWeight(3);

  // Eje X = rojo
  stroke(255, 0, 0);
  line(0, 0, 0, 100, 0, 0);

  // Eje Y = verde
  stroke(0, 255, 0);
  line(0, 0, 0, 0, 100, 0);

  // Eje Z = azul
  stroke(0, 0, 255);
  line(0, 0, 0, 0, 0, 100);

  popMatrix();
}

// ---------------- PANTALLA -----------------
void displayIMUValues() {
  fill(0);
  textSize(16);
  textAlign(LEFT, TOP);
  text("Roll (X): " + nf(degrees(roll), 1, 2) + "°", 10, 10);
  text("Pitch (Y): " + nf(degrees(pitch), 1, 2) + "°", 10, 30);
  text("Yaw (Z/Norte): " + nf(degrees(yaw), 1, 2) + "°", 10, 50);
}
