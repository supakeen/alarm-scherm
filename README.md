# alarm-scherm

The `alarm-scherm` firmware is meant for the LilyGo TTGo T-Display ESP32 module.

## Requirements

### Hardware

- LilyGo TTGo T-Display ESP32

### Software

- An MQTT server to connect to.

## Installation

`alarm-scherm` follows my general theme for firmware. This means that you should
have PlatformIO installed and ready to use.

1. Copy the `env/EXAMPLE` file to an `env/test.ini` file. Fill in your WiFi SSID and password.
2. Connect your device via USB and use the following commands to quickly get it flashed.

```bash
pio run -e test-serial -t upload
pio device monitor
```

## Usage

After booting the device shortly shows some fast text messages about connecting to WiFi and
your MQTT server, it then dims its backlight. The top button next to the screen toggles the
backlight on and off.

The bottom button clears alarms.
