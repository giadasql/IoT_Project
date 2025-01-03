CONTIKI_PROJECT = mqtt
all: $(CONTIKI_PROJECT)
CONTIKI = ../../..

MODULES += os/net/ipv6 os/net/routing os/net/app-layer/coap

include $(CONTIKI)/Makefile.dir-variables
MODULES += $(CONTIKI_NG_APP_LAYER_DIR)/mqtt

include $(CONTIKI)/Makefile.identify-target

MODULES_REL += arch/platform/$(TARGET)


include $(CONTIKI)/Makefile.include
