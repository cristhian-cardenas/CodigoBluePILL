import processing.serial.*;

Serial imuPort;

// Heading en grados (0–360)
float headingDeg = 0;

void setup() {
  size(600, 600);
  smooth();

  println("Puertos seriales disponibles:");
  println(Serial.list());

  imuPort = new Serial(this, Serial.list()[0], 115200);
  imuPort.bufferUntil('\n');
}

void draw() {
  background(230);

  translate(width/2, height/2);

  drawCompass();
  drawNeedle(headingDeg);

  drawHeadingText();
}

// ---------------- SERIAL -----------------
void serialEvent(Serial p) {
  String line = p.readStringUntil('\n');
  if (line == null) return;

  line = trim(line);
  if (!line.startsWith("@imu:")) return;

  line = line.substring(5);
  String[] values = split(line, ';');
  if (values.length < 3) return;

  float yawRaw = parseIMU(values[2]);

  // Normalizar a 0–360
  headingDeg = yawRaw % 360;
  if (headingDeg < 0) headingDeg += 360;
}

// Convierte "entero.fraccion" a float
float parseIMU(String s) {
  String[] parts = split(s, '.');
  if (parts.length != 2) return 0;

  float i = float(parts[0]);
  float f = float(parts[1]) / 1000.0;
  return i + f;
}

// ---------------- BRÚJULA -----------------
void drawCompass() {
  stroke(0);
  strokeWeight(3);
  fill(245);
  ellipse(0, 0, 400, 400);

  // Marcas cardinales
  textAlign(CENTER, CENTER);
  textSize(24);
  fill(0);

  text("N", 0, -190);
  text("S", 0, 190);
  text("E", 190, 0);
  text("O", -190, 0);

  // Marcas cada 30°
  strokeWeight(2);
  for (int a = 0; a < 360; a += 30) {
    float ang = radians(a - 90);
    float x1 = cos(ang) * 170;
    float y1 = sin(ang) * 170;
    float x2 = cos(ang) * 190;
    float y2 = sin(ang) * 190;
    line(x1, y1, x2, y2);
  }
}

// ---------------- AGUJA -----------------
void drawNeedle(float deg) {
  pushMatrix();

  // Norte = 0° arriba
  rotate(radians(deg - 90));

  strokeWeight(5);
  stroke(200, 0, 0);
  line(0, 0, 0, -150);

  fill(200, 0, 0);
  ellipse(0, 0, 12, 12);

  popMatrix();
}

// ---------------- TEXTO -----------------
void drawHeadingText() {
  fill(0);
  textSize(22);
  textAlign(CENTER);
  text(nf(headingDeg, 1, 1) + "°", 0, 260);
}
