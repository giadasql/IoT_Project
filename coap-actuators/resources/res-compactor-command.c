#include "contiki.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include "process.h"
#include <stdio.h>
#include <string.h>

bool send_compactor_command = false;
bool compactor_value_to_send = false;
extern process_event_t compactor_command_event; // Declare the event
extern struct process compactor_actuator_process; // Reference the main process

// CoAP PUT handler to configure the lid sensor endpoint
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
        } else if (strncmp((char *)payload, "turn off", len) == 0) {
            compactor_value_to_send = false;
            send_compactor_command = true;
            printf("Command received: turn off. compactor_value_to_send set to false.\n");
        } else {
            printf("Invalid command received: %.*s\n", (int)len, payload);
        }
        // Post event to wake up the main process
        process_post(&compactor_actuator_process, compactor_command_event, NULL);
    } else {
        printf("Empty payload received.\n");
    }
}


// CoAP resource for configuring the lid sensor endpoint
RESOURCE(compactor_actuator_command, "title=\"Command Compactor Actuator\";rt=\"Text\"",
         NULL, NULL, compactor_actuator_command_put_handler, NULL);
