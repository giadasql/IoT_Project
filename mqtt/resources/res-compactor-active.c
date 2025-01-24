#include "contiki.h"
#include "coap-engine.h"
#include "resource_utils.h"
#include "conversion_utils.h"
#include "dev/leds.h"
#include <stdio.h>
#include <string.h>

// Sensor State
int compactor_state = 0; // 0: false, 1: true

// Define Sensor
generic_sensor_t compactor_sensor = {
    "compactor_active", "Text", &compactor_state, "", boolean_to_string, boolean_update_state};


// Handler for GET requests
static void compactor_get_handler(coap_message_t *req, coap_message_t *res,
                                   uint8_t *buf, uint16_t size, int32_t *offset) {
    generic_get_handler(req, res, buf, size, offset, &compactor_sensor);
}

// Handler for PUT requests
static void compactor_put_handler(coap_message_t *req, coap_message_t *res,
                                   uint8_t *buf, uint16_t size, int32_t *offset) {
    // Call the generic PUT handler to update the state
    generic_put_handler(req, res, buf, size, offset, &compactor_sensor);
}

RESOURCE(compactor_active_sensor, "title=\"Compactor Active\";rt=\"Text\"",
         compactor_get_handler, NULL, compactor_put_handler, NULL);
