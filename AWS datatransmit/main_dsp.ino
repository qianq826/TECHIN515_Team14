#include "Secrets.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"
#include <Adafruit_VEML7700.h>
#include <FastLED.h>
#include <Wire.h>

#define AWS_IOT_PUBLISH_TOPIC   "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"

#define LED_PIN 18
#define NUMPIXELS 60
#define TOUCH_PIN 4
#define TRIG_PIN 26
#define ECHO_PIN 27
#define SOUND_SPEED 0.034

#define MOVING_AVERAGE_SIZE 10
float distanceReadings[MOVING_AVERAGE_SIZE];
int readIndex = 0;
float totalDistance = 0;
float averageDistance = 0;

WiFiClientSecure net;
PubSubClient client(net);
Adafruit_VEML7700 veml = Adafruit_VEML7700();
CRGB leds[NUMPIXELS];
bool ledState = false;

void connectAWS() {
    net.setCACert(AWS_CERT_CA);
    net.setCertificate(AWS_CERT_CRT);
    net.setPrivateKey(AWS_CERT_PRIVATE);
    client.setServer(AWS_IOT_ENDPOINT, 8883);
    client.connect(THINGNAME);
    client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
}

void publishMessage() {
    StaticJsonDocument<200> doc;
    doc["lux"] = veml.readALS();
    doc["distance"] = averageDistance;
    char jsonBuffer[512];
    serializeJson(doc, jsonBuffer);
    client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}

void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    connectAWS();

    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    pinMode(TOUCH_PIN, INPUT);

    FastLED.addLeds<WS2811, LED_PIN, GRB>(leds, NUMPIXELS);
    if (!veml.begin()) {
        Serial.println("Failed to communicate with VEML7700 sensor!");
        while (1);
    }

    for (int i = 0; i < MOVING_AVERAGE_SIZE; i++) {
        distanceReadings[i] = 0;
    }
}

void loop() {
    static unsigned long lastTouchTime = 0;
    if (digitalRead(TOUCH_PIN) == HIGH && millis() - lastTouchTime > 200) {
        ledState = !ledState;
        Serial.println(ledState ? "LEDs turned on" : "LEDs turned off");
        lastTouchTime = millis();
    }

    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    long duration = pulseIn(ECHO_PIN, HIGH);
    float distanceCm = duration * SOUND_SPEED / 2;
    getFilteredDistance(distanceCm); // Update distance reading with filter

    float lux = veml.readALS();
    Serial.print("Lux: ");
    Serial.print(lux);
    Serial.print(", Distance: ");
    Serial.println(averageDistance);

    if (ledState) {
        uint8_t brightness = map(lux, 0, 1000, 255, 0);
        CRGB color = (lux > 500) ? CRGB::Blue : CRGB::Yellow;
        FastLED.setBrightness(brightness);
        fill_solid(leds, NUMPIXELS, color);
        FastLED.show();
    } else {
        FastLED.clear();
        FastLED.show();
    }

    publishMessage();
    client.loop();
    delay(1000);
}

float getFilteredDistance(float newDistance) {
    totalDistance -= distanceReadings[readIndex];
    distanceReadings[readIndex] = newDistance;
    totalDistance += distanceReadings[readIndex];
    readIndex = (readIndex + 1) % MOVING_AVERAGE_SIZE;
    averageDistance = totalDistance / MOVING_AVERAGE_SIZE;
    return averageDistance;
}








