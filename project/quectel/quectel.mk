
APP_PROTO_TYPE=coap
#security method, such as dtls/nosec
APP_SEC_TYPE=nosec

PORT_TYPE=quectel

WILDDOG_TOP_DIR= wilddog

C_PREDEF += -D WILDDOG_PORT_TYPE_QUCETEL

WILDDOG_INC :=
WILDDOG_SRC :=
WILDDOG_SRC_DIR := $(WILDDOG_TOP_DIR)

WILDDOG_INC += -I $(WILDDOG_TOP_DIR)/include

#------------------------------------------------------------------------------
# Examples
#------------------------------------------------------------------------------
WILDDOG_EXAMPLE = $(wildcard $(WILDDOG_TOP_DIR)/examples/$(PORT_TYPE)/*.c)
WILDDOG_INC += -I $(WILDDOG_TOP_DIR)/examples/$(PORT_TYPE)
WILDDOG_SRC_DIR += $(WILDDOG_TOP_DIR)\examples $(WILDDOG_TOP_DIR)\examples\$(PORT_TYPE)

#------------------------------------------------------------------------------
# platform
#------------------------------------------------------------------------------
WILDDOG_PLATFORM = $(wildcard $(WILDDOG_TOP_DIR)/platform/$(PORT_TYPE)/*.c)
WILDDOG_INC += -I $(WILDDOG_TOP_DIR)/platform/$(PORT_TYPE)
WILDDOG_SRC_DIR += $(WILDDOG_TOP_DIR)\platform $(WILDDOG_TOP_DIR)\platform\$(PORT_TYPE)

#------------------------------------------------------------------------------
# src
#------------------------------------------------------------------------------
WILDDOG_COMMON = $(wildcard $(WILDDOG_TOP_DIR)/src/*.c)
WILDDOG_INC += -I $(WILDDOG_TOP_DIR)/src
WILDDOG_SRC_DIR += $(WILDDOG_TOP_DIR)\src

#------------------------------------------------------------------------------
# networking
#------------------------------------------------------------------------------
WILDDOG_NETWORK = $(wildcard $(WILDDOG_TOP_DIR)/src/networking/$(APP_PROTO_TYPE)/*.c)
WILDDOG_INC += -I $(WILDDOG_TOP_DIR)/src/networking/$(APP_PROTO_TYPE)
WILDDOG_SRC_DIR +=  $(WILDDOG_TOP_DIR)\src\networking $(WILDDOG_TOP_DIR)\src\networking\$(APP_PROTO_TYPE)
#------------------------------------------------------------------------------
# secure
#------------------------------------------------------------------------------
ifeq ($(APP_SEC_TYPE),nosec)
C_PREDEF += -D WILDDOG_PORT=5683
WILDDOG_SECURE = $(wildcard $(WILDDOG_TOP_DIR)/src/secure/$(APP_SEC_TYPE)/*.c)
WILDDOG_INC += -I $(WILDDOG_TOP_DIR)/src/secure/$(APP_SEC_TYPE)
WILDDOG_SRC_DIR += $(WILDDOG_TOP_DIR)\src\secure $(WILDDOG_TOP_DIR)\src\secure\$(APP_SEC_TYPE)
else
ifeq ($(APP_SEC_TYPE),dtls)
C_PREDEF += -D WILDDOG_PORT=5684
WILDDOG_SECURE = $(wildcard $(WILDDOG_TOP_DIR)/src/secure/$(APP_SEC_TYPE)/lib/*.c)
WILDDOG_INC += -I $(WILDDOG_TOP_DIR)/src/secure/$(APP_SEC_TYPE)/inc/
WILDDOG_SRC_DIR += $(WILDDOG_TOP_DIR)\src\secure \
$(WILDDOG_TOP_DIR)\src\secure\$(APP_SEC_TYPE) \
$(WILDDOG_TOP_DIR)\src\secure\$(APP_SEC_TYPE)\lib

else

endif
endif

#------------------------------------------------------------------------------
# serialize
#------------------------------------------------------------------------------
WILDDOG_SERIAL = $(wildcard $(WILDDOG_TOP_DIR)/src/serialize/cbor/*.c)
WILDDOG_INC += -I $(WILDDOG_TOP_DIR)/src/serialize/cbor
WILDDOG_SRC_DIR += $(WILDDOG_TOP_DIR)\src\serialize $(WILDDOG_TOP_DIR)\src\serialize\cbor

WILDDOG_TEST=
WILDDOG_TEST += $(WILDDOG_TOP_DIR)/platform/quectel/wilddog_quectel.c
#WILDDOG_TEST += $(WILDDOG_TOP_DIR)/src/serialize/cbor/wilddog_cbor.c
#WILDDOG_TEST += $(WILDDOG_TOP_DIR)/src/wilddog_common.c
#WILDDOG_TEST += $(WILDDOG_TOP_DIR)/src/wilddog_node.c
#WILDDOG_TEST += $(WILDDOG_TOP_DIR)/src/performtest.c
#WILDDOG_TEST += $(WILDDOG_TOP_DIR)/src/ramtest.c
#WILDDOG_TEST += $(WILDDOG_TOP_DIR)/src/wilddog_api.c
#WILDDOG_TEST += $(WILDDOG_TOP_DIR)/src/wilddog_ct.c
#WILDDOG_TEST += $(WILDDOG_TOP_DIR)/src/wilddog_debug.c
#WILDDOG_TEST += $(WILDDOG_TOP_DIR)/src/wilddog_url_parser.c
#WILDDOG_TEST += $(WILDDOG_TOP_DIR)/src/wilddog_store.c
#WILDDOG_TEST += $(WILDDOG_TOP_DIR)/src/wilddog_event.c
#WILDDOG_TEST += $(WILDDOG_TOP_DIR)/src/wilddog_conn.c
#WILDDOG_TEST += $(WILDDOG_TOP_DIR)/src/wilddog_sec_host.c
WILDDOG_TEST += $(patsubst %.c, $(OBJ_DIR)/%.o, $(WILDDOG_COMMON))
WILDDOG_TEST += $(WILDDOG_TOP_DIR)/src/secure/nosec/wilddog_nosec.c
WILDDOG_TEST += $(WILDDOG_TOP_DIR)/src/serialize/cbor/wilddog_cbor.c

WILDDOG_TEST += $(WILDDOG_TOP_DIR)/src/networking/coap/wilddog_conn_coap.c
WILDDOG_TEST += $(WILDDOG_TOP_DIR)/src/networking/coap/pdu.c
WILDDOG_TEST += $(WILDDOG_TOP_DIR)/src/networking/coap/option.c

#WILDDOG_SRC=$(patsubst %.c, $(OBJ_DIR)/%.o, $(WILDDOG_TEST))

WILDDOG_SRC=\
	$(patsubst %.c, $(OBJ_DIR)/%.o, $(WILDDOG_EXAMPLE)) \
	$(patsubst %.c, $(OBJ_DIR)/%.o, $(WILDDOG_PLATFORM)) \
	$(patsubst %.c, $(OBJ_DIR)/%.o, $(WILDDOG_COMMON)) \
	$(patsubst %.c, $(OBJ_DIR)/%.o, $(WILDDOG_NETWORK)) \
	$(patsubst %.c, $(OBJ_DIR)/%.o, $(WILDDOG_SECURE)) \
	$(patsubst %.c, $(OBJ_DIR)/%.o, $(WILDDOG_SERIAL)) \



OBJS += $(WILDDOG_SRC)
INCS += $(WILDDOG_INC)
SRC_DIRS += $(WILDDOG_SRC_DIR)