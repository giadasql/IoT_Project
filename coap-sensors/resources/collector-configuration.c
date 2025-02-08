#include "contiki.h"
#include "coap-engine.h"
#include <stdio.h>
#include <string.h>

// Collector Address Configuration
char collector_address[64] = ""; // Buffer to store the collector's address

// PUT Handler to Configure Collector Address
static void collector_config_put_handler(coap_message_t *req, coap_message_t *res,
                                         uint8_t *buf, uint16_t preferred_size, int32_t *offset) {
    const uint8_t *payload = NULL;
    size_t len = coap_get_payload(req, &payload);

    if (len > 0 && len < sizeof(collector_address)) {
        // Save the collector address
        strncpy(collector_address, (const char *)payload, len);
        collector_address[len] = '\0';
        printf("Configured collector address: %s\n", collector_address);

        // Set response status
        coap_set_status_code(res, CHANGED_2_04);
    } else {
        printf("Invalid or empty payload received for collector configuration.\n");
        coap_set_status_code(res, BAD_REQUEST_4_00);
    }
}

// GET Handler to Retrieve the Current Collector Address
static void collector_config_get_handler(coap_message_t *req, coap_message_t *res,
                                         uint8_t *buf, uint16_t preferred_size, int32_t *offset) {
    snprintf((char *)buf, preferred_size, "{\"collector_address\":\"%s\"}", collector_address);
    coap_set_header_content_format(res, APPLICATION_JSON);
    coap_set_payload(res, buf, strlen((char *)buf));
}

// CoAP Resource for Collector Configuration
RESOURCE(collector_config, "title=\"Collector Config\";rt=\"Text\"",
         collector_config_get_handler, NULL, collector_config_put_handler, NULL);
