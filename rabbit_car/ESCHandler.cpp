#include "ESCHandler.h"

const int ESC_MIN_PULSE_WIDTH = 1000; // Minimum pulse width in microseconds (full reverse)
const int ESC_MID_PULSE_WIDTH = 1500; // Neutral position pulse width in microseconds
const int ESC_MAX_PULSE_WIDTH = 2000; // Maximum pulse width in microseconds (full forward)

// PID control constants
float speedKP = 1;                   // Proportional gain - reduced to be less aggressive
float speedKI = 0.00;                // Integral gain - reduced to be less aggressive
float speedKD = 0.3;                 // Derivative gain
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
    // Ensure input is within valid range
    speedValue = constrain((float)speedValue, -100, 100);

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
    // Serial.print("Speed value: ");
    // Serial.print(speedValue);
    // Serial.print(" | Pulse width: ");
    // Serial.println(pulseWidth);
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
    Serial.printf("Target: %.2f, Current: %.2f, Error: %.2f, Adjustment: %.2f, PWM: %d\n",
                  targetSpeed, currentSpeed, error, adjustment, currentPWM);
}

void resetPID()
{
    previousError = 0.0;
    integral = 0.0;
    currentPWM = ESC_MID_PULSE_WIDTH;  // Reset PWM to neutral
    ESC.writeMicroseconds(currentPWM); // Apply neutral position
    Serial.println("PID state reset");
}

// Legacy functions - can be removed or kept for compatibility
void increaseMotorSpeed()
{
    MOTOR_SPEED = constrain(MOTOR_SPEED + 0.5, 1, 100);
    setMotorSpeed(MOTOR_SPEED);
}

void decreaseMotorSpeed()
{
    MOTOR_SPEED = constrain(MOTOR_SPEED - 0.5, 1, 100);
    setMotorSpeed(MOTOR_SPEED);
}