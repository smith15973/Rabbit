// ServoHandler.cpp - Enhanced with Adaptive PID and Auto-Calibration
#include "ServoHandler.h"

const int SERVO_MIN_PULSE_WIDTH = 1250;
const int SERVO_MID_PULSE_WIDTH = 1500;
const int SERVO_MAX_PULSE_WIDTH = 1750;
const float SERVO_MIN_ANGLE = 45;
const float SERVO_MID_ANGLE = 90;
const float SERVO_MAX_ANGLE = 135;
const float VALID_TURNING_RANGE = 7;
const float SERVO_LEFT_OFFSET = 3.5; // Compensation for servo mechanical offset

// Adaptive PID Parameters - these will be automatically adjusted
struct PIDGains
{
  float kp;
  float ki;
  float kd;
};

// Base PID gains for different speed ranges (corrected starting points)
PIDGains basePIDGains[] = {
    {1.0, 1.0, 0.25},  // 0-2 m/s - Your actual working values
    {1.0, 1.0, 0.25},  // 2-5 m/s - Start with same working values
    {0.7, 0.7, 0.18},  // 5-8 m/s - Scaled down proportionally
    {0.5, 0.5, 0.12},  // 8-12 m/s - More conservative
    {0.35, 0.35, 0.08} // 12+ m/s - Highly damped
};

// Current adaptive gains (will be modified during runtime)
float steerKP = 1.0; // Your actual working values
float steerKI = 1.0;
float steerKD = 0.25;

// Enhanced PID Variables
int steeringSetPoint = 7500;
int steeringError = 0;
int previousSteeringError = 0;
float steeringIntegral = 0;
unsigned long lastSteeringPIDTime = 0;

// Auto-calibration variables
struct CalibrationData
{
  float oscillationAmplitude;
  float oscillationFrequency;
  unsigned long lastOscillationTime;
  int oscillationCount;
  float maxError;
  float avgError;
  bool isOscillating;
  float currentSpeed;
  int speedBand;
};

CalibrationData calibData = {0, 0, 0, 0, 0, 0, false, 0, 0};

// Oscillation detection
float errorHistory[10] = {0}; // Rolling window of errors
int errorHistoryIndex = 0;
unsigned long lastTuneTime = 0;
const unsigned long TUNE_INTERVAL = 5000000; // 5 seconds in microseconds

// Moving average filter for smoother control
float steeringOutputHistory[5] = {0};
int outputHistoryIndex = 0;

Servo steeringServo;

void setupServo()
{
  pinMode(SERVO_PIN, OUTPUT);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  steeringServo.attach(SERVO_PIN, SERVO_MIN_PULSE_WIDTH, SERVO_MAX_PULSE_WIDTH);
  centerSteering();
  delay(500);

  Serial.println("Adaptive Steering servo initialized with auto-calibration.");
}

// Speed-adaptive PID gain selection
void updatePIDGainsForSpeed(float currentSpeed)
{
  int newSpeedBand = 0;

  if (currentSpeed < 2.0)
    newSpeedBand = 0;
  else if (currentSpeed < 5.0)
    newSpeedBand = 1;
  else if (currentSpeed < 8.0)
    newSpeedBand = 2;
  else if (currentSpeed < 12.0)
    newSpeedBand = 3;
  else
    newSpeedBand = 4;

  // Only update if speed band changed significantly
  if (newSpeedBand != calibData.speedBand)
  {
    calibData.speedBand = newSpeedBand;
    steerKP = basePIDGains[newSpeedBand].kp;
    steerKI = basePIDGains[newSpeedBand].ki;
    steerKD = basePIDGains[newSpeedBand].kd;

    // Reset integral to prevent sudden jumps
    steeringIntegral = 0;

    Serial.printf("Speed: %.1f m/s - Switched to PID band %d: KP=%.3f, KI=%.4f, KD=%.3f\n",
                  currentSpeed, newSpeedBand, steerKP, steerKI, steerKD);
  }

  // Broadcast PID changes to app when speed band changes
  bleBroadcastPID(steerKP, steerKI, steerKD, calibData.speedBand, calibData.currentSpeed, true);
}

