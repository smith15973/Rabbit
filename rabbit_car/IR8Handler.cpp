// IR8Handler.cpp
// ESP32 Line Sensor Reader using I2C method
// Only handles sensor reading and direction determination

#include "IR8Handler.h"

// I2C address of the line patrol module
const byte SENSOR_ADDR = 0x12; // Default address of the 8-channel line patrol module
const byte SENSOR_REG = 0x30;  // Register to read sensor values from
const float NEUTRAL_STEERING_ANGLE = 93;
const float MAX_STEERING_ANGLE = 14;

// Variables for line following
int sensorValues[8];     // Array to store sensor readings
int linePosition = 0;    // Position of the line (0-7000)
int lastPosition = 3500; // Last known position
bool onLine = false;     // Whether any sensor detects the line

// Variables for direction determination
int targetPosition = 3500; // Target position (middle of sensors)
int error = 0;             // Current error (negative: line is left, positive: line is right)

unsigned long lastReadTime = 0;
const unsigned long READ_INTERVAL = 20; // Check sensors every 50ms for fast response

float steerKP = 0.05;
float steerKI = 0.001;
float steerKD = 0.02;
float STEER_MAX_INTEGRAL = 1000;

// PID state variables
float previousSteeringError = 0;
float steeringIntegral = 0;
unsigned long lastPIDTime = 0;

void ir8Setup()
{
  // Initialize I2C communication
  Wire.begin(IR_SDA_PIN, IR_SCL_PIN);

  // Wait for sensor module to initialize
  delay(1000);

  Serial.println("I2C Line sensor reader ready!");
}

void followLine()
{
  // Read sensors at the specified interval for optimal performance
  //   unsigned long currentTime = millis();
  //   if (currentTime - lastReadTime >= READ_INTERVAL) {
  // lastReadTime = currentTime;

  // Read all sensors via I2C
  readSensorsI2C();

  // Calculate line position
  calculatePosition();

  // Determine direction to turn based on line position
  steer();

  // Uncomment for debugging
  printIR8DebugInfo();
}

void readSensorsI2C()
{
  byte data = 0;
  onLine = false;

  // Request sensor data from the I2C module
  Wire.beginTransmission(SENSOR_ADDR);
  Wire.write(SENSOR_REG);
  Wire.endTransmission();

  // Request 1 byte of data
  Wire.requestFrom(SENSOR_ADDR, 1);

  // Read the data if available
  if (Wire.available())
  {
    data = Wire.read();
  }

  // Extract individual sensor values from the byte
  sensorValues[0] = (data >> 7) & 0x01; // Leftmost sensor
  sensorValues[1] = (data >> 6) & 0x01;
  sensorValues[2] = (data >> 5) & 0x01;
  sensorValues[3] = (data >> 4) & 0x01;
  sensorValues[4] = (data >> 3) & 0x01;
  sensorValues[5] = (data >> 2) & 0x01;
  sensorValues[6] = (data >> 1) & 0x01;
  sensorValues[7] = data & 0x01; // Rightmost sensor

  // Adjust sensor values based on line color
  for (int i = 0; i < 8; i++)
  {
    if (!IS_WHITE_LINE)
    {
      // For black line, invert the sensor value (1 means line detected)
      sensorValues[i] = !sensorValues[i];
    }
    // Check if any sensor detects the line
    if (sensorValues[i] == 1)
    {
      onLine = true;
    }
  }
}

void calculatePosition()
{
  // Calculate weighted average for line position
  int sum = 0;
  int weightedSum = 0;

  for (int i = 0; i < 8; i++)
  {
    if (sensorValues[i] == 1)
    {
      weightedSum += i * 1000;
      sum += 1;
    }
  }

  // If we found the line, calculate position
  if (onLine)
  {
    linePosition = weightedSum / sum;
    lastPosition = linePosition;
  }
  else
  {
    // If no line is detected, use the last known position
    linePosition = lastPosition;
  }

  // Calculate error from target position
  // Negative error means line is to the left
  // Positive error means line is to the right
  error = targetPosition - linePosition;
}

