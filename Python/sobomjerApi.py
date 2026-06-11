from flask import Flask, jsonify
from flask_cors import CORS
from datetime import datetime, timedelta
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

def get_daily():
    conn = sqlite3.connect(DB_FILE)
    conn.row_factory = sqlite3.Row
    cur = conn.cursor()

    date = datetime.now() - timedelta(days=1)
    date_str = date.strftime("%d-%m-%Y")

    sql = "SELECT * FROM mjerenja WHERE datum = ? ORDER BY vrijeme"
    cur.execute(sql, (date_str,))
    rows = cur.fetchall()
    conn.close()

    return [dict(row) for row in rows]

@app.route("/latest")
def latest():
    reading = get_latest()
    if (reading is None): 
        return jsonify({"greska": "nema podataka"}), 404
    else:
        return jsonify(reading)

@app.route("/daily")
def daily():
    readings = get_daily()
    return jsonify(readings)

app.run(host="0.0.0.0", port=5000)