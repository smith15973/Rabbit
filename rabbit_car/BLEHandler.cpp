// BLEHandler.cpp
#include "BLEHandler.h"


class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    Serial.println("BLE Client Connected");
    digitalWrite(2, HIGH);
    // initializeESCOnConnect();
  }

  void onDisconnect(BLEServer* pServer) {
    Serial.println("BLE Client Disconnected");
    digitalWrite(2, LOW);
    stopESCOnDisconnect();
    BLEDevice::startAdvertising(); // Restart advertising to allow new connections
  }
};

class MyCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    String value = pCharacteristic->getValue();
    
    if (value.length() > 0) {
      Serial.println(value);
      parseDrivingData(value, &SERVO_VALUE, &ESC_VALUE);      
    }
  }
};

void setupBLE() {
  Serial.println("Starting BLE...");
  
  // Initialize BLE device
  BLEDevice::init("ESP32 Rabbit");
  
  // Create BLE server
  BLEServer *pServer = BLEDevice::createServer();
  
  // Create BLE service
  BLEService *pService = pServer->createService(SERVICE_UUID);
  
  // Create BLE characteristic
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
  
  // Set callbacks
  pCharacteristic->setCallbacks(new MyCallbacks());
  
  // Start the service
  pService->start();
  
  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // Functions that help with iPhone connections
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  
  Serial.println("BLE service started. Waiting for connections...");
}

void parseDrivingData(String input, int* x, int* y) {
  int xIndex = input.indexOf("X:");
  int yIndex = input.indexOf(",Y:");
  
  if (xIndex != -1 && yIndex != -1) {
    *x = input.substring(xIndex + 2, yIndex).toInt();
    *y = input.substring(yIndex + 3).toInt();
  } else {
    Serial.println("Invalid input format");
  }
}

void initializeESCOnConnect() {
  setupESC();
  Serial.println("ESC initialized due to BLE connection");
}

void stopESCOnDisconnect() {
  stopESC();
  ESC_VALUE = 0; // Reset ESC_VALUE to neutral
  Serial.println("ESC stopped due to BLE disconnection");
}