void steer()
{
  float steeringAngle;

  // If we've lost the line completely
  if (!onLine)
  {
    if (lastPosition < targetPosition)
    {
      // Line was on the left, turn sharply left
      steeringAngle = NEUTRAL_STEERING_ANGLE - MAX_STEERING_ANGLE; // Maximum left turn
      // Serial.println("ACTION: TURN LEFT SHARP - Line lost, last position left");
    }
    else
    {
      // Line was on the right, turn sharply right
      steeringAngle = NEUTRAL_STEERING_ANGLE + MAX_STEERING_ANGLE; // Maximum right turn
      // Serial.println("ACTION: TURN RIGHT SHARP - Line lost, last position right");
    }
  }
  else
  {
    // Direct mapping of error to steering angle
    int absError = abs(error);
    float steeringOffset;

    if ((absError) <= 0)
    {
      steeringOffset = 0;
    }
    else if (absError < 500)
    {
      // Medium error - moderate steering (±2 degrees at 2500 error)
      steeringOffset = (error > 0) ? MAX_STEERING_ANGLE *(1.0/7.0) : -MAX_STEERING_ANGLE *(1.0/7.0);
    }
    else if (absError < 1000)
    {
      // Small error - gentle steering (±1 degree at 1000 error)
      steeringOffset = (error > 0) ? MAX_STEERING_ANGLE *(2.0/7.0) : -MAX_STEERING_ANGLE *(2.0/7.0);
    }
    else if (absError < 1500)
    {
      // Medium error - moderate steering (±2 degrees at 2500 error)
      steeringOffset = (error > 0) ? MAX_STEERING_ANGLE *(3.0/7.0) : -MAX_STEERING_ANGLE *(3.0/7.0);
    }
    else if (absError < 2000)
    {
      // Medium error - moderate steering (±2 degrees at 2500 error)
      steeringOffset = (error > 0) ? MAX_STEERING_ANGLE *(4.0/7.0) : -MAX_STEERING_ANGLE *(4.0/7.0);
    }
    else if (absError < 2500)
    {
      // Medium error - moderate steering (±2 degrees at 2500 error)
      steeringOffset = (error > 0) ? MAX_STEERING_ANGLE *(5.0/7.0) : -MAX_STEERING_ANGLE *(5.0/7.0);
    }
    else if (absError < 3000)
    {
      // Medium error - moderate steering (±2 degrees at 2500 error)
      steeringOffset = (error > 0) ? MAX_STEERING_ANGLE *(6.0/7.0) : -MAX_STEERING_ANGLE *(6.0/7.0);
    }
    else
    {
      // Large error - aggressive steering (±3 degrees max)
      steeringOffset = (error > 0) ? MAX_STEERING_ANGLE *(7.0/7.0) : -MAX_STEERING_ANGLE *(7.0/7.0);
    }

    // Calculate steering angle
    steeringAngle = NEUTRAL_STEERING_ANGLE - steeringOffset;
  }

  // Set the servo value
  SERVO_ANGLE = steeringAngle;
}

// void steer()
// {
//   float steeringAngle;

//   // If we've lost the line completely
//   if (!onLine)
//   {
//     if (lastPosition < targetPosition)
//     {
//       // Line was on the left, turn sharply left
//       steeringAngle = NEUTRAL_STEERING_ANGLE - MAX_STEERING_ANGLE; // Maximum left turn
//       // Serial.println("ACTION: TURN LEFT SHARP - Line lost, last position left");
//     }
//     else
//     {
//       // Line was on the right, turn sharply right
//       steeringAngle = NEUTRAL_STEERING_ANGLE + MAX_STEERING_ANGLE; // Maximum right turn
//       // Serial.println("ACTION: TURN RIGHT SHARP - Line lost, last position right");
//     }
//   }
//   else
//   {
//     // Use non-linear gain for better control
//     // For small errors: gentle correction
//     // For large errors: more aggressive correction

