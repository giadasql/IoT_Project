#include "contiki.h"
#include "coap-engine.h"
#include "dev/button-hal.h"
#include <stdio.h>
#include <string.h>
#include "coap-blocking-api.h"

// Define thresholds for short and long presses (in clock ticks)
#define SHORT_PRESS_DURATION (CLOCK_SECOND * 1) // 1 second for short press
#define LONG_PRESS_DURATION  (CLOCK_SECOND * 3) // 3 seconds for long press

// CoAP server endpoint for the compactor sensor
static coap_endpoint_t compactor_sensor_endpoint;
static char compactor_sensor_endpoint_uri[64] = ""; // Buffer to store endpoint URI

static coap_message_t request[1];

// Actuator state
static int compactor_state = 0; // 0: off, 1: on

// Function to update compactor sensor state via CoAP PUT
static void update_compactor_sensor(int state) {
    if (strlen(compactor_sensor_endpoint_uri) == 0) {
        printf("Compactor sensor endpoint not configured. Skipping CoAP PUT.\n");
        return;
    }

    const char *payload = state ? "true" : "false";

    printf("Sending CoAP PUT to update compactor state: %s\n", payload);
    coap_init_message(request, COAP_TYPE_CON, COAP_PUT, 0);
    coap_set_header_uri_path(request, "/compactor/active");
    coap_set_payload(request, (uint8_t *)payload, strlen(payload));

    COAP_BLOCKING_REQUEST(&compactor_sensor_endpoint, request, NULL);
    printf("CoAP PUT request sent: %s\n", payload);
}

// Function to handle button press duration
static void handle_button_press_duration(button_hal_button_t *btn, clock_time_t duration) {
    printf("Button pressed for %lu ticks.\n", (unsigned long)duration);

    if (duration < SHORT_PRESS_DURATION) {
        printf("Short press detected. Turning compactor OFF.\n");
        compactor_state = 0;
    } else if (duration >= LONG_PRESS_DURATION) {
        printf("Long press detected. Turning compactor ON.\n");
        compactor_state = 1;
    }

    // Send CoAP PUT to update the sensor
    update_compactor_sensor(compactor_state);
}

// Button press event handler
static void button_event_handler(button_hal_button_t *btn) {
    if (btn->press_duration > 0) {
        handle_button_press_duration(btn, btn->press_duration);
    } else {
        printf("Button press with unknown duration.\n");
    }
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

// Process to handle button events
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
        }
    }

    PROCESS_END();
}
