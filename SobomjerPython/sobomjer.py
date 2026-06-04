import paho.mqtt.client as mqtt
import json

MQTT_BROKER = "localhost"
MQTT_TOPIC_FORMAT = "sobomjer/format"
JSON_FILE = "/var/www/html/data.json"

trenutni_podaci = {
    "datum": "--",
    "vrijeme": "--",
    "temperatura": "--",
    "vlaga": "--",
    "tlak": "--",
    "otpor-plin": "--",
    "iaq": "--",
    "buzzer": "--",
}

def save_to_json():
    try:
        with open(JSON_FILE, 'w') as f:
            json.dump(trenutni_podaci, f)
        print(f"Azurirano: { trenutni_podaci }")
    except Exception as e:
        print(f"Greska pri pisanju JSON-a: {e}")

def on_message(client, userdata, msg):
    payload = msg.payload.decode()
    topic = msg.topic

    if topic == MQTT_TOPIC_FORMAT:
        parts = payload.split(";")
        if len(parts) == 8:
            datum, vrijeme, temperatura, vlaga, tlak, plin, iaq, buzzer = parts

            trenutni_podaci["datum"] = datum
            trenutni_podaci["vrijeme"] = vrijeme
            trenutni_podaci["temperatura"] = temperatura
            trenutni_podaci["vlaga"] = vlaga
            trenutni_podaci["tlak"] = tlak
            trenutni_podaci["otpor-plin"] = plin
            trenutni_podaci["iaq"] = iaq
            trenutni_podaci["buzzer"] = buzzer

            save_to_json()

client = mqtt.Client()
client.username_pw_set("sobomjer", "Cabbage-Preview-Handgun-Reactor")
client.on_message = on_message
print("Spajanje na broker...")

client.connect(MQTT_BROKER, 1883, 60)
client.subscribe(MQTT_TOPIC_FORMAT)

print("Čekam podatke... ")

client.loop_forever()