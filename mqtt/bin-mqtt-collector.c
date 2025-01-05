#include "contiki.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include "mqtt.h"
#include "net/routing/routing.h"
#include "net/ipv6/uip.h"
#include "sys/etimer.h"
#include "sys/ctimer.h"
#include "dev/leds.h"
#include "jsmn.h"
#include "time_utils.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define LOG_MODULE "CoAP-to-MQTT"
#define LOG_LEVEL LOG_LEVEL_DBG

/* MQTT broker configuration */
#define MQTT_CLIENT_BROKER_IP_ADDR "fd00::1"
#define DEFAULT_BROKER_PORT 1883
#define PUB_TOPIC "sensors/lid"
#define CONFIG_REQUEST_TOPIC "config/request"
#define CONFIG_RESPONSE_TOPIC "config/response"

/* Unique identifier for this collector */
#define COLLECTOR_ID "coap_to_mqtt_01"

/* CoAP variables */
static coap_endpoint_t lid_server_endpoint;
static coap_endpoint_t compactor_server_endpoint;

static coap_message_t request[1];

/* MQTT variables */
static char client_id[64];
static char pub_msg[256];
static struct mqtt_connection conn;

static uint8_t state;

#define STATE_INIT 0
#define STATE_NET_OK 1
#define STATE_CONFIG_REQUEST 2
#define STATE_CONFIG_RECEIVED 3
#define STATE_CONNECTING 4
#define STATE_CONNECTED 5
#define STATE_DISCONNECTED 6

static struct etimer periodic_timer;

PROCESS(coap_to_mqtt_process, "CoAP-to-MQTT Bridge");
AUTOSTART_PROCESSES(&coap_to_mqtt_process);


/* Helper Function: Check if a token equals a string */
static int
jsmn_token_equals(const char *json, const jsmntok_t *tok, const char *key)
{
  return (tok->type == JSMN_STRING &&
          (int)strlen(key) == tok->end - tok->start &&
          strncmp(json + tok->start, key, tok->end - tok->start) == 0);
}

/* Publish Handler */
static void pub_handler(const char *topic, uint16_t topic_len, const uint8_t *chunk, uint16_t chunk_len) {
  printf("Pub Handler: topic='%s' (len=%u), chunk_len=%u\n", topic, topic_len, chunk_len);

  if (strcmp(topic, CONFIG_RESPONSE_TOPIC) == 0) {
    char received_collector_id[64] = {0};
    char lid_server_address[64] = {0};
    char compactor_server_address[64] = {0};

    jsmn_parser parser;
    jsmntok_t tokens[32]; // Adjust size based on expected tokens
    int token_count;

    // Initialize JSMN parser
    jsmn_init(&parser);
    token_count = jsmn_parse(&parser, (const char *)chunk, chunk_len, tokens, sizeof(tokens) / sizeof(tokens[0]));

    if (token_count < 0) {
      printf("Failed to parse JSON: %d\n", token_count);
      return;
    }

    /* Check if the root element is an object */
    if (token_count < 1 || tokens[0].type != JSMN_OBJECT) {
      printf("Expected JSON object as root\n");
      return;
    }

    /* Loop through all keys in the JSON object */
    for (int i = 1; i < token_count; i++) {
      if (jsmn_token_equals((const char *)chunk, &tokens[i], "collector_id")) {
        strncpy(received_collector_id, (const char *)chunk + tokens[i + 1].start,
                tokens[i + 1].end - tokens[i + 1].start);
        received_collector_id[tokens[i + 1].end - tokens[i + 1].start] = '\0';
        i++; // Skip the value token
      } else if (jsmn_token_equals((const char *)chunk, &tokens[i], "lid_server_address")) {
        strncpy(lid_server_address, (const char *)chunk + tokens[i + 1].start,
                tokens[i + 1].end - tokens[i + 1].start);
        lid_server_address[tokens[i + 1].end - tokens[i + 1].start] = '\0';
        i++; // Skip the value token
      } else if (jsmn_token_equals((const char *)chunk, &tokens[i], "compactor_server_address")) {
        strncpy(compactor_server_address, (const char *)chunk + tokens[i + 1].start,
                tokens[i + 1].end - tokens[i + 1].start);
        compactor_server_address[tokens[i + 1].end - tokens[i + 1].start] = '\0';
        i++; // Skip the value token
      }
    }

    /* Verify and use the parsed data */
    if (strcmp(received_collector_id, COLLECTOR_ID) == 0) {
      if (strcmp(received_collector_id, COLLECTOR_ID) == 0) {
  		printf("Received Lid Server Address: %s\n", lid_server_address);
  		printf("Received Compactor Server Address: %s\n", compactor_server_address);

  		// Parse the CoAP server addresses
  		coap_endpoint_parse(lid_server_address, strlen(lid_server_address), &lid_server_endpoint);
		coap_endpoint_parse(compactor_server_address, strlen(compactor_server_address), &compactor_server_endpoint);

  		state = STATE_CONFIG_RECEIVED;
	}
    } else {
      printf("Response is not for this collector. Ignored.\n");
    }
  }
}


