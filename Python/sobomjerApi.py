from flask import Flask, jsonify
from flask_cors import CORS
import sqlite3

DB_FILE = "sobomjer.db"

app = Flask(__name__)
CORS(app)

def get_latest():
    conn = sqlite3.connect(DB_FILE)
    conn.row_factory = sqlite3.Row # svojstvo konekcije da prosljeduje i imena stupaca tablice zajedno sa vrijednostima umjesto obicne torke
    cur = conn.cursor()
    sql = "SELECT * FROM mjerenja ORDER BY id DESC LIMIT 1"
    cur.execute(sql)
    row = cur.fetchone()
    conn.close()
    
    if (row is None):
        return
    else:
        return dict(row)

@app.route("/latest")
def latest():
    reading = get_latest()
    if (reading is None): 
        return jsonify({"greska": "nema podataka"}), 404
    else:
        return jsonify(reading)

app.run(host="0.0.0.0", port=5000)