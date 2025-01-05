CONTIKI_PROJECT = coap-sensors
all: $(CONTIKI_PROJECT)

MODULES += os/net/ipv6 os/net/routing os/net/app-layer/coap
MODULES_REL += ./resources
MODULES_REL += ./sensor_utils

CONTIKI = ../../..
include $(CONTIKI)/Makefile.include
