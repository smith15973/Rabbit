// rabbit_car.ino
#include "BLEHandler.h"
#include "ESCHandler.h"
#include "ServoHandler.h"
#include "config.h"

// Define direction variables
int ESC_VALUE = 0;
int SERVO_VALUE = 0;

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting Rabbit...");

  // Initialize pins
  pinMode(ESC_PIN, OUTPUT);
  pinMode(SERVO_PIN, OUTPUT);
  pinMode(BT_LED_PIN, OUTPUT);

  // Initialize ESC
  setupESC();
  // Initialize ESC
  setupServo();
  // Initialize BLE
  setupBLE();
}

void loop()
{
  setMotorSpeed(ESC_VALUE);
  setSteering(SERVO_VALUE);

  delay(100);
}