#ifndef SENSOR_UTILS_H
#define SENSOR_UTILS_H

#include "coap-engine.h"

// Generic Sensor Structure
typedef struct {
    char *name;
    char *type;
    void *state;
    void (*to_string)(char *buffer, size_t size, void *state);
    void (*update_state)(const char *payload, void *state);
} generic_sensor_t;

// Function prototypes
void generic_get_handler(coap_message_t *request, coap_message_t *response,
                         uint8_t *buffer, uint16_t preferred_size, int32_t *offset,
                         const generic_sensor_t *sensor);
void generic_put_handler(coap_message_t *request, coap_message_t *response,
                         uint8_t *buffer, uint16_t preferred_size, int32_t *offset,
                         const generic_sensor_t *sensor);

#endif // SENSOR_UTILS_H
