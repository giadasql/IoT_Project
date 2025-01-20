#include "contiki.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include <stdio.h>
#include <string.h>

// CoAP server endpoint for the compactor sensor
static char compactor_sensor_endpoint_uri[64] = ""; // Buffer to store endpoint URI
static coap_endpoint_t compactor_sensor_address;


// CoAP PUT handler to configure the compactor sensor endpoint
static void compactor_sensor_endpoint_put_handler(coap_message_t *request, coap_message_t *response,
                                                  uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    printf("CoAP handler invoked with payload: %.*s\n", preferred_size, buffer);

    size_t len = coap_get_payload(request, (const uint8_t **)&buffer);
    if (len > 0 && len < sizeof(compactor_sensor_endpoint_uri)) {
        strncpy(compactor_sensor_endpoint_uri, (char *)buffer, len);
        compactor_sensor_endpoint_uri[len] = '\0';
        coap_set_status_code(response, CHANGED_2_04);
        if (coap_endpoint_parse(compactor_sensor_endpoint_uri, strlen(compactor_sensor_endpoint_uri), &compactor_sensor_address)) {
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

static void compactor_sensor_endpoint_get_handler(coap_message_t *request, coap_message_t *response,
                                                    uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    snprintf((char *)buffer, preferred_size, "{\"uri\":\"%s\"}", compactor_sensor_endpoint_uri);
    coap_set_header_content_format(response, TEXT_PLAIN);
    coap_set_payload(response, buffer, strlen((char *)buffer));
}

// CoAP resource for configuring the endpoint
RESOURCE(compactor_sensor_endpoint, "title=\"Configure Compactor Endpoint\";rt=\"Text\"",
         compactor_sensor_endpoint_get_handler, NULL, compactor_sensor_endpoint_put_handler, NULL);
