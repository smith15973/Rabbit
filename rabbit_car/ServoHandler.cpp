// ESCHandler.cpp
#include "ServoHandler.h"

const int SERVO_MIN_PULSE_WIDTH = 1250; // Minimum pulse width in microseconds (full reverse)
const int SERVO_MID_PULSE_WIDTH = 1500; // Neutral position pulse width in microseconds
const int SERVO_MAX_PULSE_WIDTH = 1750; // Maximum pulse width in microseconds (full forward)
const float SERVO_MIN_ANGLE = 45;       // Minimum steering angle (left, in degrees)
const float SERVO_MID_ANGLE = 90;       // Minimum steering angle (left, in degrees)
const float SERVO_MAX_ANGLE = 135;      // Maximum steering angle (right, in degrees)
const float VALID_TURNING_RANGE = 7;

float steerKP = 0.05;
float steerKI = 0.001;
float steerKD = 0.02;
float STEER_MAX_INTEGRAL = 5000;

// PID Variables
int steeringSetPoint = 7500; // Target position (center of 0-7000 range)
int steeringError = 0;
int previousSteeringError = 0;
float steeringIntegral = 0;
unsigned long lastSteeringPIDTime = 0;

// Create a servo object to control the servo
Servo steeringServo;

void setupServo()
{
  pinMode(SERVO_PIN, OUTPUT);

  // Allow allocation of timers 2, 3 for servo control
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  // Initialize SERVO
  steeringServo.attach(SERVO_PIN, SERVO_MIN_PULSE_WIDTH, SERVO_MAX_PULSE_WIDTH);

  // Set to neutral position on startup
  centerSteering();
  delay(500); // Give the servo time to initialize

  Serial.println("Steering servo initialized. Ready to receive steering commands.");
}

// int setSteering(float angle)
// {
//   // Ensure input is within valid range
//   angle = constrain(angle, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE);

//   // Convert angle to pulse width
//   int pulseWidth = map(angle, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE, SERVO_MIN_PULSE_WIDTH, SERVO_MAX_PULSE_WIDTH);

//   // Send the command to the servo
//   steeringServo.writeMicroseconds(pulseWidth);

//   // Log the command
//   // Serial.print("Steering value: ");
//   // Serial.print(steeringValue);
//   // Serial.print(" | Angle: ");
//   // Serial.print(angle);
//   // Serial.print(" | Pulse width: ");
//   // Serial.println(pulseWidth);

//   return pulseWidth;
// }
int setSteering(float angle)
{
  // Ensure input is within valid range
  angle = constrain(angle, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE);

  // Convert angle to pulse width using floating-point interpolation
  float pulseWidthFloat = mapFloat(angle, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE, SERVO_MIN_PULSE_WIDTH, SERVO_MAX_PULSE_WIDTH);

  // Round to nearest integer for pulse width (servo needs integer microseconds)
  int pulseWidth = (int)(pulseWidthFloat + 0.5);

  // Send the command to the servo
  steeringServo.writeMicroseconds(pulseWidth);

  // Log the command (uncommented for debugging precision)
  Serial.print("Steering angle: ");
  Serial.print(angle, 1); // Print with 1 decimal place
  Serial.print("° | Pulse width: ");
  Serial.println(pulseWidth);

  return pulseWidth;
}

void centerSteering()
{
  steeringServo.writeMicroseconds(SERVO_MID_PULSE_WIDTH); // Set to center
}

void steerServoByPID()
{
  readIRSensorsI2C();
  printIRDebugInfo();
  int position = getPosition();

  // if (!isOnLine())
  // // search algorithm
  // {
  //   if (previousSteeringError < 0)
  //   {
  //     // turn left, line is left
  //     setSteering(SERVO_MID_ANGLE - VALID_TURNING_RANGE);
  //   }
  //   else
  //   {
  //     // turn right, line is right
  //     setSteering(SERVO_MID_ANGLE + VALID_TURNING_RANGE);
  //   }

  //   return;
  // }

  // Calculate time delta for derivative and integral
  unsigned long currentTime = micros();
  float deltaTime = micros_to_s(currentTime - lastSteeringPIDTime); // Convert to seconds
  lastSteeringPIDTime = currentTime;

  // Calculate error (how far we are from center)
  steeringError = position - steeringSetPoint; // negative: line is left, positive: line is right
  if (steeringError <= 500 && steeringError >= -500)
  {
    steeringError = 0;
  }

  // Proportional term
  float proportional = steerKP * steeringError;

  // Integral term (accumulated error over time)
  steeringIntegral += steeringError * deltaTime;
  // Prevent integral windup
  steeringIntegral = constrain(steeringIntegral, -STEER_MAX_INTEGRAL, STEER_MAX_INTEGRAL);
  float integral = steerKI * steeringIntegral;

  // Derivative term (rate of change of error)
  float derivative = 0;
  if (deltaTime > 0)
  {
    derivative = steerKD * (steeringError - previousSteeringError) / deltaTime;
  }

  // Calculate PID output
  float pidOutput = proportional + integral + derivative;

  // Convert PID output to steering angle
  // The PID output will be in position units (-3500 to +3500 roughly)
  // Map this to steering angle range
  float steeringAngle = mapFloat(pidOutput, -3500, 3500, SERVO_MID_ANGLE - VALID_TURNING_RANGE, SERVO_MID_ANGLE + VALID_TURNING_RANGE + 5.0);
  steeringAngle = constrain(steeringAngle, SERVO_MID_ANGLE - VALID_TURNING_RANGE, SERVO_MID_ANGLE + VALID_TURNING_RANGE + 5.0);

  // Apply steering
  SERVO_ANGLE = steeringAngle;
  setSteering(steeringAngle);

  // Store error for next iteration
  previousSteeringError = steeringError;

  // Debug output (uncomment if needed)

  // Serial.print("Pos: ");
  // Serial.print(position);
  // Serial.print(" | Error: ");
  // Serial.print(steeringError);
  // Serial.print(" | P: ");
  // Serial.print(proportional);
  // Serial.print(" | I: ");
  // Serial.print(integral);
  // Serial.print(" | D: ");
  // Serial.print(derivative);
  // Serial.print(" | PID: ");
  // Serial.print(pidOutput);
  // Serial.print(" | Angle: ");
  // Serial.println(steeringAngle);
}

// Add this function to reset PID when starting a new run
void resetSteeringPID()
{
  steeringIntegral = 0;
  previousSteeringError = 0;
  lastSteeringPIDTime = micros();
}

// Function to tune PID parameters during runtime
void updateSteeringPIDConstants(float kp, float ki, float kd)
{
  steerKP = kp;
  steerKI = ki;
  steerKD = kd;
}