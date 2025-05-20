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
float MOTOR_SPEED = 0;
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
float currentSpeed = 0.0;  // Current speed in m/s
float totalDistance = 0.0; // Total distance in m
unsigned long currentTime = micros();
unsigned long lastSpeedUpdateTime = micros();

float targetPaceBuffer = 0.25;

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
        // For the first adjustment, we may want to do a gentle initial acceleration
        static bool firstSpeedAdjustment = true;

        if (firstSpeedAdjustment)
        {
          // Start with a very conservative initial speed
          MOTOR_SPEED = 20.0; // Start at 20% power
          setMotorSpeed(MOTOR_SPEED);
          firstSpeedAdjustment = false;
        }

        // Use PID control for speed adjustment - run every 0.25 seconds for smoother transitions
        if (currentTime - lastSpeedUpdateTime >= s_to_micros(0.25))
        {
          // Replace the binary if/else speed adjustment with PID control
          adjustMotorSpeedPID(currentSpeed, targetPace);
          lastSpeedUpdateTime = currentTime;
        }

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