#include "contiki.h"
#include "coap-engine.h"
#include "sensor_utils.h"
#include <stdio.h>
#include <string.h>

// Simulated GPIO state for the lid sensor
static int lid_state = 0; // 0: closed, 1: open

// Conversion functions for the lid sensor state
static void lid_state_to_string(char *buffer, size_t size, void *state) {
  snprintf(buffer, size, "%s", (*(int *)state) ? "open" : "closed");
}

static void lid_state_update_state(const char *payload, void *state) {
  if (strcmp(payload, "open") == 0) {
    *(int *)state = 1;
    printf("Lid state set to: open\n");
  } else if (strcmp(payload, "closed") == 0) {
    *(int *)state = 0;
    printf("Lid state set to: closed\n");
  } else {
    printf("Invalid payload for lid state: %s\n", payload);
  }
}

// Define the generic sensor structure
static generic_sensor_t lid_sensor_data = {
  .name = "lid_sensor",
  .type = "Text",
  .state = &lid_state,
  .to_string = lid_state_to_string,
  .update_state = lid_state_update_state
};

// Handlers for CoAP requests
static void lid_sensor_get_handler(coap_message_t *request, coap_message_t *response,
                                   uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
  generic_get_handler(request, response, buffer, preferred_size, offset, &lid_sensor_data);
}

static void lid_sensor_put_handler(coap_message_t *request, coap_message_t *response,
                                   uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
  generic_put_handler(request, response, buffer, preferred_size, offset, &lid_sensor_data);
}

// Define the CoAP resource
RESOURCE(lid_sensor,
         "title=\"Lid Sensor\";rt=\"Text\"",
         lid_sensor_get_handler, // GET handler
         lid_sensor_put_handler, // PUT handler
         NULL, NULL);
