#include "contiki.h"
#include "coap-engine.h"
#include "sensor_utils.h"
#include "conversion_utils.h"
#include "dev/leds.h"
#include <stdio.h>
#include <string.h>

// Sensor State
static int compactor_state = 0; // 0: false, 1: true

// Define Sensor
static generic_sensor_t compactor_sensor = {
    "compactor_active", "Text", &compactor_state, boolean_to_string, boolean_update_state};

static void compactor_get_handler(coap_message_t *req, coap_message_t *res,
                                   uint8_t *buf, uint16_t size, int32_t *offset) {
    generic_get_handler(req, res, buf, size, offset, &compactor_sensor);
}

static void compactor_put_handler(coap_message_t *req, coap_message_t *res,
                                   uint8_t *buf, uint16_t size, int32_t *offset) {
    // Call the generic PUT handler to update the state
    generic_put_handler(req, res, buf, size, offset, &compactor_sensor);

    // Control the LEDs based on the updated state
    if (compactor_state == 1) {
        // Compactor is active - turn on red LED, turn off green LED
        leds_on(LEDS_RED);
        leds_off(LEDS_GREEN);
        printf("Compactor is active: Red LED ON, Green LED OFF\n");
    } else {
        // Compactor is not active - turn on green LED, turn off red LED
        leds_on(LEDS_GREEN);
        leds_off(LEDS_RED);
        printf("Compactor is inactive: Green LED ON, Red LED OFF\n");
    }
}

RESOURCE(compactor_active_sensor, "title=\"Compactor Active\";rt=\"Text\"",
         compactor_get_handler, compactor_put_handler, NULL, NULL);
