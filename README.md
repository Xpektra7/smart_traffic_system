
# Wireless Solar-Powered Smart Traffic System

A smart traffic light controller system built on ESP32 that uses ultrasonic sensors to detect vehicle presence and optimize traffic flow across multiple lanes. The system features wireless connectivity, a web-based dashboard, and optional Firebase integration.

## Features

- **Multi-lane Traffic Control**: Manages up to 3 traffic lanes (A, B, C) with independent green time allocation
- **Vehicle Detection**: Ultrasonic sensors (HC-SR04) detect vehicle presence and count vehicles per lane
- **Smart Timing**: Dynamic green light duration based on traffic density and vehicle speed
- **Web Dashboard**: Real-time traffic status visualization via responsive HTML/CSS interface
- **Firebase Integration**: Optional cloud synchronization of traffic data (configurable)
- **Emergency Mode**: All-red latch for emergency vehicle priority
- **Simulation Mode**: Wokwi simulator support for testing without hardware

## Hardware Components

- **Microcontroller**: ESP32 DevKit C v4
- **Sensors**: 3× HC-SR04 ultrasonic distance sensors
- **Indicators**: 9 LEDs (red, yellow, green for each lane)
- **Connectivity**: WiFi (when SIMULATE=0)

## Project Structure

```
src/
├── main.cpp          # Main traffic controller logic
├── interface.cpp     # Web server and Firebase API
├── Ultrasonic.h      # Sensor data structures
└── interface.h       # Interface declarations

data/
├── index.html        # Web dashboard UI
├── style.css         # Dashboard styling
└── main.js          # Dashboard interactivity

diagram.json         # Wokwi circuit diagram
wokwi.toml          # Wokwi simulator config
```

## Configuration

### Simulation Mode (Default)

Set `SIMULATE = 1` in `src/interface.cpp` to run locally without WiFi/Firebase.

### Real WiFi & Firebase

Set `SIMULATE = 0` and update credentials:

```cpp
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASS "YOUR_WIFI_PASSWORD"
#define FIREBASE_BASE "https://your-project-rtdb.firebaseio.com"
```

## API Endpoints

- `GET /` - Serves dashboard UI
- `GET /api/status` - JSON status of all lanes
- `GET /allred` - Trigger emergency all-red mode

## Lane Status Fields

Each lane publishes:
- `count`: Vehicle count
- `flow`: Traffic flow rate
- `avgSpeed`: Average vehicle speed
- `greenTime`: Current green light duration
- `status`: "red", "yellow", or "green"

## Development

Built with PlatformIO. Extensions recommended: PlatformIO IDE.
