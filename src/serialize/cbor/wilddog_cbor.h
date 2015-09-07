/*
 * Copyright (C) 2014-2016 Wilddog Technologies. All Rights Reserved. 
 *
 * FileName: wilddog_cbor.h
 *
 * Description: CBOR API header files.
 *
 * History:
 * Version      Author          Date        Description
 *
 * 0.4.0        Jimmy.Pan       2015-05-15  Create file.
 * 0.4.6        Jimmy.Pan       2015-09-06  Add notes.
 *
 */

#ifndef _WILDDOG_CBOR_H_
#define _WILDDOG_CBOR_H_

#ifdef __cplusplus
extern "C"
{
#endif
#include "wilddog.h"
#include "wilddog_config.h"


#define WILDDOG_CBOR_TYPE_MASK  0xe0 /*top 3 bits*/
#define WILDDOG_CBOR_TYPE_GET(_a)  (((_a) >> 5) & 0x7)

#define WILDDOG_CBOR_INFO_MASK  0x1f /*low 5 bits*/
#define WILDDOG_CBOR_INFO_GET(_a) ((_a) & 0x1f)

/*define special info means*/

#define WILDDOG_CBOR_FOLLOW_1BYTE       24
#define WILDDOG_CBOR_FOLLOW_2BYTE       25
#define WILDDOG_CBOR_FOLLOW_4BYTE       26
/*#define WILDDOG_CBOR_FOLLOW_8BYTE 27*/
#define WILDDOG_CBOR_FOLLOW_VAR         31

/*define major types*/
#define WILDDOG_CBOR_MAJORTYPE_NUM      8
#define WILDDOG_CBOR_UINT               0x00
#define WILDDOG_CBOR_NEGINT             0x20
#define WILDDOG_CBOR_BYTE_STRING        0x40
#define WILDDOG_CBOR_TEXT_STRING        0x60
#define WILDDOG_CBOR_ARRAY              0x80
#define WILDDOG_CBOR_MAP                0xa0
#define WILDDOG_CBOR_TAG                0xc0
#define WILDDOG_CBOR_SPECIAL            0xe0

/*major type 6 special */
#define WILDDOG_CBOR_FOLLOW_DATE        0x00

/*major type 7 special */
#define WILDDOG_CBOR_FALSE      (WILDDOG_CBOR_SPECIAL | 20)
#define WILDDOG_CBOR_TRUE       (WILDDOG_CBOR_SPECIAL | 21)
#define WILDDOG_CBOR_NULL       (WILDDOG_CBOR_SPECIAL | 22)
#define WILDDOG_CBOR_UNDEFINED  (WILDDOG_CBOR_SPECIAL | 23)
/* WILDDOG_CBOR_BYTE_FOLLOW == 24 */
#define WILDDOG_CBOR_FLOAT16    (WILDDOG_CBOR_SPECIAL | 25)
#define WILDDOG_CBOR_FLOAT32    (WILDDOG_CBOR_SPECIAL | 26)
#define WILDDOG_CBOR_FLOAT64    (WILDDOG_CBOR_SPECIAL | 27)
#define WILDDOG_CBOR_BREAK      (WILDDOG_CBOR_SPECIAL | 31)

#define WILDDOG_CBOR_TYPE(_a) ((_a) & WILDDOG_CBOR_TYPE_MASK)
#define WILDDOG_CBOR_INFO(_a) ((_a) & WILDDOG_CBOR_INFO_MASK)

#define WILDDOG_CBOR_HEAD_LEN  1
#define WILDDOG_CBOR_FOLLOW_1BYTE_LEN 1
#define WILDDOG_CBOR_FOLLOW_2BYTE_LEN 2
#define WILDDOG_CBOR_FOLLOW_4BYTE_LEN 4
#define WILDDOG_CBOR_FOLLOW_UNKNOW_LEN (-1)
#define WILDDOG_CBOR_FOLLOW_UNKNOW_DEFLEN 0xff
extern Wilddog_Node_T *_wilddog_cbor2Node(Wilddog_Payload_T* p_data);
extern Wilddog_Payload_T *_wilddog_node2Cbor(Wilddog_Node_T * p_node);

#ifdef __cplusplus
}
#endif

#endif /*_WILDDOG_CBOR_H_*/

