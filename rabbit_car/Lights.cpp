#include "Lights.h"

// Define pin constants (these should be in Lights.h)
// If not already defined in Lights.h, add them there:
// #define HEADLIGHT_PIN 6
// #define LEFT_LIGHT_PIN 7
// #define RIGHT_LIGHT_PIN 8

const int NUM_LEDS_PER_STRIP = 5;

// Initialize NeoPixel strip objects (GRB order, 4 LEDs per strip)
Adafruit_NeoPixel HEAD_LIGHT(NUM_LEDS_PER_STRIP, HEADLIGHT_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel LEFT_LIGHT(NUM_LEDS_PER_STRIP, LEFT_LIGHT_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel RIGHT_LIGHT(NUM_LEDS_PER_STRIP, RIGHT_LIGHT_PIN, NEO_GRB + NEO_KHZ800);

// Array of all light strips
Adafruit_NeoPixel LIGHTS[] = {
  HEAD_LIGHT,
  LEFT_LIGHT,
  RIGHT_LIGHT
};
const int NUM_LIGHTS = sizeof(LIGHTS) / sizeof(LIGHTS[0]);

// Color definitions
struct Colors {
  static uint32_t white(Adafruit_NeoPixel &strip) { return strip.Color(255, 255, 255); }
  static uint32_t red(Adafruit_NeoPixel &strip) { return strip.Color(255, 0, 0); }
  static uint32_t green(Adafruit_NeoPixel &strip) { return strip.Color(0, 255, 0); }
  static uint32_t blue(Adafruit_NeoPixel &strip) { return strip.Color(0, 0, 255); }
  static uint32_t yellow(Adafruit_NeoPixel &strip) { return strip.Color(255, 255, 0); }
  static uint32_t purple(Adafruit_NeoPixel &strip) { return strip.Color(255, 0, 255); }
  static uint32_t cyan(Adafruit_NeoPixel &strip) { return strip.Color(0, 255, 255); }
  static uint32_t off(Adafruit_NeoPixel &strip) { return strip.Color(0, 0, 0); }
};

// Get color from string name
uint32_t getColorFromName(const String &colorName, Adafruit_NeoPixel &strip) {
  if (colorName.equalsIgnoreCase("white")) return Colors::white(strip);
  if (colorName.equalsIgnoreCase("red")) return Colors::red(strip);
  if (colorName.equalsIgnoreCase("green")) return Colors::green(strip);
  if (colorName.equalsIgnoreCase("blue")) return Colors::blue(strip);
  if (colorName.equalsIgnoreCase("yellow")) return Colors::yellow(strip);
  if (colorName.equalsIgnoreCase("purple")) return Colors::purple(strip);
  if (colorName.equalsIgnoreCase("cyan")) return Colors::cyan(strip);
  if (colorName.equalsIgnoreCase("off")) return Colors::off(strip);
  
  // Default to white if unknown color
  return Colors::white(strip);
}

// Get light from string name
Adafruit_NeoPixel* getLightFromName(const String &lightName) {
    if (lightName.equalsIgnoreCase("headlight")) return &HEAD_LIGHT;
    if (lightName.equalsIgnoreCase("leftlight")) return &LEFT_LIGHT;
    if (lightName.equalsIgnoreCase("rightlight")) return &RIGHT_LIGHT;
    
    // Return nullptr if the light name is not recognized
    return nullptr;
}

// Initialize all light strips
void setupLights() {
  for (int i = 0; i < NUM_LIGHTS; i++) {
    LIGHTS[i].begin();
    LIGHTS[i].show(); // Initialize all pixels to 'off'
    LIGHTS[i].setBrightness(100); // Set brightness to 50%
  }
  cycleLights(200);
}

// Turn all lights off
void lightsOff() {
  for (int i = 0; i < NUM_LIGHTS; i++) {
    LIGHTS[i].clear();
    LIGHTS[i].show();
  }
}

// Turn off a specific light strip
void lightOff(Adafruit_NeoPixel &strip) {
  strip.clear();
  strip.show();
}

// Set all lights to a specific color
void lightsOn(const String &color) {
  for (int i = 0; i < NUM_LIGHTS; i++) {
    uint32_t pixelColor = getColorFromName(color, LIGHTS[i]);
    for (int j = 0; j < NUM_LEDS_PER_STRIP; j++) {
      LIGHTS[i].setPixelColor(j, pixelColor);
    }
    LIGHTS[i].show();
  }
}

// Set a specific light strip to a color
void lightOn(Adafruit_NeoPixel &strip, const String &color) {
  uint32_t pixelColor = getColorFromName(color, strip);
  for (int j = 0; j < NUM_LEDS_PER_STRIP; j++) {
    strip.setPixelColor(j, pixelColor);
  }
  strip.show();
}

// Cycle colors on all lights
void cycleLights(int delayMs) {
  // Array of colors to cycle through
  String colors[] = {"white", "red", "green", "blue", "yellow", "purple", "cyan"};
  const int numColors = sizeof(colors) / sizeof(colors[0]);
  
  for (int colorIndex = 0; colorIndex < numColors; colorIndex++) {
    lightsOn(colors[colorIndex]);
    delay(delayMs);
  }
  
  lightsOff();
}