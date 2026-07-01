# Ultrasonic Distance Display

An Arduino UNO R3 project that measures distance in feet using an HC-SR04 ultrasonic sensor and displays it on a 4-digit 7-segment display. Three LEDs and a buzzer provide proximity alerts.

## Demo

| Distance | Display | Green | Yellow | Red | Buzzer |
|---|---|---|---|---|---|
| > 0.4 ft | e.g. `_1.3` | ON | off | off | off |
| 0.1 – 0.4 ft | e.g. `_0.2` | off | ON | off | off |
| < 0.1 ft | e.g. `_0.0` | off | off | BLINK | BEEP |
| Out of range | `----` | ON | off | off | off |

## Components

| Component | Description |
|---|---|
| Arduino UNO R3 | Microcontroller |
| HC-SR04 | Ultrasonic distance sensor |
| 4-digit 7-segment display | 12-pin common cathode module |
| 74HC595 | 8-bit shift register (drives segment pins) |
| Red LED | Too-close alert |
| Yellow LED | Medium-range indicator |
| Green LED | Far/clear indicator |
| Buzzer | Audible too-close alert |
| 11x 220Ω resistors | 8 for segments, 1 per LED |

## Wiring

### HC-SR04
| Sensor Pin | Arduino Pin |
|---|---|
| VCC | 5V |
| GND | GND |
| TRIG | A0 |
| ECHO | A1 |

### 74HC595 Control
| IC Pin | Arduino Pin | Note |
|---|---|---|
| DS (pin 14) | D2 | Data |
| SHCP (pin 11) | D3 | Clock |
| STCP (pin 12) | D4 | Latch |
| VCC (pin 16) | 5V | |
| GND (pin 8) | GND | |
| OE (pin 13) | GND | Always enabled |
| MR (pin 10) | 5V | Never reset |

### 74HC595 Outputs → 7-Segment Pins (each through 220Ω resistor)
| IC Pin | Segment |
|---|---|
| Q0 | DP |
| Q1 | G (middle) |
| Q2 | F (top-left) |
| Q3 | E (bottom-left) |
| Q4 | D (bottom) |
| Q5 | C (bottom-right) |
| Q6 | B (top-right) |
| Q7 | A (top) |

### 4-Digit 7-Segment Display (digit select pins)
| Display Pin | Arduino Pin |
|---|---|
| Digit 1 (left) | D5 |
| Digit 2 | D6 |
| Digit 3 | D7 |
| Digit 4 (right) | D8 |

### LEDs & Buzzer
| Component | Arduino Pin | Note |
|---|---|---|
| Buzzer + | D9 | |
| Buzzer − | GND | |
| Green LED anode | D10 | Through 220Ω resistor |
| Green LED cathode | GND | |
| Yellow LED anode | D11 | Through 220Ω resistor |
| Yellow LED cathode | GND | |
| Red LED anode | D12 | Through 220Ω resistor |
| Red LED cathode | GND | |

## Configuration

All tunable parameters are defined at the top of the sketch:

```cpp
#define COMMON_ANODE   false  // true for common anode display
#define TOO_CLOSE_FT   0.1   // red LED + buzzer threshold (feet)
#define MEDIUM_DIST_FT 0.4   // yellow LED threshold (feet)
#define BEEP_INTERVAL_MS 150 // buzzer/LED blink rate (ms)
```

## Notes

- No external libraries required — uses only built-in Arduino functions.
- If your buzzer is **passive** (needs a frequency signal), replace `digitalWrite(BUZZER_PIN, HIGH/LOW)` in `handleAlert()` with `tone(BUZZER_PIN, 1000)` / `noTone(BUZZER_PIN)`.
- HC-SR04 reliable range: ~0.1 ft (2cm) to ~13 ft (4m). Readings outside this range show `----` on the display.
- The display uses multiplexed refresh at ~125Hz (4 digits × 2ms hold each). The 50µs settling delay between shifting data and enabling a digit prevents segment ghosting.
