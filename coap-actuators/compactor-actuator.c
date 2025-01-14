#include "contiki.h"
#include "coap-engine.h"
#include "dev/button-hal.h"
#include "coap-blocking-api.h"
#include <stdio.h>
#include <string.h>

// Define thresholds for short and long presses (in clock ticks)
#define SHORT_PRESS_DURATION (CLOCK_SECOND * 1) // 1 second for short press
#define LONG_PRESS_DURATION  (CLOCK_SECOND * 3) // 3 seconds for long press

// CoAP server endpoint for the compactor sensor
static coap_endpoint_t compactor_sensor_endpoint;
static char compactor_sensor_endpoint_uri[64] = ""; // Buffer to store endpoint URI

static coap_message_t request[1];

// Actuator state
static int compactor_state = 0; // 0: off, 1: on

// Shared variables for CoAP PUT operation
static int coap_put_pending = 0;
static const char *coap_put_payload = NULL;

// Timer to measure button press duration
static struct timer button_timer;

// Function to handle button press duration
static void handle_button_press_duration(clock_time_t duration) {
    printf("Button pressed for %lu ticks.\n", (unsigned long)duration);

    if (duration < SHORT_PRESS_DURATION) {
        printf("Short press detected. Turning compactor OFF.\n");
        compactor_state = 0;
    } else if (duration >= LONG_PRESS_DURATION) {
        printf("Long press detected. Turning compactor ON.\n");
        compactor_state = 1;
    }

    // Prepare for CoAP PUT request
    if (strlen(compactor_sensor_endpoint_uri) == 0) {
        printf("Compactor sensor endpoint not configured. Skipping CoAP PUT.\n");
        return;
    }

    coap_put_payload = compactor_state ? "true" : "false";
    coap_put_pending = 1; // Signal to the main thread that a request is pending
    printf("CoAP PUT request prepared with payload: %s\n", coap_put_payload);
}

// Button press event handler
static void button_event_handler(button_hal_button_t *btn) {
        // Start the timer when the button is pressed
        timer_set(&button_timer, CLOCK_SECOND * 10); // Arbitrary max duration for long press
        printf("Button press started.\n");
}

static void button_release_handler(button_hal_button_t *btn) {
    // Measure the duration of the button press
    clock_time_t duration = timer_expired(&button_timer) ? CLOCK_SECOND * 10 : timer_remaining(&button_timer);
    handle_button_press_duration(duration);
    printf("Button press ended.\n");
}

// CoAP PUT handler to configure the compactor sensor endpoint
static void configure_compactor_endpoint_handler(coap_message_t *request, coap_message_t *response,
                                                  uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    size_t len = coap_get_payload(request, (const uint8_t **)&buffer);
    if (len > 0 && len < sizeof(compactor_sensor_endpoint_uri)) {
        strncpy(compactor_sensor_endpoint_uri, (char *)buffer, len);
        compactor_sensor_endpoint_uri[len] = '\0';

        if (coap_endpoint_parse(compactor_sensor_endpoint_uri, strlen(compactor_sensor_endpoint_uri), &compactor_sensor_endpoint)) {
            printf("Configured compactor sensor endpoint: %s\n", compactor_sensor_endpoint_uri);
            coap_set_status_code(response, CHANGED_2_04);
        } else {
            printf("Invalid CoAP endpoint: %s\n", compactor_sensor_endpoint_uri);
            coap_set_status_code(response, BAD_REQUEST_4_00);
        }
    } else {
        printf("Invalid configuration payload.\n");
        coap_set_status_code(response, BAD_REQUEST_4_00);
    }
}

// CoAP resource for configuring the endpoint
RESOURCE(configure_compactor_endpoint, "title=\"Configure Compactor Endpoint\";rt=\"Text\"",
         NULL, configure_compactor_endpoint_handler, NULL, NULL);

// Process to handle button events and CoAP PUT requests
PROCESS(compactor_actuator_process, "Compactor Actuator");
AUTOSTART_PROCESSES(&compactor_actuator_process);

PROCESS_THREAD(compactor_actuator_process, ev, data)
{
    PROCESS_BEGIN();

    printf("Compactor Actuator Process started.\n");

    // Register the CoAP resource
    coap_activate_resource(&configure_compactor_endpoint, "actuator/compactor/config");

    // Initialize button-hal
    button_hal_init();

    while (1) {
        PROCESS_YIELD();

        if (ev == button_hal_press_event) {
            button_event_handler((button_hal_button_t *)data);
        } else if (ev == button_hal_release_event) {
            button_release_handler((button_hal_button_t *)data);
        }

        // Handle pending CoAP PUT request
        if (coap_put_pending && coap_put_payload != NULL) {
            printf("Sending CoAP PUT request to update compactor state: %s\n", coap_put_payload);
            coap_init_message(request, COAP_TYPE_CON, COAP_PUT, 0);
            coap_set_header_uri_path(request, "/compactor/active");
            coap_set_payload(request, (uint8_t *)coap_put_payload, strlen(coap_put_payload));

            COAP_BLOCKING_REQUEST(&compactor_sensor_endpoint, request, NULL);
            printf("CoAP PUT request sent: %s\n", coap_put_payload);

            coap_put_pending = 0; // Clear the pending flag
        }
    }

    PROCESS_END();
}
