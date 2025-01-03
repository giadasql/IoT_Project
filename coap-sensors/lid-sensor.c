#include "contiki.h"
#include "coap-engine.h"

// Declare the resource from the resource file
extern coap_resource_t lid_sensor;

PROCESS(lid_sensor_process, "Lid Sensor Process");
AUTOSTART_PROCESSES(&lid_sensor_process);

PROCESS_THREAD(lid_sensor_process, ev, data) {
  PROCESS_BEGIN();

  // Activate the CoAP resource
  coap_activate_resource(&lid_sensor, "lid/state");

  while(1) {
    PROCESS_WAIT_EVENT(); // Wait for events (CoAP requests)
  }

  PROCESS_END();
}

