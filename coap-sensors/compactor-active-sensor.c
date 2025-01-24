#include "contiki.h"
#include "coap-engine.h"
#include <stdio.h>

// Declare the resource from the resource file
extern coap_resource_t compactor_active_sensor;
extern coap_resource_t collector_config;
extern process_event_t collector_update_event;

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

    printf("Device Process running...\n");

    if (ev == collector_update_event) {
        printf("Will update the collector.\n");

    }
  }

  PROCESS_END();
}

