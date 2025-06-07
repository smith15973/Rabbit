// ServoHandler.h - Updated header with new function declarations

#ifndef SERVOHANDLER_H
#define SERVOHANDLER_H

#include <Arduino.h>
#include <ESP32Servo.h>
#include <ESP32PWM.h>
#include "config.h"
#include "conversions.h"
#include "IRHandler.h"
#include "BLEHandler.h"

// Function declarations
void setupServo();
int setSteering(float angle);
void centerSteering();

// Enhanced PID steering functions
void steerServoByPID(); // Backward compatibility
void steerServoByPID(float currentSpeed); // Speed-adaptive version
void resetSteeringPID();
void updateSteeringPIDConstants(float kp, float ki, float kd);

// Auto-calibration functions
void printCalibrationStatus();
void autoTunePID();
bool detectOscillation();
void updatePIDGainsForSpeed(float currentSpeed);

// Global variables that need to be accessible
extern float SERVO_ANGLE;
extern Servo steeringServo;





#endif