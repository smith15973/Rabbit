// IR8Handler.cpp
// ESP32 Line Sensor Reader using I2C method
// Only handles sensor reading and direction determination

#include "IR8Handler.h"

// I2C address of the line patrol module
const byte SENSOR_ADDR = 0x12; // Default address of the 8-channel line patrol module
const byte SENSOR_REG = 0x30;  // Register to read sensor values from

// Variables for line following
int sensorValues[8];  // Array to store sensor readings
int linePosition = 0; // Position of the line (0-7000)

void ir8Setup()
{
  // Initialize I2C communication
  Wire.begin(IR_SDA_PIN, IR_SCL_PIN);

  // Wait for sensor module to initialize
  delay(1000);

  Serial.println("I2C Line sensor reader ready!");
}

void readSensorsI2C()
{
  byte data = 0;

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

  // Extract individual sensor values from the byte and adjust based on line color
  // Using XOR with IS_WHITE_LINE to conditionally invert
  sensorValues[0] = ((data >> 7) & 0x01) ^ (!IS_WHITE_LINE); // Leftmost sensor
  sensorValues[1] = ((data >> 6) & 0x01) ^ (!IS_WHITE_LINE);
  sensorValues[2] = ((data >> 5) & 0x01) ^ (!IS_WHITE_LINE);
  sensorValues[3] = ((data >> 4) & 0x01) ^ (!IS_WHITE_LINE);
  sensorValues[4] = ((data >> 3) & 0x01) ^ (!IS_WHITE_LINE);
  sensorValues[5] = ((data >> 2) & 0x01) ^ (!IS_WHITE_LINE);
  sensorValues[6] = ((data >> 1) & 0x01) ^ (!IS_WHITE_LINE);
  sensorValues[7] = (data & 0x01) ^ (!IS_WHITE_LINE); // Rightmost sensor
}

int getPosition()
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

  // Check if we found the line (sum > 0 means at least one sensor detected it)
  if (sum > 0)
  {
    linePosition = weightedSum / sum;
  }

  return linePosition;
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
}

// Helper function to check if on line
bool isOnLine()
{
  for (int i = 0; i < 8; i++)
  {
    if (sensorValues[i] == 1)
    {
      return true;
    }
  }
  return false;
}