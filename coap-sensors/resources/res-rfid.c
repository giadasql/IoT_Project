#include "contiki.h"
#include "coap-engine.h"
#include "sensor_utils.h"
#include <stdio.h>
#include <string.h>

// String sensor state
char rfid_code[64] = "No data"; // Initial string value

// Conversion function for the string sensor
static void rfid_code_to_string(char *buffer, size_t size, void *state) {
    snprintf(buffer, size, "%s", (char *)state); // Copy the string state into the buffer
}

// Define the generic sensor structure
static generic_sensor_t rfid_reader_data = {
    .name = "rfid_reader",
    .type = "String",
    .state = rfid_code,
    .to_string = rfid_code_to_string,
    .update_state = NULL // No PUT handler, so no need for an update function
  };

// Handler for CoAP GET requests
static void rfid_reader_get_handler(coap_message_t *request, coap_message_t *response,
                                      uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    generic_get_handler(request, response, buffer, preferred_size, offset, &rfid_reader_data);
}

// Define the CoAP resource
RESOURCE(rfid_reader,
         "title=\"String Sensor\";rt=\"String\"",
         rfid_reader_get_handler, // GET handler
         NULL,
         NULL,                      // No PUT handler
         NULL);
