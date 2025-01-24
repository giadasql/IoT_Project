#include "contiki.h"
#include "coap-engine.h"
#include "sensor_utils.h"
#include "coap-blocking-api.h"
#include <stdio.h>

// Declare the resource from the resource file
extern coap_resource_t compactor_active_sensor;
extern coap_resource_t collector_config;
extern process_event_t collector_update_event;
extern char collector_address[64];
extern int compactor_state;

coap_endpoint_t collector_endpoint;
coap_message_t request[1];

PROCESS(device_process, "Device Process");
AUTOSTART_PROCESSES(&device_process);

PROCESS_THREAD(device_process, ev, data) {
  PROCESS_BEGIN();

  // Initialize the custom event
  collector_update_event = process_alloc_event();


  // Activate Resource
  coap_activate_resource(&compactor_active_sensor, "compactor/active");
  coap_activate_resource(&collector_config, "config/collector");


  while (1) {
    PROCESS_WAIT_EVENT();

    if (ev == collector_update_event) {
        printf("Compactor state updated. Will notify the collector: %s\n", collector_address);


        coap_endpoint_parse(collector_address, strlen(collector_address), &collector_endpoint);

         continue;

        // send a CoAP PUT request to the collector, send true if compactor is active
        coap_init_message(request, COAP_TYPE_CON, COAP_PUT, 0);
        coap_set_header_uri_path(request, "compactor/state");
        if (compactor_state) {
            coap_set_payload(request, (uint8_t *)"true", strlen("true"));
        } else {
            coap_set_payload(request, (uint8_t *)"false", strlen("false"));
        }
        //COAP_BLOCKING_REQUEST(&collector_endpoint, request, client_chunk_handler);
    }
  }

  PROCESS_END();
}

