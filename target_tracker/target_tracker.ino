// Minimal test: both motors step 2 steps forward, then 2 steps back, forever.

const int X_PUL = 9;
const int X_DIR = 8;
const int Y_PUL = 10;
const int Y_DIR = 11;

const int STEPS = 2;

void setup() {
  pinMode(X_PUL, OUTPUT);
  pinMode(X_DIR, OUTPUT);
  pinMode(Y_PUL, OUTPUT);
  pinMode(Y_DIR, OUTPUT);
}

void move(bool dir) {
  digitalWrite(X_DIR, dir);
  digitalWrite(Y_DIR, dir);
  for (int i = 0; i < STEPS; i++) {
    digitalWrite(X_PUL, HIGH);
    digitalWrite(Y_PUL, HIGH);
    delayMicroseconds(10);
    digitalWrite(X_PUL, LOW);
    digitalWrite(Y_PUL, LOW);
    delay(50);
  }
  delay(500);
}

void loop() {
  move(HIGH);
  move(LOW);
}
