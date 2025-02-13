#include "contiki.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include "mqtt.h"
#include "net/routing/routing.h"
#include "net/ipv6/uip.h"
#include "net/ipv6/uiplib.h"
#include "net/ipv6/uip-ds6.h"
#include "sys/etimer.h"
#include "sys/ctimer.h"
#include "dev/leds.h"
#include "jsmn.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>

#define LOG_MODULE "CoAP-to-MQTT"
#define LOG_LEVEL LOG_LEVEL_DBG

// MQTT broker configuration
#define MQTT_CLIENT_BROKER_IP_ADDR "fd00::1"
#define DEFAULT_BROKER_PORT 1883
#define CONFIG_REQUEST_TOPIC "config/request"
#define CONFIG_RESPONSE_TOPIC "config/response"
static char pub_msg[1024];
static char client_id[64];
static struct mqtt_connection conn;

// CoAP endpoints for all sensors that the collector will interact with
static coap_endpoint_t lid_sensor_endpoint;
static coap_endpoint_t compactor_sensor_endpoint;
static coap_endpoint_t scale_sensor_endpoint;
static coap_endpoint_t waste_level_sensor_endpoint;

static char compactor_sensor_uri[64] = {0};
static char lid_sensor_uri[64] = {0};
static char waste_level_sensor_uri[64] = {0};
static char scale_sensor_uri[64] = {0};

static coap_message_t request[1];

// Variables to store the bin ID and local IPv6 address
static char bin_id[64] = "unknown";
static char local_ipv6_address[64];

// State machine
static uint8_t state;
#define STATE_INIT 0
#define STATE_NET_OK 1
#define STATE_CONFIG_REQUEST 2
#define STATE_CONFIG_RECEIVED 3
#define STATE_CONNECTING 4
#define STATE_CONNECTED 5
#define STATE_DISCONNECTED 6

// Timer for the main process loop
static struct etimer periodic_timer;
static struct etimer advertise_timer;

// Structs to save sensor data
typedef struct {
    char value[64];
} sensor_data_t;

typedef struct {
    sensor_data_t lid_sensor;
    sensor_data_t compactor_sensor;
    sensor_data_t waste_level_sensor;
    sensor_data_t scale;
    sensor_data_t rfid;
} collector_data_t;

static collector_data_t collector_data;


PROCESS(mqtt_collector_process, "MQTT Collector Process");
AUTOSTART_PROCESSES(&mqtt_collector_process);

// Helper Function: Check if a token is equal to a string
static int jsmn_token_equals(const char *json, const jsmntok_t *tok, const char *key)
{
  return (tok->type == JSMN_STRING &&
          (int)strlen(key) == tok->end - tok->start &&
          strncmp(json + tok->start, key, tok->end - tok->start) == 0);
}

