#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>

int RedLed = 12;
int GreenLed = 11;
int Relay = 8;

RTC_DS3231 rtc;

const int coinPin = 9;   // Digital pin to which the coin acceptor is connected (kindly use interupt pin only)
const int relayPin = 8;  // Digital pin to which the relay module is connected

unsigned long lastPulseTime = 0;
unsigned long pulseWidth = 0;

bool relayActive = false;
unsigned long relayStartTime = 0;  // To store the relay start time
unsigned long relayDuration = 0;   // To store the relay duration

unsigned long previousMillis = 0;     // Stores the last time the action was performed
const unsigned long interval = 1000;  // 1 second in milliseconds

LiquidCrystal_I2C lcd(0x3F, 16, 2);

void setup() {
  Serial.begin(9600);
  pinMode(RedLed, OUTPUT);
  pinMode(GreenLed, OUTPUT);
  pinMode(coinPin, INPUT_PULLUP);
  pinMode(Relay, OUTPUT);

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1)
      ;
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  lcd.init();
  lcd.backlight();

  digitalWrite(RedLed, HIGH);
  digitalWrite(GreenLed, LOW);
  digitalWrite(Relay, LOW);

  lcd.clear();
  lcd.setCursor(5, 0);
  lcd.print("SYSTEM");
  lcd.setCursor(1, 1);
  lcd.print("INITIALIZATION");
  delay(2000);
}
void loop() {
  LedIndicator();
  DateTime now = rtc.now();

  // Read the coin acceptor's output
  int coinState = digitalRead(coinPin);
  unsigned long currentTime = millis();

  // Check for a falling edge (coin insertion)
  if (coinState == LOW && currentTime - lastPulseTime > 50) {
    pulseWidth = 0;  // Reset pulse width

    // Measure the pulse width
    while (coinState == LOW) {
      currentTime = millis();
      coinState = digitalRead(coinPin);
    }

    pulseWidth = currentTime - lastPulseTime;
    Serial.println(pulseWidth);

    // Distinguish between coin types based on pulse width
    if (pulseWidth < 1000) {
      // Coin type A detected, activate relay for 1 minute using RTC
      if (!relayActive) {
        relayDuration = 60;                  // 1 minute in seconds
        relayStartTime = now.secondstime();  // Store the current time as relay start time
        digitalWrite(relayPin, HIGH);        // Turn on the relay
        relayActive = true;
      } else {
        addTimeToRelay(60);  // Add 1 minute to the remaining time
      }
    }else {
      // Coin type B detected, activate relay for 2 minutes using RTC
      if (!relayActive) {
        relayDuration = 120;                 // 2 minutes in seconds
        relayStartTime = now.secondstime();  // Store the current time as relay start time
        digitalWrite(relayPin, HIGH);        // Turn on the relay
        relayActive = true;
      } else {
        addTimeToRelay(120);  // Add 2 minutes to the remaining time
      }
    }

    lastPulseTime = currentTime;
  }

  // Check if it's time to turn off the relay based on RTC
  if (relayActive) {
    unsigned long elapsedTime = now.secondstime() - relayStartTime;

    if (elapsedTime >= relayDuration) {
      digitalWrite(relayPin, LOW);  // Turn off the relay
      relayActive = false;
      relayDuration = 0;
    } else {
      unsigned long timeRemaining = relayDuration - elapsedTime;
      unsigned long currentMillis = millis();
      if (currentMillis - previousMillis >= interval) {
        unsigned long minutes = timeRemaining / 60;
        unsigned long seconds = timeRemaining % 60;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("SOCKET: ON");
        lcd.setCursor(0, 1);
        lcd.print("T: ");
        lcd.setCursor(3, 1);
        lcd.print(minutes);
        lcd.setCursor(5, 1);
        lcd.print("min");
        lcd.setCursor(10, 1);
        lcd.print(seconds);
        lcd.setCursor(13, 1);
        lcd.print("sec");
        Serial.print("Time Remaining: ");
        Serial.print(minutes);
        Serial.print(" min ");
        Serial.print(seconds);
        Serial.println(" sec");
        previousMillis = currentMillis;
      }
    }
  }


  // Add a small delay to avoid false detections
  delay(10);
}
void LedIndicator() {
  if (digitalRead(Relay) == LOW) {
    digitalWrite(RedLed, HIGH);
    digitalWrite(GreenLed, LOW);
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("SMART CHARGING");
    lcd.setCursor(0, 1);
    lcd.print("SOCKET: OFF");
    delay(100);
  } else if (digitalRead(Relay) == HIGH) {
    digitalWrite(GreenLed, HIGH);
    digitalWrite(RedLed, LOW);
  }
}
void addTimeToRelay(unsigned long additionalTime) {
  relayDuration += additionalTime;
}
