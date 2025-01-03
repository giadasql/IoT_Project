#include "contiki.h"
#include "coap-engine.h"
#include <string.h>
#include <stdio.h>

// Simulated GPIO state for the lid sensor
static int lid_state = 0; // 0: closed, 1: open

// Function prototypes
static void lid_state_get_handler(coap_message_t *request, coap_message_t *response,
                                  uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void lid_state_put_handler(coap_message_t *request, coap_message_t *response,
                                  uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

RESOURCE(lid_sensor, 
	"title=\"Lid Sensor\";rt=\"Text\"",
         lid_state_get_handler, // GET handler
         lid_state_put_handler, // PUT handler
         NULL, NULL);

// GET Handler: Retrieve the current state of the lid sensor
static void
lid_state_get_handler(coap_message_t *request, coap_message_t *response,
                      uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
  const char *state = (lid_state == 1) ? "open" : "closed";
  snprintf((char *)buffer, preferred_size, "{\"state\":\"%s\"}", state);
  coap_set_payload(response, buffer, strlen((char *)buffer));
}

// PUT Handler: Update the state of the lid sensor
static void
lid_state_put_handler(coap_message_t *request, coap_message_t *response,
                      uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
  const uint8_t *payload = NULL;
  size_t len = coap_get_payload(request, &payload);

  if (len > 0) {
    if (strncmp((const char *)payload, "open", len) == 0) {
      lid_state = 1;
      printf("Lid state set to: open\n");
    } else if (strncmp((const char *)payload, "closed", len) == 0) {
      lid_state = 0;
      printf("Lid state set to: closed\n");
    } else {
      coap_set_status_code(response, BAD_REQUEST_4_00);
      return;
    }

    coap_set_status_code(response, CHANGED_2_04);
  } else {
    coap_set_status_code(response, BAD_REQUEST_4_00);
  }
}

