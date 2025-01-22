#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "contiki-net.h"
#include "coap-engine.h"

/*----------------------------------------------------------------------------*/
/* The path of the resource to observe */
#define OBS_RESOURCE_URI "scale/value"

/*----------------------------------------------------------------------------*/
static coap_endpoint_t server_ipaddr; /* holds the server ip address */
static coap_observee_t *obs;

/*----------------------------------------------------------------------------*/
PROCESS(er_example_observe_client, "Erbium Coap Observe Client Example");
AUTOSTART_PROCESSES(&er_example_observe_client);

/*----------------------------------------------------------------------------*/
/*
 * Handle the response to the observe request and the following notifications
 */
static void
notification_callback(coap_observee_t *obs, void *notification,
                      coap_notification_flag_t flag)
{
  int len = 0;
  const uint8_t *payload = NULL;

  printf("Notification handler\n");
  printf("Observee URI: %s\n", obs->url);
  if(notification) {
    len = coap_get_payload(notification, &payload);
  }
  switch(flag) {
  case NOTIFICATION_OK:
    printf("NOTIFICATION OK: %*s\n", len, (char *)payload);
    break;
  case OBSERVE_OK: /* server accepeted observation request */
    printf("OBSERVE_OK: %*s\n", len, (char *)payload);
    break;
  case OBSERVE_NOT_SUPPORTED:
    printf("OBSERVE_NOT_SUPPORTED: %*s\n", len, (char *)payload);
    obs = NULL;
    break;
  case ERROR_RESPONSE_CODE:
    printf("ERROR_RESPONSE_CODE: %*s\n", len, (char *)payload);
    obs = NULL;
    break;
  case NO_REPLY_FROM_SERVER:
    printf("NO_REPLY_FROM_SERVER: "
           "removing observe registration with token %x%x\n",
           obs->token[0], obs->token[1]);
    obs = NULL;
    break;
  }
}
/*----------------------------------------------------------------------------*/
/*
 * Toggle the observation of the remote resource
 */
void
toggle_observation(void)
{
  if(obs) {
    printf("Stopping observation\n");
    obs = NULL;
  } else {
    printf("Starting observation\n");
    coap_endpoint_parse("fe80::206:6:6:6", strlen("fe80::206:6:6:6"), &server_ipaddr);
    obs = coap_obs_request_registration(&server_ipaddr,
                                        OBS_RESOURCE_URI, notification_callback, NULL);
  }
}
/*----------------------------------------------------------------------------*/
/*
 * The main (proto-)thread. It starts/stops the observation of the remote
 * resource every time the timer elapses or the button (if available) is
 * pressed
 */
PROCESS_THREAD(er_example_observe_client, ev, data)
{
  PROCESS_BEGIN();
  toggle_observation();

  while(1) {
    PROCESS_WAIT_EVENT();
  }

  PROCESS_END();
}