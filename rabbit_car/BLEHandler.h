// BLEHandler.h
#ifndef BLE_HANDLER_H
#define BLE_HANDLER_H

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include "config.h"
#include "ESCHandler.h"

void setupBLE();
void parseDrivingData(String input, int* x, int* y);
void initializeESCOnConnect();
void stopESCOnDisconnect();

#endif