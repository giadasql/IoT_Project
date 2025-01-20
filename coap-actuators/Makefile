CONTIKI_PROJECT = coap-actuators
all: $(CONTIKI_PROJECT)

MODULES += os/net/ipv6 os/net/routing os/net/app-layer/coap
MODULES_REL += ../utils
MODULES_REL += ./resources

CONTIKI = ../../..
include $(CONTIKI)/Makefile.include
