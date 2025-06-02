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

// Add these variables to your IRHandler
int previousValidPosition = 7500; // Last known good position
int positionChangeThreshold = 2500; // Maximum reasonable position change per cycle
int consecutiveValidReadings = 0;
int minConsecutiveReadings = 0; // Require X consecutive similar readings

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

int getFilteredPosition() {
    int rawPosition = getPosition();
    int positionChange = abs(rawPosition - previousValidPosition);
    bool validPattern = isValidLinePattern();
    
    // Accept if BOTH conditions are met:
    // 1. Position change is reasonable
    // 2. Sensor pattern looks like a normal line
    if (positionChange < positionChangeThreshold && validPattern) {
        previousValidPosition = rawPosition;
        return rawPosition;
    } else {
        Serial.print("REJECTED - Change: ");
        Serial.print(positionChange);
        Serial.print(" | Valid pattern: ");
        Serial.println(validPattern);
        return previousValidPosition;
    }
}

bool isValidLinePattern() {
    int activeSensors = 0;
    int firstActive = -1, lastActive = -1;
    
    for (int i = 0; i < IR_SENSOR_COUNT; i++) {
        if (sensorValues[i] == 1) {
            activeSensors++;
            if (firstActive == -1) firstActive = i;
            lastActive = i;
        }
    }
    
    int lineWidth = lastActive - firstActive + 1;
    
    // Normal line: 3-7 active sensors, width 3-7
    // Triangle/marking: 8+ active sensors, width 8+
    bool normalWidth = (activeSensors >= 2 && activeSensors <= 8);
    bool normalSpan = (lineWidth >= 2 && lineWidth <= 8);
    
    Serial.print("Active sensors: ");
    Serial.print(activeSensors);
    Serial.print(" | Line width: ");
    Serial.print(lineWidth);
    Serial.print(" | Valid: ");
    Serial.println(normalWidth && normalSpan);
    
    return normalWidth && normalSpan;
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