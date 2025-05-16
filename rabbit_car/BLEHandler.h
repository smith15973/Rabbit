// BLEHandler.h
#ifndef BLE_HANDLER_H
#define BLE_HANDLER_H

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include "config.h"
#include "ESCHandler.h"
#include <ArduinoJson.h>
#include "Lights.h"

void setupBLE();
void stopESCOnDisconnect();

#endif