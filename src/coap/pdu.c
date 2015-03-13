/* pdu.c -- CoAP message structure
 *
 * Copyright (C) 2010,2011 Olaf Bergmann <bergmann@tzi.org>
 *
 * This file is part of the CoAP library libcoap. Please see
 * README for terms of use. 
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "pdu.h"
#include "option.h"
#include "wilddog_debug.h"
#include "port.h"
void
coap_pdu_clear(coap_pdu_t *pdu, size_t size) {
  assert(pdu);

  memset(pdu, 0, sizeof(coap_pdu_t) + size);
  pdu->max_size = size;
  pdu->hdr = (coap_hdr_t *)((unsigned char *)pdu + sizeof(coap_pdu_t));
  pdu->hdr->version = COAP_DEFAULT_VERSION;

  /* data is NULL unless explicitly set by coap_add_data() */
  pdu->length = sizeof(coap_hdr_t);
}

coap_pdu_t *
coap_pdu_init(unsigned char type, unsigned char code, 
	      unsigned short id, size_t size) {
  coap_pdu_t *pdu;


  assert(size <= COAP_MAX_PDU_SIZE);
  /* Size must be large enough to fit the header. */
  if (size < sizeof(coap_hdr_t) || size > COAP_MAX_PDU_SIZE)
    return NULL;


  pdu = wd_malloc(sizeof(coap_pdu_t) + size);


  if (pdu) {
    coap_pdu_clear(pdu, size);
    pdu->hdr->id = id;
    pdu->hdr->type = type;
    pdu->hdr->code = code;
  } 
  return pdu;
}

coap_pdu_t *
coap_new_pdu() {
  coap_pdu_t *pdu;
  pdu = coap_pdu_init(0, 0, ntohs(COAP_INVALID_TID), COAP_MAX_PDU_SIZE);
  return pdu;
}

void
coap_delete_pdu(coap_pdu_t *pdu) {
  wd_free( pdu );
}

int
coap_add_token(coap_pdu_t *pdu, size_t len, const unsigned char *data) {
  const size_t HEADERLENGTH = len + 4;
  /* must allow for pdu == NULL as callers may rely on this */
  if (!pdu || len > 8 || pdu->max_size < HEADERLENGTH)
    return 0;

  pdu->hdr->token_length = len;
  if (len)
    memcpy(pdu->hdr->token, data, len);
  pdu->max_delta = 0;
  pdu->length = HEADERLENGTH;
  pdu->data = NULL;

  return 1;
}

/** @FIXME de-duplicate code with coap_add_option_later */
size_t
coap_add_option(coap_pdu_t *pdu, unsigned short type, unsigned int len, const unsigned char *data) {
  size_t optsize;
  coap_opt_t *opt;
  
  assert(pdu);
  pdu->data = NULL;

  if (type < pdu->max_delta) {
    WD_WARN("coap_add_option: options are not in correct order\n");
    return 0;
  }

  opt = (unsigned char *)pdu->hdr + pdu->length;

  /* encode option and check length */
  optsize = coap_opt_encode(opt, pdu->max_size - pdu->length, 
			    type - pdu->max_delta, data, len);

  if (!optsize) {
    WD_WARN("coap_add_option: cannot add option\n");
    /* error */
    return 0;
  } else {
    pdu->max_delta = type;
    pdu->length += optsize;
  }

  return optsize;
}

/** @FIXME de-duplicate code with coap_add_option */
unsigned char*
coap_add_option_later(coap_pdu_t *pdu, unsigned short type, unsigned int len) {
  size_t optsize;
  coap_opt_t *opt;

  assert(pdu);
  pdu->data = NULL;

  if (type < pdu->max_delta) {
     WD_WARN("coap_add_option: options are not in correct order\n");
    return NULL;
  }

  opt = (unsigned char *)pdu->hdr + pdu->length;

  /* encode option and check length */
  optsize = coap_opt_encode(opt, pdu->max_size - pdu->length,
			    type - pdu->max_delta, NULL, len);

  if (!optsize) {
    WD_WARN("coap_add_option: cannot add option\n");
    /* error */
    return NULL;
  } else {
    pdu->max_delta = type;
    pdu->length += optsize;
  }

  return ((unsigned char*)opt) + optsize - len;
}

