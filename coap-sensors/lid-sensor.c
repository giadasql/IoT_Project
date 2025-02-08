#include "contiki.h"
#include "dev/leds.h" // Include LEDs header
#include "coap-engine.h"
#include "net/ipv6/uip.h"
#include "net/ipv6/uiplib.h"
#include "net/ipv6/uip-ds6.h"

// Declare the resources from the resource file. The RFID resource is included on the same device for simplicity
// since the RFID value is generated when the lid is opened, simulating a real scenario where the RFID value is read
extern coap_resource_t lid_sensor;
extern coap_resource_t rfid_reader;

extern char rfid_code[64];

PROCESS(lid_sensor_process, "Lid Sensor Process");
AUTOSTART_PROCESSES(&lid_sensor_process);

PROCESS_THREAD(lid_sensor_process, ev, data) {
  PROCESS_BEGIN();

  // adjust the LED status to the initial state
  leds_off(LEDS_ALL); // Turn off all LEDs
  leds_on(LEDS_RED); // Turn on the red LED

  // Activate the CoAP resources
  coap_activate_resource(&lid_sensor, "lid/open");
  coap_activate_resource(&rfid_reader, "rfid/value");

  while(1) {
    PROCESS_WAIT_EVENT();
  }

  PROCESS_END();
}

