#include <Arduino.h>
#include <ESP8266MQTTClient.h>
#include <Adafruit_NeoPixel.h>
#include <TM1637Display.h>
#include <ESP8266WiFi.h>

#define WARNING_LIGHT_PIN D6
#define BUTTON_PIN D2
#define NEOPIXEL_PIN D4
#define DISPLAY_CLK D7
#define DISPLAY_DIO D1

#define MQTT_URI "ws://test.mosquitto.org:8080"
#define MQTT_TOPIC_1 "iot-experiments/evt/counter"
#define MQTT_TOPIC_2 "iot-experiments/esas/counter"

void startSmartConfig();
void mqttDataCallback(String topic, String data, bool cont);
void mqttConnectedCallback();
void mqttDisconnectedCallback();
void processConfigButton();
void processDisplay();
void processWarningLight();
void displayColor(uint32_t color);

//////////////////////////////////

MQTTClient mqttClient;
bool connectedToMqtt = false;
volatile int32_t topic1CounterValue = 0;
volatile int32_t topic2CounterValue = 0;
volatile long startWarningLight = 0;
Adafruit_NeoPixel pixels(1, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
TM1637Display display(DISPLAY_CLK, DISPLAY_DIO);
long beginDisplaySequenceMs = 0;
uint8_t emptyLabel[] = {SEG_G, SEG_G, SEG_G, SEG_G};
uint8_t topic1Label[] = {SEG_A | SEG_D | SEG_E | SEG_F | SEG_G, SEG_B | SEG_C | SEG_D | SEG_E | SEG_F, SEG_D | SEG_E | SEG_F | SEG_G, 0x00}; // eut
uint8_t topic2Label[] = {
    SEG_A | SEG_D | SEG_E | SEG_F | SEG_G,
    SEG_A | SEG_C | SEG_D | SEG_F | SEG_G,
    SEG_A | SEG_B | SEG_C | SEG_E | SEG_F | SEG_G,
    SEG_A | SEG_C | SEG_D | SEG_F | SEG_G,
    SEG_A | SEG_B | SEG_C | SEG_E | SEG_F | SEG_G,
}; // esas

//////////////////////////////////

void setup()
{
  pinMode(WARNING_LIGHT_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  digitalWrite(WARNING_LIGHT_PIN, HIGH);

  Serial.begin(115200);
  while (!Serial)
    ; // wait for serial attach

  beginDisplaySequenceMs = millis();

  pixels.begin();
  display.setBrightness(0x0f);

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

  mqttClient.onData(mqttDataCallback);
  mqttClient.onConnect(mqttConnectedCallback);
  mqttClient.onDisconnect(mqttDisconnectedCallback);

  displayColor(pixels.Color(255, 255, 0));
  mqttClient.begin(MQTT_URI);
}

void loop()
{
  processConfigButton();

  processDisplay();

  processWarningLight();

  mqttClient.handle();
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

void processDisplay()
{
  if (!connectedToMqtt)
  {
    display.setSegments(emptyLabel);
    return;
  }

  long msSinceLastBeginSequence = millis() - beginDisplaySequenceMs;
  if (msSinceLastBeginSequence < 2000)
  {
    display.setSegments(topic1Label);
  }
  else if (msSinceLastBeginSequence < 4000)
  {
    display.showNumberDec(topic1CounterValue, false);
  }
  else if (msSinceLastBeginSequence < 6000)
  {
    display.setSegments(topic2Label);
  }
  else if (msSinceLastBeginSequence < 8000)
  {
    display.showNumberDec(topic2CounterValue, false);
  }
  else
  {
    beginDisplaySequenceMs = millis();
  }
}

void processWarningLight()
{
  if (startWarningLight != 0 && (millis() - startWarningLight) < 5000)
  {
    // turn the LED on (HIGH is the voltage level)
    digitalWrite(WARNING_LIGHT_PIN, LOW);
  }
  else
  {
    // turn the LED off by making the voltage LOW
    digitalWrite(WARNING_LIGHT_PIN, HIGH);
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

void mqttConnectedCallback()
{
  Serial.println("MQTT: Connected");
  connectedToMqtt = true;
  pixels.clear();
  pixels.show();
  mqttClient.subscribe(MQTT_TOPIC_1);
  mqttClient.subscribe(MQTT_TOPIC_2);
}

void mqttDisconnectedCallback()
{
  Serial.println("MQTT: Disconnected");
  connectedToMqtt = false;
  displayColor(pixels.Color(255, 255, 0));
}

void mqttDataCallback(String topic, String data, bool cont)
{
  Serial.print("MQTT message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  Serial.println(data);

  if (strcmp(MQTT_TOPIC_1, topic.c_str()) == 0)
  {
    int32_t iValue = atoi(data.c_str());
    if (iValue != topic1CounterValue)
    {
      topic1CounterValue = iValue;
      startWarningLight = millis();
    }
  }

  if (strcmp(MQTT_TOPIC_2, topic.c_str()) == 0)
  {
    int32_t iValue = atoi(data.c_str());
    if (iValue != topic2CounterValue)
    {
      topic2CounterValue = iValue;
      startWarningLight = millis();
    }
  }
}

void displayColor(uint32_t color)
{
  pixels.setPixelColor(0, color);
  pixels.show();
}
