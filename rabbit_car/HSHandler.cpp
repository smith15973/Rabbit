#include "HSHandler.h"

const float WHEEL_CIRCUMFERENCE = PI * WHEEL_DIAMETER; // in mm
const float MEASUREMENT_INTERVAL = 200000;               // Time interval for speed calculation (ms)

// Variables
volatile unsigned long pulseCount = 0;         // Total pulse count
volatile unsigned long intervalPulseCount = 0; // Pulse count within current interval
unsigned long lastMeasurementTime = 0;         // Last time speed was calculated

// Interrupt Service Routine for hall sensor
void IRAM_ATTR hallSensorISR()
{
  pulseCount++;
  intervalPulseCount++;
}

void setupHS()
{
  pinMode(HS_PIN, INPUT_PULLUP);
  // Attach interrupt to hall sensor pin
  attachInterrupt(digitalPinToInterrupt(HS_PIN), hallSensorISR, FALLING);

  Serial.println("ESP32 Hall Sensor Speed & Distance Tracker");
  Serial.print("Wheel Diameter: ");
  Serial.print(WHEEL_DIAMETER);
  Serial.println(" mm");
  Serial.print("Wheel Circumference: ");
  Serial.print(WHEEL_CIRCUMFERENCE);
  Serial.println(" mm");
  Serial.print("Magnets on wheel: ");
  Serial.println(MAGNETS_COUNT);
  Serial.println("Ready to measure...");

  lastMeasurementTime = micros();
}

void hsUpdate(float *currentSpeed, float *averageSpeed, float *totalDistance)
{
  unsigned long currentTime = micros();

  // Calculate speed and distance every MEASUREMENT_INTERVAL
  if (currentTime - lastMeasurementTime >= MEASUREMENT_INTERVAL)
  {
    // Get pulses during this interval
    unsigned long pulsesDuringInterval = intervalPulseCount;
    intervalPulseCount = 0; // Reset interval counter

    // Calculate distance traveled during this interval (in meters)
    // Each pulse represents 1/MAGNETS_COUNT of a rotation
    float intervalDistance = (pulsesDuringInterval * WHEEL_CIRCUMFERENCE) / (MAGNETS_COUNT * 1000.0); // mm to m

    // Add the interval distance to total distance
    *totalDistance += intervalDistance;

    // Calculate current speed (m/s)
    float intervalSeconds = micros_to_s(currentTime - lastMeasurementTime);
    *currentSpeed = intervalDistance / intervalSeconds;
    // Serial.printf("Current Speed: %.2f\n", *currentSpeed);

    // Update running average speed
    float runTimeSeconds = micros_to_s(currentRunDuration);
    if (runTimeSeconds > 0)
    {
      *averageSpeed = (*totalDistance) / runTimeSeconds;
    }
    else
    {
      *averageSpeed = 0;
    }

    // Update last measurement time
    lastMeasurementTime = currentTime;

    // Display results
    // Serial.print("Pulses: ");
    // Serial.print(pulseCount);
    // Serial.print(" | Speed: ");
    // Serial.print(*currentSpeed, 2);
    // Serial.print(" m/s (");
    // Serial.print(*currentSpeed * 3.6, 2); // Convert to km/h
    // Serial.print(" km/h) | Distance: ");
    // Serial.print(*totalDistance, 3);
    // Serial.print(" m | Avg Speed: ");
    // Serial.print(*averageSpeed, 2);
    // Serial.println(" m/s");
  }
}

void hsStart()
{
  Serial.printf("HS Cleared, Last Run Length: %.2f\n", micros_to_s(currentRunDuration));

  totalDistance = 0.0;
  currentSpeed = 0.0;
  averageSpeed = 0.0;

  pulseCount = 0;
  intervalPulseCount = 0;
  lastMeasurementTime = micros();
}