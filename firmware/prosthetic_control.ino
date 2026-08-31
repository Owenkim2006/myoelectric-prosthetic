#include <Arduino.h>
#include <Servo.h>

// input signal(0/5V) > Arduino 2
// input GND > Arduino GND
// SERVO signal > Arduino 9
// Servo red cable > 5V (battery maybe)
// Servo black/brown cable > GND

Servo myServo;

const int servoPin = 9;
const int inputPin = 2;

const unsigned long activityWindow = 500;  // ms - how long a burst must persist before counting as "signal detected"
const unsigned long quietTimeout = 400;      // ms - how long with NO toggling before we consider it "rest"

int lastReading = LOW;
unsigned long lastToggleTime = 0;       // last time the input actually changed state
unsigned long burstStartTime = 0;       // when this burst of activity started
bool burstActive = false;               // are we currently inside a burst?
bool burstAlreadyHandled = false;       // prevents re-triggering multiple times within one burst

bool servoIsFlexed = false;

void setup()
{
  pinMode(inputPin, INPUT);

  myServo.attach(servoPin);
  myServo.write(0);
}

void loop()
{
  int reading = digitalRead(inputPin);
  unsigned long now = millis();

  // detect toggling (any change counts as "activity", not just rising edges)
  if (reading != lastReading)
  {
    lastToggleTime = now;

    if (!burstActive)
    {
      burstActive = true;
      burstStartTime = now;
      burstAlreadyHandled = false;
    }
  }

  // if we've gone quietTimeout ms with no toggling, the burst has ended
  if (burstActive && (now - lastToggleTime > quietTimeout))
  {
    burstActive = false;
    burstAlreadyHandled = false;
  }

  // if we're in an active burst that's lasted long enough, and haven't handled it yet, trigger the toggle
  if (burstActive && !burstAlreadyHandled && (now - burstStartTime >= activityWindow))
  {
    servoIsFlexed = !servoIsFlexed;

    if (servoIsFlexed)
    {
      myServo.write(90);
    }
    else
    {
      myServo.write(0);
    }

    burstAlreadyHandled = true; // don't re-trigger again until this burst ends and a new one starts
  }

  lastReading = reading;
}
