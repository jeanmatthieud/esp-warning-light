# Old fashioned connected warnin light
This project was originally for the ESP32 and the ESP8266.
**This branch is only for the ESP8266, and allows the use of MQTT over websockets thanks to another MQTT client library.**
It can be compiled and flashed with PlatfomIO.

The full article is available here:
https://www.iot-experiments.com/esp32-warning-light/

## Neopixel colors
* Red: looking for WiFi
* Yellow: WiFi OK; connecting to MQTT server
* Blue: smartconfig enabled. Waiting for WiFi credentials.
* Off: connected

## Components
* Old warning lamp at 12V
* MOSFET (IRF3205)
* NPN (2N2222)
* 2* 10k resistors
* DC-DC step down converter (12V to 5V)
* ESP32 or ESP8266
* 1* Neopixel

The ESP drives the NPN which activates the MOSFET.
The MOSFET allows the motor to spin.
Without any current on the NPN, the MOSFET will be activated (ON by default).