// Handler for configuration response
static void configuration_received_handler(const char *topic, uint16_t topic_len, const uint8_t *chunk, uint16_t chunk_len) {
    printf("Pub Handler: topic='%s' (len=%u), chunk_len=%u\n", topic, topic_len, chunk_len);

    // Check if the topic is the configuration response topic
    if (strcmp(topic, CONFIG_RESPONSE_TOPIC) != 0) {
        return;
    }

    // variables to store the received addresses
    char received_collector_address[64] = {0};
    char received_bin_id[64] = {0};
    char lid_sensor_address[64] = {0};
    char compactor_sensor_address[64] = {0};
    char scale_sensor_address[64] = {0};
    char waste_level_sensor_address[64] = {0};

    // Mapping between JSON keys and variables
    struct {
        char *key;
        char *buffer;
        size_t size;
    } config_mappings[] = {
        {"collector_address", received_collector_address, sizeof(received_collector_address)},
        {"bin_id", received_bin_id, sizeof(received_bin_id)},
        {"lid_sensor_address", lid_sensor_address, sizeof(lid_sensor_address)},
        {"compactor_sensor_address", compactor_sensor_address, sizeof(compactor_sensor_address)},
        {"scale_sensor_address", scale_sensor_address, sizeof(scale_sensor_address)},
        {"waste_level_sensor_address", waste_level_sensor_address, sizeof(waste_level_sensor_address)}
    };

    // Parse the JSON response using JSMN
    jsmn_parser parser;
    jsmntok_t tokens[32];
    jsmn_init(&parser);
    int token_count = jsmn_parse(&parser, (const char *)chunk, chunk_len, tokens, sizeof(tokens) / sizeof(tokens[0]));

    if (token_count < 1 || tokens[0].type != JSMN_OBJECT) {
        printf("Invalid JSON format\n");
        return;
    }

    for (int i = 1; i < token_count - 1; i++) {
        for (size_t j = 0; j < sizeof(config_mappings) / sizeof(config_mappings[0]); j++) {
          // if the token is equal to the key, copy the value to the buffer
            if (jsmn_token_equals((const char *)chunk, &tokens[i], config_mappings[j].key)) {
                size_t len = tokens[i + 1].end - tokens[i + 1].start;
                len = len < config_mappings[j].size ? len : config_mappings[j].size - 1;
                // Copy the value to the buffer
                strncpy(config_mappings[j].buffer, (const char *)chunk + tokens[i + 1].start, len);
                config_mappings[j].buffer[len] = '\0';
                i++; // Skip the value token
                break;
            }
        }
    }

    // if the message is for this collector, update the configuration
    if (strcmp(received_collector_address, local_ipv6_address) == 0) {
        printf("Received configuration for Bin ID: %s\n", received_bin_id);
        printf("Lid Sensor Address: %s\n", lid_sensor_address);
        printf("Compactor Sensor Address: %s\n", compactor_sensor_address);
        printf("Scale Sensor Address: %s\n", scale_sensor_address);
        printf("Waste Level Sensor Address: %s\n", waste_level_sensor_address);

        strncpy(bin_id, received_bin_id, sizeof(bin_id));

        coap_endpoint_parse(lid_sensor_address, strlen(lid_sensor_address), &lid_sensor_endpoint);
        coap_endpoint_parse(compactor_sensor_address, strlen(compactor_sensor_address), &compactor_sensor_endpoint);
        coap_endpoint_parse(scale_sensor_address, strlen(scale_sensor_address), &scale_sensor_endpoint);
        coap_endpoint_parse(waste_level_sensor_address, strlen(waste_level_sensor_address), &waste_level_sensor_endpoint);

        strncpy(compactor_sensor_uri, compactor_sensor_address, sizeof(compactor_sensor_uri));
        strncpy(lid_sensor_uri, lid_sensor_address, sizeof(lid_sensor_uri));
        strncpy(scale_sensor_uri, scale_sensor_address, sizeof(scale_sensor_uri));
        strncpy(waste_level_sensor_uri, waste_level_sensor_address, sizeof(waste_level_sensor_uri));

        state = STATE_CONFIG_RECEIVED;
    } else {
        printf("Response is not for this collector. Ignored.\n");
    }
}

// Handler for MQTT events
static void mqtt_event(struct mqtt_connection *m, mqtt_event_t event, void *data)
{
  switch(event) {
    case MQTT_EVENT_CONNECTED:
      printf("Application has a MQTT connection\n");
      state = STATE_CONNECTED;
      break;

    case MQTT_EVENT_DISCONNECTED:
      printf("MQTT Disconnect. Reason %u\n", *((mqtt_event_t *)data));
      state = STATE_DISCONNECTED;
      process_poll(&mqtt_collector_process);
      break;

    case MQTT_EVENT_PUBLISH:
      {
      	printf("Received MQTT message\n");
        struct mqtt_message *msg = data;
        configuration_received_handler(msg->topic, strlen(msg->topic), msg->payload_chunk, msg->payload_length);
      }
      break;

    case MQTT_EVENT_SUBACK:
      printf("Subscribed to topic successfully\n");
      break;

    case MQTT_EVENT_UNSUBACK:
      printf("Unsubscribed from topic successfully\n");
      break;

    case MQTT_EVENT_PUBACK:
      printf("Publishing complete.\n");
      break;

    default:
      printf("Unhandled MQTT event: %i\n", event);
      break;
  }
}

