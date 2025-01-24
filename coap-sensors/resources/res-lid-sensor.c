#include "contiki.h"
#include "coap-engine.h"
#include "sensor_utils.h"
#include "dev/leds.h" // Include LEDs header
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Simulated GPIO state for the lid sensor
static int lid_state = 0; // 0: closed, 1: open
extern char rfid_code[64]; // RFID value shared with the RFID resource

// List of predefined RFID values
static const char *rfid_values[] = {
    "ABC123", "DEF456", "GHI789", "JKL012", "MNO345"
};
static const size_t rfid_values_count = sizeof(rfid_values) / sizeof(rfid_values[0]);

// Conversion functions for the lid sensor state
static void lid_state_to_string(char *buffer, size_t size, void *state) {
    snprintf(buffer, size, "%s", (*(int *)state) ? "open" : "closed");
}

static void lid_state_update_state(const char *payload, void *state) {
    if (strcmp(payload, "open") == 0) {
        *(int *)state = 1;

        // Assign a random RFID value when the lid is opened
        strcpy(rfid_code, rfid_values[rand() % rfid_values_count]);
        printf("Lid state set to: open, RFID value assigned: %s\n", rfid_code);
    } else if (strcmp(payload, "closed") == 0) {
        *(int *)state = 0;

        // Reset RFID value to "No value" when the lid is closed
        strcpy(rfid_code, "No value");
        printf("Lid state set to: closed, RFID value reset to: %s\n", rfid_code);
    } else {
        printf("Invalid payload for lid state: %s\n", payload);
    }
}

// Define the generic sensor structure
static generic_sensor_t lid_sensor_data = {
    .name = "lid_sensor",
    .type = "Text",
    .state = &lid_state,
    .to_string = lid_state_to_string,
    .update_state = lid_state_update_state
};

// Handlers for CoAP requests
static void lid_sensor_get_handler(coap_message_t *request, coap_message_t *response,
                                   uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    generic_get_handler(request, response, buffer, preferred_size, offset, &lid_sensor_data);
}

static void lid_sensor_put_handler(coap_message_t *request, coap_message_t *response,
                                   uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    // Update the lid sensor state
    generic_put_handler(request, response, buffer, preferred_size, offset, &lid_sensor_data);

    // Control the LEDs based on the updated state
    if (lid_state == 1) {
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

// Define the CoAP resource
RESOURCE(lid_sensor,
         "title=\"Lid Sensor\";rt=\"Text\"",
         lid_sensor_get_handler, // GET handler
         NULL,
         lid_sensor_put_handler, NULL);
