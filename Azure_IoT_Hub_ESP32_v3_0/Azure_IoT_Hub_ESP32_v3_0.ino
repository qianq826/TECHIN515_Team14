#include <FastLED.h>
#include <Wire.h>
#include <Adafruit_VEML7700.h>
#include <WiFi.h>
#include <mqtt_client.h>
#include <az_core.h>
#include <az_iot.h>
#include <azure_ca.h>
#include "AzIoTSasToken.h"
#include "SerialLogger.h"
#include "iot_configs.h"
#include <ArduinoJson.h>

// LED and sensor configurations
#define LED_PIN 18
#define NUMPIXELS 29
#define TRIG_PIN 26
#define ECHO_PIN 27
#define TOUCH_PIN 4
#define SOUND_SPEED 0.034

CRGB leds[NUMPIXELS];
Adafruit_VEML7700 veml;
bool ledState = false;
bool messageReceived = false; // Flag to indicate if a message was received
int redFromMessage = 0, greenFromMessage = 0, blueFromMessage = 0; // Variables to store the color from the message

// Azure IoT Hub configurations
#define AZURE_SDK_CLIENT_USER_AGENT "c%2F" AZ_SDK_VERSION_STRING "(ard;esp32)"
#define MQTT_QOS1 1
#define DO_NOT_RETAIN_MSG 0
#define SAS_TOKEN_DURATION_IN_MINUTES 60
#define UNIX_TIME_NOV_13_2017 1510592825
#define PST_TIME_ZONE -8
#define PST_TIME_ZONE_DAYLIGHT_SAVINGS_DIFF 1
#define GMT_OFFSET_SECS (PST_TIME_ZONE * 3600)
#define GMT_OFFSET_SECS_DST ((PST_TIME_ZONE + PST_TIME_ZONE_DAYLIGHT_SAVINGS_DIFF) * 3600)
#define INCOMING_DATA_BUFFER_SIZE 256

static const char* ssid = IOT_CONFIG_WIFI_SSID;
static const char* password = IOT_CONFIG_WIFI_PASSWORD;
static const char* host = IOT_CONFIG_IOTHUB_FQDN;
static const char* mqtt_broker_uri = "mqtts://" IOT_CONFIG_IOTHUB_FQDN;
static const char* device_id = IOT_CONFIG_DEVICE_ID;
static const int mqtt_port = AZ_IOT_DEFAULT_MQTT_CONNECT_PORT;
static esp_mqtt_client_handle_t mqtt_client;
static az_iot_hub_client client;
static char mqtt_client_id[128];
static char mqtt_username[128];
static char mqtt_password[200];
static uint8_t sas_signature_buffer[256];
static unsigned long next_telemetry_send_time_ms = 0;
static char telemetry_topic[128];
static uint32_t telemetry_send_count = 0;
static String telemetry_payload = "{}";
static char incoming_data[INCOMING_DATA_BUFFER_SIZE];
static AzIoTSasToken sasToken(&client, AZ_SPAN_FROM_STR(IOT_CONFIG_DEVICE_KEY), AZ_SPAN_FROM_BUFFER(sas_signature_buffer), AZ_SPAN_FROM_BUFFER(mqtt_password));

void connectToWiFi() {
  Serial.println("Connecting to WIFI SSID " + String(ssid));
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected, IP address: " + WiFi.localIP().toString());
}

void initializeTime() {
  Serial.println("Setting time using SNTP");
  configTime(GMT_OFFSET_SECS, GMT_OFFSET_SECS_DST, "pool.ntp.org", "time.nist.gov");
  time_t now = time(NULL);
  while (now < UNIX_TIME_NOV_13_2017) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  Serial.println("Time initialized!");
}

static void initializeIoTHubClient() {
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  options.user_agent = AZ_SPAN_FROM_STR(AZURE_SDK_CLIENT_USER_AGENT);
  if (az_result_failed(az_iot_hub_client_init(&client, az_span_create((uint8_t*)host, strlen(host)), az_span_create((uint8_t*)device_id, strlen(device_id)), &options))) {
    Serial.println("Failed initializing Azure IoT Hub client");
    return;
  }
  size_t client_id_length;
  if (az_result_failed(az_iot_hub_client_get_client_id(&client, mqtt_client_id, sizeof(mqtt_client_id) - 1, &client_id_length))) {
    Serial.println("Failed getting client id");
    return;
  }
  if (az_result_failed(az_iot_hub_client_get_user_name(&client, mqtt_username, sizeof(mqtt_username), NULL))) {
    Serial.println("Failed to get MQTT clientId, return code");
    return;
  }
  Serial.println("Client ID: " + String(mqtt_client_id));
  Serial.println("Username: " + String(mqtt_username));
}

