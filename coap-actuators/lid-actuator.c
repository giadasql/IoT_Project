#include "contiki.h"
#include "coap-engine.h"
#include "dev/button-hal.h"
#include "coap-blocking-api.h"
#include <stdio.h>
#include <string.h>

// External declarations for the resource
extern coap_resource_t lid_sensor_config;
extern char lid_sensor_endpoint_uri[64];
extern coap_endpoint_t lid_sensor_address;

static coap_message_t request[1]; // CoAP request message
static int lid_sensor_state = 0;  // Current state of the lid sensor (0: closed, 1: open)

static int send_command_flag = 0; // Flag to send the coap command

// Function to toggle the state of the lid sensor on the remote device
static void toggle_lid_sensor_state(void) {
    // Validate that the endpoint is configured
    if (strlen(lid_sensor_endpoint_uri) == 0) {
        printf("Lid sensor endpoint not configured. Skipping toggle operation.\n");
        return;
    }

    // Toggle the state
    lid_sensor_state = !lid_sensor_state;
    printf("Toggling lid sensor state to: %d\n", lid_sensor_state);

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

    // Register the CoAP resource for configuring the lid sensor
    coap_activate_resource(&lid_sensor_config, "lid/config");

    // Initialize button handling
    button_hal_init();

    while (1) {
        PROCESS_WAIT_EVENT();

        printf("Event received\n");

        if (ev == button_hal_press_event) {
            button_event_handler((button_hal_button_t *)data);
        }

        if (send_command_flag == 1) {

            // Prepare the CoAP PUT request
            coap_init_message(request, COAP_TYPE_CON, COAP_PUT, 0);
            coap_set_header_uri_path(request, "/lid/state");
            // Corrected CoAP PUT request payload setting
			coap_set_payload(request, (uint8_t *)(lid_sensor_state ? "open" : "closed"), strlen(lid_sensor_state ? "open" : "closed"));


            // Send the CoAP request
            COAP_BLOCKING_REQUEST(&lid_sensor_address, request, client_chunk_handler);
            printf("Lid sensor state update sent: %d\n", lid_sensor_state);

            send_command_flag = 0; // Clear the flag

        }
    }

    PROCESS_END();
}