/*---------------------------------------------------------------------------*/
/* MQTT Event Handler */
static void
mqtt_event(struct mqtt_connection *m, mqtt_event_t event, void *data)
{
  switch(event) {
    case MQTT_EVENT_CONNECTED:
      printf("Application has a MQTT connection\n");
      state = STATE_CONNECTED;
      break;

    case MQTT_EVENT_DISCONNECTED:
      printf("MQTT Disconnect. Reason %u\n", *((mqtt_event_t *)data));
      state = STATE_DISCONNECTED;
      process_poll(&coap_to_mqtt_process);
      break;

    case MQTT_EVENT_PUBLISH:
      {
      	printf("Received MQTT message\n");
        struct mqtt_message *msg = data;
        pub_handler(msg->topic, strlen(msg->topic), msg->payload_chunk, msg->payload_length);
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

/*---------------------------------------------------------------------------*/
/* CoAP Response Callback for Lid */
static void client_callback_lid_state(coap_message_t *response) {
    const uint8_t *payload;
    if (response) {
        char time_buffer[32]; // Buffer for time
        get_current_time(time_buffer, sizeof(time_buffer)); // Get current time

        coap_get_payload(response, &payload);
        const char *lid_state = (const char *)payload; // Convert payload to string
        printf("CoAP Response - Lid State: %s\n", lid_state);

        // Format MQTT message with the new structure
        snprintf(pub_msg, sizeof(pub_msg),
                 "{\"collector_id\":\"%s\",\"lid_sensor\":{\"value\":\"%s\",\"time_updated\":\"%s\"}}",
                 COLLECTOR_ID, lid_state, time_buffer);

        // Publish to MQTT
        mqtt_status_t status = mqtt_publish(&conn, NULL, PUB_TOPIC, (uint8_t *)pub_msg, strlen(pub_msg), MQTT_QOS_LEVEL_0, MQTT_RETAIN_OFF);
        		if (status == MQTT_STATUS_OK) {
    		printf("Published to topic %s: %s\n", PUB_TOPIC, pub_msg);
		} else {
    		printf("Failed to publish. MQTT status: %d\n", status);
		}
        printf("Published to MQTT: %s\n", pub_msg);
    } else {
        printf("CoAP request timed out.\n");
    }
}


static bool have_connectivity(void) {
  return uip_ds6_get_global(ADDR_PREFERRED) != NULL && uip_ds6_defrt_choose() != NULL;
}

/* CoAP Response Callback for Compactor Sensor */
static void client_callback_compactor_state(coap_message_t *response) {
    const uint8_t *payload;
    if (response) {
        char time_buffer[32]; // Buffer for time
        get_current_time(time_buffer, sizeof(time_buffer)); // Get current time

        coap_get_payload(response, &payload);
        const char *compactor_state = (const char *)payload; // Convert payload to string
        printf("CoAP Response - Compactor State: %s\n", compactor_state);

        // Format MQTT message with the new structure
        snprintf(pub_msg, sizeof(pub_msg),
                 "{\"collector_id\":\"%s\",\"compactor_sensor\":{\"value\":\"%s\",\"time_updated\":\"%s\"}}",
                 COLLECTOR_ID, compactor_state, time_buffer);

        // Publish to MQTT
        mqtt_publish(&conn, NULL, "sensors/compactor", (uint8_t *)pub_msg, strlen(pub_msg), MQTT_QOS_LEVEL_0, MQTT_RETAIN_OFF);
        printf("Published to MQTT: %s\n", pub_msg);
    } else {
        printf("CoAP request for compactor sensor timed out.\n");
    }
}



/*---------------------------------------------------------------------------*/
/* Main Process */
PROCESS_THREAD(coap_to_mqtt_process, ev, data)
{
  PROCESS_BEGIN();
  snprintf(client_id, sizeof(client_id), "coap_to_mqtt_%02x%02x",
           linkaddr_node_addr.u8[6], linkaddr_node_addr.u8[7]);
  mqtt_register(&conn, &coap_to_mqtt_process, client_id, mqtt_event, 128);
  state = STATE_INIT;
  etimer_set(&periodic_timer, CLOCK_SECOND);

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

      if (state == STATE_CONFIG_REQUEST) {
        printf("Requesting CoAP server configuration...\n");

  		// Publish configuration request message
 		 snprintf(pub_msg, sizeof(pub_msg),
           "{\"collector_id\":\"%s\",\"request\":[\"lid_server_address\", \"compactor_server_address\"]}",
           COLLECTOR_ID);

 		 mqtt_status_t status = mqtt_publish(&conn, NULL, CONFIG_REQUEST_TOPIC,
                                      (uint8_t *)pub_msg, strlen(pub_msg),
                                      MQTT_QOS_LEVEL_0, MQTT_RETAIN_OFF);

  		if (status == MQTT_STATUS_OK) {
  		  printf("Published to topic %s: %s\n", CONFIG_REQUEST_TOPIC, pub_msg);
  		} else {
   			 printf("Failed to publish. MQTT status: %d\n", status);
  		}

  		etimer_reset(&periodic_timer);
      }

      if (state == STATE_CONFIG_RECEIVED) {
 		printf("Configuration received. Fetching CoAP data...\n");

	  if (state == STATE_CONFIG_RECEIVED) {

  			printf("Fetching Compactor State...\n");
  			coap_init_message(request, COAP_TYPE_CON, COAP_GET, 0);
 			coap_set_header_uri_path(request, "/compactor/active");
 			COAP_BLOCKING_REQUEST(&compactor_server_endpoint, request, client_callback_compactor_state);

 		 	printf("Fetching Lid State...\n");
 		 	coap_init_message(request, COAP_TYPE_CON, COAP_GET, 0);
 		 	coap_set_header_uri_path(request, "/lid/state");
 		 	COAP_BLOCKING_REQUEST(&lid_server_endpoint, request, client_callback_lid_state);

		}

      }

      if (state == STATE_DISCONNECTED) {
        printf("Disconnected. Retrying...\n");
        state = STATE_INIT;
      }
    }

    etimer_reset(&periodic_timer);
  }

  PROCESS_END();
}
