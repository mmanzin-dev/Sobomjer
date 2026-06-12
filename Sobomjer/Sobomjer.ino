#include <BME680-SOLDERED.h>
#include <PCF85063A-SOLDERED.h>
#include <OLED-Display-SOLDERED.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define BUZZER 15

const char* ssid = "S21FE";
const char* password = "00000000";
const char* mqtt_server = "mqtt.mononetz.com";
const char* mqtt_user = "sobomjer";
const char* mqtt_password = ""; // Staviti/ukloniti password
const char* topic_temperature = "sobomjer/temperatura";
const char* topic_humidity = "sobomjer/vlaga";
const char* topic_pressure = "sobomjer/tlak";
const char* topic_gas = "sobomjer/otpor-plin";
const char* topic_iaq = "sobomjer/iaq";
const char* topic_format = "sobomjer/format";
const char* topic_buzzer = "sobomjer/buzzer";

unsigned long lastPublish = 0;
const long interval = 5000; // 5s publish delay

const float temp_offset = -4.0;
const float hum_offset = 10.0;
String buzzer = "0";

BME680 bme;
PCF85063A rtc;
OLED_Display display;
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
  Serial.println("Wi-Fi povezan");
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
      buzzer = "1";
      digitalWrite(BUZZER, HIGH);
      Serial.println("Buzzer ON");
    } else if (received_message == "0" || received_message == "OFF" || received_message == "ISKLJUCI") {
      buzzer = "0";
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

int calculateIAQ(float temperature, float humidity, float gas) {
  float temperatureScore = 0;
  float humidityScore = 0;
  float gasScore = 0;
  
  if (temperature >= 20 && temperature <= 24) {
    temperatureScore = 0;
  } else if (temperature < 20) {
    temperatureScore = (20 - temperature) * 1.5;
  } else {
    temperatureScore = (temperature - 24) * 2.5;
  }
  if (temperatureScore > 50) {
    temperatureScore = 50;
  }

  if (humidity >= 40 && humidity <= 60) {
    humidityScore = 0;
  } else if (humidity < 40) {
    humidityScore = (40 - humidity) * 1.5;
  } else {
    humidityScore = (humidity - 60) * 2.0;
  }
  if (humidityScore > 50) {
    humidityScore = 50;
  }
  
  float cleanAir = 4000; // mOhm
  float dirtyAir = 1000; // mOhm
  
  if (gas > cleanAir) {
    gas = cleanAir;
  }

  if (gas < dirtyAir) {
    gas = dirtyAir;
  }
  
  gasScore = ((cleanAir - gas) / (cleanAir - dirtyAir)) * 400; // prosjecan zrak: (4000 - 2500) / (4000 - 1000) * 400 = 200 pts

  int iaq = (int)(gasScore + temperatureScore + humidityScore);
  
  return iaq;
}

void getTime() {
  uint8_t hours = rtc.getHour();
  uint8_t minutes = rtc.getMinute();
  uint8_t seconds = rtc.getSecond();
  uint8_t day = rtc.getDay();
  uint8_t month = rtc.getMonth();
  uint16_t year = rtc.getYear();

  Serial.print("Datum: ");
  if (day < 10) {
    Serial.print("0");
  }
  Serial.print(day); 
  Serial.print(".");

  if (month < 10) {
    Serial.print("0");
  }
  Serial.print(month);
  Serial.print(".");

  Serial.print(year);
  Serial.println(".");

  Serial.print("Vrijeme: ");
  if (hours < 10) {
    Serial.print("0");
  }
  Serial.print(hours);
  Serial.print(":");

  if (minutes < 10) {
    Serial.print("0");
  }
  Serial.print(minutes);
  Serial.print(":");

  if (seconds < 10) {
    Serial.print("0");
  }
  Serial.println(seconds);
}

String getTimestamp() {
  uint8_t hours = rtc.getHour();
  uint8_t minutes = rtc.getMinute();
  uint8_t seconds = rtc.getSecond();
  uint8_t day = rtc.getDay();
  uint8_t month = rtc.getMonth();
  uint16_t year = rtc.getYear();

  String timestamp = "";

  if (day < 10) {
    timestamp += "0";
  }
  timestamp += day;
  timestamp += "-";

  if (month < 10) {
    timestamp += "0";
  }
  timestamp += month;
  timestamp += "-";

  timestamp += year;
  timestamp += ";";

  if (hours < 10) {
    timestamp += "0";
  }
  timestamp += hours;
  timestamp += ":";

  if (minutes < 10) {
    timestamp += "0";
  }
  timestamp += minutes;
  timestamp += ":";

  if (seconds < 10) {
    timestamp += "0";
  }
  timestamp += seconds;

  return timestamp;
}

void displayInfo(float temperature, float humidity, float pressure, float gas, int iaq) {
  uint8_t hours = rtc.getHour();
  uint8_t minutes = rtc.getMinute();
  uint8_t seconds = rtc.getSecond();
  uint8_t day = rtc.getDay();
  uint8_t month = rtc.getMonth();
  uint16_t year = rtc.getYear();

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  display.print("Datum: ");
  if (day < 10) {
    display.print("0");
  }
  display.print(day);
  display.print(".");
  if (month < 10) {
    display.print("0");
  }
  display.print(month);
  display.print(".");
  display.println(year);

  display.print("Vrijeme: ");
  if (hours < 10) {
    display.print("0");
  }
  display.print(hours);
  display.print(":");
  if (minutes < 10) {
    display.print("0");
  }
  display.print(minutes);
  display.print(":");
  if (seconds < 10) {
    display.print("0");
  }
  display.println(seconds);

  display.print("Temp: ");
  display.print(temperature, 1);
  display.println(" C");

  display.print("Vlaga: ");
  display.print(humidity, 0);
  display.println(" %");

  display.print("Tlak: ");
  display.print(pressure, 0);
  display.println(" hPa");

  display.print("Otpor: ");
  display.print(gas, 0);
  display.println(" mOhm");

  display.print("IAQ: ");
  display.println(iaq);

  display.display();
}

bool rtcCheck() {
  uint8_t hours = rtc.getHour();
  uint8_t minutes = rtc.getMinute();
  uint8_t seconds = rtc.getSecond();
  uint8_t day = rtc.getDay();
  uint8_t month = rtc.getMonth();
  uint16_t year = rtc.getYear();

  if (day < 1 || day > 31) { 
    return false;
  }

  if (month < 1 || month > 12) {
    return false;
  }

  if (year < 2026 || year > 2100) {
    return false;
  }

  if (hours > 23) {
    return false;
  }

  if (minutes > 59) {
    return false;
  }

  if (seconds > 59) {
    return false;
  }

  return true;
}

bool measurementCheck(float temperature, float humidity, float pressure, float gas) {
  if (temperature < -40 || temperature > 85) {
    Serial.println("Vrijednost temperature nije ispravan");
    return false;
  }

  if (humidity < 0 || humidity > 100) {
    Serial.println("Vrijednost vlage nije ispravan");
    return false;
  }

  if (pressure < 900 || pressure > 1100) {
    Serial.println("Vrijednost tlaka nije ispravan");
    return false;
  }

  if (gas < 100 || gas > 5000) {
    Serial.println("Vrijednost otpora plina nije ispravan");
    return false;
  }

  return true;
}

void setup() {
  Serial.begin(115200);

  bme.begin();

  rtc.begin();
  // RTC vrijeme kalibracija
  //rtc.setTime(8, 55, 0);
  //rtc.setDate(2, 9, 6, 2026);
  
  display.begin();
  
  setup_wifi();
  mqttClient.setServer(mqtt_server, 1883);
  mqttClient.setCallback(process_message);

  pinMode(BUZZER, OUTPUT);

  Serial.println("BUZZER TEST");
  digitalWrite(BUZZER, HIGH);
  delay(500);
  digitalWrite(BUZZER, LOW);

  // Spaja se na MQTT server i objavljuje buzzer topic
  reconnect();
  mqttClient.publish(topic_buzzer, buzzer.c_str());
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
    float gas = bme.readGasResistance(); // mOhm
    int iaq = 0;

    if (isnan(temperature) || isnan(humidity) || isnan(pressure) || isnan(gas)) {
      Serial.print("ERROR, BME680 senzor ne radi ispravno");
      return;
    }

    // Korekcija temperature i vlage
    temperature = temperature + temp_offset;
    humidity = humidity + hum_offset;

    // Provjera izmjerenih vrijednosti da ne salje nerealna mjerenja u bazu
    if (rtcCheck() == false) {
      Serial.println("RTC modul krivo mjeri vrijeme");
      Serial.println("Izmjeniti bateriju");
      return;
    }

    if (measurementCheck(temperature, humidity, pressure, gas) == false) {
      Serial.println("BME680 senzor krivo mjeri vrijednosti");
      Serial.println("Provjeriti zice");
      return;
    }

    iaq = calculateIAQ(temperature, humidity, gas);

    String temperatureStr = String(temperature);
    String humidityStr = String(humidity);
    String pressureStr = String(pressure);
    String gasStr = String(gas);
    String iaqStr = String(iaq);

    String timestampStr = getTimestamp();
    String format = timestampStr + ";" + temperatureStr + ";" + humidityStr + ";" + pressureStr + ";" + gasStr + ";" + iaqStr + ";" + buzzer;

    displayInfo(temperature, humidity, pressure, gas, iaq);

    // Serial
    Serial.println("***** RTC *****");
    getTime();

    Serial.println("***** BME680 mjerenja *****");

    Serial.print("Temperatura: ");
    Serial.print(temperatureStr);
    Serial.println(" °C");

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

    Serial.print("Buzzer: ");
    Serial.println(buzzer);

    // MQTT
    mqttClient.publish(topic_temperature, temperatureStr.c_str());
    mqttClient.publish(topic_humidity, humidityStr.c_str());
    mqttClient.publish(topic_pressure, pressureStr.c_str());
    mqttClient.publish(topic_gas, gasStr.c_str());
    mqttClient.publish(topic_iaq, iaqStr.c_str());
    mqttClient.publish(topic_format, format.c_str());

    Serial.println("Poslano na MQTT");
    Serial.print("Poslani format: ");
    Serial.println(format);
  }
}