#include "contiki.h"
#include "coap-engine.h"
#include "net/ipv6/uip.h"
#include "net/ipv6/uiplib.h"
#include "net/ipv6/uip-ds6.h"

// Declare the resource from the resource file
extern coap_resource_t compactor_active_sensor;

PROCESS(device_process, "Compactor Sensor Process");
AUTOSTART_PROCESSES(&device_process);


PROCESS_THREAD(device_process, ev, data) {
  PROCESS_BEGIN();

  // Activate the CoAP resource
  coap_activate_resource(&compactor_active_sensor, "compactor/active");

  while (1) {
    PROCESS_WAIT_EVENT();
  }

  PROCESS_END();
}

