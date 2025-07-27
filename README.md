# ESP8266 Gas Sensor with Telegram Notifications

[Русская версия (Russian version)](README_RU.md)

A device based on ESP8266 that measures gas concentration, visually displays the level on a NeoPixel LED strip, and sends notifications to Telegram when dangerous thresholds are exceeded.

![photo_2024-05-22_15-49-10](https://github.com/user-attachments/assets/58814a38-5509-411a-855c-2041240c5e7b)

## ⚙️ Features

- **Visual Indication:** Gas level is displayed on an 8-segment NeoPixel LED scale. Color changes from green (safe) to yellow and red (dangerous).
- **Alert System:** High gas levels activate both audio (buzzer) and visual (LED flashing) alarms.
- **Telegram Integration:**
  - Sends alert notifications to specified chat.
  - Remote device status monitoring.
  - Device control through commands.
  - OTA (Over-The-Air) firmware updates directly from Telegram chat.
- **Timer:** Built-in 5-minute timer that can be started with a button. Useful for ventilation or other procedures.
- **Control Button:**
  - Starts/adds time to the timer.
  - Temporarily disables audio alarm during alerts.
  - Resets timer with long press.
- **Server Data Transmission:** Periodically sends gas level and WiFi signal quality data to a web server for logging and analysis.

## 🔌 Hardware Components

- **ESP8266** microcontroller (e.g., NodeMCU v2).
- **Gas sensor** (e.g., MQ-2, MQ-5), connected to analog pin `A0`.
- **NeoPixel (WS2812B)** addressable LED strip with 8 LEDs, connected to pin `D4` (GPIO2).
- **Piezo buzzer**, connected to pin `D5` (GPIO14).
- **Push button**, connected to pin `D6` (GPIO12).

## 📚 Libraries

The project uses the following libraries, which PlatformIO will install automatically:

- `Adafruit NeoPixel`
- `FastBot`
- `ESP8266HTTPClient`

## 🛠️ Setup

For security and protection of your confidential data (passwords, tokens), all settings are moved to a separate file `src/secrets.h`. This file is already added to `.gitignore`, so it won't be accidentally uploaded to GitHub.

**1. Create `secrets.h` file**

In your project's `src` folder, create a file named `secrets.h`.

**2. Copy and fill the template**

Copy the following template into the created `src/secrets.h` file and replace the placeholder values (`YOUR_...`) with your actual data.

```cpp
#pragma once

// 1. WiFi Settings
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASS "YOUR_WIFI_PASSWORD"

// 2. Telegram Bot Settings
//    - Get token from @BotFather in Telegram.
//    - Get chat ID from @userinfobot, for example.
#define BOT_TOKEN "YOUR_TELEGRAM_BOT_TOKEN"
#define ADMIN_CHAT_ID "YOUR_CHAT_ID"

// 3. Web Server Settings (if used)
#define SERVER_ADDRESS "http://your.server.address/" // Server address
#define SERVER_KEY "YOUR_SERVER_KEY"                // Server access key
#define STATION_ID "5"                             // Your station ID
#define SENSOR_ID "gas_sensor_1"                   // Sensor ID

// 4. Sensor Setup
//    Calibrate the offset for your sensor so that readings
//    are close to zero in clean air.
#define GasLevelOffset 76 // Gas sensor offset
```

**3. Save the file**

After entering your data, save the file. The project is now ready for building and flashing.

## 🚀 Installation and Launch

1. Install Visual Studio Code with PlatformIO IDE extension.
2. Clone this repository.
3. Open the project folder in VS Code.
4. Create and configure `src/secrets.h` file (see "Setup" section).
5. Connect your ESP8266 device to the computer.
6. Build and flash the project using PlatformIO (`Upload` button in status bar).

## 🤖 Telegram Bot Commands

Send these commands to the bot in the chat whose ID you specified in `idAdmin1`:

- `/start` — Show menu with available commands.
- `/ping` — Get current device status:
  - Uptime
  - WiFi signal level (RSSI)
  - Current gas level in percentage
- `/restart` — Restart the device

### Firmware Update (OTA)

To update the firmware "over the air," attach a new `.bin` file to your message in Telegram chat. The device will automatically download and install the update.
