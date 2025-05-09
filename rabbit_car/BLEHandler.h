// BLEHandler.h
#ifndef BLE_HANDLER_H
#define BLE_HANDLER_H

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include "config.h"
#include "ESCHandler.h"

void setupBLE();
void parseDrivingData(String, int*, int*);
void parseManualControl(String, bool*);
void parseRunning(String, bool*);
String parseDataType(String);
void initializeESCOnConnect();
void stopESCOnDisconnect();

#endif