// ============================================================
// Ultrasonic Distance Display with Proximity Alert
// ------------------------------------------------------------
// Measures distance in feet using an HC-SR04 ultrasonic sensor
// and displays it on a 4-digit 7-segment display driven by a
// 74HC595 shift register. LEDs and a buzzer indicate proximity:
//   Red + buzzer : < 0.1 ft (too close)
//   Yellow       : 0.1 – 0.4 ft (medium)
//   Green        : > 0.4 ft or no object detected (far/clear)
// ============================================================

// ---- Configuration ----
#define COMMON_ANODE false  // confirmed common cathode display

#define TRIG_PIN  A0
#define ECHO_PIN  A1

// 74HC595 pins
#define DATA_PIN  2
#define CLOCK_PIN 3
#define LATCH_PIN 4

// Digit select pins
const int digitPins[4] = {5, 6, 7, 8};

// Alert pins
#define BUZZER_PIN     9
#define LED_GREEN_PIN  10
#define LED_YELLOW_PIN 11
#define LED_PIN        12   // red LED

// Distance thresholds (feet)
#define TOO_CLOSE_FT   0.1
#define MEDIUM_DIST_FT 0.4
#define BEEP_INTERVAL_MS 150  // time between beeps/blinks

// Segment encoding: bit order A B C D E F G DP
// Physical Q-to-segment mapping (verified by hardware test):
//   Q0=DP, Q1=G, Q2=F, Q3=E, Q4=D, Q5=C, Q6=B, Q7=A
const byte digitPatterns[10] = {
  0b11111100, // 0: A B C D E F
  0b01100000, // 1: B C
  0b11011010, // 2: A B D E G
  0b11110010, // 3: A B C D G
  0b01100110, // 4: B C F G
  0b10110110, // 5: A C D F G
  0b10111110, // 6: A C D E F G
  0b11100000, // 7: A B C
  0b11111110, // 8: A B C D E F G
  0b11110110, // 9: A B C D F G
};

const byte DASH  = 0b00000010; // G only (middle segment)
const byte BLANK = 0b00000000; // all off

// ---- State ----
float lastFeet = 0;
unsigned long lastMeasure = 0;
unsigned long lastBeepToggle = 0;
bool beepState = false;

// ---- 74HC595 ----
void shiftOut595(byte data) {
  byte toSend = COMMON_ANODE ? ~data : data;
  digitalWrite(LATCH_PIN, LOW);
  shiftOut(DATA_PIN, CLOCK_PIN, LSBFIRST, toSend);
  digitalWrite(LATCH_PIN, HIGH);
}

// ---- Display ----
void clearDisplay() {
  for (int i = 0; i < 4; i++)
    digitalWrite(digitPins[i], COMMON_ANODE ? LOW : HIGH);
}

void displayDigit(int pos, byte pattern, bool dp) {
  // 1. Blank all digits before changing segment data
  clearDisplay();

  // 2. Update segment data with no digit active
  if (dp) pattern |= 0x01;
  shiftOut595(pattern);

  // 3. Settling delay for shift register to stabilize
  delayMicroseconds(50);

  // 4. Enable the target digit
  digitalWrite(digitPins[pos], COMMON_ANODE ? HIGH : LOW);
  delayMicroseconds(2000);

  // 5. Blank before returning to prevent bleed into next digit
  clearDisplay();
}

void showPattern(byte pattern[4], int dpPos) {
  for (int i = 0; i < 4; i++)
    displayDigit(i, pattern[i], i == dpPos);
}

// ---- Sensor ----
float measureDistanceFt() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return -1.0; // timeout / out of range
  return (duration / 2.0) * 0.0001125; // microseconds to feet
}

// ---- Alert ----
void handleAlert(float feet) {
  bool tooClose = (feet >= 0 && feet <= TOO_CLOSE_FT);
  bool medium   = (feet > TOO_CLOSE_FT && feet <= MEDIUM_DIST_FT);
  bool far      = (feet > MEDIUM_DIST_FT) || (feet < 0); // includes out of range

  // Red LED + buzzer: too close
  if (!tooClose) {
    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    beepState = false;
  } else {
    if (millis() - lastBeepToggle >= BEEP_INTERVAL_MS) {
      beepState = !beepState;
      lastBeepToggle = millis();
    }
    digitalWrite(LED_PIN, beepState ? HIGH : LOW);
    digitalWrite(BUZZER_PIN, beepState ? HIGH : LOW);
  }

  // Yellow LED: medium range
  digitalWrite(LED_YELLOW_PIN, medium ? HIGH : LOW);

  // Green LED: far or no object detected
  digitalWrite(LED_GREEN_PIN, far ? HIGH : LOW);
}

// ---- Setup ----
void setup() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(DATA_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(LATCH_PIN, OUTPUT);

  for (int i = 0; i < 4; i++) pinMode(digitPins[i], OUTPUT);

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(LED_YELLOW_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);

  clearDisplay();
}

// ---- Loop ----
void loop() {
  // Measure every 200ms; display refresh continues in between
  if (millis() - lastMeasure >= 200) {
    lastFeet = measureDistanceFt();
    lastMeasure = millis();
  }

  handleAlert(lastFeet);

  byte pattern[4];
  int dpPos;

  if (lastFeet < 0 || lastFeet > 13.0) {
    // Out of range: show ----
    pattern[0] = DASH;
    pattern[1] = DASH;
    pattern[2] = DASH;
    pattern[3] = DASH;
    dpPos = -1;
  } else {
    int val = (int)(lastFeet * 10 + 0.5); // e.g. 3.2 ft -> 32
    int hundreds = val / 100;
    int ones     = (val / 10) % 10;
    int tenths   = val % 10;

    if (val < 100) {
      // e.g. 3.2 ft -> [blank][blank][3.][2]
      pattern[0] = BLANK;
      pattern[1] = BLANK;
      pattern[2] = digitPatterns[ones];
      pattern[3] = digitPatterns[tenths];
      dpPos = 2;
    } else {
      // e.g. 12.5 ft -> [blank][1][2.][5]
      pattern[0] = BLANK;
      pattern[1] = digitPatterns[hundreds];
      pattern[2] = digitPatterns[ones];
      pattern[3] = digitPatterns[tenths];
      dpPos = 2;
    }
  }

  showPattern(pattern, dpPos);
}
