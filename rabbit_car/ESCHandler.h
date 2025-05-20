// ESCHandler.h
#ifndef ESC_HANDLER_H
#define ESC_HANDLER_H

#include <ESP32Servo.h>
#include "config.h"

void setupESC();
void setMotorSpeed(float speedValue);
void stopESC();
void adjustMotorSpeedPID(float currentSpeed, float targetPace);
void increaseMotorSpeed();
void decreaseMotorSpeed();

#endif