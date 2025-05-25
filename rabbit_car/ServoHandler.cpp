// ESCHandler.cpp
#include "ServoHandler.h"

const int SERVO_MIN_PULSE_WIDTH = 1000; // Minimum pulse width in microseconds (full reverse)
const int SERVO_MID_PULSE_WIDTH = 1500; // Neutral position pulse width in microseconds
const int SERVO_MAX_PULSE_WIDTH = 2000; // Maximum pulse width in microseconds (full forward)
const int SERVO_MIN_ANGLE = 45;         // Minimum steering angle (left, in degrees)
const int SERVO_MAX_ANGLE = 135;        // Maximum steering angle (right, in degrees)

// Create a servo object to control the ESC
Servo steeringServo;

void setupServo()
{
  pinMode(SERVO_PIN, OUTPUT);

  // Allow allocation of timers 2, 3 for servo control
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  // Initialize ESC
  steeringServo.setPeriodHertz(50); // Standard 50Hz servo frequency
  steeringServo.attach(SERVO_PIN, SERVO_MIN_PULSE_WIDTH, SERVO_MAX_PULSE_WIDTH);

  // Set to neutral position on startup
  centerSteering();
  delay(500); // Give the servo time to initialize

  Serial.println("Steering servo initialized. Ready to receive steering commands.");
}

/**
 * Alternative function that takes a -100 to +100 value instead
 * Negative values are reverse, positive values are forward
 *
 * @param speedValue Speed from -100 (full reverse) to +100 (full forward), 0 is stop
 * @return The actual pulse width sent to the ESC in microseconds
 */
int setSteering(float steeringValue)
{
  // Ensure input is within valid range
  steeringValue = constrain(steeringValue, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE);

  // Map the steering value to the servo angle range
  float angle;

  if (steeringValue < 90)
  {
    // Left steering: map -100 to 0 to SERVO_MIN_ANGLE to 90°
    angle = map(steeringValue, 45, 90, SERVO_MIN_ANGLE, 90);
  }
  else 
  {
    // Right steering: map 0 to 100 to 90° to SERVO_MAX_ANGLE
    angle = map(steeringValue, 90, 135, 90, SERVO_MAX_ANGLE);
  }

  // Convert angle to pulse width
  int pulseWidth = map(angle, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE, SERVO_MIN_PULSE_WIDTH, SERVO_MAX_PULSE_WIDTH);

  // Send the command to the servo
  steeringServo.writeMicroseconds(pulseWidth);
  

  // Log the command
  // Serial.print("Steering value: ");
  // Serial.print(steeringValue);
  // Serial.print(" | Angle: ");
  // Serial.print(angle);
  // Serial.print(" | Pulse width: ");
  // Serial.println(pulseWidth);

  return pulseWidth;
}

void centerSteering()
{
  steeringServo.writeMicroseconds(SERVO_MID_PULSE_WIDTH); // Set to center
}
