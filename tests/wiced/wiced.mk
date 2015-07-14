
NAME := App_wiced_$(RTOS)_$(NETWORK)
WILDDOG_TOP_DIR := ../../

#app layer protocol, such as coap, now only support coap
APP_PROTO_TYPE=coap
#security method, such as dtls/nosec
APP_SEC_TYPE=nosec

PORT_TYPE=wiced

GLOBAL_DEFINES += WILDDOG_PORT_TYPE_WICED
$(NAME)_INCLUDES += $(WILDDOG_TOP_DIR)/include
########sample
$(NAME)_SOURCES += demo.c test_demo.c
##########port
$(NAME)_SOURCES += $(WILDDOG_TOP_DIR)/port/$(PORT_TYPE)/wilddog_wiced.c
##########src
$(NAME)_INCLUDES += $(WILDDOG_TOP_DIR)/src/include
########common
$(NAME)_SOURCES +=  $(WILDDOG_TOP_DIR)/src/common/wilddog_common.c $(WILDDOG_TOP_DIR)/src/common/wilddog_url_parser.c $(WILDDOG_TOP_DIR)/src/common/wilddog_node.c $(WILDDOG_TOP_DIR)/src/common/wilddog_debug.c
########connecter
$(NAME)_SOURCES += $(WILDDOG_TOP_DIR)/src/connecter/wilddog_conn.c
######appProto
ifeq ($(APP_PROTO_TYPE),coap)
$(NAME)_SOURCES += $(WILDDOG_TOP_DIR)/src/connecter/appProto/coap/option.c $(WILDDOG_TOP_DIR)/src/connecter/appProto/coap/pdu.c $(WILDDOG_TOP_DIR)/src/connecter/appProto/coap/wilddog_conn_coap.c 
$(NAME)_INCLUDES += $(WILDDOG_TOP_DIR)/src/connecter/appProto/coap/

