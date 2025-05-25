// BLEHandler.h
#ifndef BLE_HANDLER_H
#define BLE_HANDLER_H

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "config.h"
#include "ESCHandler.h"
#include <ArduinoJson.h>
#include "Lights.h"
#include "HSHandler.h"
// #include <arduino.h>

void setupBLE();
void stopESCOnDisconnect();
bool bleBroadcastDTPS(float distance, float time, float pace, float speed, int steeringError = 0);

#endif