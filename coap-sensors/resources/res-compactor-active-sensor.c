#include "contiki.h"
#include "coap-engine.h"
#include "sensor_utils.h"
#include "conversion_utils.h"
#include "dev/leds.h"
#include <stdio.h>
#include <string.h>

// Sensor State
int compactor_state = 0; // 0: false, 1: true

// Timer for auto-reset
static struct ctimer compactor_timer;
#define COMPACTOR_ACTIVE_DURATION (CLOCK_SECOND * 10) // 10 seconds duration

// Define Sensor
static generic_sensor_t compactor_sensor = {
    "compactor_active", "Text", &compactor_state, boolean_to_string, boolean_update_state};

// Function to deactivate the compactor when the timer expires
static void deactivate_compactor(void *ptr) {
    // Set the compactor state to inactive
    compactor_state = 0;

    // Turn off the red LED
    leds_off(LEDS_RED);

    printf("Compactor timer expired. Compactor set to inactive. Red LED turned off.\n");
}

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

    if (compactor_state == 1) {
        // If compactor is active, start or maintain the timer
        if (!ctimer_expired(&compactor_timer)) {
            printf("Compactor is already active. Timer maintained.\n");
        } else {
            printf("Compactor activated. Starting timer.\n");
            ctimer_set(&compactor_timer, COMPACTOR_ACTIVE_DURATION, deactivate_compactor, NULL);
        }

        // Turn on the red LED
        leds_on(LEDS_RED);
        printf("Compactor is active: Red LED ON\n");
    } else {
        // If compactor is inactive, stop the timer and turn off the red LED
        printf("Compactor deactivated. Stopping timer and turning off the red LED.\n");
        ctimer_stop(&compactor_timer);
        leds_off(LEDS_RED);
        printf("Compactor is inactive: Red LED OFF\n");
    }
}

RESOURCE(compactor_active_sensor, "title=\"Compactor Active\";rt=\"Text\"",
         compactor_get_handler, NULL, compactor_put_handler, NULL);
