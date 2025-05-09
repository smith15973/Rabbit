// config.h
#ifndef CONFIG_H
#define CONFIG_H

// BLE UUIDs - MUST match the ones in the web app
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// Pin definitions
#define ESC_PIN 25  // GPIO pin connected to ESC signal
#define SERVO_PIN 26  // GPIO pin connected to Servo signal
#define HS_PIN 13  // GPIO pin connected to digital hall sensor
#define BT_LED_PIN 2  // GPIO pin connected to bt connected indicator

// #define IR_SDA_PIN 21
// #define IR_SCL_PIN 22
#define IR_KEY_PIN 18
#define IR_RESET_PIN 19



const int WHEEL_DIAMETER = 100; //wheel diameter in mm
const int MAGNETS_COUNT = 1; //the number of magnets spaced evenly on a wheel

// external variables
extern int ESC_VALUE;
extern int SERVO_VALUE;
extern bool manualControl;
extern bool RUNNING;

#endif