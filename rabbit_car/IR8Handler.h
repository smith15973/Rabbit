// IR8Handler.h
#ifndef IR8_HANDLER_H
#define IR8_HANDLER_H

#include <Wire.h>
#include <arduino.h>
#include "config.h"

// Function prototypes
void ir8Setup();
int followLine();
void readSensorsI2C();
void calculatePosition();
void steer();
void steerByPID();
void printIR8DebugInfo();
int getLineError();
bool isOnLine();

#endif