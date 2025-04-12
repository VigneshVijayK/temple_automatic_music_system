#include <Wire.h>
#include "RTClib.h"
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>

// ============= Configuration =============
#define LCD_ADDRESS      0x27     // I2C address: 0x27 or 0x3F
#define LCD_COLS        16
#define LCD_ROWS        2
#define DEBOUNCE_DELAY  300      // ms
#define IDLE_TIMEOUT    30000    // 30 seconds
#define EEPROM_SIGNATURE 0xAA    // EEPROM validation marker

// ============= Hardware Pins =============
const byte relayPin = 7;
const byte buttons[] = {2, 3, 4, 5, 6}; // mode, next, up, down, save

// ============= Global Objects =============
RTC_DS1307 rtc;
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLS, LCD_ROWS);

// ============= State Variables =============
struct SystemState {
  bool setupMode = false;
  bool relayState = false;
  byte settingIndex = 0;
  byte timeValues[8]; // [mStartH, mStartM, mEndH, mEndM, eStartH, eStartM, eEndH, eEndM]
  unsigned long lastDebounce = 0;
  unsigned long lastActivity = 0;
  unsigned long lastDisplayUpdate = 0;
} state;

// Function prototypes
void initHardware();
void initRTC();
void loadSettings();
void showSplashScreen();
void handleButtons();
void handleIdleTimeout(unsigned long now);
void updateDisplay(unsigned long now);
void adjustTimeValue(int delta);
void handleRelayControl();
void saveSettings();
void printTime(const DateTime &time);
void printTwoDigits(byte number);
String getSettingName(byte index);

// ============= Setup =============
void setup() {
  Serial.begin(9600);
  initHardware();
  initRTC();
  loadSettings();
  showSplashScreen();
}

void initHardware() {
  lcd.begin(LCD_COLS, LCD_ROWS);
  lcd.backlight();
  
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);
  
  for (byte i = 0; i < sizeof(buttons)/sizeof(buttons[0]); i++) {
    pinMode(buttons[i], INPUT_PULLUP);
  }
}

void initRTC() {
  if (!rtc.begin()) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("RTC NOT FOUND!");
    lcd.setCursor(0, 1);
    lcd.print("Halting...");
    while (true);
  }
  
  if (!rtc.isrunning()) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("RTC Battery Low");
    lcd.setCursor(0, 1);
    lcd.print("Time may be wrong");
    delay(2000);
  }
}

// ============= Main Loop =============
void loop() {
  unsigned long now = millis();
  
  handleButtons();
  handleIdleTimeout(now);
  
  if (now - state.lastDisplayUpdate >= 1000) {
    updateDisplay(now);
    state.lastDisplayUpdate = now;
  }
  
  if (state.setupMode) {
    // Setup mode handling is now within handleButtons()
  } else {
    handleRelayControl();
  }
}

// ============= Button Handling =============
void handleButtons() {
  if (millis() - state.lastDebounce < DEBOUNCE_DELAY) return;
  
  if (digitalRead(buttons[0]) == LOW) { // Mode button
    state.setupMode = !state.setupMode;
    state.settingIndex = 0;
    state.lastDebounce = millis();
    state.lastActivity = millis();
    return;
  }

  if (!state.setupMode) return;

  if (digitalRead(buttons[1]) == LOW) { // Next button
    state.settingIndex = (state.settingIndex + 1) % 8;
    state.lastDebounce = millis();
    state.lastActivity = millis();
  }
  else if (digitalRead(buttons[2]) == LOW) { // Up button
    adjustTimeValue(1);
    state.lastDebounce = millis();
    state.lastActivity = millis();
  }
  else if (digitalRead(buttons[3]) == LOW) { // Down button
    adjustTimeValue(-1);
    state.lastDebounce = millis();
    state.lastActivity = millis();
  }
  else if (digitalRead(buttons[4]) == LOW) { // Save button
    saveSettings();
    state.setupMode = false;
    state.lastDebounce = millis();
    state.lastActivity = millis();
    lcd.clear();
    lcd.print("Settings Saved!");
    delay(1000);
  }
}

