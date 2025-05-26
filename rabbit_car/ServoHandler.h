// ServoHandler.h
#ifndef SERVO_HANDLER_H
#define SERVO_HANDLER_H

#include <ESP32Servo.h>
#include "IR8Handler.h"
#include "config.h"

void setupServo();

int setSteering(float angle);

void centerSteering();

void steerServoByPID();

// Add this function to reset PID when starting a new run
void resetSteeringPID();

// Function to tune PID parameters during runtime
void updateSteeringPIDConstants(float kp, float ki, float kd);

#endif