// Helper function to parse the JSON payload for sensor data received from CoAP
static void parse_sensor_read_payload(const uint8_t *payload, const char *sensor_name, sensor_data_t *sensor_data) {
    jsmn_parser parser;
    jsmntok_t tokens[16];
    jsmn_init(&parser);
    int token_count = jsmn_parse(&parser, (const char *)payload, strlen((char *)payload), tokens, 16);

    if (token_count > 0 && tokens[0].type == JSMN_OBJECT) {
        for (int i = 1; i < token_count; i++) {
            if (jsmn_token_equals((char *)payload, &tokens[i], "value")) {
                snprintf(sensor_data->value, sizeof(sensor_data->value), "%.*s",
                         tokens[i + 1].end - tokens[i + 1].start,
                         (char *)payload + tokens[i + 1].start);
                printf("%s updated to: %s\n", sensor_name, sensor_data->value);
                break;
            }
        }
    } else {
        printf("Failed to parse JSON payload for %s.\n", sensor_name);
    }
}

// Callback functions for CoAP client requests
static void client_callback(coap_message_t *response, sensor_data_t *sensor, const char *sensor_name) {
    if (response) {
        parse_sensor_read_payload(response->payload, sensor_name, sensor);
    } else {
        printf("CoAP request for %s timed out.\n", sensor_name);
    }
}

static void lid_sensor_callback(coap_message_t *response) {
    client_callback(response, &collector_data.lid_sensor, "Lid Sensor");
}

static void compactor_sensor_callback(coap_message_t *response) {
    client_callback(response, &collector_data.compactor_sensor, "Compactor Sensor");
}

static void scale_sensor_callback(coap_message_t *response) {
    client_callback(response, &collector_data.scale, "Scale Sensor");
}

static void waste_level_sensor_callback(coap_message_t *response) {
    client_callback(response, &collector_data.waste_level_sensor, "Waste Level Sensor");
}

static void rfid_callback(coap_message_t *response) {
    client_callback(response, &collector_data.rfid, "RFID");
}

// Helper function to check if the collector has network connectivity
static bool have_connectivity(void) {
  return uip_ds6_get_global(ADDR_PREFERRED) != NULL && uip_ds6_defrt_choose() != NULL;
}

// Publish Aggregated MQTT Message with all sensor data
static void send_aggregated_mqtt_message(void) {
    setlocale(LC_NUMERIC, "C");

    snprintf(pub_msg, sizeof(pub_msg),
         "{\"bin_id\":\"%s\","
         "\"rfid\":\"%s\","
         "\"lid_sensor\":\"%s\","
         "\"compactor_sensor\":\"%s\","
         "\"scale\":\"%s\","
         "\"waste_level_sensor\":\"%s\"}",
         bin_id,
         collector_data.rfid.value,
         strcmp(collector_data.lid_sensor.value, "true") == 0 ? "open" : "closed",
         strcmp(collector_data.compactor_sensor.value, "true") == 0 ? "on" : "off",
         collector_data.scale.value,
         collector_data.waste_level_sensor.value);


    mqtt_publish(&conn, NULL, "bins", (uint8_t *)pub_msg, strlen(pub_msg), MQTT_QOS_LEVEL_0, MQTT_RETAIN_OFF);
    printf("Published aggregated data to MQTT: %s\n", pub_msg);
}

// Helper Function to get the local IPv6 address
static void get_local_ipv6_address(char *buffer, size_t buffer_size) {
    uip_ds6_addr_t *addr = NULL;

    // Iterate through all IPv6 addresses
    for (addr = uip_ds6_if.addr_list; addr < uip_ds6_if.addr_list + UIP_DS6_ADDR_NB; addr++) {
        // Check if the address is used and is a link-local address
        if (addr->isused && uip_is_addr_linklocal(&addr->ipaddr)) {
            uiplib_ipaddr_snprint(buffer, buffer_size, &addr->ipaddr);
            return;
        }
    }

    // If no link-local address is found, set to "unknown"
    snprintf(buffer, buffer_size, "unknown");
}

