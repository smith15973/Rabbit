// config.h
#ifndef CONFIG_H
#define CONFIG_H

#include "Conversions.h"
#include <ArduinoJson.h>

// BLE UUIDs - MUST match the ones in the web app
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define DATA_CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a9" // New UUID for data broadcasting

// Pin definitions
#define ESC_PIN 25   // GPIO pin connected to ESC signal
#define SERVO_PIN 26 // GPIO pin connected to Servo signal
#define HS_PIN 5     // GPIO pin connected to digital hall sensor
#define BT_LED_PIN 2 // GPIO pin connected to bt connected indicator

#define HEADLIGHT_PIN 12
#define LEFT_LIGHT_PIN 13
#define RIGHT_LIGHT_PIN 14

#define IR_SDA_PIN 21
#define IR_SCL_PIN 22
#define IR_KEY_PIN 18
#define IR_RESET_PIN 19

const int WHEEL_DIAMETER = 82; // wheel diameter in mm
const int MAGNETS_COUNT = 8;   // the number of magnets spaced evenly on a wheel

// external variables
extern float MOTOR_SPEED;
extern float SERVO_ANGLE;
extern bool manualControl;
extern bool RUNNING;
extern bool startRunTimer;
extern bool IS_WHITE_LINE; // determines whenther the logic follows a white or black line

extern String MODE;
extern bool BRAKE;
extern float targetTime;     // user entered run time in seconds
extern float targetDistance; // user entered target distance in meters
extern float targetSpeed;    // user entered target pace in m/s

extern float speedKP;
extern float speedKI;
extern float speedKD;
extern float SPEED_MAX_INTEGRAL;
extern float SPEED_MAX_ACCELERATION;

extern float steerKP;
extern float steerKI;
extern float steerKD;
extern float STEER_MAX_INTEGRAL;

extern volatile unsigned long startTime;
extern volatile unsigned long currentRunDuration;
extern volatile unsigned long endTime;

extern float totalDistance;
extern float currentSpeed;
extern float averageSpeed;

#endif