endif
######secure
$(NAME)_INCLUDES += $(WILDDOG_TOP_DIR)/src/connecter/secure/
ifeq ($(APP_SEC_TYPE),nosec)
$(NAME)_SOURCES += $(WILDDOG_TOP_DIR)/src/connecter/secure/nosec/wilddog_nosec.c
GLOBAL_DEFINES += WILDDOG_PORT=5683
else
ifeq ($(APP_SEC_TYPE),dtls)
GLOBAL_DEFINES += WILDDOG_PORT=5684
$(NAME)_SOURCES += $(WILDDOG_TOP_DIR)/src/connecter/secure/dtls/lib/aes.c $(WILDDOG_TOP_DIR)/src/connecter/secure/dtls/lib/aesni.c \
$(WILDDOG_TOP_DIR)/src/connecter/secure/dtls/lib/asn1parse.c $(WILDDOG_TOP_DIR)/src/connecter/secure/dtls/lib/asn1write.c \
$(WILDDOG_TOP_DIR)/src/connecter/secure/dtls/lib/base64.c $(WILDDOG_TOP_DIR)/src/connecter/secure/dtls/lib/bignum.c \
$(WILDDOG_TOP_DIR)/src/connecter/secure/dtls/lib/blowfish.c $(WILDDOG_TOP_DIR)/src/connecter/secure/dtls/lib/ccm.c \
$(WILDDOG_TOP_DIR)/src/connecter/secure/dtls/lib/certs.c $(WILDDOG_TOP_DIR)/src/connecter/secure/dtls/lib/cipher.c \
$(WILDDOG_TOP_DIR)/src/connecter/secure/dtls/lib/cipher_wrap.c $(WILDDOG_TOP_DIR)/src/connecter/secure/dtls/lib/ctr_drbg.c \
$(WILDDOG_TOP_DIR)/src/connecter/secure/dtls/lib/debug.c $(WILDDOG_TOP_DIR)/src/connecter/secure/dtls/lib/dhm.c \
$(WILDDOG_TOP_DIR)/src/connecter/secure/dtls/lib/ecdh.c $(WILDDOG_TOP_DIR)/src/connecter/secure/dtls/lib/ecdsa.c \
$(WILDDOG_TOP_DIR)/src/connecter/secure/dtls/lib/ecp.c $(WILDDOG_TOP_DIR)/src/connecter/secure/dtls/lib/ecp_curves.c \
$(WILDDOG_TOP_DIR)/src/connecter/secure/dtls/lib/entropy.c $(WILDDOG_TOP_DIR)/src/connecter/secure/dtls/lib/entropy_poll.c \
$(WILDDOG_TOP_DIR)/src/connecter/secure/dtls/lib/hmac_drbg.c $(WILDDOG_TOP_DIR)/src/connecter/secure/dtls/lib/md.c \
$(WILDDOG_TOP_DIR)/src/connecter/secure/dtls/lib/md_wrap.c $(WILDDOG_TOP_DIR)/src/connecter/secure/dtls/lib/md5.c \
$(WILDDOG_TOP_DIR)/src/connecter/secure/dtls/lib/net.c $(WILDDOG_TOP_DIR)/src/connecter/secure/dtls/lib/oid.c \
$(WILDDOG_TOP_DIR)/src/connecter/secure/dtls/lib/pem.c $(WILDDOG_TOP_DIR)/src/connecter/secure/dtls/lib/pk.c \
$(WILDDOG_TOP_DIR)/src/connecter/secure/dtls/lib/pk_wrap.c $(WILDDOG_TOP_DIR)/src/connecter/secure/dtls/lib/pkcs5.c \
$(WILDDOG_TOP_DIR)/src/connecter/secure/dtls/lib/pkcs12.c $(WILDDOG_TOP_DIR)/src/connecter/secure/dtls/lib/pkparse.c \
$(WILDDOG_TOP_DIR)/src/connecter/secure/dtls/lib/ripemd160.c $(WILDDOG_TOP_DIR)/src/connecter/secure/dtls/lib/rsa.c \
$(WILDDOG_TOP_DIR)/src/connecter/secure/dtls/lib/sha1.c $(WILDDOG_TOP_DIR)/src/connecter/secure/dtls/lib/sha256.c \
$(WILDDOG_TOP_DIR)/src/connecter/secure/dtls/lib/ssl_ciphersuites.c $(WILDDOG_TOP_DIR)/src/connecter/secure/dtls/lib/ssl_cli.c \
$(WILDDOG_TOP_DIR)/src/connecter/secure/dtls/lib/ssl_srv.c $(WILDDOG_TOP_DIR)/src/connecter/secure/dtls/lib/ssl_tls.c \
$(WILDDOG_TOP_DIR)/src/connecter/secure/dtls/lib/timing.c $(WILDDOG_TOP_DIR)/src/connecter/secure/dtls/lib/wilddog_dtls.c \
$(WILDDOG_TOP_DIR)/src/connecter/secure/dtls/lib/x509.c $(WILDDOG_TOP_DIR)/src/connecter/secure/dtls/lib/x509_crt.c

$(NAME)_INCLUDES += $(WILDDOG_TOP_DIR)/src/connecter/secure/dtls/inc/
endif
endif
########container
$(NAME)_SOURCES += $(WILDDOG_TOP_DIR)/src/container/wilddog_api.c $(WILDDOG_TOP_DIR)/src/container/wilddog_ct.c
########dataType
$(NAME)_SOURCES += $(WILDDOG_TOP_DIR)/src/datatype/cbor/wilddog_cbor.c
$(NAME)_INCLUDES += $(WILDDOG_TOP_DIR)/src/datatype/cbor/ $(WILDDOG_TOP_DIR)/src/datatype/
########store
$(NAME)_SOURCES += $(WILDDOG_TOP_DIR)/src/store/wilddog_event.c $(WILDDOG_TOP_DIR)/src/store/wilddog_store.c


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
