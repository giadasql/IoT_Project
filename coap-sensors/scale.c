#include "contiki.h"
#include "coap-engine.h"
#include <stdio.h>
#include "net/ipv6/uip.h"
#include "net/ipv6/uiplib.h"
#include "net/ipv6/uip-ds6.h"

// Declare the resource from the resource file
extern coap_resource_t scale_sensor;

PROCESS(scale_sensor_process, "Scale Sensor Process");
AUTOSTART_PROCESSES(&scale_sensor_process);

PROCESS_THREAD(scale_sensor_process, ev, data)
{
  PROCESS_BEGIN();

  // Activate the CoAP resource
  coap_activate_resource(&scale_sensor, "scale/value");

  while (1) {
    PROCESS_WAIT_EVENT();
  }

  PROCESS_END();
}
