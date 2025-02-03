#include "contiki.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include <stdio.h>
#include <string.h>

bool send_command_flag = false; // Flag to send the CoAP command
bool value = false; // value to send

// CoAP PUT handler to configure the lid sensor endpoint
static void compactor_actuator_command_put_handler(coap_message_t *request, coap_message_t *response,
                                            uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    printf("CoAP handler invoked with payload: %.*s\n", preferred_size, buffer);

    size_t len = coap_get_payload(request, (const uint8_t **)&buffer);
    if (len > 0) {
        if (strncmp((char *)payload, "turn on", len) == 0) {
            value = true;
            send_command_flag = true;
            printf("Command received: turn on. Value set to true.\n");
        } else if (strncmp((char *)payload, "turn off", len) == 0) {
            value = false;
            send_command_flag = true;
            printf("Command received: turn off. Value set to false.\n");
        } else {
            printf("Invalid command received: %.*s\n", (int)len, payload);
        }
    } else {
        printf("Empty payload received.\n");
    }
}


// CoAP resource for configuring the lid sensor endpoint
RESOURCE(compactor_actuator_command, "title=\"Command Compactor Actuator\";rt=\"Text\"",
         NULL, NULL, compactor_actuator_command_put_handler, NULL);