static int initializeMqttClient() {
#ifndef IOT_CONFIG_USE_X509_CERT
  if (sasToken.Generate(SAS_TOKEN_DURATION_IN_MINUTES) != 0) {
    Serial.println("Failed generating SAS token");
    return 1;
  }
#endif
  esp_mqtt_client_config_t mqtt_config = {};
  mqtt_config.uri = mqtt_broker_uri;
  mqtt_config.port = mqtt_port;
  mqtt_config.client_id = mqtt_client_id;
  mqtt_config.username = mqtt_username;
#ifdef IOT_CONFIG_USE_X509_CERT
  mqtt_config.client_cert_pem = IOT_CONFIG_DEVICE_CERT;
  mqtt_config.client_key_pem = IOT_CONFIG_DEVICE_CERT_PRIVATE_KEY;
#else
  mqtt_config.password = (const char*)az_span_ptr(sasToken.Get());
#endif
  mqtt_config.keepalive = 30;
  mqtt_config.disable_clean_session = 0;
  mqtt_config.disable_auto_reconnect = false;
  mqtt_config.event_handle = mqtt_event_handler;
  mqtt_config.user_context = NULL;
  mqtt_config.cert_pem = (const char*)ca_pem;
  mqtt_client = esp_mqtt_client_init(&mqtt_config);
  if (mqtt_client == NULL) {
    Serial.println("Failed creating mqtt client");
    return 1;
  }
  esp_err_t start_result = esp_mqtt_client_start(mqtt_client);
  if (start_result != ESP_OK) {
    Serial.println("Could not start mqtt client; error code:" + String(start_result));
    return 1;
  } else {
    Serial.println("MQTT client started");
    return 0;
  }
}

void establishConnection() {
  connectToWiFi();
  initializeTime();
  initializeIoTHubClient();
  (void)initializeMqttClient();
}

static void generateTelemetryPayload(float distance, float lux) {
  telemetry_payload = "{ \"msgCount\": " + String(telemetry_send_count++) + ", \"distance\": " + String(distance) + ", \"lux\": " + String(lux) + " }";
}

static void sendTelemetry(float distance, float lux) {
  Serial.println("Sending telemetry ...");
  if (az_result_failed(az_iot_hub_client_telemetry_get_publish_topic(&client, NULL, telemetry_topic, sizeof(telemetry_topic), NULL))) {
    Serial.println("Failed az_iot_hub_client_telemetry_get_publish_topic");
    return;
  }
  generateTelemetryPayload(distance, lux);
  if (esp_mqtt_client_publish(mqtt_client, telemetry_topic, (const char*)telemetry_payload.c_str(), telemetry_payload.length(), MQTT_QOS1, DO_NOT_RETAIN_MSG) == 0) {
    Serial.println("Failed publishing");
  } else {
    Serial.println("Message published successfully");
  }
}

int calculateBrightness(float lux) {
  // Define the max and min brightness levels
  const int maxBrightness = 255;  // Maximum LED brightness
  const int minBrightness = 50;   // Minimum LED brightness to avoid complete darkness

  // Normalize lux values to your typical working conditions (e.g., 0-1000 lux)
  float normalizedLux = constrain(lux, 0, 1000);

  // Calculate brightness inversely proportional to lux
  int brightness = map(normalizedLux, 0, 1000, maxBrightness, minBrightness);

  return brightness;
}

CRGB calculateColor(float lux) {
  // Define cool and warm colors
  CRGB coolColor = CRGB::White;  // White for bright conditions
  CRGB warmColor = CRGB(255, 147, 41);  // Warm orange for darker conditions

  // Normalize lux values to your typical working conditions (e.g., 0-1000 lux)
  float normalizedLux = constrain(lux, 0, 1000);

  // Calculate color based on lux
  uint8_t blendFactor = map(normalizedLux, 0, 1000, 0, 255);  // Determine how much to blend

  // Blend between warm and cool colors based on ambient light
  return blend(warmColor, coolColor, blendFactor);
}

