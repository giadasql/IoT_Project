#include "contiki.h"
#include "coap-engine.h"
#include "dev/button-hal.h"
#include "coap-blocking-api.h"
#include <stdio.h>
#include <string.h>

// Sensor Configuration Structure
typedef struct {
    char endpoint_uri[64];
    coap_endpoint_t address;
    const char *name;
    const char *resource_path;
} sensor_config_t;

// External declarations for the combined sensor configurations
extern coap_resource_t lid_sensor_config, scale_sensor_config, waste_level_sensor_config;
extern sensor_config_t lid_sensor, scale_sensor, waste_level_sensor;

static coap_message_t request[1]; // CoAP request message
static int lid_sensor_state = 0;  // Current state of the lid sensor (0: closed, 1: open)
static int send_command_flag = 0; // Flag to send the CoAP command

// Function to toggle the state of the lid sensor on the remote device
static void toggle_lid_sensor_state(void) {
    // Validate that the lid sensor endpoint is configured
    if (strlen(lid_sensor.endpoint_uri) == 0) {
        printf("Lid sensor endpoint not configured. Skipping toggle operation.\n");
        return;
    }

    // Toggle the state
    lid_sensor_state = !lid_sensor_state;
    printf("Toggling lid sensor state to: %d\n", lid_sensor_state);

    // Set flag to send the CoAP command
    send_command_flag = 1;
}

void client_chunk_handler(coap_message_t *response) {
    const uint8_t *chunk;

    if (response == NULL) {
        puts("Request timed out");
        return;
    }

    int len = coap_get_payload(response, &chunk);
    printf("|%.*s\n", len, (char *)chunk);
}

// Button event handler to toggle the lid sensor
static void button_event_handler(button_hal_button_t *btn) {
    printf("Button pressed. Toggling lid sensor.\n");
    toggle_lid_sensor_state();
}

// Main process for the actuator
PROCESS(lid_actuator_process, "Lid Actuator");
AUTOSTART_PROCESSES(&lid_actuator_process);

PROCESS_THREAD(lid_actuator_process, ev, data) {
    PROCESS_BEGIN();

    printf("Lid Actuator Process started.\n");

    // Register the sensor configuration resources
    coap_activate_resource(&lid_sensor_config, lid_sensor.resource_path);
    coap_activate_resource(&scale_sensor_config, scale_sensor.resource_path);
    coap_activate_resource(&waste_level_sensor_config, waste_level_sensor.resource_path);

    // Initialize button handling
    button_hal_init();

    while (1) {
        PROCESS_WAIT_EVENT();

        if (ev == button_hal_press_event) {
            button_event_handler((button_hal_button_t *)data);
        }

        if (send_command_flag) {
            // Prepare the CoAP PUT request
            coap_init_message(request, COAP_TYPE_CON, COAP_PUT, 0);
            coap_set_header_uri_path(request, "/lid/state");
            coap_set_payload(request, (uint8_t *)(lid_sensor_state ? "open" : "closed"),
                             strlen(lid_sensor_state ? "open" : "closed"));

            // Send the CoAP request to the lid sensor address
            COAP_BLOCKING_REQUEST(&lid_sensor.address, request, client_chunk_handler);
            printf("Lid sensor state update sent: %d\n", lid_sensor_state);

            // Reset the send command flag
            send_command_flag = 0;
        }
    }

    PROCESS_END();
}
