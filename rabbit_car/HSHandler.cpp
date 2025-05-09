#include "HSHandler.h"

const float WHEEL_CIRCUMFERENCE = PI * WHEEL_DIAMETER; //in mm
const float MEASUREMENT_INTERVAL = 1000; // Time interval for speed calculation (ms)

// Variables
volatile unsigned long pulseCount = 0; // Total pulse count
volatile unsigned long intervalPulseCount = 0; // Pulse count within current interval
unsigned long lastMeasurementTime = 0; // Last time speed was calculated

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
  
  lastMeasurementTime = millis();
}

void hsUpdate(float* currentSpeed, float* totalDistance) {
  unsigned long currentTime = millis();
  
  // Calculate speed and distance every MEASUREMENT_INTERVAL
  if (currentTime - lastMeasurementTime >= MEASUREMENT_INTERVAL) {
    // Calculate speed
    unsigned long pulsesDuringInterval = intervalPulseCount;
    intervalPulseCount = 0; // Reset interval counter
    
    // Calculate distance traveled during this interval (in km)
    // Adjust for number of magnets (each pulse represents 1/MAGNETS_COUNT of a rotation)
    float intervalDistance = (pulsesDuringInterval * WHEEL_CIRCUMFERENCE) / (MAGNETS_COUNT * 1000.0);
    
    // Add the interval distance to total distance
    *totalDistance += intervalDistance;
    
    // Calculate current speed (km/h)
    float intervalSeconds = (float)(currentTime - lastMeasurementTime) / 1000.0;
    *currentSpeed = (intervalDistance / intervalSeconds) * 3600.0; // Convert to km/h
    
    // Update last measurement time
    lastMeasurementTime = currentTime;
    
    // Display results
    Serial.print("Pulses: ");
    Serial.print(pulseCount);
    Serial.print(" | Speed: ");
    Serial.print(*currentSpeed, 2);
    Serial.print(" km/h | Distance: ");
    Serial.print(*totalDistance * 1000, 3); // Convert km to m for display
    Serial.println(" m");
  }
}