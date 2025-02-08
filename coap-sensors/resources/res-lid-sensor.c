#include "contiki.h"
#include "coap-engine.h"
#include "sensor_utils.h"
#include "dev/leds.h" // Include LEDs header
#include "conversion_utils.h" // Include conversion utilities
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>


static bool lid_state = false; // false: closed, true: open
extern char rfid_code[64]; // RFID value shared with the RFID resource

// List of predefined RFID values - for simulation purposes
static const char *rfid_values[] = {
    "ABC123", "DEF456", "GHI789", "JKL012", "MNO345"
};
static const size_t rfid_values_count = sizeof(rfid_values) / sizeof(rfid_values[0]);

// Conversion functions for the lid sensor
// this is only for simulation purposes, in a real scenario the value would be read from the sensor
static void lid_state_update_state(const char *payload, void *state) {
    if (strcmp(payload, "true") == 0) {
        *(bool *)state = true;
        // Assign a random RFID value when the lid is opened - for simulation purposes
        strcpy(rfid_code, rfid_values[rand() % rfid_values_count]);
        printf("Lid state set to: open, RFID value assigned: %s\n", rfid_code);
    } else if (strcmp(payload, "false") == 0) {
        *(bool *)state = false;
        // Reset RFID value to "No value" when the lid is closed
        strcpy(rfid_code, "No value");
        printf("Lid state set to: closed, RFID value reset to: %s\n", rfid_code);
    } else {
        printf("Invalid payload for lid state: %s\n", payload);
    }
}

// define the generic sensor structure
static generic_sensor_t lid_sensor_data = {
    .name = "lid_sensor",
    .type = "Boolean",
    .state = &lid_state,
    .to_string = boolean_to_string,
    .update_state = lid_state_update_state
};

// GET handler for the lid sensor
static void lid_sensor_get_handler(coap_message_t *request, coap_message_t *response,
                                   uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    generic_get_handler(request, response, buffer, preferred_size, offset, &lid_sensor_data);
}

// PUT handler for the lid sensor
// this is only for simulation purposes, in a real scenario the value would be read from the sensor
static void lid_sensor_put_handler(coap_message_t *request, coap_message_t *response,
                                   uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    generic_put_handler(request, response, buffer, preferred_size, offset, &lid_sensor_data);

    // Control the LEDs based on the updated state
    if (lid_state) {
        // Lid is open - turn on green LED, turn off red LED
        leds_on(LEDS_GREEN);
        leds_off(LEDS_RED);
        printf("Lid is open: Green LED ON, Red LED OFF\n");
    } else {
        // Lid is closed - turn on red LED, turn off green LED
        leds_on(LEDS_RED);
        leds_off(LEDS_GREEN);
        printf("Lid is closed: Red LED ON, Green LED OFF\n");
    }
}

RESOURCE(lid_sensor,
         "title=\"Lid Sensor\";rt=\"Boolean\"",
         lid_sensor_get_handler,
         NULL,
         lid_sensor_put_handler, NULL);