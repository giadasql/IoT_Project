import paho.mqtt.client as mqtt
import mysql.connector
import xml.etree.ElementTree as ET
import json
from datetime import datetime

# MySQL connection details
DB_CONFIG = {
    "host": "localhost",
    "user": "iot-project",
    "password": "iot-password",
    "database": "scrap"
}

# MQTT configuration
BROKER_ADDRESS = "10.65.2.143"
# BROKER_ADDRESS = "172.20.10.13"
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
        INSERT INTO bins_current_state (bin_id, rfid, lid_state, compactor_state, waste_level, scale_weight)
        VALUES (%s, %s, %s, %s, %s, %s)
        ON DUPLICATE KEY UPDATE
            rfid = VALUES(rfid),
            lid_state = VALUES(lid_state),
            compactor_state = VALUES(compactor_state),
            waste_level = VALUES(waste_level),
            scale_weight = VALUES(scale_weight),
            last_updated = CURRENT_TIMESTAMP;
    """
    cursor.execute(query, (
        bin_id,
        data.get("rfid"),
        data.get("lid_sensor"),
        data.get("compactor_sensor"),
        data.get("waste_level_sensor"),
        data.get("scale")
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

# Utility function to normalize decimal values
def normalize_decimal(value):
    """Convert a numeric string with commas to a proper decimal format."""
    if isinstance(value, str):
        return value.replace(",", ".")
    return value

# Track the start and end times of lid open/close states
def handle_sensor_update(client, data):
    """Handle a sensor update message."""
    bin_id = data.get("bin_id")
    if not bin_id:
        print("No bin_id found in update message. Skipping...")
        return

    rfid = data.get("rfid")
    scale_weight = normalize_decimal(data.get("scale"))
    waste_level = normalize_decimal(data.get("waste_level_sensor"))

    # Check the in-memory state
    prev_state = bins_state.get(bin_id, {})
    prev_lid_state = prev_state.get("lid_sensor")
    changes = {}

    # Detect changes by comparing new values with the in-memory state
    sensors = {
        "rfid": rfid,
        # set open or closed if true or false
        "lid_sensor": data.get("lid_sensor"),
        "compactor_sensor": data.get("compactor_sensor"),
        "waste_level_sensor": waste_level,
        "scale": scale_weight
    }

    for sensor, value in sensors.items():
        if value is not None and str(value) != str(prev_state.get(sensor)):
            changes[sensor] = value

    # Update the database and in-memory state if changes are detected
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
        bins_state[bin_id] = {**sensors, "timestamp": datetime.now()}

        print(f"Database updated for bin {bin_id}. Changes: {changes}")

    # If lid closes, process transaction
    if prev_lid_state == "open" and sensors["lid_sensor"] == "closed":
        try:
            # Connect to MySQL
            db = connect_to_db()
            cursor = db.cursor()

            # Get the most recent lid open time from the database
            lid_open_query = """
                SELECT change_timestamp
                FROM bins_change_log
                WHERE bin_id = %s AND sensor_name = 'lid_sensor' AND new_value = 'open'
                ORDER BY change_timestamp DESC
                LIMIT 1;
            """
            cursor.execute(lid_open_query, (bin_id,))
            lid_open_record = cursor.fetchone()
            
            lid_closed_query = """
                SELECT change_timestamp
                FROM bins_change_log
                WHERE bin_id = %s AND sensor_name = 'lid_sensor' AND new_value = 'closed'
                ORDER BY change_timestamp DESC
                LIMIT 1;
            """
            cursor.execute(lid_closed_query, (bin_id,))
            lid_closed_record = cursor.fetchone()

            if lid_open_record and lid_closed_record:
                lid_open_time = lid_open_record[0]
                lid_close_time = lid_closed_record[0]
                print(f"Lid opened for bin {bin_id} at {lid_open_time}")

                # Query the most recent weight before the lid opened
                weight_before_query = """
                    SELECT new_value, change_timestamp
                    FROM bins_change_log
                    WHERE bin_id = %s AND sensor_name = 'scale' AND change_timestamp < %s
                    ORDER BY change_timestamp DESC
                    LIMIT 1;
                """
                cursor.execute(weight_before_query, (bin_id, lid_open_time))
                weight_before = cursor.fetchone()

                if weight_before:
                    start_weight = float(weight_before[0])  # Weight just before the lid opened
                    start_time = weight_before[1]
                    print(f"Start weight for bin {bin_id}: {start_weight} at {start_time}")

                    # Query the latest weight when the lid closes
                    weight_after_query = """
                        SELECT new_value, change_timestamp
                        FROM bins_change_log
                        WHERE bin_id = %s AND sensor_name = 'scale' AND change_timestamp <= %s
                        ORDER BY change_timestamp DESC
                        LIMIT 1;
                    """
                    cursor.execute(weight_after_query, (bin_id, lid_close_time))
                    weight_after = cursor.fetchone()

                    if weight_after:
                        end_weight = float(weight_after[0])  # Weight when the lid closed
                        end_time = weight_after[1]
                        print(f"End weight for bin {bin_id}: {end_weight} at {end_time}")

                        # Calculate weight difference
                        weight_diff = end_weight - start_weight
                        
                        # get last valid rfid for the bin from the log
                        rfid_query = """
                            SELECT new_value
                            FROM bins_change_log
                            WHERE bin_id = %s AND sensor_name = 'rfid' AND new_value <> 'No value'
                            ORDER BY change_timestamp DESC
                            LIMIT 1;
                        """
                        
                        cursor.execute(rfid_query, (bin_id,))
                        rfid = cursor.fetchone()[0]

                        # Insert transaction record
                        transaction_query = """
                            INSERT INTO bins_rfid_transactions (rfid, bin_id, weight_diff, start_time, end_time)
                            VALUES (%s, %s, %s, %s, %s);
                        """
                        cursor.execute(transaction_query, (rfid, bin_id, weight_diff, start_time, end_time))
                        db.commit()

                        print(f"RFID transaction recorded for bin {bin_id} (RFID: {rfid}). Weight difference: {weight_diff}")
                    else:
                        print(f"No weight record found when the lid closed for bin {bin_id}.")
                else:
                    print(f"No weight record found before lid open time for bin {bin_id}.")
            else:
                print(f"No lid open record found for bin {bin_id}.")

        except Exception as query_error:
            print(f"Error processing weight records: {query_error}")
        finally:
            db.close()

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
