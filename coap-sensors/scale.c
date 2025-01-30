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

static void get_local_ipv6_address(char *buffer, size_t buffer_size) {
  uip_ds6_addr_t *addr = NULL;

  // Iterate through all IPv6 addresses
  for (addr = uip_ds6_if.addr_list; addr < uip_ds6_if.addr_list + UIP_DS6_ADDR_NB; addr++) {
    // Check if the address is used and is a link-local address
    if (addr->isused && uip_is_addr_linklocal(&addr->ipaddr)) {
      uiplib_ipaddr_snprint(buffer, buffer_size, &addr->ipaddr);
      return;
    }
  }

  // If no link-local address is found, set to "unknown"
  snprintf(buffer, buffer_size, "unknown");
}

static char local_ipv6_address[64];

PROCESS_THREAD(scale_sensor_process, ev, data)
{
  PROCESS_BEGIN();

  // Activate the CoAP resource
  printf("Starting Scale Sensor...\n");
  coap_activate_resource(&scale_sensor, "scale/value");

  while (1) {
    PROCESS_WAIT_EVENT(); // Wait for events (e.g., CoAP requests)

    printf("Device Process Event. Scale Sensor. Address: %s\n", local_ipv6_address);
  }

  PROCESS_END();
}
