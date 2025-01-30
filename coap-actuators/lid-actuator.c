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

// Sensor Configuration Structure
typedef struct {
    char endpoint_uri[64];
    coap_endpoint_t address;
    const char *name;
    const char *resource_path;
} sensor_config_t;

// External declarations for the combined sensor configurations
extern coap_resource_t lid_sensor_config;
extern sensor_config_t lid_sensor;

static coap_message_t request[1]; // CoAP request message
static bool lid_sensor_state = false;  // Current state of the lid sensor (0: closed, 1: open)
static bool send_command_flag = false; // Flag to send the CoAP command

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

static void get_local_ipv6_address(char *buffer, size_t buffer_size) {
    uip_ds6_addr_t *addr = NULL;

    // Iterate through all IPv6 addresses
    for (addr = uip_ds6_if.addr_list; addr < uip_ds6_if.addr_list + UIP_DS6_ADDR_NB; addr++) {
        // Check if the address is used and is a link-local address
        if (addr->isused && uip_is_addr_linklocal(&addr->ipaddr)) {
            uiplib_ipaddr_snprint(buffer, buffer_size, &addr->ipaddr);
            return;
        }
    }

    // If no link-local address is found, set to "unknown"
    snprintf(buffer, buffer_size, "unknown");
}

static char local_ipv6_address[64];

PROCESS_THREAD(lid_actuator_process, ev, data) {
    PROCESS_BEGIN();

    printf("Lid Actuator Process started.\n");

    // Register the sensor configuration resources
    coap_activate_resource(&lid_sensor_config, lid_sensor.resource_path);

    // Initialize button handling
    button_hal_init();

      get_local_ipv6_address(local_ipv6_address, sizeof(local_ipv6_address));
    printf("Local IPv6 Address: %s\n", local_ipv6_address);

    while (1) {
        PROCESS_WAIT_EVENT();

        printf("Device Process Event. Lid Actuator. Address: %s\n", local_ipv6_address);

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
