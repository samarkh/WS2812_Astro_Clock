// Created: 2021-04-25 16:00:00
/*

This Arduino code creates a 24-hour clock visualization using an LED strip (332 LEDs). It:

1. Fetches sunrise, sunset, and solar noon times daily from the sunrise-sunset.org API
2. Maps the 24-hour day across the LED strip
3. Shows current time as a yellow LED
4. Displays daylight hours with very dim blue LEDs
5. Keeps solar noon LED off
6. Hour markers (dim red LEDs)
7. Calculates LED positions using seconds-per-LED ratio (24hrs/332 LEDs)
8. Includes serial debugging output for time and LED positions

Key features:
- Updates sun data once per day
- Uses real-world astronomical data
- Safety checks for LED array bounds
- UTC/local time handling

*/
/*
24-Hour LED Clock with Daylight Tracking
This code drives a 332 LED strip to create a 24-hour clock showing current time,
daylight period, hour markers, and solar events.
*/

/*

1. Added clear section headers with comment separators
2. Grouped related constants and configurations together
3. Added BRIGHTNESS as a define for easier adjustment
4. Improved spacing and alignment for readability
5. Added more descriptive comments for each section
6. Split long lines into multiple lines where appropriate
7. Organized code into logical sections:
   - Configuration Constants
   - Solstice Time Definitions
   - Data Structures
   - API Functions
   - Setup & Loop
8. Added color descriptions in comments
9. Improved formatting of debug output

Would you like me to make any additional organization changes or adjust any of the sections?
*/

#include <Arduino.h>
#include <FastLED.h>
#include <WiFi.h>
#include <time.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

//-----------------------------------------------------------------------------
// Configuration Constants
//-----------------------------------------------------------------------------
// LED configuration
#define LED_PIN         48      // Data pin for LED strip
#define NUM_LEDS        332     // Number of LEDs in the strip
#define LED_TYPE        WS2812B // LED strip type
#define COLOR_ORDER     GRB     // LED colour order
#define BRIGHTNESS      50      // LED brightness (0-255)

// Network configuration
const char* ssid      = "TP-LINK_2B90";
const char* password  = "35702504";
const char* ntpServer = "pool.ntp.org";

// Location configuration
const double LATITUDE  = 51.85090238;   // Latitude for Gloucester, UK
const double LONGITUDE = -2.2010652;    // Longitude for Gloucester, UK

// Time calculations
static const int SECONDS_PER_LED = (24*60*60) / NUM_LEDS;   // Seconds per LED

// LED array
CRGB leds[NUM_LEDS];

//-----------------------------------------------------------------------------
// Solstice Time Definitions
//-----------------------------------------------------------------------------
// Time conversion utility function
int convertTimeToMinutes(const char* timeStr) 
{
    int hours, minutes;
    sscanf(timeStr, "%d:%d", &hours, &minutes);
    return (hours * 60) + minutes;
}

// Define solstice times
const int winterSolsticeSunrise = convertTimeToMinutes("08:47");    // Winter solstice sunrise
const int winterSolsticeSunset  = convertTimeToMinutes("16:02");    // Winter solstice sunset
const int summerSolsticeSunrise = convertTimeToMinutes("03:47");    // Summer solstice sunrise
const int summerSolsticeSunset  = convertTimeToMinutes("20:34");    // Summer solstice sunset

// Calculate LED positions for solstices
const int winterSolsticeSunriseLED = (winterSolsticeSunrise * 60) / SECONDS_PER_LED;    // Winter solstice sunrise LED
const int winterSolsticeSunsetLED  = (winterSolsticeSunset  * 60) / SECONDS_PER_LED;    // Winter solstice sunset LED
const int summerSolsticeSunriseLED = (summerSolsticeSunrise * 60) / SECONDS_PER_LED;    // Summer solstice sunrise LED
const int summerSolsticeSunsetLED  = (summerSolsticeSunset  * 60) / SECONDS_PER_LED;    // Summer solstice sunset LED

//-----------------------------------------------------------------------------
// Data Structures
//-----------------------------------------------------------------------------
struct SunData 
{
    int sunriseMinutes;
    int sunsetMinutes;
    int solarNoonMinutes;
    int daySeconds;
    unsigned long lastUpdate;
};

//-----------------------------------------------------------------------------
// API Functions
//-----------------------------------------------------------------------------
SunData getSunData() 
{
    HTTPClient http;
    SunData data = {0};
    String url = "https://api.sunrise-sunset.org/json?lat=" + 
                 String(LATITUDE, 6) + "&lng=" + 
                 String(LONGITUDE, 6) + "&formatted=0";
    http.begin(url);
    
    if (http.GET() > 0) 
    {
        JsonDocument doc;
        if (!deserializeJson(doc, http.getString())) 
        {
            struct tm sunrise = {0}, sunset = {0}, solarNoon = {0};
            strptime(doc["results"]["sunrise"], "%Y-%m-%dT%H:%M:%S+00:00", &sunrise);
            strptime(doc["results"]["sunset"], "%Y-%m-%dT%H:%M:%S+00:00", &sunset);
            strptime(doc["results"]["solar_noon"], "%Y-%m-%dT%H:%M:%S+00:00", &solarNoon);
            
            data.sunriseMinutes = sunrise.tm_hour * 60 + sunrise.tm_min;
            data.sunsetMinutes = sunset.tm_hour * 60 + sunset.tm_min;
            data.solarNoonMinutes = solarNoon.tm_hour * 60 + solarNoon.tm_min;
            data.daySeconds = doc["results"]["day_length"];
            data.lastUpdate = millis();
        }
    }
    http.end();
    return data;
}

