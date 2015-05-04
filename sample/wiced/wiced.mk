
NAME := App_wiced_$(RTOS)_$(NETWORK)
WILDDOG_TOP_DIR := ../../

$(NAME)_SOURCES += $(WILDDOG_TOP_DIR)/src/coap/option.c $(WILDDOG_TOP_DIR)/src/coap/pdu.c $(WILDDOG_TOP_DIR)/ports/wiced/wilddog_wiced.c

$(NAME)_SOURCES += $(WILDDOG_TOP_DIR)/src/cjson/cJSON.c

$(NAME)_SOURCES += $(WILDDOG_TOP_DIR)/src/url_parser.c $(WILDDOG_TOP_DIR)/src/wilddog_debug.c $(WILDDOG_TOP_DIR)/src/wilddog.c

$(NAME)_SOURCES += wilddog_client.c

$(NAME)_INCLUDES += $(WILDDOG_TOP_DIR)/src $(WILDDOG_TOP_DIR)/src/cjson $(WILDDOG_TOP_DIR)/src/coap $(WILDDOG_TOP_DIR)/ports $(WILDDOG_TOP_DIR)/ports/wiced $(WILDDOG_TOP_DIR)/sample/wiced

#==============================================================================
# Configuration
#==============================================================================

WIFI_CONFIG_DCT_H := wifi_config_dct.h

FreeRTOS_START_STACK := 600
ThreadX_START_STACK  := 600

#==============================================================================
# Global defines
#==============================================================================
GLOBAL_DEFINES += UART_BUFFER_SIZE=128

ifeq ($(PLATFORM),$(filter $(PLATFORM), BCM94390WCD1 BCM94390WCD2 BCM94390WCD3))
GLOBAL_DEFINES += TX_PACKET_POOL_SIZE=14 RX_PACKET_POOL_SIZE=14
GLOBAL_DEFINES += PBUF_POOL_TX_SIZE=8 PBUF_POOL_RX_SIZE=8
else
ifneq ($(PLATFORM),BCM943362WCD2)
GLOBAL_DEFINES += TX_PACKET_POOL_SIZE=15 RX_PACKET_POOL_SIZE=15
GLOBAL_DEFINES += PBUF_POOL_TX_SIZE=8 PBUF_POOL_RX_SIZE=8
endif
endif
