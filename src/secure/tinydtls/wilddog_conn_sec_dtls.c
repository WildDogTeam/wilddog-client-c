#include "tinydtls.h" 
#include "dtls_config.h"
/* This is needed for apple */
#define __APPLE_USE_RFC_3542

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <netinet/in.h>
//#include <errno.h>
//#include <sys/types.h>
//#include <sys/socket.h>
//#include <sys/time.h>
//#include <arpa/inet.h>
//#include <netdb.h>
//#include <signal.h>

#include "global.h" 
#include "debug.h" 
#include "dtls.h" 
#include "session.h" 

#include "wilddog.h"
#include "wilddog_port.h"

#ifdef __GNUC__
#define UNUSED_PARAM __attribute__((unused))
#else
#define UNUSED_PARAM
#endif /* __GNUC__ */

typedef struct WILDDOG_CONN_SEC_T{
	size_t d_fd;
	dtls_context_t *dtls_context;
	session_t dst;
	unsigned short d_delstState;
	u8 *p_recvbuf;
	u32 d_recvlen;
	u8 d_recvFig;
}Wilddog_Conn_Sec_T;
static Wilddog_Conn_Sec_T d_conn_sec_dtls;
#if 0
static const unsigned char server_pub_x[]=
{
	0x20,0x8f,0xb9,0xbb][42][e8][71][ee][2d][69][b][8d][35][b][ea][9e][4d][a7][1e][93][97][6e][6b][89][17][89][c7][80][31][62][a0][4c]

};
static const unsigned char server_pub_y[]=
{
	[6b][54][5c][4b][3a][95][38][92][96][9e][c][86][9b][42][51][18][8e][9b][19][d7][8b][a8][fa][3e][d][52][9c][dd][a6][34][e7][ea]

};
#endif

/*publick key*/
static const unsigned char server_pub_key_x[] = {

	0x20,0x8f,0xb9,0xbb,0x42,0xe8,0x71,0xee,0x2d,0x69,0xb,0x8d,0x35,0xb,0xea,0x9e,
	0x4d,0xa7,0x1e,0x93,0x97,0x6e,0x6b,0x89,0x17,0x89,0xc7,0x80,0x31,0x62,0xa0,0x4c
	};

static const unsigned char server_pub_key_y[] = {
			
			0x6b,0x54,0x5c,0x4b,0x3a,0x95,0x38,0x92,0x96,0x9e,0xc,0x86,0x9b,0x42,0x51,0x18,
			0x8e,0x9b,0x19,0xd7,0x8b,0xa8,0xfa,0x3e,0xd,0x52,0x9c,0xdd,0xa6,0x34,0xe7,0xea,
			};

/* todo */
static const unsigned char ecdsa_priv_key[] = {
			0x41, 0xC1, 0xCB, 0x6B, 0x51, 0x24, 0x7A, 0x14,
			0x43, 0x21, 0x43, 0x5B, 0x7A, 0x80, 0xE7, 0x14,
			0x89, 0x6A, 0x33, 0xBB, 0xAD, 0x72, 0x94, 0xCA,
			0x40, 0x14, 0x55, 0xA1, 0x94, 0xA9, 0x49, 0xFA};

static const unsigned char ecdsa_pub_key_x[] = {
			0x36, 0xDF, 0xE2, 0xC6, 0xF9, 0xF2, 0xED, 0x29,
			0xDA, 0x0A, 0x9A, 0x8F, 0x62, 0x68, 0x4E, 0x91,
			0x63, 0x75, 0xBA, 0x10, 0x30, 0x0C, 0x28, 0xC5,
			0xE4, 0x7C, 0xFB, 0xF2, 0x5F, 0xA5, 0x8F, 0x52};

static const unsigned char ecdsa_pub_key_y[] = {
			0x71, 0xA0, 0xD4, 0xFC, 0xDE, 0x1A, 0xB8, 0x78,
			0x5A, 0x3C, 0x78, 0x69, 0x35, 0xA7, 0xCF, 0xAB,
			0xE9, 0x3F, 0x98, 0x72, 0x09, 0xDA, 0xED, 0x0B,
			0x4F, 0xAB, 0xC3, 0x6F, 0xC7, 0x72, 0xF8, 0x29};

