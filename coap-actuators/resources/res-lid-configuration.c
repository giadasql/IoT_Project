#include "contiki.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include <stdio.h>
#include <string.h>

// --------------------- ONLY NEEDED FOR SIMULATION ---------------------
// For simulation purposes, we need to store the address of the lid sensor because the
// lid actuator needs to send CoAP requests to the lid sensor to toggle its state.

// Store the address of the lid sensor
char lid_sensor_endpoint_uri[64] = "";
coap_endpoint_t lid_sensor_address;

// CoAP PUT handler to configure the lid sensor address
static void lid_sensor_endpoint_put_handler(coap_message_t *request, coap_message_t *response,
                                            uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {

    printf("CoAP handler invoked with payload: %.*s\n", preferred_size, buffer);
    size_t len = coap_get_payload(request, (const uint8_t **)&buffer);

    if (len > 0 && len < sizeof(lid_sensor_endpoint_uri)) {
        strncpy(lid_sensor_endpoint_uri, (char *)buffer, len);
        lid_sensor_endpoint_uri[len] = '\0';

        if (coap_endpoint_parse(lid_sensor_endpoint_uri, strlen(lid_sensor_endpoint_uri), &lid_sensor_address)) {
            printf("Configured lid sensor endpoint: %s\n", lid_sensor_endpoint_uri);
            coap_set_status_code(response, CHANGED_2_04);
        } else {
            printf("Invalid CoAP endpoint: %s\n", lid_sensor_endpoint_uri);
            coap_set_status_code(response, BAD_REQUEST_4_00);
        }
    } else {
        printf("Invalid configuration payload.\n");
        coap_set_status_code(response, BAD_REQUEST_4_00);
    }
}

// CoAP GET handler to retrieve the lid sensor address
static void lid_sensor_endpoint_get_handler(coap_message_t *request, coap_message_t *response,
                                            uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    snprintf((char *)buffer, preferred_size, "{\"uri\":\"%s\"}", lid_sensor_endpoint_uri);
    coap_set_header_content_format(response, TEXT_PLAIN);
    coap_set_payload(response, buffer, strlen((char *)buffer));
}

// CoAP resource for configuring the lid sensor address
RESOURCE(lid_sensor_endpoint, "title=\"Configure Lid Sensor Endpoint\";rt=\"Text\"",
         lid_sensor_endpoint_get_handler, NULL, lid_sensor_endpoint_put_handler, NULL);
