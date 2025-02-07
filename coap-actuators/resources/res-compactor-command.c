#include "contiki.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include <stdio.h>
#include <string.h>

bool send_compactor_command = false;
bool compactor_value_to_send = false;

// Event that will be posted when a command is received
process_event_t compactor_command_event;

// CoAP PUT handler to receive commands for the compactor actuator. Accepted values: turn on, turn off
static void compactor_actuator_command_put_handler(coap_message_t *request, coap_message_t *response,
                                            uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {

    size_t len = coap_get_payload(request, (const uint8_t **)&buffer);
    char* payload = (char *)buffer;

    printf("CoAP handler invoked with payload: %.*s\n", preferred_size, buffer);

    if (len > 0) {
        if (strncmp((char *)payload, "turn on", len) == 0) {
            compactor_value_to_send = true;
            send_compactor_command = true;
            printf("Command received: turn on. compactor_value_to_send set to true.\n");
            coap_set_status_code(response, CHANGED_2_04);
        } else if (strncmp((char *)payload, "turn off", len) == 0) {
            compactor_value_to_send = false;
            send_compactor_command = true;
            printf("Command received: turn off. compactor_value_to_send set to false.\n");
            coap_set_status_code(response, CHANGED_2_04);
        } else {
            printf("Invalid command received: %.*s\n", (int)len, payload);
            coap_set_status_code(response, BAD_REQUEST_4_00);
        }
        // Post event to wake up the main process that will handle the command
        process_post(PROCESS_BROADCAST, compactor_command_event, NULL);
    } else {
        printf("Empty payload received.\n");
        coap_set_status_code(response, BAD_REQUEST_4_00);
    }
}

// Configure the CoAP resource for the compactor actuator command
RESOURCE(compactor_actuator_command, "title=\"Command Compactor Actuator\";rt=\"Text\"",
         NULL, NULL, compactor_actuator_command_put_handler, NULL);
