import paho.mqtt.client as mqtt
import json
import time

# MQTT Configuration
BROKER_ADDRESS = "10.65.2.143"  # Replace with the IP address of your MQTT broker
BROKER_PORT = 1883
CONFIG_REQUEST_TOPIC = "config/request"
CONFIG_RESPONSE_TOPIC = "config/response"

# Configuration data mapped by collector_id
CONFIGURATIONS = {
    "coap_to_mqtt_01": {
        "lid_server_address": "fe80::202:2:2:2",
        "compactor_server_address": "fe80::205:5:5:5",
        "scale_server_address": "fe80::206:6:6:6",
        "waste_level_server_address": "fe80::207:7:7:7"
    },
    "coap_to_mqtt_02": {
        "lid_server_address": "fe80::203:3:3:3",
        "compactor_server_address": "fe80::206:6:6:6",
        "scale_server_address": "fe80::210:10:10:10",
        "waste_level_server_address": "fe80::211:11:11:11"
    }
    # Add more configurations as needed
}

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to MQTT broker")
        client.subscribe(CONFIG_REQUEST_TOPIC)
        print(f"Subscribed to topic: {CONFIG_REQUEST_TOPIC}")
    else:
        print(f"Failed to connect, return code {rc}")

def on_message(client, userdata, msg):
    print(f"Received message on topic {msg.topic}: {msg.payload.decode()}")
    try:
        # Parse the incoming request
        request = json.loads(msg.payload.decode())
        collector_id = request.get("collector_id")

        if collector_id and collector_id in CONFIGURATIONS:
            # Retrieve the appropriate configuration and add the collector_id
            response = {"collector_id": collector_id}
            response.update(CONFIGURATIONS[collector_id])

            # Publish the response
            client.publish(CONFIG_RESPONSE_TOPIC, json.dumps(response))
            print(f"Published response to topic {CONFIG_RESPONSE_TOPIC}: {response}")
        else:
            print(f"Invalid or unknown collector_id: {collector_id}")
    except json.JSONDecodeError:
        print("Invalid JSON payload")

def main():
    # Create MQTT client
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message

    # Connect to the broker
    print(f"Connecting to MQTT broker at {BROKER_ADDRESS}:{BROKER_PORT}")
    client.connect(BROKER_ADDRESS, BROKER_PORT, 60)

    # Start the MQTT loop
    try:
        client.loop_start()
        while True:
            time.sleep(1)  # Keep the program running
    except KeyboardInterrupt:
        print("Shutting down MQTT client")
        client.loop_stop()
        client.disconnect()

if __name__ == "__main__":
    main()
