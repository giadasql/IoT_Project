#include "contiki.h"
#include "coap-engine.h"

// Declare the resource from the resource file
extern coap_resource_t scale_sensor;

PROCESS(scale_sensor_process, "Scale Sensor Process");
AUTOSTART_PROCESSES(&scale_sensor_process);

PROCESS_THREAD(scale_sensor_process, ev, data)
{
  PROCESS_BEGIN();

  // Activate the CoAP resource
  printf("Starting Scale Sensor...\n");
  coap_activate_resource(&scale_sensor, "scale/value");

  while (1) {
    PROCESS_WAIT_EVENT(); // Wait for events (e.g., CoAP requests)
  }

  PROCESS_END();
}
