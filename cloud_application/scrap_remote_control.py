from flask import Flask, render_template, jsonify
import mysql.connector
from apscheduler.schedulers.background import BackgroundScheduler
import logging
import asyncio
from aiocoap import Context, Message, Code

# Configuration
DATABASE_CONFIG = {
    'host': 'localhost',
    'user': 'iot-project',
    'password': 'iot-password',
    'database': 'scrap'
}

WASTE_LEVEL_THRESHOLD = 80.0  # Set your waste level threshold here
POLLING_INTERVAL = 10  # Polling interval in seconds
COAP_SERVER_ADDRESS = "coap://fd00::205:5:5:5/compactor/active"  # Replace with your CoAP server address

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

# Function to send a CoAP PUT request
async def send_coap_put_request():
    try:
        # Create CoAP context
        context = await Context.create_client_context()

        # Prepare the CoAP PUT request
        payload = b"true"  # Payload to turn compactor ON
        request = Message(code=Code.PUT, payload=payload, uri=COAP_SERVER_ADDRESS)

        # Send the request
        response = await context.request(request).response

        # Log the response
        logging.info(f"CoAP PUT request sent. Response: {response.payload.decode()}")
    except Exception as e:
        logging.error(f"CoAP request failed: {e}")

# Function to check waste level and log if above threshold
def check_waste_level():
    bins = fetch_bin_data()
    for bin in bins:
        if bin['waste_level'] > WASTE_LEVEL_THRESHOLD:
            logging.warning(f"Waste level exceeded in bin {bin['bin_id']}: {bin['waste_level']}%")
            # Send CoAP PUT request asynchronously
            asyncio.run(send_coap_put_request())

# Scheduled task for polling
scheduler = BackgroundScheduler()
scheduler.add_job(check_waste_level, 'interval', seconds=POLLING_INTERVAL)
scheduler.start()

# Flask route to display the current state of bins
@app.route('/')
def index():
    bins = fetch_bin_data()
    return render_template('index.html', bins=bins)

# Flask route to serve JSON data
@app.route('/api/bins')
def get_bins():
    bins = fetch_bin_data()
    return jsonify(bins)

# Run the Flask app
if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)