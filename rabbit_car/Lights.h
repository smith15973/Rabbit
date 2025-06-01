// IRHandler.h
#ifndef LIGHTS_H
#define LIGHTS_H

#include <Adafruit_NeoPixel.h>
#include "config.h"

// Get color from string name
uint32_t getColorFromName(const String &colorName, Adafruit_NeoPixel &strip);

Adafruit_NeoPixel *getLightFromName(const String &lightName);

// Initialize all light strips
void setupLights();

// Turn all lights off
void lightsOff();

// Turn off a specific light strip
void lightOff(Adafruit_NeoPixel &strip);

// Set all lights to a specific color
void lightsOn(const String &color = "white");

// Set a specific light strip to a color
void lightOn(Adafruit_NeoPixel &strip, const String &color = "white");

// Cycle colors on all lights
void cycleLights(int delayMs = 500);

#endif