# 🩺 Smart Medibox Monitoring System

An IoT-based smart Medibox system using ESP32 and Node-RED for real-time monitoring of temperature and light intensity, featuring dynamic servo-based shade control to ensure safe medicine storage.

---

## 🚀 Features

- 📡 **Real-time Monitoring**: Measures ambient temperature and light intensity using DHT11 and LDR sensors.
- 🤖 **Automatic Shade Control**: Adjusts servo angle based on configurable light/temperature thresholds.
- 📊 **Node-RED Dashboard**: Real-time visualizations with sliders to adjust:
  - Sampling rate
  - Sending interval
  - Minimum servo angle
  - Control factor
  - Ideal temperature
- 🌐 **MQTT Communication**: Reliable sensor-to-dashboard communication using MQTT protocol.

---

## 🛠️ Tech Stack

- **Hardware**: ESP32, DHT11, LDR, Servo Motor  
- **Software**: Node-RED, Arduino IDE  
- **Protocols**: MQTT (Mosquitto Broker)

---

## 📷 Screenshots

> *(Insert screenshots of your Node-RED dashboard and hardware setup here)*

---

## 🧰 How to Use

1. **Set up Node-RED**  
   - Install Node-RED on your local machine or Raspberry Pi  
   - Import the provided dashboard flow (if available)

2. **Connect Hardware to ESP32**  
   - LDR to analog pin  
   - DHT11 to digital pin  
   - Servo motor to PWM pin

3. **Upload Code**  
   - Use Arduino IDE to upload the ESP32 sketch

4. **Configure MQTT**  
   - Ensure your broker is running and matches the topic settings in the code

---

## 📂 Folder Structure

