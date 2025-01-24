#include "contiki.h"
#include "coap-engine.h"
#include <stdio.h>

// Declare the resource from the resource file
extern coap_resource_t compactor_active_sensor;
extern coap_resource_t collector_config;
extern bool send_update;

PROCESS(device_process, "Device Process");
AUTOSTART_PROCESSES(&device_process);

PROCESS_THREAD(device_process, ev, data) {
  PROCESS_BEGIN();

  // Activate Resource
  coap_activate_resource(&compactor_active_sensor, "compactor/active");
  coap_activate_resource(&collector_config, "config/collector");


  while (1) {
    PROCESS_WAIT_EVENT();

    if(send_update) {
        printf("Sending update...\n");
    }
  }

  PROCESS_END();
}

