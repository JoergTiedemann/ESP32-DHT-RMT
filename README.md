# ESP32-DHT-RMT
Non blocking DHT22 Library for ESP32 using RMT Module


This library works with DHT22 / AM2302 modules like these:

![ModuleDHT22](https://github.com/user-attachments/assets/4d5a24fa-a26c-43e7-865a-bec4b7b953e4)


## Features

- ✅ Uses ESP32 **hardware RMT peripheral** — immune to WiFi/BLE interrupt timing issues
- ✅ Works on **ESP32, ESP32-S2, ESP32-S3, ESP32-C3**
- ✅ No external dependencies beyond ESP-IDF (included in Arduino ESP32 core)

## Installation

### Arduino IDE

1. Download this repository as ZIP
2. Arduino IDE → Sketch → Include Library → Add .ZIP Library
3. Select the downloaded ZIP

### PlatformIO

```ini
lib_deps = 
    https://github.com/joergTiedemann/ESP32-DHT-RMT.git
```

## Wiring

```
DHT22 Pin 1 (VCC)  → 3.3V
DHT22 Pin 2 (DATA) → GPIO14 (or any free GPIO)
DHT22 Pin 4 (GND)  → GND

Optional but recommended: 4.7kΩ resistor between DATA and 3.3V
```

> **Note:** The ESP32 internal pull-up (~45kΩ) is often too weak for reliable DHT22 communication, especially at low temperatures. An external 4.7kΩ pull-up resistor significantly improves stability.

## Quick Start

```cpp
#include <ESP32-DHT22-RMT.h>

DHT22Clone dht(14);  // GPIO14

void setup() {
  Serial.begin(115200);
}

void loop() {
  DHT22Clone_Result result = dht.read();

  if (result.error == DHT22_OK) {
    Serial.printf("Temp: %.1f°C  Hum: %.1f%%\n", 
      result.temperature, result.humidity);
  } else {
    Serial.printf("Error: %s\n", DHT22Clone::errorToString(result.error));
  }

  delay(3000);
}
```

## API Reference

### Constructor

```cpp
DHT22 dht(pin);                    // Auto-detect encoding (recommended)
DHT22 dht(pin, DHT22_CLONE);       // Force two's complement
DHT22 dht(pin, DHT22_ORIGINAL);    // Force sign-bit encoding
```

### Reading

```cpp
DHT22Clone_Result result = dht.read();

result.temperature  // float, °C
result.humidity     // float, %
result.error        // uint8_t, 0 = OK
result.raw[0..4]    // raw bytes from sensor
```

### Convenience methods

```cpp
dht.getTemperature()   // last successful temperature
dht.getHumidity()      // last successful humidity
dht.getLastError()     // last error code
```

### Error codes

| Code | Name              | Meaning                                    |
|------|-------------------|--------------------------------------------|
| 0    | `DHT22_OK`        | Success                                    |
| 1    | `DHT22_DRIVER`    | RMT peripheral error                       |
| 2    | `DHT22_TIMEOUT`   | Sensor did not respond                     |
| 3    | `DHT22_NACK`      | Invalid acknowledgment pulse               |
| 4    | `DHT22_BAD_DATA`  | Unexpected pulse timing                    |
| 5    | `DHT22_CHECKSUM`  | Data integrity check failed                |
| 6    | `DHT22_UNDERFLOW` | Received fewer than 40 bits                |
| 7    | `DHT22_OVERFLOW`  | Received more than 40 bits                 |


## Troubleshooting

## Credits

RMT-based reading approach inspired by [dhtESP32-rmt](https://github.com/htmltiger/dhtESP32-rmt) by junkfix/htmltiger.
and
DHT22 Clone for ESP32 inspired by [dhtESP32-rmt](https://github.com/Zapivara/DHT22_Clone_ESP32) by Zapivara.


