# ESP32-DHT-RMT
Non blocking DHT22 Library for ESP32 using RMT Module


This library works with DHT22 / AM2302 modules.




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

CDHT dht;  // GPIO14

void setup() {
  Serial.begin(115200);
}

void loop() {
    float tempC = 0.0;
    float humi = 0.0;

    uint8_t error=dht.read_dht(tempC, humi, 14, DHT22);
    if(error)
    {
      Serial.println("Error:", dht.errorToString(error));
    }
    else
    {
      Serial.printf("Temp: %.1f°C  Hum: %.1f%%\n", tempC, humi);

    }
  delay(3000);
}
```

## API Reference

* Read Humidity and temperature

``` 
uint8_t read_dht(float &temperature, float &humidity, uint8_t pin, uint8_t dhttype);
```

* get duration of ACK low and high pulse (for diagnostic) 
```
void get_nakduration(float &duration0, float &duration1); 
```
* get min and max duration of ACK low and high pulse (to adjust bad sensor timings) 
```
void set_minmaxpulse(int minpulse, int maxpulse); 
```
* get human readable String for errors
```
String errorToString(uint8_t error);
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


