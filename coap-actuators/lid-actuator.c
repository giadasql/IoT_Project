#include "contiki.h"
#include "coap-engine.h"
#include "dev/button-hal.h"
#include "coap-blocking-api.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h> // For rand()
#include "net/ipv6/uip.h"
#include "net/ipv6/uiplib.h"
#include "net/ipv6/uip-ds6.h"

// CoAP resource for the lid sensor address configuration - only needed for simulation
extern char lid_sensor_endpoint_uri[64]; // Buffer to store endpoint URI
extern coap_endpoint_t lid_sensor_address;

static coap_message_t request[1]; // CoAP request message

static bool lid_sensor_state = false;  // Current state of the lid sensor (false: closed, true: open)
static bool send_command_pending = false; // Flag to execute the command when button is pressed

// resource for the lid actuator command
extern coap_resource_t lid_actuator_command;
extern coap_resource_t lid_sensor_endpoint;
extern bool send_lid_command;
extern bool lid_value_to_send; // flag to execute the command when requested via CoAP

// Function to toggle the lid state - only needed for simulation
// The lid actuator will send a CoAP PUT request to the lid sensor to toggle its state
static void toggle_lid_sensor_state(void) {
    // Validate that the lid sensor endpoint is configured
    if (strlen(lid_sensor_endpoint_uri) == 0) {
        printf("Lid sensor endpoint not configured. Skipping toggle operation.\n");
        return;
    }

    // Toggle the state
    lid_sensor_state = !lid_sensor_state;
    printf("Toggling lid sensor state to: %d\n", lid_sensor_state);

    // Set flag to send the CoAP command
    send_command_pending = 1;
}

// Button event handler to toggle the lid sensor
static void button_event_handler(button_hal_button_t *btn) {
    printf("Button pressed. Toggling lid sensor.\n");
    toggle_lid_sensor_state();
}

// Event that will be posted when a command is received
extern process_event_t lid_command_event;

PROCESS(lid_actuator_process, "Lid Actuator");
AUTOSTART_PROCESSES(&lid_actuator_process);

PROCESS_THREAD(lid_actuator_process, ev, data) {
    PROCESS_BEGIN();

    printf("Lid Actuator Process started.\n");

    // Register the resources
    coap_activate_resource(&lid_sensor_endpoint, "lid/config");
    coap_activate_resource(&lid_actuator_command, "lid/command");

    // Initialize button handling
    button_hal_init();

    // Register the button event handler
    lid_command_event = process_alloc_event();

    while (1) {
        PROCESS_WAIT_EVENT();

        // if event is a button press event, call the button event handler
        if (ev == button_hal_press_event) {
            button_event_handler((button_hal_button_t *)data);
        }

        if(send_lid_command) {
            send_command_pending = 1;
            lid_sensor_state = lid_value_to_send;
            send_lid_command = 0;
        }

        if (send_command_pending) {
            // Prepare the CoAP PUT request
            coap_init_message(request, COAP_TYPE_CON, COAP_PUT, 0);
            coap_set_header_uri_path(request, "/lid/open");
            coap_set_payload(request, (uint8_t *)(lid_sensor_state ? "true" : "false"),
                             strlen(lid_sensor_state ? "true" : "false"));

            // Send the CoAP request to the lid sensor address - only needed for simulation
            COAP_BLOCKING_REQUEST(&lid_sensor_address, request, NULL);
            printf("Lid sensor state update sent: %d\n", lid_sensor_state);

            // Reset the flag
            send_command_pending = 0;
        }
    }

    PROCESS_END();
}