//     float gain;
//     int absError = abs(error);

//     if (absError < 1000)
//     {
//       // Small error - gentle steering
//       gain = 0.05;
//     }
//     else if (absError < 2500)
//     {
//       // Medium error - moderate steering
//       gain = 0.003;
//     }
//     else
//     {
//       // Large error - aggressive steering
//       gain = 0.001;
//     }

//     // Calculate steering angle
//     steeringAngle = NEUTRAL_STEERING_ANGLE - (error * gain);
//   }

//   // Set the servo value
//   SERVO_ANGLE = steeringAngle;
// }

void steerByPID()
{
  unsigned long currentTime = micros();
  float deltaTime = micros_to_s(currentTime - lastPIDTime);

  // Don't run PID too frequently
  if (deltaTime < 0.01)
    return; // Minimum 10ms between PID calculations

  float steeringAngle;

  // If we've lost the line completely, use emergency steering
  if (!onLine)
  {
    // Reset PID terms when line is lost
    steeringIntegral = 0;
    previousSteeringError = 0;

    if (lastPosition < targetPosition)
    {
      steeringAngle = NEUTRAL_STEERING_ANGLE - MAX_STEERING_ANGLE; // Maximum left turn
    }
    else
    {
      steeringAngle = NEUTRAL_STEERING_ANGLE + MAX_STEERING_ANGLE; // Maximum right turn
    }
  }
  else
  {
    // PID Control
    float pidOutput = calculateSteeringPID(error, deltaTime);

    // Convert PID output to steering angle
    steeringAngle = NEUTRAL_STEERING_ANGLE - pidOutput; // NEUTRAL_STEERING_ANGLE is center, subtract for left/right

    // Constrain to valid steering range
    steeringAngle = constrain(steeringAngle, NEUTRAL_STEERING_ANGLE - MAX_STEERING_ANGLE, NEUTRAL_STEERING_ANGLE + MAX_STEERING_ANGLE);
  }

  // Set the servo angle
  SERVO_ANGLE = steeringAngle;
  lastPIDTime = currentTime;
}

float calculateSteeringPID(float currentError, float deltaTime)
{
  // Proportional term
  float proportional = steerKP * currentError;

  // Integral term (accumulated error over time)
  steeringIntegral += currentError * deltaTime;
  // Prevent steeringIntegral windup
  steeringIntegral = constrain(steeringIntegral, -STEER_MAX_INTEGRAL, STEER_MAX_INTEGRAL);
  float steeringIntegralTerm = steerKI * steeringIntegral;

  // Derivative term (rate of change of error)
  float derivative = 0;
  if (deltaTime > 0)
  {
    derivative = (currentError - previousSteeringError) / deltaTime;
  }
  float derivativeTerm = steerKD * derivative;

  // Calculate total PID output
  float pidOutput = proportional + steeringIntegralTerm + derivativeTerm;

  // Store current error for next iteration
  previousSteeringError = currentError;

  // Optional: Print PID debug info
  // Serial.printf("P:%.2f I:%.2f D:%.2f Out:%.2f\n", proportional, steeringIntegralTerm, derivativeTerm, pidOutput);

  return pidOutput;
}

// Add this function to reset PID when starting a new run
void resetSteeringPID()
{
  steeringIntegral = 0;
  previousSteeringError = 0;
  lastPIDTime = micros();
}

void printIR8DebugInfo()
{
  // Print sensor values
  Serial.print("Sensors: ");
  for (int i = 0; i < 8; i++)
  {
    Serial.print(sensorValues[i]);
    Serial.print(" ");
  }

  // Print position and error
  Serial.print(" | Position: ");
  Serial.print(linePosition);
  Serial.print(" | Error: ");
  Serial.println(error);
}

// Getter functions to access IR8 data from main sketch
int getLineError()
{
  return error;
}

bool isOnLine()
{
  return onLine;
}