#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define WARNING_LIGHT_PIN 0
#define BUTTON_PIN 2
#define MQTT_SERVER_HOST "test.mosquitto.org"
#define MQTT_SERVER_PORT 1883
#define MQTT_TOPIC_1 "iot-experiments/evt/counter"
#define MQTT_TOPIC_2 "iot-experiments/esas/counter"

void startSmartConfig();
boolean reconnectMqttClient();
void mqttCallback(char *topic, byte *payload, unsigned int length);

//////////////////////////////////

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
long lastReconnectAttempt = 0;
char clientId[20];
volatile int32_t topic1CounterValue;
volatile int32_t topic2CounterValue;
volatile boolean startWarningLight = false;

//////////////////////////////////

void setup()
{
  pinMode(WARNING_LIGHT_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.begin(115200);

  //The chip ID is essentially its MAC address(length: 6 bytes).
  uint64_t chipid = ESP.getEfuseMac();
  sprintf(clientId, "ESP_%04X%08X", (uint16_t)(chipid >> 32), (uint32_t)chipid);

  // Tries to connect with previous credentials or starts smartconfig
  if (WiFi.begin() == WL_CONNECT_FAILED)
  {
    startSmartConfig();
  }

  //Wait for WiFi to connect to AP
  Serial.println("Waiting for WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi Connected.");

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  mqttClient.setServer(MQTT_SERVER_HOST, MQTT_SERVER_PORT);
  mqttClient.setCallback(mqttCallback);
}

void loop()
{
  if (digitalRead(BUTTON_PIN) == LOW)
  {
    delay(1000);
    if (digitalRead(BUTTON_PIN) == LOW)
    {
      startSmartConfig();
    }
  }

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
    // turn the LED on (HIGH is the voltage level)
    digitalWrite(WARNING_LIGHT_PIN, HIGH);
    // wait for a second
    delay(5000);
    // turn the LED off by making the voltage LOW
    digitalWrite(WARNING_LIGHT_PIN, LOW);
  }
}

void startSmartConfig()
{
  // Init WiFi as Station, start SmartConfig
  WiFi.mode(WIFI_AP_STA);
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
  Serial.println("Attempting MQTT connection...");
  if (mqttClient.connect(clientId))
  {
    Serial.println("Connected to MQTT server");
    mqttClient.publish("iot-experiments/devices", clientId);
    mqttClient.subscribe(MQTT_TOPIC_1);
    mqttClient.subscribe(MQTT_TOPIC_2);
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
      iValue = topic1CounterValue;
      startWarningLight = true;
    }
  }
  
  if (strcmp(MQTT_TOPIC_2, topic) == 0)
  {
    int32_t iValue = atoi(value);
    if (iValue != topic2CounterValue)
    {
      iValue = topic2CounterValue;
      startWarningLight = true;
    }
  }
}