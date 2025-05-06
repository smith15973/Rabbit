// ESCHandler.h
#ifndef ESC_HANDLER_H
#define ESC_HANDLER_H

#include <ESP32Servo.h>
#include "config.h"

void setupESC();
int setMotorSpeed(int speedValue);
void stopESC();

#endif