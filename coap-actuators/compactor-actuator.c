#include "contiki.h"
#include "coap-engine.h"
#include "dev/button-hal.h"
#include "coap-blocking-api.h"
#include <stdio.h>
#include <string.h>

extern coap_resource_t compactor_sensor_endpoint;
extern coap_endpoint_t compactor_sensor_address;



// Shared variables for CoAP PUT operation
static int coap_put_pending = 0;

// Button press event handler
static void button_event_handler(button_hal_button_t *btn) {
    coap_put_pending = 1; // Signal to the main thread that a request is pending
    printf("CoAP PUT request prepared to turn compactor ON.\n");
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

// Process to handle button events and CoAP PUT requests
PROCESS(compactor_actuator_process, "Compactor Actuator");
AUTOSTART_PROCESSES(&compactor_actuator_process);

PROCESS_THREAD(compactor_actuator_process, ev, data)
{
    PROCESS_BEGIN();

    printf("Compactor Actuator Process started.\n");

    // Register the CoAP resource
    coap_activate_resource(&compactor_sensor_endpoint, "compactor/config");

    // Initialize button-hal
    button_hal_init();

    while (1) {
        PROCESS_WAIT_EVENT();

        if (ev == button_hal_press_event) {
            button_event_handler((button_hal_button_t *)data);
        }

        // Handle pending CoAP PUT request
        if (coap_put_pending) {
            printf("Sending CoAP PUT request to turn compactor ON.\n");

            //const coap_endpoint_t compactor_address = get_compactor_sensor_address();

            if (strlen(compactor_uri) == 0) {
                printf("Compactor sensor endpoint not configured. Aborting request.\n");
            } else {
                static coap_message_t request[1];
                // Prepare the CoAP PUT request
                coap_init_message(request, COAP_TYPE_CON, COAP_PUT, 0);
                coap_set_header_uri_path(request, "/compactor/active");
                coap_set_payload(request, (uint8_t *)"true", strlen("true"));

                // Send the CoAP request
                COAP_BLOCKING_REQUEST(&compactor_sensor_address, request, client_chunk_handler);
                printf("CoAP PUT request sent to turn compactor ON.\n");
            }

            coap_put_pending = 0; // Clear the pending flag
        }
    }

    PROCESS_END();
}
