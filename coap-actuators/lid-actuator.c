#include "contiki.h"
#include "coap-engine.h"
#include "dev/button-hal.h"
#include "coap-blocking-api.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h> // For rand()

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

// Function to generate a random value between min and max
static int get_random_value(int min, int max) {
    return (rand() % (max - min + 1)) + min;
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
          	// send command to scale to update weight
            if (strlen(scale_sensor.endpoint_uri) == 0) {
        		printf("Scale sensor endpoint not configured. Skipping update.\n");
      		}
            else{
             	int random_value = get_random_value(5, 30);
              	// Prepare the JSON payload
    			char payload[32];
    			snprintf(payload, sizeof(payload), "%.2d", random_value);

    			// Prepare the CoAP PUT request
    			coap_init_message(request, COAP_TYPE_CON, COAP_PUT, 0);
    			coap_set_header_uri_path(request, "/scale/value");
    			coap_set_payload(request, (uint8_t *)payload, strlen(payload));

    			// Send the CoAP request to the scale sensor address
    			printf("Sending updated scale weight to: %s, Payload: %s\n", scale_sensor.endpoint_uri, payload);
    			COAP_BLOCKING_REQUEST(&scale_sensor.address, request, client_chunk_handler);
            }

            // send command to waste level sensor to update level
            if (strlen(waste_level_sensor.endpoint_uri) == 0) {
                printf("Waste level sensor endpoint not configured. Skipping update.\n");
            }
            else{
                int random_value = get_random_value(0, 20);
                // Prepare the JSON payload
                char payload[32];
                snprintf(payload, sizeof(payload), "%d", random_value);

	            // Prepare the CoAP PUT request
                coap_init_message(request, COAP_TYPE_CON, COAP_PUT, 0);
                coap_set_header_uri_path(request, "/waste/level");
                coap_set_payload(request, (uint8_t *)payload, strlen(payload));

                // Send the CoAP request to the waste level sensor address
                printf("Sending updated waste level to: %s, Payload: %s\n", waste_level_sensor.endpoint_uri, payload);
                COAP_BLOCKING_REQUEST(&waste_level_sensor.address, request, client_chunk_handler);
            }

          	// send the command to close the bin
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
