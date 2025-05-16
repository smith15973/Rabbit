// rabbit_car.ino
#include "BLEHandler.h"
#include "ESCHandler.h"
#include "ServoHandler.h"
#include "HSHandler.h"
#include "IR8Handler.h"
#include "Lights.h"
#include "config.h"

// Define direction variables
int MOTOR_SPEED = 0;
int SERVO_ANGLE = 90;
bool RUNNING = false;

bool manualControl = false;
bool IS_WHITE_LINE = false; // determines whenther the logic follows a white or black line

unsigned long startTime = 0; // start of pacing
unsigned long endTime = 0;   // final end time

float targetDistance = 0.0; // user entered target distance in meters
float targetTime = 0.0;     // user entered run time in seconds
float targetPace = 0.0;     // user entered target pace in m/s

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
  // Initialize IR8 line sensor
  ir8Setup();
  // Initialize BLE
  setupBLE();
  // Initialize Lights
  setupLights();
}

void loop()
{
  if (manualControl)
  {
    setMotorSpeed(MOTOR_SPEED);
    setSteering(SERVO_ANGLE);
  }

  else // follow the line
  {
    if (RUNNING)
    {

      unsigned long currentTime = micros();
      unsigned long targetEndTime = startTime + (targetTime * 1000000UL); // UL ensures it's treated as unsigned long

      // Stop car if time is done
      if (currentTime >= targetEndTime)
      {
        stopESC();
        centerSteering();
        RUNNING = false;
        endTime = currentTime;
        Serial.print(currentTime);
        Serial.println(" - RUN ENDED!");
        Serial.print("Target end time was: ");
        Serial.println(targetEndTime);
        Serial.print("Total Run Time: ");
        Serial.printf("%.3f seconds\n", (endTime - startTime) / 1000000.0);
      }
      else
      {
        Serial.printf("Motor Speed: %d\n", MOTOR_SPEED);
        setMotorSpeed(MOTOR_SPEED);
        followLine();
        setSteering(SERVO_ANGLE);
      }
    }
    else
    {
      stopESC();
      centerSteering();
    }
  }

  // hsUpdate(&currentSpeed, &totalDistance);

  delay(10);
}