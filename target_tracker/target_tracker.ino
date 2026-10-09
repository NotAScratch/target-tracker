// Test: base (X) motor swings 10 degrees back and forth,
// upper (Y) motor swings 5 degrees back and forth, forever.

const int X_PUL = 9;
const int X_DIR = 8;
const int Y_PUL = 10;
const int Y_DIR = 11;

// 1.8 deg motor = 200 steps/rev. Set MICROSTEP to match the driver's
// DIP switches (1 = full step, 2, 4, 8, 16, ...).
const int MICROSTEP = 1;
const float STEPS_PER_DEG = 200.0 * MICROSTEP / 360.0;

const float X_DEG = 10;
const float Y_DEG = 5;

void setup() {
  pinMode(X_PUL, OUTPUT);
  pinMode(X_DIR, OUTPUT);
  pinMode(Y_PUL, OUTPUT);
  pinMode(Y_DIR, OUTPUT);
}

void turn(int pul, int dir, float deg, bool forward) {
  long steps = lround(deg * STEPS_PER_DEG);
  digitalWrite(dir, forward);
  for (long i = 0; i < steps; i++) {
    digitalWrite(pul, HIGH);
    delayMicroseconds(10);
    digitalWrite(pul, LOW);
    delay(20);
  }
}

void loop() {
  turn(X_PUL, X_DIR, X_DEG, HIGH);
  turn(Y_PUL, Y_DIR, Y_DEG, HIGH);
  delay(500);
  turn(X_PUL, X_DIR, X_DEG, LOW);
  turn(Y_PUL, Y_DIR, Y_DEG, LOW);
  delay(500);
}
