#include "ESCHandler.h"

const int ESC_MIN_PULSE_WIDTH = 1000; // Minimum pulse width in microseconds (full reverse)
const int ESC_MID_PULSE_WIDTH = 1500; // Neutral position pulse width in microseconds
const int ESC_MAX_PULSE_WIDTH = 2000; // Maximum pulse width in microseconds (full forward)

// PID control constants
float speedKP = 2.5;                   // Proportional gain - reduced to be less aggressive
float speedKI = 0.55;                // Integral gain - reduced to be less aggressive
float speedKD = 0.1;                 // Derivative gain
float SPEED_MAX_INTEGRAL = 20.0;     // Maximum integral accumulation to prevent windup
float SPEED_MAX_ACCELERATION = 20.0; // Maximum change in speed per update to prevent wheelies

// Variables for PID control
float previousError = 0.0;
float integral = 0.0;

int currentPWM = ESC_MID_PULSE_WIDTH; // Initialize to neutral

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
void setMotorSpeed(float speedValue)
{
    float MIN_VALUE = 1000.0;
    float MID_VALUE = 1500.0;
    float MAX_VALUE = 2000.0;
    // Ensure input is within valid range
    speedValue = constrain((float)speedValue, MIN_VALUE, MAX_VALUE);

    // Calculate the pulse width based on the speed value
    int pulseWidth;

    if (speedValue < MID_VALUE)
    {
        // Reverse speed (map -100-0 to ESC_MIN_PULSE_WIDTH-ESC_MID_PULSE_WIDTH)
        pulseWidth = map(speedValue, MIN_VALUE, MID_VALUE, ESC_MIN_PULSE_WIDTH, ESC_MID_PULSE_WIDTH);
    }
    else if (speedValue > MAX_VALUE)
    {
        // Forward speed (map 0-100 to ESC_MID_PULSE_WIDTH-ESC_MAX_PULSE_WIDTH)
        pulseWidth = map(speedValue, MID_VALUE, MAX_VALUE, ESC_MID_PULSE_WIDTH, ESC_MAX_PULSE_WIDTH);
    }
    else
    {
        // Neutral position
        pulseWidth = ESC_MID_PULSE_WIDTH;
    }

    // Send the command to the ESC
    ESC.writeMicroseconds(speedValue);
    // ESC.writeMicroseconds(pulseWidth);

    // Log the command (optional)
    Serial.print("Speed value: ");
    Serial.print(speedValue);
    Serial.print(" | Pulse width: ");
    Serial.println(pulseWidth);
}

void stopESC()
{
    ESC.writeMicroseconds(ESC_MID_PULSE_WIDTH); // Set to neutral position
}
void brakeESC()
{
    ESC.writeMicroseconds(ESC_MID_PULSE_WIDTH - 150); // Set to neutral position
    delay(2000);
    BRAKE = false;
}

void adjustMotorSpeedPID(float currentSpeed, float targetSpeed)
{
    // Calculate error
    float error = targetSpeed - currentSpeed;

    // Calculate integral component with anti-windup
    integral += error;
    integral = constrain(integral, -SPEED_MAX_INTEGRAL, SPEED_MAX_INTEGRAL);

    // Calculate derivative component
    float derivative = error - previousError;

    // Calculate PID output (this will be in PWM microseconds)
    float adjustment = (speedKP * error) + (speedKI * integral) + (speedKD * derivative);

    // Limit the adjustment rate to prevent sudden changes
    adjustment = constrain(adjustment, -SPEED_MAX_ACCELERATION, SPEED_MAX_ACCELERATION);

    // Update PWM value directly
    currentPWM = constrain(currentPWM + (int)adjustment, ESC_MIN_PULSE_WIDTH, ESC_MAX_PULSE_WIDTH);

    // Apply the new PWM value directly to ESC
    ESC.writeMicroseconds(currentPWM);

    // Save error for next iteration
    previousError = error;

    // Debug output
    // Serial.printf("Target: %.2f, Current: %.2f, Error: %.2f, Adjustment: %.2f, PWM: %d\n",
    //               targetSpeed, currentSpeed, error, adjustment, currentPWM);
}

void resetPID()
{
    previousError = 0.0;
    integral = 0.0;
    currentPWM = ESC_MID_PULSE_WIDTH;  // Reset PWM to neutral
    ESC.writeMicroseconds(currentPWM); // Apply neutral position
    Serial.println("PID state reset");
}