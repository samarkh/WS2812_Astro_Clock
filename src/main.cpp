#include <Arduino.h>
#include <FastLED.h>
#include <WiFi.h>
#include <time.h>
#include <HTTPClient.h>
#include <ArduinoJson.h> folder name is AstroWS2812

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
const char* ssid      = "Your_SSID";
const char* password  = "Your_PASSWORD";
const char* ntpServer = "pool.ntp.org";

// Location configuration
const double LATITUDE  = 51.4785810;    // Your latitude
const double LONGITUDE = -0.0012920;    // Your Longitude

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

// Define solstice times Found using https://www.timeanddate.com

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
