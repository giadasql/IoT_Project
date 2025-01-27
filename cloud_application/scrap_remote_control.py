from flask import Flask, render_template, jsonify
import mysql.connector
from apscheduler.schedulers.background import BackgroundScheduler
import logging

# Configuration
DATABASE_CONFIG = {
    'host': 'localhost',
    'user': 'root',
    'password': 'moka',
    'database': 'scrap'
}

WASTE_LEVEL_THRESHOLD = 80.0  # Set your waste level threshold here
POLLING_INTERVAL = 10  # Polling interval in seconds

# Initialize Flask app
app = Flask(__name__)

# Initialize logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(message)s')

# Function to fetch data from the database
def fetch_bin_data():
    try:
        connection = mysql.connector.connect(**DATABASE_CONFIG)
        cursor = connection.cursor(dictionary=True)
        cursor.execute("SELECT * FROM bins_current_state")
        bins = cursor.fetchall()
        cursor.close()
        connection.close()
        return bins
    except mysql.connector.Error as err:
        logging.error(f"Database error: {err}")
        return []

# Function to check waste level and log if above threshold
def check_waste_level():
    bins = fetch_bin_data()
    for bin in bins:
        if bin['waste_level'] > WASTE_LEVEL_THRESHOLD:
            logging.warning(f"Waste level exceeded in bin {bin['bin_id']}: {bin['waste_level']}%")

# Scheduled task for polling
scheduler = BackgroundScheduler()
scheduler.add_job(check_waste_level, 'interval', seconds=POLLING_INTERVAL)
scheduler.start()

# Flask route to display the current state of bins
@app.route('/')
def index():
    bins = fetch_bin_data()
    return render_template('index.html', bins=bins)

@app.route('/api/bins')
def get_bins():
    bins = fetch_bin_data()
    return jsonify(bins)

# Run the Flask app
if __name__ == '__main__':
    app.run(debug=True)