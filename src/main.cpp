#include <Arduino.h>
#include <WebSocketsClient.h>
#include <Adafruit_NeoPixel.h>
#include <TM1637Display.h>
#ifdef ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif
#include "seven_segment_ascii.h"

#ifdef ESP32
#define WARNING_LIGHT_PIN 17
#define BUTTON_PIN 2
#define NEOPIXEL_PIN 4
#define DISPLAY_CLK 16
#define DISPLAY_DIO 17
#else
#define WARNING_LIGHT_PIN D6
#define BUTTON_PIN D2
#define NEOPIXEL_PIN D4
#define DISPLAY_CLK D7
#define DISPLAY_DIO D1
#endif

#define WS_SERVER_HOST "172.24.1.1"
#define WS_SERVER_PORT 8080

inline int min(int a, int b) { return ((a) < (b) ? (a) : (b)); }

void startSmartConfig();
void processConfigButton();
void processMessage(const uint8_t *message);
void displayColor(uint32_t color);
void displayText(String text);
void webSocketEvent(WStype_t type, uint8_t *payload, size_t length);

//////////////////////////////////

WebSocketsClient webSocket;
char clientId[20];
Adafruit_NeoPixel pixels(1, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
TM1637Display display(DISPLAY_CLK, DISPLAY_DIO);
uint8_t emptyLabel[] = {SEG_G, SEG_G, SEG_G, SEG_G};

//////////////////////////////////

void setup()
{
  pinMode(WARNING_LIGHT_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  digitalWrite(WARNING_LIGHT_PIN, HIGH);

  Serial.begin(115200);
  while (!Serial)
    ; // wait for serial attach

  pixels.begin();
  display.setBrightness(0x0f);

#ifdef ESP32
  //The chip ID is essentially its MAC address(length: 6 bytes).
  uint64_t chipid = ESP.getEfuseMac();
  sprintf(clientId, "ESP_%04X%08X", (uint16_t)(chipid >> 32), (uint32_t)chipid);
#else
  uint32_t chipid = ESP.getChipId();
  sprintf(clientId, "ESP_%08X", chipid);
#endif

  displayColor(pixels.Color(255, 0, 0));
  display.setSegments(emptyLabel); // TODO : Display something for 'WiFi'

  // Tries to connect with previous credentials or starts smartconfig
  if (WiFi.begin() == WL_CONNECT_FAILED)
  {
    startSmartConfig();
  }

  //Wait for WiFi to connect to AP
  Serial.println("Waiting for WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    processConfigButton();
    delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi Connected.");
  displayColor(pixels.Color(255, 255, 0));

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

#ifdef ESP8266
  // The ESP8266 isn't capable to check SSL certificates by it's own
  // wifiClient.setInsecure();
#endif

  webSocket.begin(WS_SERVER_HOST, WS_SERVER_PORT, "/");
  webSocket.onEvent(webSocketEvent);
  // webSocket.setAuthorization(clientId, "password");
  webSocket.setReconnectInterval(5000);
  // start heartbeat (optional)
  // ping server every 15000 ms
  // expect pong from server within 3000 ms
  // consider connection disconnected if pong is not received 2 times
  webSocket.enableHeartbeat(15000, 3000, 2);
}

void loop()
{
  processConfigButton();

  webSocket.loop();
}

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
  case WStype_DISCONNECTED:
    Serial.println("[WSc] Disconnected!");
    displayColor(pixels.Color(255, 255, 0));
    break;
  case WStype_CONNECTED:
  {
    Serial.printf("[WSc] Connected to url: %s\n", payload);
    pixels.clear();
    pixels.show();
  }
  break;
  case WStype_TEXT:
    Serial.printf("[WSc] get text: %s\n", payload);

    processMessage(payload);

    // send message to server
    // webSocket.sendTXT("message here");
    break;
  case WStype_BIN:
    Serial.printf("[WSc] get binary length: %u\n", length);
    // hexdump(payload, length);
    // send data to server
    // webSocket.sendBIN(payload, length);
    break;
  case WStype_PING:
    Serial.printf("[WSc] get ping\n");
    break;
  case WStype_PONG:
    Serial.printf("[WSc] get pong\n");
    break;
  }
}

void processMessage(const uint8_t *message)
{
  String value = String((char *)message);
  if (value.length() == 0)
  {
    return;
  }

  webSocket.sendTXT(message);
  if (value.startsWith("AT+COLOR="))
  {
    int r, g, b;
    sscanf(value.c_str(), "AT+COLOR=%d;%d;%d", &r, &g, &b);
    displayColor(pixels.Color(r, g, b));
  }
  else if (value.startsWith("AT+LAMP=ON"))
  {
    // turn the LED on (HIGH is the voltage level)
    digitalWrite(WARNING_LIGHT_PIN, LOW);
  }
  else if (value.startsWith("AT+LAMP=OFF"))
  {
    // turn the LED off by making the voltage LOW
    digitalWrite(WARNING_LIGHT_PIN, HIGH);
  }
  else if (value.startsWith("AT+DISPLAY_TXT="))
  {
    displayText(value.substring(15));
  }
  else
  {
    webSocket.sendTXT("ERROR");
  }
}

void processConfigButton()
{
  if (digitalRead(BUTTON_PIN) == LOW)
  {
    delay(1000);
    if (digitalRead(BUTTON_PIN) == LOW)
    {
      startSmartConfig();
    }
  }
}

void startSmartConfig()
{
  displayColor(pixels.Color(0, 0, 255));

  // Init WiFi as Station, start SmartConfig
  WiFi.mode(WIFI_STA);
  WiFi.beginSmartConfig();

  //Wait for SmartConfig packet from mobile
  Serial.println("Waiting for SmartConfig.");
  while (!WiFi.smartConfigDone())
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("SmartConfig received.");
}

void displayColor(uint32_t color)
{
  pixels.setPixelColor(0, color);
  pixels.show();
}

void displayText(String text)
{
  uint8_t label[] = {0x00, 0x00, 0x00, 0x00};
  for (int i = 0; i < min(text.length(), 4); i++)
  {
    int index = text.charAt(i) - 32;
    if (index >= 0 && index < 96)
    {
      Serial.println(index);
      label[i] = SevenSegmentASCII[index];
    }
  }
  display.setSegments(label);
}
