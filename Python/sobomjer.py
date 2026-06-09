import paho.mqtt.client as mqtt
import sqlite3

MQTT_BROKER = "localhost"
MQTT_TOPIC_FORMAT = "sobomjer/format"
DB_FILE = "sobomjer.db"

conn = sqlite3.connect(DB_FILE, check_same_thread=False)
cur = conn.cursor()
sql = """
CREATE TABLE IF NOT EXISTS mjerenja (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    datum TEXT,
    vrijeme TEXT,
    temperatura REAL,
    vlaga REAL,
    tlak REAL,
    plin REAL,
    iaq INTEGER,
    buzzer INTEGER
)
"""
cur.execute(sql)
conn.commit()

def on_message(client, userdata, msg):
    if msg.topic != MQTT_TOPIC_FORMAT:
        return

    payload = msg.payload.decode()
    parts = payload.split(";")

    if len(parts) != 8:
        return
    else:
        datum, vrijeme, temperatura, vlaga, tlak, plin, iaq, buzzer = parts

    sql = """
    INSERT INTO mjerenja (datum, vrijeme, temperatura, vlaga, tlak, plin, iaq, buzzer)
    VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    """
    values = (
        str(datum),
        str(vrijeme),
        float(temperatura),
        float(vlaga),
        float(tlak),
        float(plin),
        int(iaq),
        int(buzzer)
    )
    cur.execute(sql, values)
    conn.commit()

client = mqtt.Client()
client.username_pw_set("sobomjer", "")
client.on_message = on_message

print("Spajanje na MQTT broker..")
client.connect(MQTT_BROKER, 1883, 60)
client.subscribe(MQTT_TOPIC_FORMAT)
print("Spojen")

print("Podaci se spremaju u sobomjer.db")

client.loop_forever()