// Main process
PROCESS_THREAD(mqtt_collector_process, ev, data)
{
  PROCESS_BEGIN();

  // Inizialize the MQTT connection
  snprintf(client_id, sizeof(client_id), "coap_to_mqtt_%02x%02x",
           linkaddr_node_addr.u8[6], linkaddr_node_addr.u8[7]);
  mqtt_register(&conn, &mqtt_collector_process, client_id, mqtt_event, 128);
  state = STATE_INIT;
  etimer_set(&periodic_timer, CLOCK_SECOND);

  // Get the local IPv6 address, it will be used to request the configuration for this device
  get_local_ipv6_address(local_ipv6_address, sizeof(local_ipv6_address));


  while(1) {
    PROCESS_YIELD();

    if((ev == PROCESS_EVENT_TIMER && data == &periodic_timer) || ev == PROCESS_EVENT_POLL) {
      if (state == STATE_INIT && have_connectivity()) {
        state = STATE_NET_OK;
      }

      if (state == STATE_NET_OK) {
        printf("Connecting to MQTT broker...\n");
        mqtt_connect(&conn, MQTT_CLIENT_BROKER_IP_ADDR, DEFAULT_BROKER_PORT, (30 * CLOCK_SECOND), MQTT_CLEAN_SESSION_ON);
        state = STATE_CONNECTING;
      }

      if (state == STATE_CONNECTED) {
		if (mqtt_subscribe(&conn, NULL, CONFIG_RESPONSE_TOPIC, MQTT_QOS_LEVEL_0) == MQTT_STATUS_OK) {
    		printf("Subscribed to topic: %s\n", CONFIG_RESPONSE_TOPIC);
    		state = STATE_CONFIG_REQUEST;
  		} else {
    		printf("Failed to subscribe to topic: %s\n", CONFIG_RESPONSE_TOPIC);
  		}
      }

      // Request the configuration via MQTT
      if (state == STATE_CONFIG_REQUEST) {
        printf("Requesting CoAP server configuration...\n");

  		// Publish configuration request message
 		 snprintf(pub_msg, sizeof(pub_msg), "{\"collector_address\":\"%s\"}", local_ipv6_address);

 		 mqtt_status_t status = mqtt_publish(&conn, NULL, CONFIG_REQUEST_TOPIC,
                                      (uint8_t *)pub_msg, strlen(pub_msg),
                                      MQTT_QOS_LEVEL_0, MQTT_RETAIN_OFF);

  		if (status == MQTT_STATUS_OK) {
  		  printf("Published to topic %s: %s\n", CONFIG_REQUEST_TOPIC, pub_msg);
  		} else {
   			 printf("Failed to publish. MQTT status: %d\n", status);
  		}
      }

	  if (state == STATE_CONFIG_RECEIVED) {
        // Read data from all the sensors
        printf("Fetching sensor states...\n");

        coap_init_message(request, COAP_TYPE_CON, COAP_GET, 0);
        coap_set_header_uri_path(request, "/compactor/active");
        COAP_BLOCKING_REQUEST(&compactor_sensor_endpoint, request, compactor_sensor_callback);

        coap_init_message(request, COAP_TYPE_CON, COAP_GET, 0);
        coap_set_header_uri_path(request, "/lid/open");
        COAP_BLOCKING_REQUEST(&lid_sensor_endpoint, request, lid_sensor_callback);

        coap_init_message(request, COAP_TYPE_CON, COAP_GET, 0);
        coap_set_header_uri_path(request, "/rfid/value");
        COAP_BLOCKING_REQUEST(&lid_sensor_endpoint, request, rfid_callback);

        coap_init_message(request, COAP_TYPE_CON, COAP_GET, 0);
        coap_set_header_uri_path(request, "/scale/value");
        COAP_BLOCKING_REQUEST(&scale_sensor_endpoint, request, scale_sensor_callback);

        coap_init_message(request, COAP_TYPE_CON, COAP_GET, 0);
        coap_set_header_uri_path(request, "/waste/level");
        COAP_BLOCKING_REQUEST(&waste_level_sensor_endpoint, request, waste_level_sensor_callback);

        send_aggregated_mqtt_message();
      }

      if (state == STATE_DISCONNECTED) {
        printf("Disconnected. Retrying...\n");
        state = STATE_INIT;
      }

      etimer_reset(&periodic_timer);
    }
  }

  PROCESS_END();
}
