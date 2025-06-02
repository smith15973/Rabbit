// irHandler.h
#ifndef IR_HANDLER_H
#define IR_HANDLER_H

#include <Wire.h>
#include <arduino.h>
#include "config.h"

// Function prototypes
void irSetup();
void readIRSensorsI2C();
int getPosition();
int getFilteredPosition();
bool isValidLinePattern();
void resetSteeringPID();
void printIRDebugInfo();
bool isOnLine();

#endif