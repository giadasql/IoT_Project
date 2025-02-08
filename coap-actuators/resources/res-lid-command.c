#include "contiki.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include <stdio.h>
#include <string.h>

bool send_lid_command = false; // Flag to send the CoAP command
bool lid_value_to_send = false; // value to send

// Event that will be posted when a command is received
process_event_t lid_command_event;

// CoAP PUT handler to receive commands for the lid actuator. Accepted values: open, close
static void lid_actuator_command_put_handler(coap_message_t *request, coap_message_t *response,
                                            uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {

    printf("CoAP handler invoked with payload: %.*s\n", preferred_size, buffer);
    size_t len = coap_get_payload(request, (const uint8_t **)&buffer);
    char* payload = (char *)buffer;

    if (len > 0) {
        if (strncmp((char *)payload, "open", len) == 0) {
          // command to open the lid, set the value to send to true and set the flag to send the command
            lid_value_to_send = true;
            send_lid_command = true;
            printf("Command received: open. Value set to true.\n");
            coap_set_status_code(response, CHANGED_2_04);
        } else if (strncmp((char *)payload, "close", len) == 0) {
            // command to close the lid, set the value to send to false and set the flag to send the command
            lid_value_to_send = false;
            send_lid_command = true;
            printf("Command received: close. Value set to false.\n");
            coap_set_status_code(response, CHANGED_2_04);
        } else {
            printf("Invalid command received: %.*s\n", (int)len, payload);
            coap_set_status_code(response, BAD_REQUEST_4_00);
        }

        // Post event to wake up the main process that will handle the command
        process_post(PROCESS_BROADCAST, lid_command_event, NULL);
    } else {
        printf("Empty payload received.\n");
        coap_set_status_code(response, BAD_REQUEST_4_00);
    }
}

// CoAP resource for configuring the lid actuator command
RESOURCE(lid_actuator_command, "title=\"Command Lid Actuator\";rt=\"Text\"",
         NULL, NULL, lid_actuator_command_put_handler, NULL);
