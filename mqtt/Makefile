CONTIKI_PROJECT = mqtt
all: $(CONTIKI_PROJECT)

MODULES += os/net/ipv6 os/net/routing os/net/app-layer/coap
MODULES_REL += ./resources

include $(CONTIKI)/Makefile.dir-variables
MODULES += $(CONTIKI_NG_APP_LAYER_DIR)/mqtt

include $(CONTIKI)/Makefile.identify-target

MODULES_REL += arch/platform/$(TARGET)

CONTIKI = ../../..
include $(CONTIKI)/Makefile.include
