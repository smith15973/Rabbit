// BLEHandler.cpp
#include "BLEHandler.h"

class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    Serial.println("BLE Client Connected");
    digitalWrite(2, HIGH);
  }

  void onDisconnect(BLEServer *pServer)
  {
    Serial.println("BLE Client Disconnected");
    digitalWrite(2, LOW);
    stopESCOnDisconnect();
    RUNNING = false;               // Reset running state
    manualControl = false;         // Reset manual control
    BLEDevice::startAdvertising(); // Restart advertising to allow new connections
  }
};

class MyCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    String rxValue = pCharacteristic->getValue();

    if (rxValue.length() > 0)
    {
      String receivedData = String(rxValue.c_str());
      Serial.println("Received: " + receivedData);

      // Parse JSON
      DynamicJsonDocument doc(1024); // Adjust size as needed
      DeserializationError error = deserializeJson(doc, receivedData);

      if (error)
      {
        Serial.print("JSON parsing failed: ");
        Serial.println(error.c_str());
        return;
      }

      // Process based on type
      const char *dataType = doc["type"];

      if (strcmp(dataType, "movement") == 0)
      {
        if (manualControl)
        {
          SERVO_ANGLE = doc["angle"].as<int>();
          MOTOR_SPEED = doc["motorSpeed"].as<int>();
        }
      }
      else if (strcmp(dataType, "manualControl") == 0)
      {
        manualControl = doc["enabled"];
      }
      else if (strcmp(dataType, "running") == 0)
      {
        RUNNING = doc["running"];

        // You can also extract other fields if needed
        if (doc.containsKey("distance") && !doc["distance"].as<String>().isEmpty())
        {
          targetDistance = doc["distance"].as<float>();
          Serial.print("distance: ");
          Serial.println(targetDistance);
        }
        if (doc.containsKey("time") && !doc["time"].as<String>().isEmpty())
        {
          targetTime = doc["time"].as<float>();
          Serial.print("time: ");
          Serial.println(targetTime);
        }
        if (doc.containsKey("pace") && !doc["pace"].as<String>().isEmpty())
        {
          targetPace = doc["pace"].as<float>();
          MOTOR_SPEED = (int)targetPace;
          Serial.print("pace: ");
          Serial.println(targetPace);
        }
        if (doc.containsKey("isWhiteLine") && !doc["isWhiteLine"].as<String>().isEmpty())
        {
          IS_WHITE_LINE = doc["isWhiteLine"];
          Serial.print("isWhiteLine: ");
          Serial.println(IS_WHITE_LINE);
        }

        // start timer
        if (doc["running"])
        {
          startTime = micros();
          Serial.print(startTime);
          Serial.println(" - START RUN!");
          Serial.print(startTime + (targetTime * 1000000UL));
          Serial.println(" - IS END TIME!");
        }
      }
      else if (strcmp(dataType, "isWhiteLine") == 0)
      {
        IS_WHITE_LINE = doc["enabled"];
        if (IS_WHITE_LINE) {
          lightsOn();
        } else {
          lightsOff();
        }
      }
      else if (strcmp(dataType, "lights") == 0)
      {
        // placeholder
      }

      else
      {
        Serial.println("Unknown data type");
      }
    }
  }
};

void setupBLE()
{
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
          BLECharacteristic::PROPERTY_WRITE);

  // Set callbacks
  pCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06); // Functions that help with iPhone connections
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("BLE service started. Waiting for connections...");
}

void stopESCOnDisconnect()
{
  stopESC();
  MOTOR_SPEED = 0; // Reset MOTOR_SPEED to neutral
  Serial.println("ESC stopped due to BLE disconnection");
}