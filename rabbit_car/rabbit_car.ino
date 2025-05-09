// rabbit_car.ino
#include "BLEHandler.h"
#include "ESCHandler.h"
#include "ServoHandler.h"
#include "HSHandler.h"
#include "config.h"

// Define direction variables
int ESC_VALUE = 0;
int SERVO_VALUE = 90;
bool RUNNING = false;

bool manualControl = false;
float targetSpeed = 5.3;   // target average speed in m/s
float averageSpeed = 0.0;  // average speed for the current run
float currentSpeed = 0.0;  // Current speed in km/h
float totalDistance = 0.0; // Total distance in m

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting Rabbit...");

  pinMode(BT_LED_PIN, OUTPUT);

  // Initialize ESC
  setupESC();
  // Initialize ESC
  setupServo();
  // Initialize HS
  setupHS();
  // Initialize BLE
  setupBLE();
}

void loop()
{
  if (manualControl)
  {
    setMotorSpeed(ESC_VALUE);
    setSteering(SERVO_VALUE);
  }
  else
  {
    if (RUNNING)
    {
      setMotorSpeed(ESC_VALUE);
      // delay(2000);
      // RUNNING = false;
    }
    else
    {
      stopESC();
    }
  }


  // hsUpdate(&currentSpeed, &totalDistance);

  delay(10);
}