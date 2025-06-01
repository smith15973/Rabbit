// IRHandler.cpp
// ESP32 Line Sensor Reader using I2C method
// Only handles sensor reading and direction determination

#include "IRHandler.h"

// I2C address of the line patrol module
const int IR_SENSOR_COUNT = 16;
const byte SENSOR_ADDR = 0x12; // Default address of the 8-channel line patrol module
const byte SENSOR_REG = 0x30;  // Register to read sensor values from

// Variables for line following
int sensorValues[IR_SENSOR_COUNT]; // Array to store sensor readings
int linePosition = 0;              // Position of the line (0-7000)

void irSetup()
{
  // Initialize I2C communication
  Wire.begin(IR1_SDA_PIN, IR1_SCL_PIN);
  Wire1.begin(IR2_SDA_PIN, IR2_SCL_PIN);

  // Wait for sensor module to initialize
  delay(1000);

  Serial.println("I2C Line sensor reader ready!");
}

void readIRSensorsI2C() {
    // Read from first module (sensors 0-7)
    Wire.beginTransmission(SENSOR_ADDR);
    Wire.write(SENSOR_REG);
    Wire.endTransmission();
    Wire.requestFrom(SENSOR_ADDR, 1);
    
    byte data1 = 0;
    if (Wire.available()) {
        data1 = Wire.read();
    }
    
    // Read from second module (sensors 8-15)
    Wire1.beginTransmission(SENSOR_ADDR);  // Assuming same address
    Wire1.write(SENSOR_REG);
    Wire1.endTransmission();
    Wire1.requestFrom(SENSOR_ADDR, 1);
    
    byte data2 = 0;
    if (Wire1.available()) {
        data2 = Wire1.read();
    }
    
    // Extract sensor values from first module (0-7)
    for (int i = 0; i < 8; i++) {
        sensorValues[i] = ((data1 >> (7-i)) & 0x01) ^ (!IS_WHITE_LINE);
    }
    
    // Extract sensor values from second module (8-15)
    for (int i = 0; i < 8; i++) {
        sensorValues[i + 8] = ((data2 >> (7-i)) & 0x01) ^ (!IS_WHITE_LINE);
    }
}

int getPosition()
{
  // Calculate weighted average for line position
  int sum = 0;
  int weightedSum = 0;

  for (int i = 0; i < IR_SENSOR_COUNT; i++)
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
    linePosition = weightedSum / sum; //This gives 0-15000 range now
  }

  return linePosition;
}

void printIRDebugInfo()
{
  // Print sensor values
  Serial.print("Sensors: ");
  for (int i = 0; i < IR_SENSOR_COUNT; i++)
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
  for (int i = 0; i < IR_SENSOR_COUNT; i++)
  {
    if (sensorValues[i] == 1)
    {
      return true;
    }
  }
  return false;
}