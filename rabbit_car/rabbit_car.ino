// rabbit_car.ino
#include "BLEHandler.h"
#include "ESCHandler.h"
#include "ServoHandler.h"
#include "IR8Handler.h"
#include "HSHandler.h"
#include "Lights.h"
#include "config.h"
#include "conversions.h"

// Define direction variables
int MOTOR_SPEED = 0;
int SERVO_ANGLE = 90;
bool RUNNING = false;
bool startRunTimer = true;

bool manualControl = false;
bool IS_WHITE_LINE = false; // determines whenther the logic follows a white or black line

volatile unsigned long startTime = 0;          // start of pacing
volatile unsigned long currentRunDuration = 0; // current total time of the run
volatile unsigned long endTime = 0;            // final end time

float targetDistance = 0.0; // user entered target distance in meters
float targetTime = 0.0;     // user entered run time in seconds
float targetPace = 0.0;     // user entered target pace in m/s

float averageSpeed = 0.0;  // average speed for the current run
float currentSpeed = 0.0;  // Current speed in km/h
float totalDistance = 0.0; // Total distance in m
unsigned long currentTime = micros();

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting Rabbit...");

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
  if (startRunTimer)
  {
    hsStart();
    startTime = micros();
    startRunTimer = false;
  }
  currentTime = micros();
  currentRunDuration = currentTime - startTime;

  if (manualControl)
  {
    setMotorSpeed(MOTOR_SPEED);
    setSteering(SERVO_ANGLE);
    hsUpdate(&currentSpeed, &averageSpeed, &totalDistance);
    bleBroadcastDTPS(totalDistance, micros_to_s(currentRunDuration), averageSpeed, currentSpeed);
  }

  else // pace mode
  {
    if (RUNNING) // follow the line
    {

      unsigned long targetEndTime = startTime + s_to_micros(targetTime);
      bool shouldEnd = currentTime >= targetEndTime;
      Serial.printf("SHOULD END: %d\n", shouldEnd);
      if (shouldEnd)
      {
        stopESC();
        centerSteering();
        RUNNING = false;
        startRunTimer = false;
        endTime = currentTime;

        Serial.println("RUN ENDED!");
        Serial.print("Total Run Time: ");
        Serial.printf("%.3f seconds\n", micros_to_s(endTime - startTime));
      }
      else
      {
        Serial.printf("Motor Speed: %d\n", MOTOR_SPEED);
        setMotorSpeed(MOTOR_SPEED);
        followLine();
        setSteering(SERVO_ANGLE);
        hsUpdate(&currentSpeed, &averageSpeed, &totalDistance);
        bleBroadcastDTPS(totalDistance, micros_to_s(currentRunDuration), averageSpeed, currentSpeed);
      }
    }
    else
    {
      stopESC();
      centerSteering();
    }
  }

  delay(10);
}