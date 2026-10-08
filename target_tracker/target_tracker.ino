/*
  2-Axis Stepper Motor Control - Arduino Nano
  -------------------------------------------
  Drives two stepper drivers (TB6600 / DM542 / A4988 style, STEP/DIR input).

  Wiring (common-cathode: PUL- and DIR- of both drivers go to Nano GND):

    X axis (bottom / pan) driver      Y axis (upper / tilt) driver
      PUL+ (P+)  -> D8                  PUL+ (P+)  -> D10
      DIR+ (D+)  -> D9                  DIR+ (D+)  -> D11
      PUL-, DIR- -> GND                 PUL-, DIR- -> GND

  No external libraries needed. Both axes step at the same time
  (non-blocking), so diagonal moves work.

  Travel is limited to +/-X_LIMIT_STEPS and +/-Y_LIMIT_STEPS from the
  start position so the wires can't get tangled.

  Serial commands (115200 baud, end each line with newline):
    X<steps>        move X relative,   e.g.  X20    X-15
    Y<steps>        move Y relative,   e.g.  Y10    Y-30
    M<x> <y>        move both relative at the same time, e.g. M20 -10
    G<x> <y>        go to absolute position,               e.g. G0 0
    S<steps/sec>    set speed for both axes,               e.g. S100
    Z               set current position as zero (0,0)
    P               print current position
    STOP            stop both motors immediately
*/

// ---------------- Pin definitions ----------------
const uint8_t X_PUL_PIN = 8;   // X axis P+ (step)
const uint8_t X_DIR_PIN = 9;   // X axis D+ (direction)
const uint8_t Y_PUL_PIN = 10;  // Y axis P+ (step)
const uint8_t Y_DIR_PIN = 11;  // Y axis D+ (direction)

// ---------------- Settings ----------------
const unsigned int PULSE_WIDTH_US  = 5;     // step pulse high time (>= 2.5us for TB6600)
const float        DEFAULT_SPEED   = 100.0; // steps per second (kept slow for testing)
const float        MAX_SPEED       = 1000.0;

// Soft travel limits so the motors can't wind up the wires.
// Each axis may only move between -LIMIT and +LIMIT steps from where it was
// at power-up (or at the last Z command). On a 1.8 deg motor at full step,
// 50 steps = 1/4 turn; with microstepping it is even less. Raise these once
// the wiring is sorted.
const long         X_LIMIT_STEPS   = 50;
const long         Y_LIMIT_STEPS   = 50;
const bool         X_INVERT_DIR    = false; // flip if X turns the wrong way
const bool         Y_INVERT_DIR    = false; // flip if Y turns the wrong way

// ---------------- Axis state ----------------
struct Axis {
  uint8_t pulPin;
  uint8_t dirPin;
  bool invertDir;
  long position;              // current position in steps
  long target;                // target position in steps
  unsigned long intervalUs;   // time between steps
  unsigned long lastStepUs;
};

Axis xAxis = { X_PUL_PIN, X_DIR_PIN, X_INVERT_DIR, 0, 0, 0, 0 };
Axis yAxis = { Y_PUL_PIN, Y_DIR_PIN, Y_INVERT_DIR, 0, 0, 0, 0 };

float speedStepsPerSec = DEFAULT_SPEED;

char inputBuf[32];
uint8_t inputLen = 0;

// ---------------- Motor helpers ----------------
void setupAxis(Axis &a) {
  pinMode(a.pulPin, OUTPUT);
  pinMode(a.dirPin, OUTPUT);
  digitalWrite(a.pulPin, LOW);
  digitalWrite(a.dirPin, LOW);
}

void stepAxis(Axis &a, bool forward) {
  digitalWrite(a.dirPin, forward != a.invertDir ? HIGH : LOW);
  delayMicroseconds(5);                 // direction setup time
  digitalWrite(a.pulPin, HIGH);
  delayMicroseconds(PULSE_WIDTH_US);
  digitalWrite(a.pulPin, LOW);
  a.position += forward ? 1 : -1;
}

// Run one axis: step if it is not at target and its interval has elapsed.
void runAxis(Axis &a) {
  if (a.position == a.target) return;
  unsigned long now = micros();
  if (now - a.lastStepUs >= a.intervalUs) {
    a.lastStepUs = now;
    stepAxis(a, a.target > a.position);
  }
}

