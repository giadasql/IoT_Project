#include "contiki.h"
#include "coap-engine.h"
#include "resource_utils.h"
#include "conversion_utils.h"
#include <stdio.h>
#include <string.h>

// Scale sensor state
static float scale_value = 32.0; // Initial weight value

// Conversion functions for the scale sensor
static void scale_value_to_string(char *buffer, size_t size, void *state) {
  snprintf(buffer, size, "%.2f", *(float *)state); // Format as a decimal number with 2 decimal places
}

static void scale_value_update_state(const char *payload, void *state) {
  *(float *)state = atof(payload); // Convert payload string to float
  printf("Scale sensor value updated to: %.2f\n", *(float *)state);
}

// Define the generic sensor structure
static generic_sensor_t scale_sensor_data = {
  .name = "scale_sensor",
  .type = "Numeric",
  .state = &scale_value,
  .to_string = scale_value_to_string,
  .update_state = scale_value_update_state
};

// Handlers for CoAP requests
static void scale_sensor_get_handler(coap_message_t *request, coap_message_t *response,
                                     uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
  generic_get_handler(request, response, buffer, preferred_size, offset, &scale_sensor_data);
}

static void scale_sensor_put_handler(coap_message_t *request, coap_message_t *response,
                                     uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
  generic_put_handler(request, response, buffer, preferred_size, offset, &scale_sensor_data);
}

// Define the CoAP resource
RESOURCE(scale_sensor,
         "title=\"Scale Sensor\";rt=\"Numeric\"",
         scale_sensor_get_handler, // GET handler
         NULL,
         scale_sensor_put_handler, // PUT handler
         NULL);
