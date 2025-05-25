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
float MOTOR_SPEED = 1500;
int SERVO_ANGLE = 90;
bool RUNNING = false;
bool startRunTimer = true;
bool manualControl = false;
bool IS_WHITE_LINE = false; // determines whether the logic follows a white or black line

volatile unsigned long startTime = 0;          // start of pacing
volatile unsigned long currentRunDuration = 0; // current total time of the run
volatile unsigned long endTime = 0;            // final end time

// Mode variables
String MODE = "RACE"; // Current mode: "RACE", "TEMPO", "DISTANCE_PACE"
bool BRAKE = false;

// Universal parameters (set based on mode)
float targetDistance = 0.0; // user entered target distance in meters
float targetTime = 0.0;     // user entered run time in seconds
float targetSpeed = 0.0;    // user entered target speed in m/s (for TEMPO and DISTANCE_PACE)

// Runtime tracking
float averageSpeed = 0.0;  // average speed for the current run
float currentSpeed = 0.0;  // Current speed in m/s
float totalDistance = 0.0; // Total distance in m
unsigned long currentTime = micros();
unsigned long lastSpeedUpdateTime = micros();

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting Rabbit...");

  // Initialize ESC
  setupESC();
  // Initialize Servo
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
  if (BRAKE) {
    brakeESC();
    
  }
  if (startRunTimer)
  {
    resetPID();
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
      bool shouldEnd = false;
      float currentTargetSpeed = 0.0;

      // Mode-specific logic
      if (MODE == "RACE")
      {
        // RACE mode: Time + Distance → vary speed to hit exact finish time
        shouldEnd = checkRaceEndCondition();
        currentTargetSpeed = calculateRacePace();
      }
      else if (MODE == "TEMPO")
      {
        // TEMPO mode: Speed + Time → maintain constant speed
        shouldEnd = checkTempoEndCondition();
        currentTargetSpeed = targetSpeed;
      }
      else if (MODE == "DISTANCE_PACE")
      {
        // DISTANCE_PACE mode: Speed + Distance → maintain constant speed until distance complete
        shouldEnd = checkDistancePaceEndCondition();
        currentTargetSpeed = targetSpeed;
      }

      if (shouldEnd)
      {
        stopESC();
        centerSteering();
        RUNNING = false;
        startRunTimer = false;
        endTime = currentTime;
        printRunSummary();
      }
      else
      {
        // Use PID control for speed adjustment - run every 0.25 seconds for smoother transitions
        if (currentTime - lastSpeedUpdateTime >= s_to_micros(0.25))
        {
          adjustMotorSpeedPID(currentSpeed, currentTargetSpeed);
          lastSpeedUpdateTime = currentTime;
        }

        // followLine();
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

// RACE mode: Adjust pace dynamically to finish distance in target time
bool checkRaceEndCondition()
{
  // End when we've reached the target distance
  return (totalDistance >= targetDistance) ;
}

float calculateRacePace()
{
  float elapsedTime = micros_to_s(currentRunDuration);
  float remainingDistance = targetDistance - totalDistance;
  float remainingTime = targetTime - elapsedTime;

  // Prevent division by zero and negative time
  if (remainingTime <= 0.1) // 0.1 second buffer
  {
    return 0.0; // Stop if we're out of time
  }

  // Calculate required pace to finish on time
  float requiredPace = remainingDistance / remainingTime;

  // Clamp to reasonable limits (adjust these based on your car's capabilities)
  requiredPace = constrain(requiredPace, 0.5, 15.0);

  Serial.printf("Race Mode - Remaining: %.2fm in %.2fs, Required pace: %.2fm/s\n",
                remainingDistance, remainingTime, requiredPace);

  return requiredPace;
}

// TEMPO mode: Maintain constant speed for set time
bool checkTempoEndCondition()
{
  float elapsedTime = micros_to_s(currentRunDuration);
  return elapsedTime >= targetTime;
}

// DISTANCE_PACE mode: Maintain constant speed until distance is reached
bool checkDistancePaceEndCondition()
{
  return totalDistance >= targetDistance;
}

void printRunSummary()
{
  float finalTime = micros_to_s(endTime - startTime);

  Serial.println("=== RUN SUMMARY ===");
  Serial.printf("Mode: %s\n", MODE.c_str());
  Serial.printf("Total Distance: %.2f m\n", totalDistance);
  Serial.printf("Total Time: %.2f s\n", finalTime);
  Serial.printf("Average Speed: %.2f m/s\n", averageSpeed);

  if (MODE == "RACE")
  {
    Serial.printf("Target: %.2fm in %.2fs\n", targetDistance, targetTime);
    Serial.printf("Distance Error: %.2fm\n", targetDistance - totalDistance);
    Serial.printf("Time Error: %.2fs\n", targetTime - finalTime);
  }
  else if (MODE == "TEMPO")
  {
    Serial.printf("Target: %.2fm/s for %.2fs\n", targetSpeed, targetTime);
    Serial.printf("Speed Error: %.2fm/s\n", targetSpeed - averageSpeed);
  }
  else if (MODE == "DISTANCE_PACE")
  {
    Serial.printf("Target: %.2fm at %.2fm/s\n", targetDistance, targetSpeed);
    Serial.printf("Distance Error: %.2fm\n", targetDistance - totalDistance);
    Serial.printf("Speed Error: %.2fm/s\n", targetSpeed - averageSpeed);
  }

  Serial.println("==================");
}

// Function to be called when BLE receives new parameters
void updateModeParameters(String mode, float param1, float param2)
{
  MODE = mode;

  if (MODE == "RACE")
  {
    // RACE: param1 = distance, param2 = time
    targetDistance = param1;
    targetTime = param2;
    Serial.printf("RACE Mode: %.2fm in %.2fs\n", targetDistance, targetTime);
  }
  else if (MODE == "TEMPO")
  {
    // TEMPO: param1 = speed, param2 = time
    targetSpeed = param1;
    targetTime = param2;
    Serial.printf("TEMPO Mode: %.2fm/s for %.2fs\n", targetSpeed, targetTime);
  }
  else if (MODE == "DISTANCE_PACE")
  {
    // DISTANCE_PACE: param1 = speed, param2 = distance
    targetSpeed = param1;
    targetDistance = param2;
    Serial.printf("DISTANCE_PACE Mode: %.2fm/s for %.2fm\n", targetSpeed, targetDistance);
  }

  // Reset run state for new parameters
  if (!manualControl)
  {
    RUNNING = false;
    startRunTimer = true;
    totalDistance = 0.0;
    averageSpeed = 0.0;
  }
}