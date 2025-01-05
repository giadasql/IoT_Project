#include "contiki.h"
#include "coap-engine.h"
#include "sensor_utils.h"
#include <stdio.h>
#include <string.h>

// Sensor State
static int compactor_state = 0; // 0: false, 1: true

// Conversion Functions
static void boolean_to_string(char *buffer, size_t size, void *state) {
    snprintf(buffer, size, "%s", (*(int *)state) ? "true" : "false");
}

static void boolean_update_state(const char *payload, void *state) {
    *(int *)state = (strcmp(payload, "true") == 0) ? 1 : 0;
}

// Define Sensor
static generic_sensor_t compactor_sensor = {
    "compactor_active", "Text", &compactor_state, boolean_to_string, boolean_update_state};

static void compactor_get_handler(coap_message_t *req, coap_message_t *res,
                                   uint8_t *buf, uint16_t size, int32_t *offset) {
    generic_get_handler(req, res, buf, size, offset, &compactor_sensor);
}

static void compactor_put_handler(coap_message_t *req, coap_message_t *res,
                                   uint8_t *buf, uint16_t size, int32_t *offset) {
    generic_put_handler(req, res, buf, size, offset, &compactor_sensor);
}

RESOURCE(compactor_active_sensor, "title=\"Compactor Active\";rt=\"Text\"",
         compactor_get_handler, compactor_put_handler, NULL, NULL);
