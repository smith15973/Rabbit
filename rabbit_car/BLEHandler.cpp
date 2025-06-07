// BLEHandler.cpp
#include "BLEHandler.h"

// Global BLE objects
BLEServer *pServer = NULL;
BLECharacteristic *pDataCharacteristic = NULL;
bool deviceConnected = false;

class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    Serial.println("BLE Client Connected");
    digitalWrite(BT_LED_PIN, HIGH);
    deviceConnected = true;
  }

  void onDisconnect(BLEServer *pServer)
  {
    Serial.println("BLE Client Disconnected");
    digitalWrite(BT_LED_PIN, LOW);
    stopESCOnDisconnect();
    RUNNING = false;       // Reset running state
    manualControl = false; // Reset manual control
    deviceConnected = false;
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
          SERVO_ANGLE = doc["angle"].as<float>();
          MOTOR_SPEED = doc["motorSpeed"].as<float>();
        }
      }
      else if (strcmp(dataType, "manualControl") == 0)
      {
        bool previousMode = manualControl;
        manualControl = doc["enabled"];
        if (manualControl != previousMode)
        {
          startRunTimer = true;
        }
      }
      else if (strcmp(dataType, "running") == 0)
      {
        MODE = doc["mode"].as<String>();
        speedKP = doc["speedKP"].as<float>();
        speedKI = doc["speedKI"].as<float>();
        speedKD = doc["speedKD"].as<float>();
        SPEED_MAX_INTEGRAL = doc["SPEED_MAX_INTEGRAL"].as<float>();
        SPEED_MAX_ACCELERATION = doc["SPEED_MAX_ACCELERATION"].as<float>();

        // steerKP = doc["steerKP"].as<float>();
        // steerKI = doc["steerKI"].as<float>();
        // steerKD = doc["steerKD"].as<float>();
        // STEER_MAX_INTEGRAL = doc["STEER_MAX_INTEGRAL"].as<float>();

        if (doc["running"])
        {
          startRunTimer = true;
        }
        else
        {
          BRAKE = true;
        }
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
          targetSpeed = doc["pace"].as<float>();
          Serial.print("pace: ");
          Serial.println(targetSpeed);
        }
        if (doc.containsKey("isWhiteLine") && !doc["isWhiteLine"].as<String>().isEmpty())
        {
          IS_WHITE_LINE = doc["isWhiteLine"];
          Serial.print("isWhiteLine: ");
          Serial.println(IS_WHITE_LINE);
        }
      }
      else if (strcmp(dataType, "isWhiteLine") == 0)
      {
        IS_WHITE_LINE = doc["enabled"];
        if (IS_WHITE_LINE)
        {
          lightsOn();
        }
        else
        {
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

/**
 * Function to broadcast Distance, Time, Pace, and Speed data over BLE
 * @param distance Distance value in meters
 * @param time Time value in seconds
 * @param pace Pace value in meters per second
 * @param speed Speed value in meters per second
 * @return bool True if data was sent successfully, false otherwise
 */
bool bleBroadcastDTPS(float distance, float time, float pace, float speed, float steeringAngle, bool forceBroadcast)
{
  // Check if 100ms has elapsed since the last broadcast
  static unsigned long lastBroadcastTime = 0;
  if (millis() - lastBroadcastTime < 100 && !forceBroadcast)
  {
    return false; // Exit early if it's too soon to broadcast
  }

  if (!deviceConnected)
  {
    // No need to print a message here as it could flood the serial output
    return false;
  }

  // Create a JSON document
  StaticJsonDocument<200> doc;

  // Create nested objects
  JsonObject speedObj = doc["currentSpeed"].to<JsonObject>();
  speedObj["value"] = speed;
  speedObj["units"] = "mps";

  JsonObject distObj = doc["distance"].to<JsonObject>();
  distObj["value"] = distance;
  distObj["units"] = "m";

  JsonObject paceObj = doc["averagePace"].to<JsonObject>();
  paceObj["value"] = pace;
  paceObj["units"] = "mps";

  JsonObject timeObj = doc["time"].to<JsonObject>();
  timeObj["value"] = time;
  timeObj["units"] = "s";

  doc["steeringAngle"] = steeringAngle;

  // Serialize JSON to string
  String jsonString;
  serializeJson(doc, jsonString);

  // Send the value to the app
  pDataCharacteristic->setValue(jsonString.c_str());
  pDataCharacteristic->notify();

  // Update the last broadcast time
  lastBroadcastTime = millis();

  // Only print every 500ms to avoid flooding the serial console
  // Serial.print("Sent data: ");
  // Serial.println(jsonString);

  return true;
}

bool bleBroadcastPID(float kp, float ki, float kd, int speedBand, float currentSpeed, bool forceBroadcast)
{
  // Check if values actually changed or force broadcast
  static float lastKP = -1, lastKI = -1, lastKD = -1;
  static int lastSpeedBand = -1;

  if (!forceBroadcast && kp == lastKP && ki == lastKI && kd == lastKD && speedBand == lastSpeedBand)
  {
    return false; // No change, don't broadcast
  }

  if (!deviceConnected)
  {
    return false;
  }

  // Create a JSON document for PID data
  StaticJsonDocument<250> doc;

  JsonObject pidObj = doc["pidValues"].to<JsonObject>();
  pidObj["kp"] = kp;
  pidObj["ki"] = ki;
  pidObj["kd"] = kd;

  JsonObject speedObj = doc["currentSpeed"].to<JsonObject>();
  speedObj["value"] = currentSpeed;
  speedObj["units"] = "mps";

  doc["speedBand"] = speedBand;
  doc["timestamp"] = millis();
  doc["type"] = "pid_update";

  // Serialize JSON to string
  String jsonString;
  serializeJson(doc, jsonString);

  // Send the value to the app (you may need a separate characteristic for PID data)
  pDataCharacteristic->setValue(jsonString.c_str());
  pDataCharacteristic->notify();

  // Update last values
  lastKP = kp;
  lastKI = ki;
  lastKD = kd;
  lastSpeedBand = speedBand;

  Serial.print("PID Update Sent: ");
  Serial.println(jsonString);

  return true;
}

bool bleBroadcastRunStopped(const JsonDocument &doc)
{
  if (!deviceConnected)
  {
    return false;
  }
  // Serialize JSON to string
  String jsonString;
  serializeJson(doc, jsonString);

  // Send the value to the app
  pDataCharacteristic->setValue(jsonString.c_str());
  pDataCharacteristic->notify();

  return true;
}

void setupBLE()
{
  Serial.println("Starting BLE...");
  pinMode(BT_LED_PIN, OUTPUT);
  // Initialize BLE device
  BLEDevice::init("ESP32 Rabbit");

  // Create BLE server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create BLE service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create BLE characteristic for commands
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE);

  // Set callbacks for command characteristic
  pCharacteristic->setCallbacks(new MyCallbacks());

  // Create BLE characteristic for data broadcasting (DTPS data)
  pDataCharacteristic = pService->createCharacteristic(
      DATA_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_NOTIFY);

  // Add descriptor for client configuration (required for notifications)
  pDataCharacteristic->addDescriptor(new BLE2902());

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
  MOTOR_SPEED = 1500; // Reset MOTOR_SPEED to neutral
  Serial.println("ESC stopped due to BLE disconnection");
}