#include "contiki.h"
#include "coap-engine.h"
#include "sensor_utils.h"
#include "conversion_utils.h"
#include <stdio.h>
#include <string.h>

// Scale sensor state
static int scale_value = 32; // Initial weight value

// Conversion functions for the scale sensor
static void scale_value_to_string(char *buffer, size_t size, void *state) {
  snprintf(buffer, size, "%.2d", *(int *)state); // Format as a decimal number with 2 decimal places
}

static void scale_value_update_state(const char *payload, void *state) {
  *(int *)state += atoi(payload); // Convert payload string to float and add to the current value
  // Clamp to minimum 0
    if (*(int *)state < 0) {
        *(int *)state = 0;
    }
  printf("Scale sensor value updated to: %.2d\n", *(int *)state);
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