#ifdef DTLS_PSK
ssize_t
read_from_file(char *arg, unsigned char *buf, size_t max_buf_len) {
  FILE *f;
  ssize_t result = 0;

  f = fopen(arg, "r");
  if (f == NULL)
    return -1;

  while (!feof(f)) {
    size_t bytes_read;
    bytes_read = fread(buf, 1, max_buf_len, f);
    if (ferror(f)) {
      result = -1;
      break;
    }

    buf += bytes_read;
    result += bytes_read;
    max_buf_len -= bytes_read;
  }

  fclose(f);
  return result;
}

/* The PSK information for DTLS */
#define PSK_ID_MAXLEN 256
#define PSK_MAXLEN 256
static unsigned char psk_id[PSK_ID_MAXLEN];
static size_t psk_id_length = 0;
static unsigned char psk_key[PSK_MAXLEN];
static size_t psk_key_length = 0;

/* This function is the "key store" for tinyDTLS. It is called to
 * retrieve a key for the given identity within this particular
 * session. */
static int
get_psk_info(struct dtls_context_t *ctx UNUSED_PARAM,
	    const session_t *session UNUSED_PARAM,
	    dtls_credentials_type_t type,
	    const unsigned char *id, size_t id_len,
	    unsigned char *result, size_t result_length) {

  switch (type) {
  case DTLS_PSK_IDENTITY:
    if (id_len) {
      dtls_debug("got psk_identity_hint: '%.*s'\n", id_len, id);
    }

    if (result_length < psk_id_length) {
      dtls_warn("cannot set psk_identity -- buffer too small\n");
      return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
    }

    memcpy(result, psk_id, psk_id_length);
    return psk_id_length;
  case DTLS_PSK_KEY:
    if (id_len != psk_id_length || memcmp(psk_id, id, id_len) != 0) {
      dtls_warn("PSK for unknown id requested, exiting\n");
      return dtls_alert_fatal_create(DTLS_ALERT_ILLEGAL_PARAMETER);
    } else if (result_length < psk_key_length) {
      dtls_warn("cannot set psk -- buffer too small\n");
      return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
    }

    memcpy(result, psk_key, psk_key_length);
    return psk_key_length;
  default:
    dtls_warn("unsupported request type: %d\n", type);
  }

  return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
}
#endif /* DTLS_PSK */

#ifdef DTLS_ECC
static int
get_ecdsa_key(struct dtls_context_t *ctx,
	      const session_t *session,
	      const dtls_ecdsa_key_t **result) {
  static const dtls_ecdsa_key_t ecdsa_key = {
    .curve = DTLS_ECDH_CURVE_SECP256R1,
    .priv_key = ecdsa_priv_key,
    .pub_key_x = ecdsa_pub_key_x,
    .pub_key_y = ecdsa_pub_key_y
  };

  *result = &ecdsa_key;
  return 0;
}
void os_printf_nchar(unsigned char *p,int n)
{
	int i=0;
	if(!p)
		return;
	for(i=0;i<n;i++){
		printf("[%0x]",p[i]);
	}
	printf("\n");
}
static int
verify_ecdsa_key(struct dtls_context_t *ctx,
		 const session_t *session,
		 const unsigned char *other_pub_x,
		 const unsigned char *other_pub_y,
		 size_t key_size) {
#if 0
  printf("<><>other_pub_x\n");
  os_printf_nchar(other_pub_x,32);
    printf("<><>other_pub_y\n");
  os_printf_nchar(other_pub_y,32);
#endif  
  if(	memcmp(server_pub_key_x,other_pub_x,32 ) != 0 || 
  		memcmp(server_pub_key_y,other_pub_y,32 ) != 0)
  		return -1;

  return 0;
}
#endif /* DTLS_ECC */
static int
read_from_peer(struct dtls_context_t *ctx, 
	       session_t *session, uint8 *data, size_t len) {
  size_t i,readlen;
  printf("read_from_peer :\n");
  if(d_conn_sec_dtls.p_recvbuf == NULL)
  	return 0;
  d_conn_sec_dtls.d_recvFig = 1;
  readlen = (d_conn_sec_dtls.d_recvlen > len)?len:d_conn_sec_dtls.d_recvlen;
  for (i = 0; i < len; i++)
  {
    printf("%p", data[i]);
  	d_conn_sec_dtls.p_recvbuf[i] = data[i];
    }
    d_conn_sec_dtls.d_recvlen = readlen;
  return 0;
}

