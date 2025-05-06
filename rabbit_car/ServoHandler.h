// ServoHandler.h
#ifndef SERVO_HANDLER_H
#define SERVO_HANDLER_H

#include <ESP32Servo.h>
#include "config.h"

/**
 * Initializes the steering servo.
 */
void setupServo();

/**
 * Sets the steering angle based on an input value.
 * @param steerValue Steering value from -100 (full left) to +100 (full right), 0 is center.
 * @return The actual pulse width sent to the servo in microseconds.
 */
int setSteering(int steerValue);

/**
 * Centers the steering servo (sets to neutral position).
 */
void centerSteering();

#endif