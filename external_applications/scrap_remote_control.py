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

WASTE_LEVEL_THRESHOLD = 80.0  # Waste level threshold in percentage
POLLING_INTERVAL = 1  # Polling interval in seconds

# Parse the config.xml file to get CoAP server addresses
def parse_config_xml():
    tree = ET.parse('config.xml')
    root = tree.getroot()
    bins = {}
    for bin in root.findall('bin'):
        bin_id = bin.get('id')
        bins[bin_id] = {
            'collector_address': bin.find('collector_address').text,
            'lid_sensor_address': bin.find('lid_sensor_address').text,
            'lid_actuator_address': bin.find('lid_actuator_address').text,
            'compactor_sensor_address': bin.find('compactor_sensor_address').text,
            'scale_sensor_address': bin.find('scale_sensor_address').text,
            'waste_level_sensor_address': bin.find('waste_level_sensor_address').text,
            'compactor_actuator_address': bin.find('compactor_actuator_address').text,
        }
    return bins

# Initialize Flask app
app = Flask(__name__)

# Initialize logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(message)s')

# Function to fetch the current state from the database
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

# Function to fetch transaction data from the database
def fetch_transaction_data():
    try:
        connection = mysql.connector.connect(**DATABASE_CONFIG)
        cursor = connection.cursor(dictionary=True)
        cursor.execute("SELECT * FROM bins_rfid_transactions ORDER BY end_time DESC")
        transactions = cursor.fetchall()
        cursor.close()
        connection.close()
        return transactions
    except mysql.connector.Error as err:
        logging.error(f"Database error: {err}")
        return []

# Function to send CoAP PUT requests
def send_coap_put_request(bin_id, path, payload):
    bins = parse_config_xml()
    if bin_id not in bins:
        logging.error(f"Bin ID {bin_id} not found in config.xml")
        return False

    # Get CoAP server address based on the path
    coap_sensor_address = None
    if path.startswith('/compactor/config') or path.startswith('/compactor/command'):
        coap_sensor_address = (bins[bin_id]['compactor_actuator_address'], 5683)
    elif path.startswith('/lid/config') or path.startswith('/lid/command'):
        coap_sensor_address = (bins[bin_id]['lid_actuator_address'], 5683)
    elif path.startswith('/compactor'):
        coap_sensor_address = (bins[bin_id]['compactor_sensor_address'], 5683)
    elif path.startswith('/lid'):
        coap_sensor_address = (bins[bin_id]['lid_sensor_address'], 5683)
    elif path.startswith('/scale'):
        coap_sensor_address = (bins[bin_id]['scale_sensor_address'], 5683)
    elif path.startswith('/waste'):
        coap_sensor_address = (bins[bin_id]['waste_level_sensor_address'], 5683)

    if not coap_sensor_address:
        logging.error(f"No CoAP server address found for path {path}")
        return False

    try:
        client = HelperClient(server=coap_sensor_address)
        response = client.put(path, payload)
        client.stop()
        logging.info(f"CoAP PUT request sent to {path}. Response: {response.payload}")
        return True
    except Exception as e:
        logging.error(f"CoAP request failed: {e}")
        return False

# Function to insert an alarm into the database
def insert_alarm(bin_id, message):
    try:
        connection = mysql.connector.connect(**DATABASE_CONFIG)
        cursor = connection.cursor()
        cursor.execute("INSERT INTO alarms (bin_id, time, message, acknowledged) VALUES (%s, NOW(), %s, FALSE)", (bin_id, message))
        connection.commit()
        cursor.close()
        connection.close()
    except mysql.connector.Error as err:
        logging.error(f"Database error: {err}")

# Dictionary to store the compactor state for each bin, used to determine if an alarm should be inserted
compactor_states = {}