static int
send_to_peer(struct dtls_context_t *ctx, 
	     session_t *session, uint8 *data, size_t len) {
  int fd = *(int *)dtls_get_app_data(ctx);
  int res = 0;
  Wilddog_Address_T addr_inSend;
  printf("send_to_peer: fd = %d/n",fd);
  printf("iplen=%d",session->size);
#if 0
  res =  sendto(fd, data, len, 0/*MSG_DONTWAIT*/,
		&session->addr.sa, session->size);
#endif	

  addr_inSend.len = session->size;
  addr_inSend.port = session->addr.sin.sin_port;
  
  memcpy(addr_inSend.ip,&session->addr.sin.sin_addr,session->size);
  res = wilddog_send(fd,&addr_inSend,data,len);

  printf("res = %d\n",res);
  return res;
//  return wilddog_send(fd,);
}

static int
dtls_handle_read(struct dtls_context_t *ctx) {
  int fd;
  session_t *p_session = &d_conn_sec_dtls.dst;
  Wilddog_Address_T addr_in;
#define MAX_READ_BUF 2000
  static uint8 buf[MAX_READ_BUF];
  int len;

  fd = *(int *)dtls_get_app_data(ctx);
  if (!fd)
    return -1;

  addr_in.len = d_conn_sec_dtls.dst.size;
  addr_in.port = d_conn_sec_dtls.dst.addr.sin.sin_port;
  memcpy(addr_in.ip,&d_conn_sec_dtls.dst.addr.sin.sin_addr,d_conn_sec_dtls.dst.size);
#if 0
  wilddog_debug("fd=%d;",fd); 
  wilddog_debug( "addr_in->port = %d, ip = %u.%u.%u.%u\n", addr_in.port, addr_in.ip[0], \
  					   addr_in.ip[1], addr_in.ip[2], addr_in.ip[3]);
 #endif
   memset(buf, 0, MAX_READ_BUF);
  len = wilddog_receive(fd,&addr_in,buf,MAX_READ_BUF,WILDDOG_RECEIVE_TIMEOUT);
  if (len < 0) {
  	
    return -1;
  } else if(len >0){
  
    dtls_dsrv_log_addr(DTLS_LOG_DEBUG, "peer", p_session);
    dtls_debug_dump("bytes from peer", buf, len);
    return dtls_handle_message(ctx, p_session, buf, len);
  }
  return 0;
}    
int handle_dtls_event (dtls_context_t *ctx, 
        session_t *session, dtls_alert_level_t level, unsigned short code)
{
	printf("handle_dtls_event ::  code = %p \n",code);
	d_conn_sec_dtls.d_delstState = code;
    if (level > 0) {
        perror("DTLS ERROR EVENT");
    }
    return 0;
}
static dtls_handler_t cb = {
  .write = send_to_peer,
  .read  = read_from_peer,
  .event = handle_dtls_event,
#ifdef DTLS_PSK
  .get_psk_info = get_psk_info,
#endif /* DTLS_PSK */
#ifdef DTLS_ECC
  .get_ecdsa_key = get_ecdsa_key,
  .verify_ecdsa_key = verify_ecdsa_key
#endif /* DTLS_ECC */
};
/*@ wilddog dtls api
*/
#define DTLS_CLIENT_CMD_CLOSE "client:close"
#define DTLS_CLIENT_CMD_RENEGOTIATE "client:renegotiate"
#define WD_DEBUG_DTLS 0 // DTLS_LOG_EMERG for non  DTLS_LOG_DEBUG for all

