#include "contiki.h"
#include "coap-engine.h"
#include "dev/button-hal.h"
#include "coap-blocking-api.h"
#include <stdio.h>
#include <string.h>

// Shared variables for CoAP PUT operation
static int coap_put_pending = 0;

extern coap_resource_t compactor_sensor_endpoint;

// Button press event handler
static void button_event_handler(button_hal_button_t *btn) {
    printf("Button pressed. Turning compactor ON.\n");

    /*
    // Prepare for CoAP PUT request
    const char *endpoint_uri = get_compactor_sensor_endpoint_uri();
    if (strlen(endpoint_uri) == 0) {
        printf("Compactor sensor endpoint not configured. Skipping CoAP PUT.\n");
        return;
    }

    coap_put_pending = 1; // Signal to the main thread that a request is pending
    printf("CoAP PUT request prepared to turn compactor ON.\n"); */
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
    // button_hal_init();

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

            /* coap_endpoint_t *endpoint = get_compactor_sensor_endpoint();
            COAP_BLOCKING_REQUEST(endpoint, request, NULL);
            printf("CoAP PUT request sent to turn compactor ON.\n"); */

            coap_put_pending = 0; // Clear the pending flag
        }
    }

    PROCESS_END();
}
