#include "resource_utils.h"
#include "coap-engine.h"
#include "conversion_utils.h"
#include <stdio.h>
#include <string.h>

// Generic GET Handler
void generic_get_handler(coap_message_t *request, coap_message_t *response,
                         uint8_t *buffer, uint16_t preferred_size, int32_t *offset,
                         const generic_sensor_t *sensor) {
    char state_buffer[64];

    // Convert the sensor state to a string
    sensor->to_string(state_buffer, sizeof(state_buffer), sensor->state);

    // Format the response payload
    snprintf((char *)buffer, preferred_size,
             "{\"%s\":{\"value\":\"%s\"}}",
             sensor->name, state_buffer);

    // Set the CoAP response payload
    coap_set_payload(response, buffer, strlen((char *)buffer));
}

// Generic PUT Handler
void generic_put_handler(coap_message_t *request, coap_message_t *response,
                         uint8_t *buffer, uint16_t preferred_size, int32_t *offset,
                         const generic_sensor_t *sensor) {
    const uint8_t *payload = NULL;
    size_t len = coap_get_payload(request, &payload);

    if (len > 0) {
        // Update the sensor state using the payload
        sensor->update_state((const char *)payload, sensor->state);
        coap_set_status_code(response, CHANGED_2_04);
    } else {
        coap_set_status_code(response, BAD_REQUEST_4_00);
    }
}
