#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <Adafruit_NeoPixel.h>
#ifdef ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif

#ifdef ESP32
#define WARNING_LIGHT_PIN 17
#define BUTTON_PIN 2
#define NEOPIXEL_PIN 4
#else
#define WARNING_LIGHT_PIN D3
#define BUTTON_PIN D2
#define NEOPIXEL_PIN D4
#endif

#define MQTT_SERVER_HOST "test.mosquitto.org"
#define MQTT_SERVER_PORT 8883
#define MQTT_TOPIC_1 "iot-experiments/evt/counter"
#define MQTT_TOPIC_2 "iot-experiments/esas/counter"

#define COLOR_SATURATION 128

void startSmartConfig();
boolean reconnectMqttClient();
void mqttCallback(char *topic, byte *payload, unsigned int length);
void processConfigButton();
void displayColor(uint32_t color);

//////////////////////////////////

WiFiClientSecure wifiClient;
PubSubClient mqttClient(wifiClient);
long lastReconnectAttempt = 0;
char clientId[20];
volatile int32_t topic1CounterValue;
volatile int32_t topic2CounterValue;
volatile boolean startWarningLight = false;
Adafruit_NeoPixel pixels(1, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

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

#ifdef ESP32
  //The chip ID is essentially its MAC address(length: 6 bytes).
  uint64_t chipid = ESP.getEfuseMac();
  sprintf(clientId, "ESP_%04X%08X", (uint16_t)(chipid >> 32), (uint32_t)chipid);
#else
  uint32_t chipid = ESP.getChipId();
  sprintf(clientId, "ESP_%08X", chipid);
#endif

  displayColor(pixels.Color(255, 0, 0));

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

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

#ifdef ESP8266
  // The ESP8266 isn't capable to check SSL certificates by it's own
  wifiClient.setInsecure();
#endif
  mqttClient.setServer(MQTT_SERVER_HOST, MQTT_SERVER_PORT);
  mqttClient.setCallback(mqttCallback);
}

void loop()
{
  processConfigButton();

  if (!mqttClient.connected())
  {
    long now = millis();
    if (now - lastReconnectAttempt > 5000)
    {
      lastReconnectAttempt = now;
      if (reconnectMqttClient())
      {
        lastReconnectAttempt = 0;
      }
    }
  }
  else
  {
    mqttClient.loop();
  }

  if (startWarningLight)
  {
    startWarningLight = false;
    // turn the LED on (HIGH is the voltage level)
    digitalWrite(WARNING_LIGHT_PIN, LOW);
    // wait for a second
    delay(5000);
    // turn the LED off by making the voltage LOW
    digitalWrite(WARNING_LIGHT_PIN, HIGH);
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

boolean reconnectMqttClient()
{
  displayColor(pixels.Color(255, 255, 0));

  Serial.println("Attempting MQTT connection...");
  if (mqttClient.connect(clientId))
  {
    Serial.println("Connected to MQTT server");
    mqttClient.publish("iot-experiments/devices", clientId);
    mqttClient.subscribe(MQTT_TOPIC_1);
    mqttClient.subscribe(MQTT_TOPIC_2);

    pixels.clear();
    pixels.show();
  }
  else
  {
    Serial.print("Failed, rc=");
    Serial.println(mqttClient.state());
  }
  return mqttClient.connected();
}

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("MQTT message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  char value[length + 1];
  strncpy(value, (char *)payload, length);
  value[length] = '\0';
  Serial.println(value);

  if (strcmp(MQTT_TOPIC_1, topic) == 0)
  {
    int32_t iValue = atoi(value);
    if (iValue != topic1CounterValue)
    {
      topic1CounterValue = iValue;
      startWarningLight = true;
    }
  }

  if (strcmp(MQTT_TOPIC_2, topic) == 0)
  {
    int32_t iValue = atoi(value);
    if (iValue != topic2CounterValue)
    {
      topic2CounterValue = iValue;
      startWarningLight = true;
    }
  }
}

void displayColor(uint32_t color) {
  pixels.setPixelColor(0, color);
  pixels.show();
}
