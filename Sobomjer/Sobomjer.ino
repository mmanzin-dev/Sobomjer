#include <BME680-SOLDERED.h>
#include <PCF85063A-SOLDERED.h>
#include <OLED-Display-SOLDERED.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define R_LED 12 // Crveni LED
#define Y_LED 13 // Zuti LED
#define G_LED 14 // Zeleni LED
#define BUZZER 15

const char* ssid = "S21FE";
const char* password = "00000000";
const char* mqtt_server = "10.150.235.236";
const char* mqtt_user = "manzin";
const char* mqtt_password = "00000000";
const char* topic_temperature = "sobomjer/temperatura";
const char* topic_humidity = "sobomjer/vlaga";
const char* topic_pressure = "sobomjer/tlak";
const char* topic_iaq = "sobomjer/iaq";
const char* topic_buzzer = "sobomjer/buzzer";

unsigned long lastPublish= 0;
const long interval = 5000;

BME680 bme;
WiFiClient espClient;
PubSubClient mqttClient(espClient);

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Spajanje na: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi povezan");
  Serial.print("IP adresa: ");
  Serial.println(WiFi.localIP());
}

void process_message(char* topic, byte* content, unsigned int length) {
  String received_message = "";
  Serial.print("Poruka je stigla na MQTT temu: ");
  Serial.println(topic);

  for (unsigned int i = 0; i < length; i++) {
    received_message += (char)content[i];
  }

  Serial.print("Sadrzaj poruke: ");
  Serial.println(received_message);

  // Obrada poruke
  if (String(topic) == topic_buzzer) {
    if (received_message == "1" || received_message == "ON" || received_message == "UKLJUCI") {
      digitalWrite(BUZZER, HIGH);
      Serial.println("Buzzer ON");
    } else if (received_message == "0" || received_message == "OFF" || received_message == "ISKLJUCI") {
      digitalWrite(BUZZER, LOW);
      Serial.println("Buzzer OFF");
    }
  }
}

void reconnect() {
  while (!mqttClient.connected()) {
    Serial.print("Spajanje na MQTT broker.. ");
    
    String clientId = "ESP8266Client-8400";

    if (mqttClient.connect(clientId.c_str(), mqtt_user, mqtt_password)) {
      Serial.println("povezan");
      mqttClient.subscribe(topic_buzzer);
      Serial.print("Uspjesno pretplacen na temu: ");
      Serial.println(topic_buzzer);
    } else {
      Serial.print("ERROR, rc=");
      Serial.println(mqttClient.state());
      delay(5000);
    }
  }
}

int calculateIAQ(float humidity, float gas) {
  float humidityScore = 0;
  float gasScore = 0;
  
  if (humidity >= 30 && humidity <= 50) {
    humidityScore = 0;
  } else if (humidity < 30) {
    humidityScore = (30 - humidity) * 2.5; // ako 20%, 10 * 2.5 = 25 pts
  } else {
    humidityScore = (humidity - 50) * 1.0; // ako 80%, 30 * 1.0 = 30 pts
  }
  if (humidityScore > 50) humidityScore = 50;
  
  float gasOhms = gas * 100.0; // mOhm u Ohms
  float cleanAir = 150000;  // Ohms
  float dirtyAir = 20000;   // Ohms
  
  if (gasOhms > cleanAir) {
    gasOhms = cleanAir;
  }
  if (gasOhms < dirtyAir) {
    gasOhms = dirtyAir;
  }
  
  gasScore = ((cleanAir - gasOhms) / (cleanAir - dirtyAir)) * 450.0; // prosjecan zrak: (150000 - 111200) / (150000 - 20000) * 450.0 = 134 pts
  
  int iaq = (int)(humidityScore + gasScore);
  
  return iaq;
}

void updateLED(int iaq) {
  digitalWrite(R_LED, LOW);
  digitalWrite(Y_LED, LOW);
  digitalWrite(G_LED, LOW);
  
  if (iaq <= 100) {
    // Odlicno
    digitalWrite(G_LED, HIGH);
  } 
  else if (iaq <= 200) {
    // Prosjecno
    digitalWrite(Y_LED, HIGH);
  } 
  else {
    // Lose
    digitalWrite(R_LED, HIGH);
  }
}

void setup() {
  Serial.begin(115200);
  bme.begin();
  setup_wifi();
  mqttClient.setServer(mqtt_server, 1883);
  mqttClient.setCallback(process_message);

  pinMode(R_LED, OUTPUT);
  pinMode(Y_LED, OUTPUT);
  pinMode(G_LED, OUTPUT);

  pinMode(BUZZER, OUTPUT);

  Serial.println("BUZZER TEST");
  digitalWrite(BUZZER, HIGH);
  delay(500);  // Drži 1 sekundu
  digitalWrite(BUZZER, LOW);
}

void loop() {
  if (!mqttClient.connected()) {
    reconnect();
  }
  
  mqttClient.loop();

  unsigned long now = millis();
  if (now - lastPublish >= interval) {
    lastPublish = now;

    float temperature = bme.readTemperature();
    float humidity = bme.readHumidity();
    float pressure = bme.readPressure();
    float gas = bme.readGasResistance(); // mOhm (raw je Ohm)
    int iaq = 0;

    if (isnan(temperature) || isnan(humidity) || isnan(pressure) || isnan(gas)) {
      Serial.print("ERROR, BME680 senzor ne radi ispravno");
      return;
    }

    iaq = calculateIAQ(humidity, gas);
    updateLED(iaq);

    String temperatureStr = String(temperature);
    String humidityStr = String(humidity);
    String pressureStr = String(pressure);
    String gasStr = String(gas);
    String iaqStr = String(iaq);

    // Serial
    Serial.println("***** BME680 mjerenja *****");

    Serial.print("Temperatura: ");
    Serial.print(temperatureStr);
    Serial.println(" C");

    Serial.print("Vlaga: ");
    Serial.print(humidityStr);
    Serial.println(" %");

    Serial.print("Tlak: ");
    Serial.print(pressureStr);
    Serial.println(" hPa");

    Serial.print("Otpor plina: ");
    Serial.print(gasStr);
    Serial.println(" mOhm");

    Serial.print("IAQ: ");
    Serial.println(iaqStr);

    // MQTT
    mqttClient.publish(topic_temperature, temperatureStr.c_str());
    mqttClient.publish(topic_humidity, humidityStr.c_str());
    mqttClient.publish(topic_pressure, pressureStr.c_str());
    mqttClient.publish(topic_iaq, iaqStr.c_str());

    Serial.println("Poslano na MQTT");
  }
}