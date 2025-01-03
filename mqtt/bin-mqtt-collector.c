#include "contiki.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include "mqtt.h"
#include "net/routing/routing.h"
#include "net/ipv6/uip.h"
#include "sys/etimer.h"
#include "sys/ctimer.h"
#include "dev/leds.h"
#include "os/sys/log.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/*---------------------------------------------------------------------------*/
#define LOG_MODULE "CoAP-to-MQTT"
#define LOG_LEVEL LOG_LEVEL_DBG

/* MQTT broker configuration */
#define MQTT_CLIENT_BROKER_IP_ADDR "10.65.2.134" // Replace with your broker's IP
#define DEFAULT_BROKER_PORT 1883
#define PUB_TOPIC "sensors/lid"

/* CoAP server configuration */
#define COAP_SERVER_IP "fe80::202:2:2:2" // Replace with the CoAP server's IPù
#define COAP_RESOURCE "/lid/state"

/*---------------------------------------------------------------------------*/
/* MQTT variables */
static char client_id[64];
static char pub_msg[64];
static char broker_address[64];

static struct mqtt_connection conn;
static uint8_t state;

#define STATE_INIT 0
#define STATE_NET_OK 1
#define STATE_CONNECTING 2
#define STATE_CONNECTED 3
#define STATE_SUBSCRIBED 4
#define STATE_DISCONNECTED 5

/*---------------------------------------------------------------------------*/
/* CoAP variables */
static coap_endpoint_t server_endpoint;
static coap_message_t request[1];

/* Timers */
#define STATE_MACHINE_PERIODIC (CLOCK_SECOND >> 1)
static struct etimer periodic_timer;

/*---------------------------------------------------------------------------*/
PROCESS(coap_to_mqtt_process, "CoAP-to-MQTT Bridge");
AUTOSTART_PROCESSES(&coap_to_mqtt_process);

/*---------------------------------------------------------------------------*/
/* MQTT Event Callback */
static void mqtt_event(struct mqtt_connection *m, mqtt_event_t event, void *data) {
  switch (event) {
    case MQTT_EVENT_CONNECTED:
      printf("MQTT connected to broker at %s\n", broker_address);
      state = STATE_CONNECTED;
      break;

    case MQTT_EVENT_DISCONNECTED:
      printf("MQTT disconnected. Attempting to reconnect...\n");
      state = STATE_DISCONNECTED;
      process_poll(&coap_to_mqtt_process);
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

/*---------------------------------------------------------------------------*/
/* Check Connectivity */
static bool have_connectivity(void) {
  return uip_ds6_get_global(ADDR_PREFERRED) != NULL && uip_ds6_defrt_choose() != NULL;
}

/*---------------------------------------------------------------------------*/
/* Main Process */
PROCESS_THREAD(coap_to_mqtt_process, ev, data) {
  PROCESS_BEGIN();

  // Initialize MQTT
  snprintf(client_id, sizeof(client_id), "coap_to_mqtt_%02x%02x",
           linkaddr_node_addr.u8[6], linkaddr_node_addr.u8[7]);
  mqtt_register(&conn, &coap_to_mqtt_process, client_id, mqtt_event, 128);

  // Initialize CoAP
  coap_endpoint_parse(COAP_SERVER_IP, COAP_SERVER_PORT, &server_endpoint);

  state = STATE_INIT;
  etimer_set(&periodic_timer, STATE_MACHINE_PERIODIC);

  while (1) {
    PROCESS_YIELD();

    if ((ev == PROCESS_EVENT_TIMER && data == &periodic_timer) || ev == PROCESS_EVENT_POLL) {
      if (state == STATE_INIT && have_connectivity()) {
        state = STATE_NET_OK;
      }

      if (state == STATE_NET_OK) {
        printf("Connecting to MQTT broker...\n");
        memcpy(broker_address, MQTT_CLIENT_BROKER_IP_ADDR, strlen(MQTT_CLIENT_BROKER_IP_ADDR));
        mqtt_connect(&conn, broker_address, DEFAULT_BROKER_PORT, (30 * CLOCK_SECOND), MQTT_CLEAN_SESSION_ON);
        state = STATE_CONNECTING;
      }

      if (state == STATE_CONNECTED) {
        // Fetch the lid state using CoAP
        coap_init_message(request, COAP_TYPE_CON, COAP_GET, 0);
        coap_set_header_uri_path(request, COAP_RESOURCE);
        COAP_BLOCKING_REQUEST(&server_endpoint, request, client_callback_lid_state);
        etimer_reset(&periodic_timer);
      }

      if (state == STATE_DISCONNECTED) {
        printf("Disconnected. Retrying...\n");
        state = STATE_INIT;
      }
    }
  }

  PROCESS_END();
}