void setLights(int brightness, CRGB color) {
  FastLED.setBrightness(brightness);
  fill_solid(leds, NUMPIXELS, color);
  FastLED.show();
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(TOUCH_PIN, INPUT);
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUMPIXELS);
  FastLED.setBrightness(255);
  if (!veml.begin()) {
    Serial.println("Failed to initialize VEML7700 sensor!");
    while (1);
  }
  establishConnection();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
  } else if (sasToken.IsExpired()) {
    Serial.println("SAS token expired; reconnecting with a new one.");
    (void)esp_mqtt_client_destroy(mqtt_client);
    initializeMqttClient();
  } else if (millis() > next_telemetry_send_time_ms) {
    float lux = veml.readLux();
    if (digitalRead(TOUCH_PIN) == HIGH) {
      delay(50);
      if (digitalRead(TOUCH_PIN) == HIGH) {
        ledState = !ledState;
        Serial.println(ledState ? "LEDs turned on" : "LEDs turned off");
      }
    }
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    long duration = pulseIn(ECHO_PIN, HIGH);
    float distance = duration * SOUND_SPEED / 2;
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.print(" cm, Lux: ");
    Serial.println(lux);
    if (ledState) {
      if (messageReceived) {
        // Set the color to the last received color from the message
        setLights(255, CRGB(redFromMessage, greenFromMessage, blueFromMessage));
      } else {
        // Use the default color function based on VEML 7700 sensor
        CRGB color = calculateColor(lux);
        int brightness = calculateBrightness(lux);
        setLights(brightness, color);
        Serial.print("Adjusted to lux: ");
        Serial.print(lux);
        Serial.print(" - Brightness: ");
        Serial.print(brightness);
        Serial.print(" - Color set as per lux level.");
        Serial.println();
      }
    } else {
      // When LED is turned off, clear the LEDs
      FastLED.clear();
      FastLED.show();
      // Reset the message received flag
      messageReceived = false;
    }
    sendTelemetry(distance, lux);
    next_telemetry_send_time_ms = millis() + 1000;
  }
}

void handleIncomingMessage(const char* message, size_t length) {
  DynamicJsonDocument doc(256); // Increased size
  DeserializationError error = deserializeJson(doc, message, length);
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  if (doc.containsKey("red") && doc.containsKey("green") && doc.containsKey("blue")) {
    redFromMessage = doc["red"];
    greenFromMessage = doc["green"];
    blueFromMessage = doc["blue"];
    Serial.printf("Setting color to R: %d, G: %d, B: %d\n", redFromMessage, greenFromMessage, blueFromMessage);
    messageReceived = true; // Set the flag to indicate a message was received
  } else {
    Serial.println("JSON message does not contain required keys.");
  }
}

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event) {
  static int i;
  static int r;
  switch (event->event_id) {
    case MQTT_EVENT_ERROR:
      Serial.println("MQTT event MQTT_EVENT_ERROR");
      break;
    case MQTT_EVENT_CONNECTED:
      Serial.println("MQTT event MQTT_EVENT_CONNECTED");
      r = esp_mqtt_client_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC, 1);
      if (r == -1) {
        Serial.println("Could not subscribe for cloud-to-device messages.");
      } else {
        Serial.println("Subscribed for cloud-to-device messages; message id:" + String(r));
      }
      break;
    case MQTT_EVENT_DISCONNECTED:
      Serial.println("MQTT event MQTT_EVENT_DISCONNECTED");
      break;
    case MQTT_EVENT_SUBSCRIBED:
      Serial.println("MQTT event MQTT_EVENT_SUBSCRIBED");
      break;
    case MQTT_EVENT_UNSUBSCRIBED:
      Serial.println("MQTT event MQTT_EVENT_UNSUBSCRIBED");
      break;
    case MQTT_EVENT_PUBLISHED:
      Serial.println("MQTT event MQTT_EVENT_PUBLISHED");
      break;
    case MQTT_EVENT_DATA:
      Serial.println("MQTT event MQTT_EVENT_DATA");
      Serial.println("Message received: ");
      Serial.println(String(event->data).substring(0, event->data_len));
      handleIncomingMessage(event->data, event->data_len);
      break;
    case MQTT_EVENT_BEFORE_CONNECT:
      Serial.println("MQTT event MQTT_EVENT_BEFORE_CONNECT");
      break;
    default:
      Serial.println("MQTT event UNKNOWN");
      break;
  }
  return ESP_OK;
}
