#include <BME680-SOLDERED.h>
#include "OLED-Display-SOLDERED.h"
#include "MQ-Sensor-SOLDERED.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <PubSubClient.h>

#define LED_PIN_1 12 // Crveni LED
#define LED_PIN_2 13 // Zuti LED
#define LED_PIN_3 14 // Zeleni LED
#define LED_PIN_GAS 15 // Crveni LED (plin)
#define MQ9_PIN A0

BME680 bme;
OLED_Display oled;
MQ9 mq9(MQ9_PIN);

// WiFi
const char* wifi_ssid = "S21FE";
const char* wifi_password = "00000000";

// MQTT
const char* mqttServer = "";
const int mqttPort = 1883;
const char* mqttUsername = "";
const char* mqttPassword = "";

// WiFi & MQTT client
WiFiClient espClient;
PubSubClient client(espClient);

// IAQ funckija


void setup() {
  Serial.begin(115200);

  if (!bme.begin()) {
    Serial.println("ERROR: BME680 se nije uspio inicijalizirati");
  }

  if (!oled.begin()) {
    Serial.println("ERROR: SSD1306 se nije uspio inicijalizirati");
  }

  mq9.begin();

  Serial.print("Povezivanje na mrezu: ");
  Serial.print(wifi_ssid);
  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("-");
  }

  Serial.println("WiFi povezan");
  Serial.print("IP adresa: ");
  Serial.println(WiFi.localIP());

  pinMode(LED_PIN_1, OUTPUT);
  pinMode(LED_PIN_2, OUTPUT);
  pinMode(LED_PIN_3, OUTPUT);
  pinMode(LED_PIN_GAS, OUTPUT);
}

void loop() {
  // BME680 mjerenja
  float temperature, humidity, pressure, altitude, gas;
  int iaq; // Indoor Air Quality

  temperature = bme.readTemperature();
  humidity = bme.readHumidity();
  pressure = bme.readPressure();
  altitude = bme.readAltitude();
  gas = bme.readGasResistance();

  Serial.println(
  "Temperatura: " + String(temperature) + "°C\n" +
  "Vlaga: " + String(humidity) + "%\n" +
  "Tlak: " + String(pressure) + "hPa\n" +
  "Nadmorska visina: " + String(altitude) + "m\n" //+
  //"IAQ: " + String(iaq) "\n"
  );

  delay(1000);
}