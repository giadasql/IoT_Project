#include "contiki.h"
#include "coap-engine.h"
#include "dev/button-hal.h"
#include "coap-blocking-api.h"
#include <stdio.h>
#include <string.h>

// Shared variables for CoAP PUT operation
static int coap_put_pending = 0;

extern coap_resource_t compactor_sensor_endpoint;
extern coap_endpoint_t compactor_sensor_address;

// Button press event handler
static void button_event_handler(button_hal_button_t *btn) {
    coap_put_pending = 1; // Signal to the main thread that a request is pending
    printf("CoAP PUT request prepared to turn compactor ON.\n");
}

// Process to handle button events and CoAP PUT requests
PROCESS(compactor_actuator_process, "Compactor Actuator");
AUTOSTART_PROCESSES(&compactor_actuator_process);

PROCESS_THREAD(compactor_actuator_process, ev, data)
{
    PROCESS_BEGIN();

    printf("Compactor Actuator Process started 2.\n");

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
            static coap_message_t request[1];
            coap_init_message(request, COAP_TYPE_CON, COAP_PUT, 0);
            coap_set_header_uri_path(request, "/compactor/active");
            coap_set_payload(request, (uint8_t *)"true", strlen("true"));

            // check if the compactor_sensor_address is defined
            // if not, print an error message
            if (compactor_sensor_address.port == 0) {
                printf("Error: Compactor sensor address is not defined.\n");
                continue;
            }

            COAP_BLOCKING_REQUEST(compactor_sensor_address, request, NULL);
            printf("CoAP PUT request sent to turn compactor ON.\n");

            coap_put_pending = 0; // Clear the pending flag
        }
    }

    PROCESS_END();
}