//-----------------------------------------------------------------------------
// Setup & Loop
//-----------------------------------------------------------------------------
void setup() 
{
    // Initialize serial communication
    Serial.begin(115200);
    
    // Initialize LED strip
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(BRIGHTNESS);
    
    // Initialize WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) 
    {
        delay(500);
        Serial.print(".");
    }
    
    // Initialize time
    configTime(0, 0, ntpServer);

    // Print initial solstice times for debugging
    Serial.println("\nSolstice times in minutes:");
    Serial.printf("Winter Solstice - Sunrise: %d minutes (%02d:%02d), Sunset: %d minutes (%02d:%02d)\n", 
                 winterSolsticeSunrise, winterSolsticeSunrise/60, winterSolsticeSunrise%60,
                 winterSolsticeSunset, winterSolsticeSunset/60, winterSolsticeSunset%60);
    Serial.printf("Summer Solstice - Sunrise: %d minutes (%02d:%02d), Sunset: %d minutes (%02d:%02d)\n", 
                 summerSolsticeSunrise, summerSolsticeSunrise/60, summerSolsticeSunrise%60,
                 summerSolsticeSunset, summerSolsticeSunset/60, summerSolsticeSunset%60);
}

void loop() 
{
    static SunData sun;
    
    // Update sun data daily
    if (millis() - sun.lastUpdate > 24*60*60*1000 || sun.lastUpdate == 0) 
    {
        sun = getSunData();
    }

    // Get current time
    time_t now;
    time(&now);
    struct tm* local_time = localtime(&now);
    int currentSecond = (local_time->tm_hour * 60 + local_time->tm_min) * 60 + local_time->tm_sec;
    
    // Calculate LED positions
    int ledPosition = currentSecond / SECONDS_PER_LED;
    int sunriseLED = (sun.sunriseMinutes * 60) / SECONDS_PER_LED;
    int sunsetLED = (sun.sunsetMinutes * 60) / SECONDS_PER_LED;
    int solarNoonLED = (sun.solarNoonMinutes * 60) / SECONDS_PER_LED;

    // Clear all LEDs
    FastLED.clear();
    
    // Current daylight period (blue background)
    for (int i = sunriseLED; i <= sunsetLED; i++) 
    {
        if (i != solarNoonLED && leds[i].r == 0) 
        {
            leds[i] = CRGB(0, 0, 8);    // Dark blue for daylight period
        }
    }

    // Hour markers (red)
    for(int hour = 0; hour < 24; hour++) 
    {
        int hourLED = (hour * 3600) / SECONDS_PER_LED;
        if(hourLED >= 0 && hourLED < NUM_LEDS && hourLED != solarNoonLED) 
        {
            leds[hourLED] = CRGB(32, 0, 0);  // Dark red for hours
        }
    }

    // Solstice markers (green)
    if (winterSolsticeSunriseLED >= 0 && winterSolsticeSunriseLED < NUM_LEDS) 
    {
        leds[winterSolsticeSunriseLED] = CRGB(0, 255, 0);  // Bright green
    }
    if (winterSolsticeSunsetLED >= 0 && winterSolsticeSunsetLED < NUM_LEDS) 
    {
        leds[winterSolsticeSunsetLED] = CRGB(0, 255, 0);
    }
    if (summerSolsticeSunriseLED >= 0 && summerSolsticeSunriseLED < NUM_LEDS) 
    {
        leds[summerSolsticeSunriseLED] = CRGB(0, 255, 0);
    }
    if (summerSolsticeSunsetLED >= 0 && summerSolsticeSunsetLED < NUM_LEDS) 
    {
        leds[summerSolsticeSunsetLED] = CRGB(0, 255, 0);
    }

    // Current sun position (yellow)
    if (ledPosition >= 0 && ledPosition < NUM_LEDS && 
        ledPosition != solarNoonLED && 
        ledPosition >= sunriseLED && 
        ledPosition <= sunsetLED) 
    {
        leds[ledPosition] = CRGB(255, 255, 0);  // Bright yellow
    }
    
    // Debug output
    Serial.printf("Current LED: %d (Hour: %d, Minute: %d)\n", 
                 ledPosition, local_time->tm_hour, local_time->tm_min);
    Serial.printf("Sunrise: %02d:%02d (LED: %d)\n", 
                 sun.sunriseMinutes/60, sun.sunriseMinutes%60, sunriseLED);
    Serial.printf("Solar Noon: %02d:%02d (LED: %d)\n", 
                 sun.solarNoonMinutes/60, sun.solarNoonMinutes%60, solarNoonLED);
    Serial.printf("Sunset: %02d:%02d (LED: %d)\n", 
                 sun.sunsetMinutes/60, sun.sunsetMinutes%60, sunsetLED);
    
    // Update LED strip
    FastLED.show();
    delay(1000);
}

/*
Here's a breakdown of the code's key components:

CONSTANTS & SETUP:
- 332 LED strip on pin 48 using WS2812B protocol
- WiFi and geolocation settings for Gloucester, UK
- NTP time server with no timezone offset

DATA STRUCTURES:
- `SunData`: Stores daily sun event times in minutes and last update timestamp
- `getSunData()`: Fetches sunrise/sunset/solar noon from API, converts UTC to local time

MAIN LOOP CALCULATIONS:
- Seconds per LED = 86400 (24hrs) / 332 LEDs
- Current time mapped to LED position: currentSecond / SECONDS_PER_LED
- Sun events mapped similarly using their minute values

LED CONTROL:
- Daylight period (sunrise to sunset): Dim blue (0,0,8)
- Current time: Yellow (255,255,0)
- Solar noon: LED remains off
- Updates every second with bounds checking

DEBUGGING:
Serial output shows:
- Current time and LED position
- Sun event times and corresponding LEDs
- Loop cycle marker

The system creates a 24-hour clock visualization where one LED represents about 4.3 minutes of time.

*/