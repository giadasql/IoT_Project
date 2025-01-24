#include "contiki.h"
#include "coap-engine.h"
#include "sensor_utils.h"
#include "coap-blocking-api.h"
#include <stdio.h>

// Declare the resource from the resource file
extern coap_resource_t compactor_active_sensor;
extern coap_resource_t collector_config;
extern char collector_address[64];
extern int compactor_state;

static coap_endpoint_t collector_endpoint;
static coap_message_t request[1];


// Flag to track whether an update is required
extern bool update_required;

// Timer for periodic checking
static struct etimer update_timer;

PROCESS(device_process, "Device Process");
AUTOSTART_PROCESSES(&device_process);

PROCESS_THREAD(device_process, ev, data) {
  PROCESS_BEGIN();

  // Set the timer to check for updates every second
  etimer_set(&update_timer, CLOCK_SECOND * 5);

  // Activate Resources
  coap_activate_resource(&compactor_active_sensor, "compactor/active");
  coap_activate_resource(&collector_config, "config/collector");

  while (1) {
    PROCESS_YIELD();

    // Check if the timer expired
    if((ev == PROCESS_EVENT_TIMER && data == &update_timer)) {
        if (update_required) {
            printf("Compactor state updated. Will notify the collector: %s\n", collector_address);
            coap_endpoint_parse(collector_address, strlen(collector_address), &collector_endpoint);

            // Send a CoAP PUT request to the collector, send "true" if compactor is active
            coap_init_message(request, COAP_TYPE_CON, COAP_PUT, 0);
            coap_set_header_uri_path(request, "/compactor/state");

            if (compactor_state) {
                coap_set_payload(request, (uint8_t *)"true", strlen("true"));
            } else {
                coap_set_payload(request, (uint8_t *)"false", strlen("false"));
            }
            COAP_BLOCKING_REQUEST(&collector_endpoint, request, client_chunk_handler);

            // Reset the flag after sending the update
            update_required = false;
        }

        // Reset the timer
        etimer_reset(&update_timer);
    }
  }

  PROCESS_END();
}
