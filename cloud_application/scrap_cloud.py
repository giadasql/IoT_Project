import paho.mqtt.client as mqtt
import mysql.connector
import xml.etree.ElementTree as ET
import json
from datetime import datetime

# MySQL connection details
DB_CONFIG = {
    "host": "localhost",
    "user": "root",
    "password": "moka",
    "database": "scrap"
}

# MQTT configuration
BROKER_ADDRESS = "10.65.2.143"
BROKER_PORT = 1883
MQTT_TOPIC = "bins"
CONFIG_REQUEST_TOPIC = "config/request"
CONFIG_RESPONSE_TOPIC = "config/response"

# In-memory state storage
bins_state = {}
bins_config = {}

# Load configuration from XML
def load_config_from_xml(xml_file):
    """Load bin configuration from an XML file."""
    global bins_config
    tree = ET.parse(xml_file)
    root = tree.getroot()
    for bin_element in root.findall("bin"):
        bin_id = bin_element.get("id")
        bins_config[bin_id] = {
            "collector_address": bin_element.find("collector_address").text,
            "lid_server_address": bin_element.find("lid_server_address").text,
            "compactor_server_address": bin_element.find("compactor_server_address").text,
            "scale_server_address": bin_element.find("scale_server_address").text,
            "waste_level_server_address": bin_element.find("waste_level_server_address").text,
            "compactor_actuator_address": bin_element.find("compactor_actuator_address").text,
            "lid_actuator_address": bin_element.find("lid_actuator_address").text
        }
    print(f"Loaded configuration for {len(bins_config)} bins.")

# Handle configuration requests
def handle_config_request(request):
    """Handle an incoming configuration request."""
    collector_address = request.get("collector_address")
    for bin_id, config in bins_config.items():
        if config["collector_address"] == collector_address:
            response = {"collector_address": collector_address, "bin_id": bin_id}
            response.update(config)
            return response
    return None


# Connect to the database
def connect_to_db():
    """Establish a connection to the MySQL database."""
    return mysql.connector.connect(**DB_CONFIG)

# Update the current state in the database
def update_current_state(cursor, bin_id, data):
    """Update the current state of the bin in the database."""
    query = """
        INSERT INTO bins_current_state (bin_id, lid_state, compactor_state, waste_level, scale_weight)
        VALUES (%s, %s, %s, %s, %s)
        ON DUPLICATE KEY UPDATE
            lid_state = VALUES(lid_state),
            compactor_state = VALUES(compactor_state),
            waste_level = VALUES(waste_level),
            scale_weight = VALUES(scale_weight),
            last_updated = CURRENT_TIMESTAMP;
    """
    cursor.execute(query, (
        bin_id,
        data.get("lid_sensor", {}).get("value"),
        data.get("compactor_sensor", {}).get("value"),
        data.get("waste_level_sensor", {}).get("value"),
        data.get("scale", {}).get("value")
    ))

# Log changes in the database
def log_changes(cursor, bin_id, changes):
    """Log changes to the bin's state."""
    query = """
        INSERT INTO bins_change_log (bin_id, sensor_name, new_value, change_timestamp)
        VALUES (%s, %s, %s, %s);
    """
    for sensor_name, new_value in changes.items():
        cursor.execute(query, (bin_id, sensor_name, new_value, datetime.now()))

# Handle incoming MQTT messages
def on_message(client, userdata, msg):
    """Handle incoming MQTT messages."""
    print(f"Received message on topic {msg.topic}: {msg.payload.decode()}")
    try:
        # Parse JSON payload
        data = json.loads(msg.payload.decode())

        # Handle configuration requests
        if msg.topic == CONFIG_REQUEST_TOPIC:
            handle_config_request_message(client, data)
            return

        # Handle sensor updates
        if msg.topic == MQTT_TOPIC:
            handle_sensor_update(client, data)
            return

        print(f"Unhandled message on topic {msg.topic}.")
    except Exception as e:
        print(f"Error processing message: {e}")


# Handle configuration request messages
def handle_config_request_message(client, data):
    """Handle a configuration request message."""
    collector_address = data.get("collector_address")
    if not collector_address:
        print("No collector_address found in configuration request. Skipping...")
        return

    # Fetch configuration for the collector
    response = handle_config_request(data)
    if response:
        client.publish(CONFIG_RESPONSE_TOPIC, json.dumps(response))
        print(f"Published configuration: {response}")
    else:
        print(f"No configuration found for collector_address: {collector_address}")


# Handle sensor update messages
def handle_sensor_update(client, data):
    """Handle a sensor update message."""
    bin_id = data.get("bin_id")
    if not bin_id:
        print("No bin_id found in update message. Skipping...")
        return

    # Check the in-memory state
    prev_state = bins_state.get(bin_id, {})
    changes = {}

    # Detect changes by comparing new values with the in-memory state
    sensors = {
        "lid_sensor": data.get("lid_sensor", {}).get("value"),
        "compactor_sensor": data.get("compactor_sensor", {}).get("value"),
        "waste_level_sensor": data.get("waste_level_sensor", {}).get("value"),
        "scale": data.get("scale", {}).get("value")
    }

    for sensor, value in sensors.items():
        if value is not None and str(value) != str(prev_state.get(sensor)):
            changes[sensor] = value

    # If there are changes, update the database and in-memory state
    if changes or bin_id not in bins_state:
        try:
            # Connect to MySQL
            db = connect_to_db()
            cursor = db.cursor()

            # Update the current state and log changes
            update_current_state(cursor, bin_id, data)
            log_changes(cursor, bin_id, changes)

            # Commit changes to the database
            db.commit()
        except Exception as db_error:
            print(f"Database error: {db_error}")
        finally:
            db.close()

        # Update in-memory state
        bins_state[bin_id] = sensors

        print(f"Database updated for bin {bin_id}. Changes: {changes}")
    else:
        print(f"No changes detected for bin {bin_id}. Skipping database update.")


# Main function
def main():
    load_config_from_xml("config.xml")  # Load configuration from the XML file
    client = mqtt.Client()
    client.on_connect = lambda c, u, f, rc: client.subscribe([(MQTT_TOPIC, 0), (CONFIG_REQUEST_TOPIC, 0)])
    client.on_message = on_message

    client.connect(BROKER_ADDRESS, BROKER_PORT, 60)
    client.loop_forever()

if __name__ == "__main__":
    main()
