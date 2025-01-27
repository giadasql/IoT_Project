#include "contiki.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include <stdio.h>
#include <string.h>

// Sensor Configuration Structure
typedef struct {
    char endpoint_uri[64];
    coap_endpoint_t address;
    const char *name;
    const char *resource_path;
} sensor_config_t;

// Sensor Configurations
sensor_config_t lid_sensor = {"", {}, "lid_sensor", "lid/config"};

// Helper Function to Configure Sensor
static bool configure_sensor(const char *payload, sensor_config_t *sensor) {
    if (strlen(payload) >= sizeof(sensor->endpoint_uri)) {
        printf("Payload too large for %s configuration.\n", sensor->name);
        return false;
    }

    strncpy(sensor->endpoint_uri, payload, sizeof(sensor->endpoint_uri) - 1);
    sensor->endpoint_uri[sizeof(sensor->endpoint_uri) - 1] = '\0';

    if (coap_endpoint_parse(sensor->endpoint_uri, strlen(sensor->endpoint_uri), &sensor->address)) {
        printf("Configured %s endpoint: %s\n", sensor->name, sensor->endpoint_uri);
        return true;
    } else {
        printf("Invalid CoAP endpoint for %s: %s\n", sensor->name, sensor->endpoint_uri);
        return false;
    }
}

// Generic PUT Handler for Sensor Configuration
static void sensor_config_put_handler(coap_message_t *req, coap_message_t *res,
                                      uint8_t *buf, uint16_t preferred_size, int32_t *offset,
                                      sensor_config_t *sensor) {
    const uint8_t *payload;
    size_t len = coap_get_payload(req, &payload);

    if (len > 0 && configure_sensor((const char *)payload, sensor)) {
        coap_set_status_code(res, CHANGED_2_04);
    } else {
        printf("Failed to configure %s.\n", sensor->name);
        coap_set_status_code(res, BAD_REQUEST_4_00);
    }
}

// Generic GET Handler for Sensor Configuration
static void sensor_config_get_handler(coap_message_t *req, coap_message_t *res,
                                      uint8_t *buf, uint16_t preferred_size, int32_t *offset,
                                      sensor_config_t *sensor) {
    snprintf((char *)buf, preferred_size, "{\"uri\":\"%s\"}", sensor->endpoint_uri);
    coap_set_header_content_format(res, APPLICATION_JSON);
    coap_set_payload(res, buf, strlen((char *)buf));
}

// CoAP Handlers for Lid Sensor
static void lid_sensor_put_handler(coap_message_t *req, coap_message_t *res,
                                   uint8_t *buf, uint16_t preferred_size, int32_t *offset) {
    sensor_config_put_handler(req, res, buf, preferred_size, offset, &lid_sensor);
}

static void lid_sensor_get_handler(coap_message_t *req, coap_message_t *res,
                                   uint8_t *buf, uint16_t preferred_size, int32_t *offset) {
    sensor_config_get_handler(req, res, buf, preferred_size, offset, &lid_sensor);
}

// CoAP Resources
RESOURCE(lid_sensor_config, "title=\"Lid Sensor Config\";rt=\"Text\"",
         lid_sensor_get_handler, NULL, lid_sensor_put_handler, NULL);
