#include "contiki.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include "mqtt.h"
#include "net/routing/routing.h"
#include "net/ipv6/uip.h"
#include "sys/etimer.h"
#include "sys/ctimer.h"
#include "dev/leds.h"
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
static coap_endpoint_t server_endpoint;
static coap_message_t request[1];
static char coap_server_address[64];

/* MQTT variables */
static char client_id[64];
static char pub_msg[128];
static char broker_address[64];
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

/*---------------------------------------------------------------------------*/
/* Publish Handler */
static void
pub_handler(const char *topic, uint16_t topic_len, const uint8_t *chunk,
            uint16_t chunk_len)
{
  printf("Pub Handler: topic='%s' (len=%u), chunk_len=%u\n", topic, topic_len, chunk_len);

  if(strcmp(topic, CONFIG_RESPONSE_TOPIC) == 0) {
    char received_collector_id[64];
    char received_coap_address[64];

    if (sscanf((char *)chunk, "{\"collector_id\":\"%63[^\"]\",\"coap_server_address\":\"%63[^\"]\"}",
               received_collector_id, received_coap_address) == 2) {
      if (strcmp(received_collector_id, COLLECTOR_ID) == 0) {
        snprintf(coap_server_address, sizeof(coap_server_address), "%s", received_coap_address);
        printf("Received CoAP server address: %s\n", coap_server_address);
        coap_endpoint_parse(coap_server_address, strlen(coap_server_address), &server_endpoint);
        state = STATE_CONFIG_RECEIVED;
      } else {
        printf("Response is not for this collector. Ignored.\n");
      }
    } else {
      printf("Malformed JSON in response. Ignored.\n");
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
/* CoAP Response Callback */
static void client_callback_lid_state(coap_message_t *response) {
  const uint8_t *payload;
  if (response) {
    coap_get_payload(response, &payload);
    const char *lid_state = (const char *)payload; // Convert payload to string
    printf("CoAP Response - Lid State: %s\n", lid_state);

    // Format MQTT message
    snprintf(pub_msg, sizeof(pub_msg), "{\"lid_state\":\"%s\"}", lid_state);

    // Publish to MQTT
    mqtt_publish(&conn, NULL, PUB_TOPIC, (uint8_t *)pub_msg, strlen(pub_msg), MQTT_QOS_LEVEL_0, MQTT_RETAIN_OFF);
    printf("Published to MQTT: %s\n", pub_msg);

    // LED logic (optional)
    if (strcmp(lid_state, "open") == 0) {
      leds_on(LEDS_GREEN);
    } else if (strcmp(lid_state, "closed") == 0) {
      leds_off(LEDS_GREEN);
    }
  } else {
    printf("CoAP request timed out.\n");
  }
}

static bool have_connectivity(void) {
  return uip_ds6_get_global(ADDR_PREFERRED) != NULL && uip_ds6_defrt_choose() != NULL;
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

      if (state == STATE_CONFIG_REQUEST) {
        printf("Requesting CoAP server configuration...\n");
        snprintf(pub_msg, sizeof(pub_msg), "{\"collector_id\":\"%s\",\"request\":\"coap_server_address\"}", COLLECTOR_ID);
        mqtt_publish(&conn, NULL, CONFIG_REQUEST_TOPIC, (uint8_t *)pub_msg, strlen(pub_msg), MQTT_QOS_LEVEL_0, MQTT_RETAIN_OFF);
        etimer_reset(&periodic_timer);
      }

      if (state == STATE_CONFIG_RECEIVED) {
        printf("Configuration received. Fetching CoAP data...\n");
        coap_init_message(request, COAP_TYPE_CON, COAP_GET, 0);
        coap_set_header_uri_path(request, "/lid/state");
        COAP_BLOCKING_REQUEST(&server_endpoint, request, client_callback_lid_state);
        state = STATE_CONNECTED;
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
