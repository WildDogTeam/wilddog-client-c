



NAME = App_skel

$(NAME)_SOURCES := main.c src/url_parser.c src/wilddog_debug.c src/Wilddog.c src/cjson/cJSON.c port/wiced.c

$(NAME)_INCLUDES := src src/cjson src/coap

WIFI_CONFIG_DCT_H := wifi_config_dct.h

#sudo ./make wilddog.skel-BCM943362WCD4-ThreadX-NetX_Duo-SDIO
