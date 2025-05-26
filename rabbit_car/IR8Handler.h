// IR8Handler.h
#ifndef IR8_HANDLER_H
#define IR8_HANDLER_H

#include <Wire.h>
#include <arduino.h>
#include "config.h"

// Function prototypes
void ir8Setup();
void readSensorsI2C();
int getPosition();
void resetSteeringPID();
void printIR8DebugInfo();
bool isOnLine();

#endif