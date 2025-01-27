from flask import Flask, render_template, jsonify, request
import mysql.connector
from apscheduler.schedulers.background import BackgroundScheduler
import logging
from coapthon.client.helperclient import HelperClient
import xml.etree.ElementTree as ET

# Configuration
DATABASE_CONFIG = {
    'host': 'localhost',
    'user': 'iot-project',
    'password': 'iot-password',
    'database': 'scrap'
}

WASTE_LEVEL_THRESHOLD = 80.0  # Set your waste level threshold here
POLLING_INTERVAL = 2  # Polling interval in seconds

# Parse the config.xml file to get CoAP server addresses
def parse_config_xml():
    tree = ET.parse('config.xml')
    root = tree.getroot()
    bins = {}
    for bin in root.findall('bin'):
        bin_id = bin.get('id')
        bins[bin_id] = {
            'collector_address': bin.find('collector_address').text,
            'lid_server_address': bin.find('lid_server_address').text,
            'lid_actuator_address': bin.find('lid_actuator_address').text,
            'compactor_server_address': bin.find('compactor_server_address').text,
            'scale_server_address': bin.find('scale_server_address').text,
            'waste_level_server_address': bin.find('waste_level_server_address').text,
            'compactor_actuator_address': bin.find('compactor_actuator_address').text,
        }
    return bins

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

def fetch_transaction_data():
    try:
        connection = mysql.connector.connect(**DATABASE_CONFIG)
        cursor = connection.cursor(dictionary=True)
        cursor.execute("SELECT * FROM bins_rfid_transactions")
        transactions = cursor.fetchall()
        cursor.close()
        connection.close()
        return transactions
    except mysql.connector.Error as err:
        logging.error(f"Database error: {err}")
        return []

# Function to send a CoAP PUT request using CoAPthon3
def send_coap_put_request(bin_id, path, payload):
    bins = parse_config_xml()
    if bin_id not in bins:
        logging.error(f"Bin ID {bin_id} not found in config.xml")
        return False

    coap_server_address = None
    if path.startswith('/compactor'):
        coap_server_address = (bins[bin_id]['compactor_server_address'], 5683)
    elif path.startswith('/lid'):
        coap_server_address = (bins[bin_id]['lid_server_address'], 5683)
    elif path.startswith('/scale'):
        coap_server_address = (bins[bin_id]['scale_server_address'], 5683)
    elif path.startswith('/waste'):
        coap_server_address = (bins[bin_id]['waste_level_server_address'], 5683)

    if not coap_server_address:
        logging.error(f"No CoAP server address found for path {path}")
        return False

    try:
        client = HelperClient(server=coap_server_address)
        response = client.put(path, payload)
        client.stop()
        logging.info(f"CoAP PUT request sent to {path}. Response: {response.payload}")
        return True
    except Exception as e:
        logging.error(f"CoAP request failed: {e}")
        return False

# Function to check waste level and log if above threshold
def check_waste_level():
    bins = fetch_bin_data()
    for bin in bins:
        if bin['waste_level'] > WASTE_LEVEL_THRESHOLD:
            if bin['compactor_state'] != 'active':  # Check if compactor is not already active
                logging.warning(f"Waste level exceeded in bin {bin['bin_id']}: {bin['waste_level']}%")
                # Send CoAP PUT request to activate compactor
                send_coap_put_request(bin['bin_id'], "compactor/active", "true")
            else:
                logging.info(f"Compactor is already active for bin {bin['bin_id']}. Skipping CoAP request.")

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

@app.route('/api/transactions')
def get_transactions():
    transactions = fetch_transaction_data()
    return jsonify(transactions)

# Flask route to handle CoAP requests from the UI
@app.route('/coap', methods=['POST'])
def handle_coap_request():
    data = request.json
    bin_id = data.get('bin_id')
    path = data.get('path')
    payload = data.get('payload')

    if not bin_id or not path or not payload:
        return jsonify({'message': 'Bin ID, path, and payload are required'}), 400

    if send_coap_put_request(bin_id, path, payload):
        return jsonify({'message': 'CoAP request sent successfully'}), 200
    else:
        return jsonify({'message': 'CoAP request failed'}), 500

# Run the Flask app
if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)