#include "contiki.h"
#include "dev/leds.h" // Include LEDs header
#include "coap-engine.h"

// Declare the resource from the resource file
extern coap_resource_t lid_sensor;
extern coap_resource_t rfid_reader;

extern char rfid_code[64];

PROCESS(lid_sensor_process, "Lid Sensor Process");
AUTOSTART_PROCESSES(&lid_sensor_process);

PROCESS_THREAD(lid_sensor_process, ev, data) {
  PROCESS_BEGIN();

  leds_off(LEDS_ALL); // Turn off all LEDs
  leds_on(LEDS_RED); // Turn on the red LED

  // Activate the CoAP resource
  coap_activate_resource(&lid_sensor, "lid/state");
  coap_activate_resource(&rfid_reader, "rfid/value");

  while(1) {
    PROCESS_WAIT_EVENT(); // Wait for events (CoAP requests)
  }

  PROCESS_END();
}

