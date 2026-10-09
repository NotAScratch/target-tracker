// TB6600 test: base (X) motor swings 10 degrees back and forth,
// upper (Y) motor swings 5 degrees back and forth, both at the same time.
//
// TB6600 DIP switches (check against the table printed on your driver):
//   Microstep 8  -> S1 OFF, S2 ON,  S3 OFF   (must match MICROSTEP below)
//   Current 1.0A -> S4 ON,  S5 OFF, S6 ON    (raise to motor's rated current if weak)
//   All switches OFF = 32 microstep + 3.5A max current -> motors overheat!

const int X_PUL = 9;
const int X_DIR = 8;
const int Y_PUL = 10;
const int Y_DIR = 11;

const int MICROSTEP = 8;                                // match S1-S3
const float STEPS_PER_DEG = 200.0 * MICROSTEP / 360.0;  // 1.8 deg motor

const float X_DEG = 10;
const float Y_DEG = 5;
const float SPEED_DEG_PER_SEC = 20;  // base motor speed, keep slow for testing

void setup() {
  pinMode(X_PUL, OUTPUT);
  pinMode(X_DIR, OUTPUT);
  pinMode(Y_PUL, OUTPUT);
  pinMode(Y_DIR, OUTPUT);
}

void pulse(int pin) {
  digitalWrite(pin, HIGH);
  delayMicroseconds(20);
  digitalWrite(pin, LOW);
}

void wait(unsigned long us) {
  delay(us / 1000);
  delayMicroseconds(us % 1000);
}

// Move both motors together so they start and finish at the same time.
void swing(bool forward) {
  long xSteps = lround(X_DEG * STEPS_PER_DEG);
  long ySteps = lround(Y_DEG * STEPS_PER_DEG);
  unsigned long stepUs = 1000000.0 / (SPEED_DEG_PER_SEC * STEPS_PER_DEG);

  digitalWrite(X_DIR, forward);
  digitalWrite(Y_DIR, forward);
  delayMicroseconds(20);  // TB6600 needs DIR set before the first pulse

  long yDone = 0;
  for (long i = 1; i <= xSteps; i++) {
    pulse(X_PUL);
    if (yDone < i * ySteps / xSteps) {
      pulse(Y_PUL);
      yDone++;
    }
    wait(stepUs);
  }
}

void loop() {
  swing(HIGH);
  delay(1000);
  swing(LOW);
  delay(1000);
}