void adjustTimeValue(int delta) {
  byte maxValue = (state.settingIndex % 2 == 0) ? 23 : 59;
  state.timeValues[state.settingIndex] = (state.timeValues[state.settingIndex] + delta + maxValue + 1) % (maxValue + 1);
}

// ============= Display Handling =============
void updateDisplay(unsigned long now) {
  lcd.clear();
  
  if (state.setupMode) {
    lcd.setCursor(0, 0);
    lcd.print("Set ");
    lcd.print(getSettingName(state.settingIndex));
    lcd.setCursor(0, 1);
    printTwoDigits(state.timeValues[state.settingIndex]);
    lcd.print(state.settingIndex % 2 ? " min" : " hour");
  } else {
    DateTime time = rtc.now();
    lcd.setCursor(0, 0);
    lcd.print("Time: ");
    printTime(time);
    
    lcd.setCursor(0, 1);
    lcd.print("Relay: ");
    lcd.print(state.relayState ? "ON " : "OFF");
    
    if (!rtc.isrunning()) {
      lcd.setCursor(13, 1);
      lcd.print("LOW");
    }
  }
}

void printTime(const DateTime &time) {
  printTwoDigits(time.hour());
  lcd.print(":");
  printTwoDigits(time.minute());
  lcd.print(":");
  printTwoDigits(time.second());
}

void printTwoDigits(byte number) {
  if (number < 10) lcd.print("0");
  lcd.print(number);
}

// ============= Relay Control =============
void handleRelayControl() {
  DateTime now = rtc.now();
  int currentMin = now.hour() * 60 + now.minute();
  
  struct TimePeriod {
    int start;
    int end;
  };
  
  TimePeriod activePeriods[2] = {
    {state.timeValues[0] * 60 + state.timeValues[1], 
     state.timeValues[2] * 60 + state.timeValues[3]},
    {state.timeValues[4] * 60 + state.timeValues[5], 
     state.timeValues[6] * 60 + state.timeValues[7]}
  };

  bool shouldBeOn = false;
  for (byte i = 0; i < 2; i++) {
    if (activePeriods[i].start <= activePeriods[i].end) {
      if (currentMin >= activePeriods[i].start && currentMin < activePeriods[i].end) {
        shouldBeOn = true;
        break;
      }
    } else {
      if (currentMin >= activePeriods[i].start || currentMin < activePeriods[i].end) {
        shouldBeOn = true;
        break;
      }
    }
  }

  if (shouldBeOn != state.relayState) {
    state.relayState = shouldBeOn;
    digitalWrite(relayPin, state.relayState ? HIGH : LOW);
  }
}

// ============= EEPROM Handling =============
void loadSettings() {
  if (EEPROM.read(0) != EEPROM_SIGNATURE) {
    const byte defaults[8] = {8, 0, 10, 0, 18, 0, 22, 0};
    memcpy(state.timeValues, defaults, sizeof(defaults));
    saveSettings();
  } else {
    for (byte i = 0; i < 8; i++) {
      state.timeValues[i] = EEPROM.read(i + 1);
    }
  }
}

void saveSettings() {
  EEPROM.write(0, EEPROM_SIGNATURE);
  for (byte i = 0; i < 8; i++) {
    EEPROM.write(i + 1, state.timeValues[i]);
  }
}

// ============= UI Helpers =============
void showSplashScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Relay Controller");
  lcd.setCursor(0, 1);
  lcd.print("v2.1 Starting...");
  delay(1500);
}

String getSettingName(byte index) {
  const String names[] = {
    "Morn Start", "Morn End", 
    "Eve Start", "Eve End"
  };
  return names[index / 2] + (index % 2 ? " Min" : " Hour");
}

// ============= Idle Timeout =============
void handleIdleTimeout(unsigned long now) {
  if (state.setupMode && (now - state.lastActivity > IDLE_TIMEOUT)) {
    state.setupMode = false;
    state.settingIndex = 0;
    lcd.clear();
    lcd.print("Returning to");
    lcd.setCursor(0, 1);
    lcd.print("normal mode");
    delay(1000);
  }
}
