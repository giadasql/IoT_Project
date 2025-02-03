#include "contiki.h"
#include "coap-engine.h"
#include "net/ipv6/uip.h"
#include "net/ipv6/uiplib.h"
#include "net/ipv6/uip-ds6.h"

extern coap_resource_t compactor_active_sensor;

PROCESS(device_process, "Device Process");
AUTOSTART_PROCESSES(&device_process);


PROCESS_THREAD(device_process, ev, data) {
  PROCESS_BEGIN();

  coap_activate_resource(&compactor_active_sensor, "compactor/active");

  while (1) {
    PROCESS_WAIT_EVENT();
  }

  PROCESS_END();
}

