# LED Sun Position Clock

This project creates a 24-hour clock using an LED strip that displays the current time, daylight hours, and significant solar events throughout the day. It uses real-time sunrise and sunset data for your location, and includes markers for summer and winter solstices.

## Features

- Real-time clock display using LED strip
- Live sunrise and sunset times based on geographical location
- Summer and winter solstice markers
- Hour markers for easy time reference
- Solar noon indication
- Dynamic daylight period visualization
- WiFi connectivity for time synchronization and solar data

## Hardware Requirements

- ESP32 microcontroller (or compatible)
- WS2812B LED strip (332 LEDs)
- Power supply suitable for your LED strip
- WiFi connection

## Dependencies

The following Arduino libraries are required:
- FastLED
- WiFi
- HTTPClient
- ArduinoJson
- Time

## Configuration

Before uploading the code, you need to configure the following parameters in the code:

### LED Configuration
```cpp
#define LED_PIN         48      // Data pin for LED strip
#define NUM_LEDS        332     // Number of LEDs in the strip
#define LED_TYPE        WS2812B // LED strip type
#define COLOR_ORDER     GRB     // LED colour order
#define BRIGHTNESS      50      // LED brightness (0-255)
```

### Network Configuration
```cpp
const char* ssid      = "Your_SSID";     // Your WiFi network name
const char* password  = "Your_PASSWORD";  // Your WiFi password
```

### Location Configuration
```cpp
const double LATITUDE  = 51.4785810;  // Your latitude
const double LONGITUDE = -0.0012920;  // Your longitude
```

## Display Color Coding

The LED strip uses different colors to indicate various elements:
- **Blue**: Daylight period background
- **Red**: Hour markers
- **Green**: Solstice markers (sunrise and sunset times)
- **Yellow**: Current sun position
- **Dark**: Night time period

## How It Works

1. The system connects to WiFi and synchronizes time with an NTP server
2. It fetches daily sunrise/sunset data from the sunrise-sunset.org API
3. The LED strip displays:
   - Current time as a yellow LED
   - Daylight hours as a blue background
   - Hour markers in red
   - Summer and winter solstice times in green

Each LED represents approximately 4.3 minutes of time (24 hours / 332 LEDs).

## API Usage

The system uses the sunrise-sunset.org API to fetch daily solar data. The data is cached and updated once per day to minimize API calls.

## Debug Output

The system outputs debug information via Serial communication at 115200 baud, including:
- Current LED position and time
- Sunrise time and LED position
- Solar noon time and LED position
- Sunset time and LED position

## Installation

1. Install all required libraries using the Arduino Library Manager
2. Configure your WiFi credentials and location coordinates
3. Connect your LED strip to the specified pin
4. Upload the code to your ESP32
5. Power up the system

## Notes

- The LED strip should be positioned so that LED 0 represents midnight
- The system automatically adjusts for daylight saving time
- The brightness can be adjusted by modifying the BRIGHTNESS define
- The system requires a stable internet connection for initial setup and daily updates

## Troubleshooting

If the system isn't working as expected:
1. Check serial output for debugging information
2. Verify WiFi connectivity
3. Confirm correct latitude and longitude values
4. Ensure LED strip is properly powered and connected
5. Verify all required libraries are installed

## Power Considerations

With 332 LEDs, power consumption can be significant. Make sure to:
- Use an adequate power supply
- Calculate power requirements based on maximum brightness
- Consider using a level shifter for the data line if needed