/* do not malloc session*/
STATIC int _wilddog_sec_setSession(int fd, Wilddog_Address_T * addr_in)
{
	d_conn_sec_dtls.d_fd = fd;
	d_conn_sec_dtls.dst.addr.sin.sin_family = 2;//AF_INET  ipv4
	memcpy(&d_conn_sec_dtls.dst.addr.sin.sin_addr,addr_in->ip,addr_in->len);
	d_conn_sec_dtls.dst.size = addr_in->len;
	
	d_conn_sec_dtls.dst.addr.sin.sin_port = addr_in->port;
}
/*@
*dtls_context_t  Fields
	unsigned char 	cookie_secret [DTLS_COOKIE_SECRET_LENGTH]
	clock_time_t 	cookie_secret_age
	dtls_peer_t * 	peers
	void * 	app
	dtls_handler_t * 	h
	unsigned char 	readbuf [DTLS_MAX_BUF]
*/
Wilddog_Return_T _wilddog_sec_init
    (int fd, 
    Wilddog_Address_T * addr_in
    )
{
	int res = 0;
	memset(&d_conn_sec_dtls, 0, sizeof(Wilddog_Conn_Sec_T));
	_wilddog_sec_setSession(fd,addr_in);
	
	dtls_init();
	dtls_set_log_level(WD_DEBUG_DTLS);
	d_conn_sec_dtls.dtls_context = dtls_new_context(&d_conn_sec_dtls.d_fd);
	if(d_conn_sec_dtls.dtls_context == NULL)
		return -1;
	/*@ register dtls cb*/
	dtls_set_handler(d_conn_sec_dtls.dtls_context, &cb);
	/*@ Establishes a DTLS channel with the specified remote peer dst.  
	**@ star client Hello
	*/
	res = dtls_connect(d_conn_sec_dtls.dtls_context, &d_conn_sec_dtls.dst);
	while(d_conn_sec_dtls.d_delstState != DTLS_EVENT_CONNECTED)
	{
		res = dtls_handle_read(d_conn_sec_dtls.dtls_context);
		if(res < 0)
			break;
	}
	return res;
}
Wilddog_Return_T _wilddog_sec_send
    (
    int fd, 
    Wilddog_Address_T * addr_in, 
    void* p_data, 
    s32 len
    )
{		
	uint8 *p_buf = p_data;
	s32 unsendlen = len;
	int res;
	
	_wilddog_sec_setSession(fd,addr_in);
	
	if(d_conn_sec_dtls.d_delstState != DTLS_EVENT_CONNECTED)
		return 0;
	while(unsendlen)
	{		
		res = dtls_write(d_conn_sec_dtls.dtls_context, &d_conn_sec_dtls.dst, (uint8 *)p_buf, unsendlen);
		printf("res = %d;unsendlen=%d,len=%d\n",res,unsendlen,len);
		if (res >= 0 && (res != unsendlen)) {
			   memmove(p_buf, p_buf + res, unsendlen - res);
			   unsendlen -= res;
		 }
		 else 
		 {
			if(res >= 0 )
				res = len;
			break;
		 }
	}
	
	return res;
}
int _wilddog_sec_recv
    (
    int fd, 
    Wilddog_Address_T * addr_in, 
    void* p_data, 
    s32 len
    )
{
	int res = 0;
	_wilddog_sec_setSession(fd,addr_in);
	d_conn_sec_dtls.p_recvbuf = p_data;
	d_conn_sec_dtls.d_recvlen = len;
	d_conn_sec_dtls.d_recvFig = 0;
	/* decode recv */
	
	res = dtls_handle_read(d_conn_sec_dtls.dtls_context);
	if( d_conn_sec_dtls.d_recvFig )
		res = d_conn_sec_dtls.d_recvlen;
	return res;
	
}

Wilddog_Return_T _wilddog_sec_deinit
    (
    int fd, 
    Wilddog_Address_T * addr_in
    )
{
	
	  dtls_free_context(d_conn_sec_dtls.dtls_context);
	  return 0;
}