// Detect oscillation patterns
bool detectOscillation()
{
  // Check if we have enough history
  if (errorHistoryIndex < 8)
    return false;

  // Look for sign changes in recent errors (oscillation indicator)
  int signChanges = 0;
  float maxRecentError = 0;

  for (int i = 1; i < 8; i++)
  {
    int currentIndex = (errorHistoryIndex - i + 10) % 10;
    int prevIndex = (errorHistoryIndex - i - 1 + 10) % 10;

    if ((errorHistory[currentIndex] > 0) != (errorHistory[prevIndex] > 0))
    {
      signChanges++;
    }
    maxRecentError = max(maxRecentError, abs(errorHistory[currentIndex]));
  }

  // Oscillation detected if we have many sign changes and significant error
  bool oscillating = (signChanges >= 4 && maxRecentError > 800);

  if (oscillating && !calibData.isOscillating)
  {
    Serial.printf("OSCILLATION DETECTED! Sign changes: %d, Max error: %.0f\n",
                  signChanges, maxRecentError);
  }

  calibData.isOscillating = oscillating;
  calibData.oscillationAmplitude = maxRecentError;

  return oscillating;
}

// Auto-tune PID parameters based on performance
void autoTunePID()
{
  unsigned long currentTime = micros();

  // Only tune every few seconds to allow system to settle
  if (currentTime - lastTuneTime < TUNE_INTERVAL)
    return;

  bool isOscillating = detectOscillation();

  if (isOscillating)
  {
    // Reduce gains to dampen oscillation
    float reduction = 0.85; // Reduce by 15%

    // Prioritize reducing derivative and proportional gains
    steerKD *= reduction;
    steerKP *= reduction;
    steerKI *= 0.9; // Slightly reduce integral

    // Update the base gains for this speed band
    basePIDGains[calibData.speedBand].kp = steerKP;
    basePIDGains[calibData.speedBand].ki = steerKI;
    basePIDGains[calibData.speedBand].kd = steerKD;

    Serial.printf("AUTO-TUNE: Reduced gains - KP=%.3f, KI=%.4f, KD=%.3f\n",
                  steerKP, steerKI, steerKD);

    // Broadcast PID changes to app
    bleBroadcastPID(steerKP, steerKI, steerKD, calibData.speedBand, calibData.currentSpeed, true);

    calibData.oscillationCount++;
  }
  else
  {
    // System is stable - could potentially increase responsiveness slightly
    float avgRecentError = 0;
    for (int i = 0; i < 8; i++)
    {
      avgRecentError += abs(errorHistory[(errorHistoryIndex - i + 10) % 10]);
    }
    avgRecentError /= 8;

    // If error is consistently low and we're not oscillating, slightly increase gains
    if (avgRecentError < 300 && calibData.oscillationCount == 0)
    {
      steerKP *= 1.02; // Very small increase
      steerKD *= 1.01;

      // Cap maximum gains for safety (adjusted for your scale)
      steerKP = min(steerKP, 1.5f); // Reasonable maximum for your scale
      steerKD = min(steerKD, 0.4f);

      Serial.printf("AUTO-TUNE: Small gain increase - KP=%.3f, KD=%.3f\n", steerKP, steerKD);

      // Broadcast PID changes to app when speed band changes
      bleBroadcastPID(steerKP, steerKI, steerKD, calibData.speedBand, calibData.currentSpeed, true);
    }
  }

  lastTuneTime = currentTime;
  calibData.oscillationCount = 0; // Reset counter
}

