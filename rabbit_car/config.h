// config.h
#ifndef CONFIG_H
#define CONFIG_H

// BLE UUIDs - MUST match the ones in the web app
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// Pin definitions
#define ESC_PIN 25  // GPIO pin connected to ESC signal
#define SERVO_PIN 26  // GPIO pin connected to Servo signal
#define BT_LED_PIN 2  // GPIO pin connected to bt connected indicator

// Direction variables
extern int ESC_VALUE;
extern int SERVO_VALUE;

#endif