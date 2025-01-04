import paho.mqtt.client as mqtt
import json
import time

# MQTT Configuration
BROKER_ADDRESS = "192.168.0.97"  # Replace with the IP address of your MQTT broker
BROKER_PORT = 1883
CONFIG_REQUEST_TOPIC = "config/request"
CONFIG_RESPONSE_TOPIC = "config/response"

# Hardcoded configuration response
COLLECTOR_ID = "coap_to_mqtt_01"
COAP_SERVER_ADDRESS = "fe80::202:2:2:2"

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
        if "collector_id" in request:
            # Prepare the response
            response = {
                "collector_id": request["collector_id"],
                "coap_server_address": COAP_SERVER_ADDRESS
            }
            # Publish the response
            client.publish(CONFIG_RESPONSE_TOPIC, json.dumps(response))
            print(f"Published response to topic {CONFIG_RESPONSE_TOPIC}: {response}")
        else:
            print("Invalid request: 'collector_id' not found in request payload")
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
