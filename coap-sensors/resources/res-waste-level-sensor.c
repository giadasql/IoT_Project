#include "contiki.h"
#include "coap-engine.h"
#include "sensor_utils.h"
#include "conversion_utils.h"
#include <stdio.h>
#include <string.h>

static int waste_level = 0; // Initial waste level (percentage)

static void waste_level_to_string(char *buffer, size_t size, void *state) {
    snprintf(buffer, size, "%.2d", *(int *)state);
}

static void waste_level_update_state(const char *payload, void *state) {
    *(int *)state += atoi(payload); // Convert payload string to int and add to the current value

    if (*(int *)state < 0.0) {
        *(int *)state = 0.0; // Clamp to minimum 0%
    } else if (*(int *)state > 100.0) {
        *(int *)state = 100.0; // Clamp to maximum 100%
    }
    printf("Waste level updated to: %.2d%%\n", *(int *)state);
}

static generic_sensor_t waste_level_sensor_data = {
    .name = "waste_level_sensor",
    .type = "Numeric",
    .state = &waste_level,
    .to_string = waste_level_to_string,
    .update_state = waste_level_update_state
};

static void waste_level_sensor_get_handler(coap_message_t *request, coap_message_t *response,
                                           uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    generic_get_handler(request, response, buffer, preferred_size, offset, &waste_level_sensor_data);
}

static void waste_level_sensor_put_handler(coap_message_t *request, coap_message_t *response,
                                           uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    generic_put_handler(request, response, buffer, preferred_size, offset, &waste_level_sensor_data);
}

RESOURCE(waste_level_sensor,
         "title=\"Waste Level Sensor\";rt=\"Numeric\"",
         waste_level_sensor_get_handler,
         NULL,
         waste_level_sensor_put_handler, NULL);
