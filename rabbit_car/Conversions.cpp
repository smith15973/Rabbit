#include <Conversions.h>

// distance conversions
float mm_to_m(float dist)
{
    return dist / 1000.0;
}

float m_to_km(float dist)
{
    return dist / 1000.0;
}

float km_to_m(float dist)
{
    return dist * 1000.0;
}

float m_to_mm(float dist)
{
    return dist * 1000.0;
}

float m_to_in(float dist)
{
    return dist * 39.3701;
}

float in_to_m(float dist)
{
    return dist / 39.3701;
}

float in_to_cm(float dist)
{
    return dist * 2.54;
}

float cm_to_in(float dist)
{
    return dist / 2.54;
}

float m_to_ft(float dist)
{
    return dist * 3.28084;
}

float ft_to_m(float dist)
{
    return dist / 3.28084;
}

float m_to_miles(float dist)
{
    return dist * 0.000621371;
}

float miles_to_m(float dist)
{
    return dist / 0.000621371;
}

float km_to_miles(float dist)
{
    return dist * 0.621371;
}

float miles_to_km(float dist)
{
    return dist / 0.621371;
}

// speed conversions
float mps_to_kmh(float speed)
{
}
float mps_to_miph(float speed)
{
}

float miph_to_kmh(float speed)
{
}

float miph_to_mps(float speed)
{
}

float kmh_to_mps(float speed)
{
}

float kmh_to_miph(float speed)
{
}

// time conversions
float micros_to_s(unsigned long time)
{
    return time / 1000000.0;
}

unsigned long s_to_micros(float time)
{
    return (unsigned long)(time * 1000000);
}

float millis_to_s(unsigned long time)
{
    return time / 1000.0;
}

unsigned long s_to_millis(float time)
{
    return (unsigned long)(time * 1000);
}

float s_to_min(float time)
{
    return time / 60.0;
}

float min_to_s(float time)
{
    return time * 60.0;
}

float s_to_hr(float time)
{
    return time / 3600.0;
}

float hr_to_s(float time)
{
    return time * 3600.0;
}

float min_to_hr(float time)
{
    return time / 60.0;
}

float hr_to_min(float time)
{
    return time * 60.0;
}

String s_to_hr_min_s(float time)
{
    int hours = (int)(time / 3600);
    int minutes = (int)((time - hours * 3600) / 60);
    int seconds = (int)(time - hours * 3600 - minutes * 60);

    String result = "";

    if (hours > 0)
    {
        result += String(hours) + "h ";
    }

    if (minutes > 0 || hours > 0)
    {
        result += String(minutes) + "m ";
    }

    result += String(seconds) + "s";

    return result;
}

// Time string parsing functions
float time_str_to_s(const String &timeStr)
{
    // Parse a string in format "hr:min:sec.ms" or "min:sec.ms" or "sec.ms" to seconds

    float totalSeconds = 0.0;
    int hours = 0, minutes = 0;
    float seconds = 0.0;

    // Count colons to determine format
    int colonCount = 0;
    for (unsigned int i = 0; i < timeStr.length(); i++)
    {
        if (timeStr.charAt(i) == ':')
            colonCount++;
    }

    if (colonCount == 2)
    {
        // Format is "hr:min:sec.ms"
        int firstColon = timeStr.indexOf(':');
        int secondColon = timeStr.indexOf(':', firstColon + 1);

        hours = timeStr.substring(0, firstColon).toInt();
        minutes = timeStr.substring(firstColon + 1, secondColon).toInt();
        seconds = timeStr.substring(secondColon + 1).toFloat();
    }
    else if (colonCount == 1)
    {
        // Format is "min:sec.ms"
        int colon = timeStr.indexOf(':');

        minutes = timeStr.substring(0, colon).toInt();
        seconds = timeStr.substring(colon + 1).toFloat();
    }
    else
    {
        // Format is "sec.ms" or just "sec"
        seconds = timeStr.toFloat();
    }

    totalSeconds = hours * 3600.0 + minutes * 60.0 + seconds;
    return totalSeconds;
}

float hhmmss_to_s(int hours, int minutes, float seconds)
{
    // Convert hours, minutes, seconds (and optionally milliseconds) to total seconds
    return hours * 3600.0 + minutes * 60.0 + seconds;
}

// Overloaded version for integers only (no milliseconds)
float hhmmss_to_s(int hours, int minutes, int seconds)
{
    return hours * 3600.0 + minutes * 60.0 + seconds;
}

// Other time parsing functions
float mmss_to_s(int minutes, float seconds)
{
    return minutes * 60.0 + seconds;
}

// Overloaded version for integers only
float mmss_to_s(int minutes, int seconds)
{
    return minutes * 60.0 + seconds;
}