# Old fashioned connected warnin light
This project is for the ESP32 or the ESP8266.
It can be compiled and flashed with PlatfomIO.

The full article is available here:
https://www.iot-experiments.com/esp32-warning-light/

This branch is quite different as the master.
In this release, the ESP connects on WiFi to a websocket server, receive some commands and execute them.

## Neopixel colors
* Red: looking for WiFi
* Yellow: WiFi OK; connecting to WS server
* Blue: smartconfig enabled. Waiting for WiFi credentials.
* Off: connected

After initialisation, the ESP displays the color sent by the server.

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