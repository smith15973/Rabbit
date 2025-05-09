// ESCHandler.cpp
#include "ESCHandler.h"

const int ESC_MIN_PULSE_WIDTH = 1000; // Minimum pulse width in microseconds (full reverse)
const int ESC_MID_PULSE_WIDTH = 1500; // Neutral position pulse width in microseconds
const int ESC_MAX_PULSE_WIDTH = 2000; // Maximum pulse width in microseconds (full forward)
// actually starts to move backward at 1407
// actually starts to move forward 1583

// Create a servo object to control the ESC
Servo ESC;

void setupESC()
{
  pinMode(ESC_PIN, OUTPUT);

  // Allow allocation of timers 0,1 for ESC control
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);

  // Initialize ESC
  ESC.setPeriodHertz(50); // Standard 50Hz servo frequency
  ESC.attach(ESC_PIN, ESC_MIN_PULSE_WIDTH, ESC_MAX_PULSE_WIDTH);

  // Set to neutral position on startup
  ESC.writeMicroseconds(ESC_MID_PULSE_WIDTH);
  delay(1000); // Give the ESC time to initialize

  Serial.println("ESC initialized. Ready to receive speed commands.");
}

/**
 * Alternative function that takes a -100 to +100 value instead
 * Negative values are reverse, positive values are forward
 *
 * @param speedValue Speed from -100 (full reverse) to +100 (full forward), 0 is stop
 * @return The actual pulse width sent to the ESC in microseconds
 */
int setMotorSpeed(int speedValue)
{
  // Ensure input is within valid range
  speedValue = constrain(speedValue, -100, 100);

  // Calculate the pulse width based on the speed value
  int pulseWidth;

  if (speedValue < 0)
  {
    // Reverse speed (map -100-0 to ESC_MIN_PULSE_WIDTH-ESC_MID_PULSE_WIDTH)
    pulseWidth = map(speedValue, -100, 0, ESC_MIN_PULSE_WIDTH, ESC_MID_PULSE_WIDTH);
  }
  else if (speedValue > 0)
  {
    // Forward speed (map 0-100 to ESC_MID_PULSE_WIDTH-ESC_MAX_PULSE_WIDTH)
    pulseWidth = map(speedValue, 0, 100, ESC_MID_PULSE_WIDTH, ESC_MAX_PULSE_WIDTH);
  }
  else
  {
    // Neutral position
    pulseWidth = ESC_MID_PULSE_WIDTH;
  }

  // Send the command to the ESC
  ESC.writeMicroseconds(pulseWidth);

  // Log the command (optional)
  Serial.print("Speed value: ");
  Serial.print(speedValue);
  Serial.print(" | Pulse width: ");
  Serial.println(pulseWidth);

  return pulseWidth;
}

void stopESC()
{
  ESC.writeMicroseconds(ESC_MID_PULSE_WIDTH); // Set to neutral position
}