int
coap_add_data(coap_pdu_t *pdu, unsigned int len, const unsigned char *data) {
  assert(pdu);
  assert(pdu->data == NULL);

  if (len == 0)
    return 1;

  if (pdu->length + len + 1 > pdu->max_size) {
    WD_WARN("coap_add_data: cannot add: data too large for PDU\n");
    assert(pdu->data == NULL);
    return 0;
  }

  pdu->data = (unsigned char *)pdu->hdr + pdu->length;
  *pdu->data = COAP_PAYLOAD_START;
  pdu->data++;

  memcpy(pdu->data, data, len);
  pdu->length += len + 1;
  return 1;
}

int
coap_get_data(coap_pdu_t *pdu, size_t *len, unsigned char **data) {
  assert(pdu);
  assert(len);
  assert(data);

  if (pdu->data) {
    *len = (unsigned char *)pdu->hdr + pdu->length - pdu->data;
    *data = pdu->data;
  } else {			/* no data, clear everything */
    *len = 0;
    *data = NULL;
  }

  return *data != NULL;
}


/**
 * Advances *optp to next option if still in PDU. This function 
 * returns the number of bytes opt has been advanced or @c 0
 * on error.
 */
static size_t
next_option_safe(coap_opt_t **optp, size_t *length) {
  coap_option_t option;
  size_t optsize;

  assert(optp); assert(*optp); 
  assert(length);

  optsize = coap_opt_parse(*optp, *length, &option);
  if (optsize) {
    assert(optsize <= *length);

    *optp += optsize;
    *length -= optsize;
  }

  return optsize;
}

int
coap_pdu_parse(unsigned char *data, size_t length, coap_pdu_t *pdu) {
  coap_opt_t *opt;

  assert(data);
  assert(pdu);

  if (pdu->max_size < length) {
    WD_DEBUG("insufficient space to store parsed PDU\n");
    return 0;
  }

  if (length < sizeof(coap_hdr_t)) {
    WD_DEBUG("discarded invalid PDU\n");
  }

  pdu->hdr->version = data[0] >> 6;
  pdu->hdr->type = (data[0] >> 4) & 0x03;
  pdu->hdr->token_length = data[0] & 0x0f;
  pdu->hdr->code = data[1];
  pdu->data = NULL;

  /* sanity checks */
  if (pdu->hdr->code == 0) {
    if (length != sizeof(coap_hdr_t) || pdu->hdr->token_length) {
      WD_DEBUG("coap_pdu_parse: empty message is not empty\n");
      goto discard;
    }
  }

  if (length < sizeof(coap_hdr_t) + pdu->hdr->token_length
      || pdu->hdr->token_length > 8) {
    WD_DEBUG("coap_pdu_parse: invalid Token\n");
    goto discard;
  }

  /* Copy message id in network byte order, so we can easily write the
   * response back to the network. */
  memcpy(&pdu->hdr->id, data + 2, 2);

  /* append data (including the Token) to pdu structure */
  memcpy(pdu->hdr + 1, data + sizeof(coap_hdr_t), length - sizeof(coap_hdr_t));
  pdu->length = length;
  
  /* Finally calculate beginning of data block and thereby check integrity
   * of the PDU structure. */

  /* skip header + token */
  length -= (pdu->hdr->token_length + sizeof(coap_hdr_t));
  opt = (unsigned char *)(pdu->hdr + 1) + pdu->hdr->token_length;

  while (length && *opt != COAP_PAYLOAD_START) {

    if (!next_option_safe(&opt, (size_t *)&length)) {
      WD_DEBUG("coap_pdu_parse: drop\n");
      goto discard;
    }
  }

  /* end of packet or start marker */
  if (length) {
    assert(*opt == COAP_PAYLOAD_START);
    opt++; length--;

    if (!length) {
      WD_DEBUG("coap_pdu_parse: message ending in payload start marker\n");
      goto discard;
    }

    WD_DEBUG("set data to %p (pdu ends at %p)\n", (unsigned char *)opt,
	  (unsigned char *)pdu->hdr + pdu->length);
    pdu->data = (unsigned char *)opt;
  }

  return 1;

 discard:
  return 0;
}
