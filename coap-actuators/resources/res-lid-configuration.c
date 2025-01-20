#include "contiki.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include <stdio.h>
#include <string.h>

// Shared variables for the lid sensor
char lid_sensor_endpoint_uri[64] = ""; // Buffer to store the lid sensor endpoint URI
coap_endpoint_t lid_sensor_address;    // Parsed CoAP endpoint for the lid sensor

// CoAP PUT handler to configure the lid sensor endpoint
static void lid_sensor_address_put_handler(coap_message_t *req, coap_message_t *res,
                                           uint8_t *buf, uint16_t preferred_size, int32_t *offset) {
    size_t len = coap_get_payload(req, (const uint8_t **)&buf);
    if (len > 0 && len < sizeof(lid_sensor_endpoint_uri)) {
        // Save the URI and parse it into an endpoint
        strncpy(lid_sensor_endpoint_uri, (char *)buf, len);
        lid_sensor_endpoint_uri[len] = '\0';

        if (coap_endpoint_parse(lid_sensor_endpoint_uri, strlen(lid_sensor_endpoint_uri), &lid_sensor_address)) {
            printf("Configured lid sensor endpoint: %s\n", lid_sensor_endpoint_uri);
            coap_set_status_code(res, CHANGED_2_04);
        } else {
            printf("Invalid CoAP endpoint: %s\n", lid_sensor_endpoint_uri);
            coap_set_status_code(res, BAD_REQUEST_4_00);
        }
    } else {
        printf("Invalid configuration payload.\n");
        coap_set_status_code(res, BAD_REQUEST_4_00);
    }
}

// CoAP GET handler to retrieve the current configured lid sensor endpoint
static void lid_sensor_address_get_handler(coap_message_t *req, coap_message_t *res,
                                           uint8_t *buf, uint16_t preferred_size, int32_t *offset) {
    snprintf((char *)buf, preferred_size, "{\"uri\":\"%s\"}", lid_sensor_endpoint_uri);
    coap_set_header_content_format(res, APPLICATION_JSON);
    coap_set_payload(res, buf, strlen((char *)buf));
}

RESOURCE(lid_sensor_config, "title=\"Lid Sensor Config\";rt=\"Text\"",
         lid_sensor_address_get_handler, NULL, lid_sensor_address_put_handler, NULL);
