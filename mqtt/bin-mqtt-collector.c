#include "contiki.h"
#include "coap-engine.h"
#include "mqtt.h"
#include <string.h>
#include <stdio.h>

#define LOG_MODULE "CoAP-to-MQTT"
#define LOG_LEVEL LOG_LEVEL_INFO

// MQTT Configuration
#define BROKER_IP "10.65.2.143" // Replace with your broker's IP address
#define MQTT_PORT 1883
#define PUB_TOPIC "sensors/lid"

// CoAP Configuration
#define COAP_SERVER_IP "fe80::202:2:2:2"  // Replace with the lid-sensor's IP address
#define COAP_SERVER_PORT 5683
#define COAP_RESOURCE "/lid/state"

// MQTT connection object
static struct mqtt_connection mqtt_conn;
static char mqtt_client_id[32];
static char pub_msg[64];

// CoAP variables
static coap_endpoint_t server_endpoint;
static coap_message_t request[1];

// Timer for periodic requests
static struct etimer periodic_timer;

PROCESS(coap_to_mqtt_process, "CoAP-to-MQTT Bridge");
AUTOSTART_PROCESSES(&coap_to_mqtt_process);

// MQTT Event Callback
static void
mqtt_event(struct mqtt_connection *conn, mqtt_event_t event, void *data) {
  if (event == MQTT_EVENT_CONNECTED) {
    printf("MQTT connected to broker at %s\n", BROKER_IP);
  } else if (event == MQTT_EVENT_DISCONNECTED) {
    printf("MQTT disconnected. Attempting to reconnect...\n");
    mqtt_connect(&mqtt_conn, BROKER_IP, MQTT_PORT, 1000);
  }
}

// CoAP Response Callback
static void
coap_callback(coap_callback_request_state_t *callback_state) {
  if (callback_state->response) {
    char *response_payload = (char *)callback_state->response->payload;
    printf("CoAP Response: %s\n", response_payload);

    // Format the MQTT message
    snprintf(pub_msg, sizeof(pub_msg), "{\"lid_state\":%s}", response_payload);

    // Publish to MQTT
    mqtt_publish(&mqtt_conn, NULL, PUB_TOPIC, (uint8_t *)pub_msg, strlen(pub_msg), MQTT_QOS_LEVEL_0, MQTT_RETAIN_OFF);
    printf("Published to MQTT: %s\n", pub_msg);
  } else {
    printf("CoAP request failed or no response received.\n");
  }
}

PROCESS_THREAD(coap_to_mqtt_process, ev, data) {
  PROCESS_BEGIN();

  // Initialize MQTT
  snprintf(mqtt_client_id, sizeof(mqtt_client_id), "coap_to_mqtt_%02x%02x",
           linkaddr_node_addr.u8[6], linkaddr_node_addr.u8[7]);
  mqtt_register(&mqtt_conn, &coap_to_mqtt_process, mqtt_client_id, mqtt_event, 128);
  mqtt_connect(&mqtt_conn, BROKER_IP, MQTT_PORT, 1000);

  // Initialize CoAP
  coap_endpoint_parse(COAP_SERVER_IP, COAP_SERVER_PORT, &server_endpoint);

  // Start periodic timer
  etimer_set(&periodic_timer, CLOCK_SECOND * 10);

  while (1) {
    PROCESS_WAIT_EVENT();

    if (etimer_expired(&periodic_timer)) {
      // Create a CoAP GET request
      coap_init_message(request, COAP_TYPE_CON, COAP_GET, 0);
      coap_set_header_uri_path(request, COAP_RESOURCE);

      // Send the CoAP GET request
      coap_send_request(&server_endpoint, request, coap_callback);

      // Reset the timer
      etimer_reset(&periodic_timer);
    }
  }

  PROCESS_END();
}
