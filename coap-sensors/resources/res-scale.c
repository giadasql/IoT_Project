#include "contiki.h"
#include "coap-engine.h"
#include "sensor_utils.h"
#include "conversion_utils.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Measurement value of the scale sensor
static float scale_value = 0.0;

// Conversion function for the scale sensor
static void scale_value_to_string(char *buffer, size_t size, void *state) {
   float value = *(float *)state;

   // separe the integer and decimal part of the value in two integers
   // because float is not supported by snprintf on the device (it is in the simulator)
    int integer_part = (int)value;
    int decimal_part = (int)((value - integer_part) * 100);

   snprintf(buffer, size, "%d.%02d", integer_part, decimal_part);
}

// Update function for the scale sensor. The received payload is added to the current value
// this is only for simulation purposes, in a real scenario the value would be read from the sensor
static void scale_value_update_state(const char *payload, void *state) {
  printf("Updating scale sensor value with payload: %s\n", payload);

  float new_value = atof(payload); // Convert payload string to float
  *(float *)state += new_value;

  // Clamp to minimum 0
  if (*(float *)state < 0.0f) {
      *(float *)state = 0.0f;
  }

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

// GET handler for the scale sensor
static void scale_sensor_get_handler(coap_message_t *request, coap_message_t *response,
                                     uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
  generic_get_handler(request, response, buffer, preferred_size, offset, &scale_sensor_data);
}

// PUT handler for the scale sensor
// this is only for simulation purposes, in a real scenario the value would be read from the sensor
static void scale_sensor_put_handler(coap_message_t *request, coap_message_t *response,
                                     uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
  generic_put_handler(request, response, buffer, preferred_size, offset, &scale_sensor_data);
}

// Define the CoAP resource
RESOURCE(scale_sensor,
         "title=\"Scale Sensor\";rt=\"Numeric\"",
         scale_sensor_get_handler,
         NULL,
         scale_sensor_put_handler,
         NULL);