# Function to check the waste level of each bin and activate the compactor if the waste level exceeds the threshold
# Also inserts an alarm into the database if the compactor is turned off while the waste level exceeds the threshold
def check_waste_level():
    bins = fetch_bin_data()
    global compactor_states
    
    for bin in bins:
        if bin['bin_id'] not in compactor_states:
            compactor_states[bin['bin_id']] = bin['compactor_state']
        
        # Check if waste level exceeds the threshold
        if bin['waste_level'] > WASTE_LEVEL_THRESHOLD:
            if bin['compactor_state'] == "off" and compactor_states[bin['bin_id']] == "on": # compactor just turned off
                # check if there is an alarm for this bin already, if not insert one
                alarms = fetch_alarm_data()
                alarm_exists = False
                for alarm in alarms:
                    if alarm['bin_id'] == bin['bin_id'] and not alarm['acknowledged']:
                        alarm_exists = True
                        break
                if not alarm_exists:
                    insert_alarm(bin['bin_id'], f"Waste level exceeded in bin {bin['bin_id']}: {bin['waste_level']}%")
                    logging.warning(f"Compactor turned off while waste level exceeded in bin {bin['bin_id']}. Alarm inserted into database.")
                    
            if bin['compactor_state'] != 'on':  # Check if compactor is not already active
                logging.warning(f"Waste level exceeded in bin {bin['bin_id']}: {bin['waste_level']}%")
                # Send CoAP PUT request to activate compactor
                send_coap_put_request(bin['bin_id'], "/compactor/command", "turn on")
            else:
                logging.info(f"Compactor is already active for bin {bin['bin_id']}. Skipping CoAP request.")
        
        # Update compactor state
        compactor_states[bin['bin_id']] = bin['compactor_state']

# send configuration over coap to compactor actuator and lid actuator (FOR SIMULATION PURPOSES ONLY)
def send_configuration_over_coap():
    bins = parse_config_xml()
    for bin_id, bin_data in bins.items():
        # Send configuration to compactor actuator
        send_coap_put_request(bin_id, "/compactor/config", f"{bin_data['compactor_sensor_address']}")
        # Send configuration to lid actuator
        send_coap_put_request(bin_id, "/lid/config", f"{bin_data['lid_sensor_address']}")
        
# Send configuration to actuators on startup
send_configuration_over_coap()

# Scheduled task for polling
scheduler = BackgroundScheduler()
scheduler.add_job(check_waste_level, 'interval', seconds=POLLING_INTERVAL)
scheduler.start()

# Flask route to fetch current bin data
@app.route('/api/bins')
def get_bins():
    bins = fetch_bin_data()
    return jsonify(bins)

# Function to fetch transactions from the database
@app.route('/api/transactions')
def get_transactions():
    transactions = fetch_transaction_data()
    return jsonify(transactions)

# Flask route to send CoAP requests from the UI
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


# Function to fetch alarms from the database
def fetch_alarm_data():
    try:
        connection = mysql.connector.connect(**DATABASE_CONFIG)
        cursor = connection.cursor(dictionary=True)
        cursor.execute("SELECT * FROM alarms ORDER BY time DESC")
        alarms = cursor.fetchall()
        cursor.close()
        connection.close()
        return alarms
    except mysql.connector.Error as err:
        logging.error(f"Database error: {err}")
        return []

# Flask route to serve alarm data as JSON
@app.route('/api/alarms')
def get_alarms():
    alarms = fetch_alarm_data()
    return jsonify(alarms)

# Flask route to acknowledge an alarm
@app.route('/api/acknowledge_alarm', methods=['POST'])
def acknowledge_alarm():
    data = request.json
    alarm_id = data.get('alarm_id')

    if not alarm_id:
        return jsonify({'message': 'Alarm ID is required'}), 400

    try:
        connection = mysql.connector.connect(**DATABASE_CONFIG)
        cursor = connection.cursor()
        cursor.execute("UPDATE alarms SET acknowledged = TRUE WHERE alarm_id = %s", (alarm_id,))
        connection.commit()
        cursor.close()
        connection.close()
        return jsonify({'message': 'Alarm acknowledged successfully'}), 200
    except mysql.connector.Error as err:
        logging.error(f"Database error: {err}")
        return jsonify({'message': 'Failed to acknowledge alarm'}), 500


# Run the Flask app
if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)