float rotX = 0;
float rotY = 0;
float rotZ = 0;

void setup() {
  size(900, 600, P3D);
}

void draw() {
  background(180, 200, 255);
  lights();

  // Controles entrada cordenas mouse
  rotY = map(mouseX, 0, width, -PI, PI);
  rotX = map(mouseY, 0, height, -PI, PI);

  if (keyPressed) {
    if (key == 'w') rotZ += 0.03;
    if (key == 's') rotZ -= 0.03;
  }

  // Centro de la escena
  translate(width/2, height/2, 0);

  // Rotaciones
  rotateX(rotX);
  rotateY(rotY);
  rotateZ(rotZ);

  // Auto (modelo simple)
  drawCar();
}

void drawCar() {
  // Cuerpo
  fill(200, 0, 0);
  box(220, 60, 100);

  // Techo
  translate(-10, -45, 0);
  fill(180, 0, 0);
  box(120, 50, 80);

  // Volver al centro
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
}