// Enhanced steering control with filtering and adaptation
void steerServoByPID(float currentSpeed)
{
  readIRSensorsI2C();
  int position = getPosition();

  // Update PID gains based on current speed
  updatePIDGainsForSpeed(currentSpeed);
  calibData.currentSpeed = currentSpeed;

  unsigned long currentTime = micros();
  float deltaTime = micros_to_s(currentTime - lastSteeringPIDTime);
  lastSteeringPIDTime = currentTime;

  // Calculate error with deadband for stability
  steeringError = position - steeringSetPoint;
  int deadband = (currentSpeed > 5.0) ? 300 : 200; // Larger deadband at high speed
  if (abs(steeringError) <= deadband)
  {
    steeringError = 0;
  }

  // Store error in history for oscillation detection
  errorHistory[errorHistoryIndex] = steeringError;
  errorHistoryIndex = (errorHistoryIndex + 1) % 10;

  // PID calculations with speed-dependent modifications
  float proportional = steerKP * steeringError;

  // Integral with simple windup protection (removed complex speed-dependent limits)
  steeringIntegral += steeringError * deltaTime;
  // Simple integral windup protection
  float maxIntegral = 3000; // Simplified constant limit
  steeringIntegral = constrain(steeringIntegral, -maxIntegral, maxIntegral);
  float integral = steerKI * steeringIntegral;

  // Derivative with additional filtering at high speeds
  float derivative = 0;
  if (deltaTime > 0)
  {
    float rawDerivative = steerKD * (steeringError - previousSteeringError) / deltaTime;

    // Apply low-pass filter to derivative at high speeds
    if (currentSpeed > 6.0)
    {
      static float filteredDerivative = 0;
      float alpha = 0.3; // Filter strength
      filteredDerivative = alpha * rawDerivative + (1 - alpha) * filteredDerivative;
      derivative = filteredDerivative;
    }
    else
    {
      derivative = rawDerivative;
    }
  }

  // Calculate PID output
  float pidOutput = proportional + integral + derivative;

  // Apply output smoothing for high-speed stability
  steeringOutputHistory[outputHistoryIndex] = pidOutput;
  outputHistoryIndex = (outputHistoryIndex + 1) % 5;

  if (currentSpeed > 8.0)
  {
    // Use moving average at very high speeds
    float smoothedOutput = 0;
    for (int i = 0; i < 5; i++)
    {
      smoothedOutput += steeringOutputHistory[i];
    }
    pidOutput = smoothedOutput / 5.0;
  }

  // Convert to steering angle with speed-dependent limits
  float maxTurnRange = VALID_TURNING_RANGE;
  if (currentSpeed > 10.0)
  {
    maxTurnRange *= 0.7; // Reduce turning range at very high speeds
  }
  else if (currentSpeed > 6.0)
  {
    maxTurnRange *= 0.85;
  }

  float steeringAngle = mapFloat(pidOutput, -3500, 3500,
                                 SERVO_MID_ANGLE - maxTurnRange,
                                 SERVO_MID_ANGLE + maxTurnRange);
  steeringAngle = constrain(steeringAngle,
                            SERVO_MID_ANGLE - maxTurnRange,
                            SERVO_MID_ANGLE + maxTurnRange);

  // Apply servo offset compensation
  if (steeringAngle < SERVO_MID_ANGLE)
  {
    // Left turn - add offset to compensate
    steeringAngle += SERVO_LEFT_OFFSET;
  }

  // Apply steering
  SERVO_ANGLE = steeringAngle;
  setSteering(steeringAngle);

  previousSteeringError = steeringError;

  // Perform auto-tuning
  autoTunePID();

  // Debug output (can be disabled for performance)
  if (currentTime % 100000 == 0)
  { // Print every 0.1 seconds
    Serial.printf("Speed: %.1f | Error: %d | PID: %.1f | Angle: %.1f | Band: %d\n",
                  currentSpeed, steeringError, pidOutput, steeringAngle, calibData.speedBand);
  }
}

// Wrapper for backward compatibility
void steerServoByPID()
{
  steerServoByPID(0.0); // Default to low-speed parameters
}

int setSteering(float angle)
{
  angle = constrain(angle, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE);
  float pulseWidthFloat = mapFloat(angle, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE,
                                   SERVO_MIN_PULSE_WIDTH, SERVO_MAX_PULSE_WIDTH);
  int pulseWidth = (int)(pulseWidthFloat + 0.5);
  steeringServo.writeMicroseconds(pulseWidth);
  return pulseWidth;
}

void centerSteering()
{
  steeringServo.writeMicroseconds(SERVO_MID_PULSE_WIDTH);
}

void resetSteeringPID()
{
  steeringIntegral = 0;
  previousSteeringError = 0;
  lastSteeringPIDTime = micros();

  // Reset calibration data
  memset(errorHistory, 0, sizeof(errorHistory));
  memset(steeringOutputHistory, 0, sizeof(steeringOutputHistory));
  errorHistoryIndex = 0;
  outputHistoryIndex = 0;
  calibData.oscillationCount = 0;
  calibData.isOscillating = false;

  Serial.println("PID and calibration data reset for new run.");
}

// Manual PID tuning (keeps your existing interface)
void updateSteeringPIDConstants(float kp, float ki, float kd)
{
  steerKP = kp;
  steerKI = ki;
  steerKD = kd;

  // Update the current speed band's base values
  if (calibData.speedBand >= 0 && calibData.speedBand < 5)
  {
    basePIDGains[calibData.speedBand].kp = kp;
    basePIDGains[calibData.speedBand].ki = ki;
    basePIDGains[calibData.speedBand].kd = kd;
  }
}

// Get current calibration status
void printCalibrationStatus()
{
  Serial.println("=== PID CALIBRATION STATUS ===");
  for (int i = 0; i < 5; i++)
  {
    Serial.printf("Band %d (%.0f-%.0f m/s): KP=%.3f, KI=%.4f, KD=%.3f\n",
                  i, i * 2.5, (i + 1) * 2.5,
                  basePIDGains[i].kp, basePIDGains[i].ki, basePIDGains[i].kd);
  }
  Serial.printf("Current Speed: %.1f m/s (Band %d)\n",
                calibData.currentSpeed, calibData.speedBand);
  Serial.printf("Oscillating: %s | Oscillation Count: %d\n",
                calibData.isOscillating ? "YES" : "NO", calibData.oscillationCount);
  Serial.println("=============================");
}