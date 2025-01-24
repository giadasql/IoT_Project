#include "contiki.h"
#include "coap-engine.h"
#include "resource_utils.h"
#include "conversion_utils.h"
#include <stdio.h>
#include <string.h>

// Waste level sensor state
static float waste_level = 0.0; // Initial waste level (percentage)

// Conversion functions for the waste level sensor
static void waste_level_to_string(char *buffer, size_t size, void *state) {
    snprintf(buffer, size, "%.2f", *(float *)state); // Format as a percentage with 2 decimal places
}

static void waste_level_update_state(const char *payload, void *state) {
    *(float *)state = atof(payload); // Convert payload string to float

    printf("Waste level updated to: %.2f%%\n", *(float *)state);
}

// Define the generic sensor structure
static generic_sensor_t waste_level_sensor_data = {
    .name = "waste_level_sensor",
    .type = "Numeric",
    .state = &waste_level,
    .to_string = waste_level_to_string,
    .update_state = waste_level_update_state
};

// Handlers for CoAP requests
static void waste_level_sensor_get_handler(coap_message_t *request, coap_message_t *response,
                                           uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    generic_get_handler(request, response, buffer, preferred_size, offset, &waste_level_sensor_data);
}

static void waste_level_sensor_put_handler(coap_message_t *request, coap_message_t *response,
                                           uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    generic_put_handler(request, response, buffer, preferred_size, offset, &waste_level_sensor_data);
}

// Define the CoAP resource
RESOURCE(waste_level_sensor,
         "title=\"Waste Level Sensor\";rt=\"Numeric\"",
         waste_level_sensor_get_handler, // GET handler
         NULL,
         waste_level_sensor_put_handler, NULL);
