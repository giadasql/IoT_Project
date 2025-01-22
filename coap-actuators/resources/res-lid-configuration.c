#include "contiki.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Shared variables for the sensors
char lid_sensor_endpoint_uri[64] = "";
char scale_sensor_endpoint_uri[64] = "";
char waste_level_sensor_endpoint_uri[64] = "";
coap_endpoint_t lid_sensor_address;
coap_endpoint_t scale_sensor_address;
coap_endpoint_t waste_level_sensor_address;

// Helper function to parse and configure a single sensor
static bool configure_sensor(const char *json_key, const char *json_payload, coap_endpoint_t *endpoint, char *uri_buffer) {
    char key[32] = {0};
    char uri[64] = {0};
    int result = sscanf(json_payload, "{\"%31[^\"]\":\"%63[^\"]\"}", key, uri);

    if (result == 2 && strcmp(json_key, key) == 0) {
        strncpy(uri_buffer, uri, sizeof(uri_buffer) - 1);
		uri_buffer[sizeof(uri_buffer) - 1] = '\0'; // Ensure null-termination

        if (coap_endpoint_parse(uri, strlen(uri), endpoint)) {
            printf("Configured sensor endpoint for %s: %s\n", json_key, uri);
            return true;
        } else {
            printf("Invalid endpoint URI for %s: %s\n", json_key, uri);
            return false;
        }
    }

    return false;
}

// Combined PUT handler
static void combined_sensor_config_put_handler(coap_message_t *req, coap_message_t *res,
                                               uint8_t *buf, uint16_t preferred_size, int32_t *offset) {
    const uint8_t *payload = NULL;
    size_t len = coap_get_payload(req, &payload);

    if (len == 0 || payload == NULL) {
        printf("Empty payload received.\n");
        coap_set_status_code(res, BAD_REQUEST_4_00);
        return;
    }

	printf("Received payload length: %d\n", (int)len);
    printf("Received configuration payload: %.*s\n", (int)len, payload);

    bool lid_configured = configure_sensor("lid_sensor", (const char *)payload, &lid_sensor_address, lid_sensor_endpoint_uri);
    bool scale_configured = configure_sensor("scale_sensor", (const char *)payload, &scale_sensor_address, scale_sensor_endpoint_uri);
    bool waste_level_configured = configure_sensor("waste_level_sensor", (const char *)payload, &waste_level_sensor_address, waste_level_sensor_endpoint_uri);

    if (lid_configured || scale_configured || waste_level_configured) {
        coap_set_status_code(res, CHANGED_2_04);
    } else {
        printf("No valid configurations found in payload.\n");
        coap_set_status_code(res, BAD_REQUEST_4_00);
    }
}

// Combined GET handler
static void combined_sensor_config_get_handler(coap_message_t *req, coap_message_t *res,
                                               uint8_t *buf, uint16_t preferred_size, int32_t *offset) {
    snprintf((char *)buf, preferred_size,
             "{\"lid_sensor\":\"%s\",\"scale_sensor\":\"%s\",\"waste_level_sensor\":\"%s\"}",
             lid_sensor_endpoint_uri, scale_sensor_endpoint_uri, waste_level_sensor_endpoint_uri);
    coap_set_header_content_format(res, APPLICATION_JSON);
    coap_set_payload(res, buf, strlen((char *)buf));
}

// Combined resource definition
RESOURCE(sensor_config, "title=\"Sensor Config\";rt=\"JSON\"",
         combined_sensor_config_get_handler, NULL, combined_sensor_config_put_handler, NULL);
