// IR8Handler.cpp
// ESP32 Line Sensor Reader using I2C method
// Only handles sensor reading and direction determination

#include "IR8Handler.h"

// I2C address of the line patrol module
const byte SENSOR_ADDR = 0x12; // Default address of the 8-channel line patrol module
const byte SENSOR_REG = 0x30;  // Register to read sensor values from

// Variables for line following
int sensorValues[8];     // Array to store sensor readings
int linePosition = 0;    // Position of the line (0-7000)
int lastPosition = 3500; // Last known position
bool onLine = false;     // Whether any sensor detects the line

// Variables for direction determination
int targetPosition = 3500; // Target position (middle of sensors)
int error = 0;             // Current error (negative: line is left, positive: line is right)

unsigned long lastReadTime = 0;
const unsigned long READ_INTERVAL = 50; // Check sensors every 50ms for fast response

float steerKP;
float steerKI;
float steerKD;
float STEER_MAX_INTEGRAL;

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
  // printIR8DebugInfo();
  //   }
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
    if (IS_WHITE_LINE)
    {
      // For white line, invert the sensor value (0 means line detected)
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
  if (sum > 0)
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
  int steeringAngle;

  // If we've lost the line completely
  if (!onLine)
  {
    if (lastPosition < targetPosition)
    {
      // Line was on the left, turn sharply left
      steeringAngle = 45; // Maximum left turn
      // Serial.println("ACTION: TURN LEFT SHARP - Line lost, last position left");
    }
    else
    {
      // Line was on the right, turn sharply right
      steeringAngle = 135; // Maximum right turn
      // Serial.println("ACTION: TURN RIGHT SHARP - Line lost, last position right");
    }
  }
  else
  {
    // Use non-linear gain for better control
    // For small errors: gentle correction
    // For large errors: more aggressive correction

    float gain;
    int absError = abs(error);

    if (absError < 1000)
    {
      // Small error - gentle steering
      gain = 0.03;
    }
    else if (absError < 2500)
    {
      // Medium error - moderate steering
      gain = 0.02;
    }
    else
    {
      // Large error - aggressive steering
      gain = 0.01;
    }

    // Calculate steering angle
    steeringAngle = 90 + (error * gain);

    // Debug output
    // Serial.print("ACTION: STEER - Error: ");
    // Serial.print(error);
    // Serial.print(" | Gain: ");
    // Serial.print(gain, 3);
    // Serial.print(" | Angle: ");
    // Serial.println(steeringAngle);
  }

  // Set the servo value
  SERVO_ANGLE = steeringAngle;
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