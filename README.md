# IoT Humidity Sensor System
An ESP32-based IoT system that monitors ambient humidity and light levels in real time, triggers automated actuator control, and streams live data to a cloud dashboard via MQTT.

Built as part of an IoT module at Ngee Ann Polytechnic.
---
## What It Does

Plant-covered buildings in Singapore rely on fixed irrigation schedules that can't respond to real environmental conditions — leading to over- or under-watering. This system solves that by continuously sensing humidity and light, then automatically controlling a relay (e.g. ventilation fan or irrigation valve) to keep conditions within a safe range.

- Humidity below 30%RH → relay activates, red LED + buzzer alert
- Humidity above 70%RH → relay activates, red LED + buzzer alert  
- Humidity within range → green LED, relay off
- Light level dim → relay activates independently
- All data published via MQTT → stored in InfluxDB → visualised in Grafana

---

## System Architecture

```
[DHT Sensor + LDR]
        |
    [ESP32]  ──── Wi-Fi/MQTT ────►  [Node-RED]
        |                                |
   [OLED Display]                  [InfluxDB]
   [LED + Buzzer]                       |
   [Relay Module]                  [Grafana Dashboard]
```

**Components:**
- **ESP32** — main microcontroller; reads sensors, controls actuators, handles Wi-Fi/MQTT
- **DHT Humidity Sensor** (GPIO 35) — reads relative humidity (%RH)
- **LDR Light Sensor** (GPIO 34) — reads ambient light level
- **Relay Module** (GPIO 4) — controls external actuator (fan/irrigation valve)
- **OLED Display** (I2C, 0x3C) — shows live readings locally
- **SPDT Switch** — selects between High/Low humidity alarm mode
- **Node-RED** — MQTT broker subscriber; processes and routes data
- **InfluxDB** — time-series database for sensor data storage
- **Grafana** — real-time dashboard with gauges, trend graphs, relay state log

---

## Key Technical Challenge — GPIO Pin Conflict

During development, the LDR was initially connected to **GPIO4**. While GPIO4 supports analog input, it shares internal functions with the ESP32's Wi-Fi subsystem. When Wi-Fi was active, the ADC readings became unstable and produced inaccurate lux values.

**Fix:** Moved the LDR to **GPIO34**, a dedicated input-only ADC pin with no shared functions. Readings immediately became stable and consistent.

This taught me to always cross-reference the ESP32 pin configuration table against peripheral assignments — especially when Wi-Fi or Bluetooth are active, since they can interfere with certain ADC channels.

---

## Tech Stack

| Layer | Technology |
|---|---|
| Hardware | ESP32, DHT sensor, LDR, relay module, SSD1306 OLED |
| Firmware | Arduino IDE (C++) |
| Protocol | MQTT over Wi-Fi (HiveMQ / Mosquitto, TLS supported) |
| Data flow | Node-RED → InfluxDB |
| Visualisation | Grafana |
| Libraries | PubSubClient, ArduinoJson, Adafruit GFX, Adafruit SSD1306 |

---

## How to Run

1. Clone the repo
2. Copy `credentials.h.example` to `credentials.h` and fill in your Wi-Fi and MQTT credentials
3. Set your MQTT broker in `parameters.h` (HiveMQ cloud or local Mosquitto)
4. Flash to ESP32 via Arduino IDE (set board to **ESP32 Dev Module**)
5. Open Serial Monitor at 115200 baud to verify sensor readings
6. Import the Node-RED flow JSON and configure your InfluxDB connection
7. Open Grafana and import the dashboard

---
## Node-RED Workflow
<img width="772" height="384" alt="Screenshot 2026-07-19 154000" src="https://github.com/user-attachments/assets/302c9f61-3bab-453d-815c-e0512c712a14" />

### Flow Breakdown
- **HumidSensor/tyl9243a/M** — MQTT subscribe node, ingests raw sensor readings
- **Extract&Transform** — parses payload, extracts humidity value + timestamp
- **switch** — routes based on threshold (e.g. >70% triggers actuation)
- **Powermode / Actuation** — function nodes controlling device state
- **mqtt (publish)** — sends actuation command back to device
- **logging / debug** — dev-time monitoring, catch-all error handling

---

## Dashboard
<img width="746" height="455" alt="photo_6134106214258905243_x" src="https://github.com/user-attachments/assets/6ab4bbcf-5107-48c6-8e14-3dbe6668566f" />

---

## Schematic & Prototype
<img width="860" height="409" alt="photo_6136607057751248467_y" src="https://github.com/user-attachments/assets/3fa3ebeb-9c16-40cb-85fc-88c7ddd2492a" />

<img width="858" height="448" alt="photo_6136607057751248468_y" src="https://github.com/user-attachments/assets/10e2d8fc-3d55-457a-adb8-340aa3569b53" />

---

## Reflections

The biggest learning from this project was understanding how data physically travels from a sensor pin all the way to a browser dashboard — ADC → firmware processing → MQTT publish → Node-RED flow → InfluxDB write → Grafana query. Each layer has its own failure mode, and debugging the GPIO4 ADC instability made me appreciate how important hardware-software awareness is in embedded systems work.
