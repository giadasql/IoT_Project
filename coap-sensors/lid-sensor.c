#include "contiki.h"
#include "dev/leds.h" // Include LEDs header
#include "coap-engine.h"
#include "net/ipv6/uip.h"
#include "net/ipv6/uiplib.h"
#include "net/ipv6/uip-ds6.h"

// Declare the resource from the resource file
extern coap_resource_t lid_sensor;
extern coap_resource_t rfid_reader;

extern char rfid_code[64];

PROCESS(lid_sensor_process, "Lid Sensor Process");
AUTOSTART_PROCESSES(&lid_sensor_process);

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

PROCESS_THREAD(lid_sensor_process, ev, data) {
  PROCESS_BEGIN();

  leds_off(LEDS_ALL); // Turn off all LEDs
  leds_on(LEDS_RED); // Turn on the red LED

  // Activate the CoAP resource
  coap_activate_resource(&lid_sensor, "lid/state");
  coap_activate_resource(&rfid_reader, "rfid/value");
    get_local_ipv6_address(local_ipv6_address, sizeof(local_ipv6_address));
    printf("Local IPv6 Address: %s\n", local_ipv6_address);

  while(1) {
    PROCESS_WAIT_EVENT(); // Wait for events (CoAP requests)

    printf("Device Process Event. Lid Sensor. Address: %s\n", local_ipv6_address);
  }

  PROCESS_END();
}

