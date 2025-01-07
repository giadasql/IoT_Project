#include "contiki.h"
#include "coap-engine.h"

// Declare the resource from the resource file
extern coap_resource_t waste_level_sensor;

PROCESS(waste_level_sensor_process, "Waste Level Sensor Process");
AUTOSTART_PROCESSES(&waste_level_sensor_process);

PROCESS_THREAD(waste_level_sensor_process, ev, data)
{
  PROCESS_BEGIN();

  // Activate the CoAP resource
  printf("Starting Waste Level Sensor...\n");
  coap_activate_resource(&waste_level_sensor, "waste/level");

  while (1) {
    PROCESS_WAIT_EVENT(); // Wait for events (e.g., CoAP requests)
  }

  PROCESS_END();
}
