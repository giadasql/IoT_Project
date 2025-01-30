#include "contiki.h"
#include "coap-engine.h"

// Declare the resource from the resource file
extern coap_resource_t compactor_active_sensor;

PROCESS(device_process, "Device Process");
AUTOSTART_PROCESSES(&device_process);

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

PROCESS_THREAD(device_process, ev, data) {
  PROCESS_BEGIN();

  // Activate Resource
  coap_activate_resource(&compactor_active_sensor, "compactor/active");

  get_local_ipv6_address(local_ipv6_address, sizeof(local_ipv6_address));
    printf("Local IPv6 Address: %s\n", local_ipv6_address);

  while (1) {
    PROCESS_WAIT_EVENT();

    printf("Device Process Event. Compactor Active Sensor. Address: %s\n", local_ipv6_address);
  }

  PROCESS_END();
}

