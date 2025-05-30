#ifndef CONVERSIONS_HANDLER_H
#define CONVERSIONS_HANDLER_H

#include <arduino.h>

// distance conversions
float mm_to_m(float dist);

float m_to_km(float dist);

float km_to_m(float dist);

float m_to_mm(float dist);

float m_to_in(float dist);

float in_to_m(float dist);

float in_to_cm(float dist);

float cm_to_in(float dist);

float m_to_ft(float dist);

float ft_to_m(float dist);

float m_to_miles(float dist);

float miles_to_m(float dist);

float km_to_miles(float dist);

float miles_to_km(float dist);

// speed conversions
float mps_to_kmh(float speed);

float mps_to_miph(float speed);

float miph_to_kmh(float speed);

float miph_to_mps(float speed);

float kmh_to_mps(float speed);

float kmh_to_miph(float speed);

// time conversions
float micros_to_s(unsigned long time);

unsigned long s_to_micros(float time);

float millis_to_s(unsigned long time);

unsigned long s_to_millis(float time);

float s_to_min(float time);

float min_to_s(float time);

float s_to_hr(float time);

float hr_to_s(float time);

float min_to_hr(float time);

float hr_to_min(float time);

String s_to_hr_min_s(float time);

// Time string parsing functions
float time_str_to_s(const String &timeStr);

float hhmmss_to_s(int hours, int minutes, float seconds);

// Overloaded version for integers only (no milliseconds)
float hhmmss_to_s(int hours, int minutes, int seconds);

// Other time parsing functions
float mmss_to_s(int minutes, float seconds);
// Overloaded version for integers only

float mmss_to_s(int minutes, int seconds);

float mapFloat(float value, float fromLow, float fromHigh, float toLow, float toHigh);
#endif