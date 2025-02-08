#include "contiki.h"
#include "coap-engine.h"
#include "sensor_utils.h"
#include <stdio.h>
#include <string.h>

char rfid_code[64] = "No data"; // default RFID value when no code is read

// Conversion functions for the RFID reader
static void rfid_code_to_string(char *buffer, size_t size, void *state) {
    snprintf(buffer, size, "%s", (char *)state);
}

static generic_sensor_t rfid_reader_data = {
    .name = "rfid_reader",
    .type = "String",
    .state = rfid_code,
    .to_string = rfid_code_to_string,
    .update_state = NULL
  };

// GET handler for the RFID reader
static void rfid_reader_get_handler(coap_message_t *request, coap_message_t *response,
                                      uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    generic_get_handler(request, response, buffer, preferred_size, offset, &rfid_reader_data);
}

RESOURCE(rfid_reader,
         "title=\"String Sensor\";rt=\"String\"",
         rfid_reader_get_handler,
         NULL,
         NULL,
         NULL);