bool isMoving() {
  return xAxis.position != xAxis.target || yAxis.position != yAxis.target;
}

// Start a move to absolute targets. The longer axis runs at full speed and the
// shorter one is slowed down so both axes finish together (straight-line move).
void moveTo(long xTarget, long yTarget) {
  long xc = constrain(xTarget, -X_LIMIT_STEPS, X_LIMIT_STEPS);
  long yc = constrain(yTarget, -Y_LIMIT_STEPS, Y_LIMIT_STEPS);
  if (xc != xTarget || yc != yTarget) {
    Serial.println(F("Limit reached - move clipped"));
  }
  xTarget = xc;
  yTarget = yc;

  long dx = labs(xTarget - xAxis.position);
  long dy = labs(yTarget - yAxis.position);
  long longest = max(dx, dy);
  if (longest == 0) return;

  float durationUs = (float)longest / speedStepsPerSec * 1000000.0;
  xAxis.intervalUs = dx ? (unsigned long)(durationUs / dx) : 0;
  yAxis.intervalUs = dy ? (unsigned long)(durationUs / dy) : 0;

  unsigned long now = micros();
  xAxis.lastStepUs = now;
  yAxis.lastStepUs = now;
  xAxis.target = xTarget;
  yAxis.target = yTarget;
}

void moveBy(long dx, long dy) {
  moveTo(xAxis.target + dx, yAxis.target + dy);
}

void stopAll() {
  xAxis.target = xAxis.position;
  yAxis.target = yAxis.position;
}

void printPosition() {
  Serial.print(F("X="));
  Serial.print(xAxis.position);
  Serial.print(F("  Y="));
  Serial.print(yAxis.position);
  Serial.print(F("  speed="));
  Serial.println(speedStepsPerSec);
}

// ---------------- Serial command parser ----------------
void handleCommand(char *cmd) {
  while (*cmd == ' ') cmd++;
  if (*cmd == '\0') return;

  char c = toupper(cmd[0]);
  char *args = cmd + 1;

  if (strcasecmp(cmd, "STOP") == 0) {
    stopAll();
    Serial.println(F("Stopped"));
  } else if (c == 'X') {
    moveBy(atol(args), 0);
  } else if (c == 'Y') {
    moveBy(0, atol(args));
  } else if (c == 'M' || c == 'G') {
    char *end;
    long a = strtol(args, &end, 10);
    long b = strtol(end, NULL, 10);
    if (c == 'M') moveBy(a, b);
    else          moveTo(a, b);
  } else if (c == 'S') {
    float s = atof(args);
    if (s > 0 && s <= MAX_SPEED) {
      speedStepsPerSec = s;
      Serial.print(F("Speed set to "));
      Serial.println(s);
    } else {
      Serial.println(F("Speed out of range"));
    }
  } else if (c == 'Z') {
    stopAll();
    xAxis.position = xAxis.target = 0;
    yAxis.position = yAxis.target = 0;
    Serial.println(F("Position zeroed"));
  } else if (c == 'P') {
    printPosition();
  } else {
    Serial.print(F("Unknown command: "));
    Serial.println(cmd);
  }
}

void readSerial() {
  while (Serial.available()) {
    char ch = Serial.read();
    if (ch == '\n' || ch == '\r') {
      inputBuf[inputLen] = '\0';
      handleCommand(inputBuf);
      inputLen = 0;
    } else if (inputLen < sizeof(inputBuf) - 1) {
      inputBuf[inputLen++] = ch;
    }
  }
}

// ---------------- Arduino entry points ----------------
void setup() {
  setupAxis(xAxis);
  setupAxis(yAxis);

  Serial.begin(115200);
  Serial.println(F("2-axis stepper ready"));
  Serial.println(F("Commands: X<n> Y<n> M<x> <y> G<x> <y> S<speed> Z P STOP"));
}

void loop() {
  static bool wasMoving = false;

  readSerial();
  runAxis(xAxis);
  runAxis(yAxis);

  bool moving = isMoving();
  if (wasMoving && !moving) {
    Serial.print(F("Done. "));
    printPosition();
  }
  wasMoving = moving;
}
