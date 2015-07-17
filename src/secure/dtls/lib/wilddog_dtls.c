#include <string.h>
#include <stdio.h>

#include "wilddog.h"
#include "wilddog_debug.h"
#include "wilddog_config.h"
#include "wilddog_port.h"
#include "polarssl/net.h"
#include "polarssl/debug.h"
#include "polarssl/ssl.h"
#include "polarssl/entropy.h"
#include "polarssl/ctr_drbg.h"
#include "polarssl/error.h"
#include "polarssl/certs.h"
#include "polarssl/x509_crt.h"


#include "test_lib.h"
#define SERVER_NAME "Li"
extern int debug_get_threshold(void);
typedef struct _ctx
{
	int fd;
	Wilddog_Address_T * addr_in;
}CTX;

CTX ctx;
ssl_context ssl;
entropy_context entropy;
ctr_drbg_context ctr_drbg;    
x509_crt cacert;


/*
 * Function:    _wilddog_dtls_debug
 * Description: DTLS debug printf function
 * Input:       ctx: The context
 *              level: The printing level
 *              str: The printing string
 * Output:      N/A
 * Return:      N/A
*/
STATIC void _wilddog_dtls_debug( void *ctx, int level, const char *str )
{
    ((void) level);

	printf( "%s", str);
	fflush(stdout);
}

/*
 * Function:    wilddog_getssl
 * Description: get ssl context struct
 * Input:       N/A
 * Output:      N/A
 * Return:      ssl context struct
*/
STATIC ssl_context *wilddog_getssl(void)
{
	return &ssl;
}

/*
 * Function:    _net_send
 * Description: The net send function without timeout
 * Input:       ctx: The context   
 *				buf: The buffer which store the sending data
 *              len: The length of the wanting send data             
 * Return:      The length of the actual sending data
*/
int _net_send( void *ctx, const unsigned char *buf, size_t len )
{
	int fd;
	Wilddog_Address_T * addr_in;
	CTX content;
	
	content = *((CTX *)ctx);
	fd = content.fd;
	addr_in = content.addr_in;
	
	return wilddog_send(fd, addr_in, (void*)buf, (s32)len);

}

/*
 * Function:    _net_recv
 * Description: The net recv function without timeout
 * Input:       ctx: The context   
 *				len: The length of the wanting receive data
 * Output:      buf: The buffer which store the receiving data
 * Return:      The length of the actual receiving data
*/
int _net_recv( void *ctx, unsigned char *buf, size_t len )
{
	int fd;
	Wilddog_Address_T * addr_in;
	CTX content;
	
	content = *((CTX *)ctx);
	fd = content.fd;
	addr_in = content.addr_in;
	
	return wilddog_receive(fd, addr_in, (void*)buf, (s32)len, \
							WILDDOG_RECEIVE_TIMEOUT);

}

/*
 * Function:    _net_recv_timeout
 * Description: The net recv function with timeout
 * Input:       ctx: The context   
 *				len: The length of the wanting receive data
 *				timeout: Timeout
 * Output:      buf: The buffer which store the receiving data
 * Return:      The length of the actual receiving data
*/
int _net_recv_timeout
	( 
	void *ctx, 
	unsigned char *buf, 
	size_t len , 
	uint32_t timeout
	)
{
	int fd;
	Wilddog_Address_T * addr_in;
	CTX content;
	
	content = *((CTX *)ctx);
	fd = content.fd;
	addr_in = content.addr_in;
	
	return wilddog_receive(fd, addr_in, (void*)buf, (s32)len, timeout);

}

/*
 * Function:    _wilddog_sec_send
 * Description: Dtls security send function
 * Input:       fd: socket id    
 *				addr_in: The address which contains ip and port
 *				p_data: The buffer which store the sending data
 *				len: The length of the wanting send data
 * Output:      N/A
 * Return:      Success:0
*/
Wilddog_Return_T _wilddog_sec_send
	(
	int fd, 
	Wilddog_Address_T * addr_in, 
	void* p_data, 
	s32 len
	)
{
	int ret;

    do ret = ssl_write( wilddog_getssl(), (unsigned char *) p_data, len );
    while( ret == POLARSSL_ERR_NET_WANT_READ ||
           ret == POLARSSL_ERR_NET_WANT_WRITE );

	return WILDDOG_ERR_NOERR;
}

/*
 * Function:    _wilddog_sec_recv
 * Description: Dtls security recv function
 * Input:       fd: socket id    
 *				addr_in: The address which contains ip and port
 *				len: The length of the wanting receive data
 * Output:      p_data: The buffer which store the receiving data
 * Return:      The length of the actual receiving data
*/
int _wilddog_sec_recv
	(
	int fd, 
	Wilddog_Address_T * addr_in, 
	void* p_data, 
	s32 len
	)
{
	int ret;


    do ret = ssl_read( wilddog_getssl(), (unsigned char *)p_data, len );
    while( ret == POLARSSL_ERR_NET_WANT_READ ||
           ret == POLARSSL_ERR_NET_WANT_WRITE );


	return wilddog_getssl()->in_left ;
}

/*
 * Function:    _wilddog_sec_init
 * Description: Initialize dtls security session
 * Input:       fd: socket id    
 				addr_in: The address which contains ip and port
 * Output:      N/A
 * Return:      Success: 0    Faied: <0
*/
Wilddog_Return_T _wilddog_sec_init(int fd, Wilddog_Address_T * addr_in)
{
	int ret;
    const char *pers = "dtls_client";



    ctx.fd = fd;
    ctx.addr_in = addr_in;


    debug_set_threshold( 0 );
    wilddog_debug_level(WD_DEBUG_LOG, "debug_threshold: %d\n", \
										debug_get_threshold());
    /*
     * 0. Initialize the RNG and the session data
     */
    memset( &ssl, 0, sizeof( ssl_context ) );
    x509_crt_init( &cacert );

    wilddog_debug_level(WD_DEBUG_LOG, \
							"\n  . Seeding the random number generator..." );
    fflush( stdout );

    entropy_init( &entropy );
    if( ( ret = ctr_drbg_init( &ctr_drbg, entropy_func, &entropy,
                               (const unsigned char *) pers,
                               strlen( pers ) ) ) != 0 )
    {
        wilddog_debug_level(WD_DEBUG_ERROR, \
							" failed\n  ! ctr_drbg_init returned %d\n", ret );
        //goto exit;
        return WILDDOG_ERR_INVALID;
    }

    wilddog_debug_level(WD_DEBUG_LOG, " ok\n" );

    /*
     * 0. Initialize certificates
     */
    wilddog_debug_level(WD_DEBUG_LOG, \
    							"  . Loading the CA root certificate ..." );
    fflush( stdout );

#if defined(POLARSSL_CERTS_C)
 #ifdef WILDDOG_SELFTEST                            
		ramtest_skipLastmalloc();
#endif 

    ret = x509_crt_parse( &cacert, (const unsigned char *) test_ca_list,
                          strlen( test_ca_list ) );
 #ifdef WILDDOG_SELFTEST                            
	ramtest_caculate_x509Ram();
#endif 

#else
    ret = 1;
    wilddog_debug_level(WD_DEBUG_LOG, "POLARSSL_CERTS_C not defined.");
#endif
    wilddog_debug_level(WD_DEBUG_LOG, "cacert version:%d\n",cacert.version);
    wilddog_debug_level(WD_DEBUG_LOG, "cacert next version:%d\n", \
														cacert.next->version);


    if( ret < 0 )
    {
        wilddog_debug_level(WD_DEBUG_ERROR,  \
					" failed\n  !  x509_crt_parse returned -0x%x\n\n", -ret );
        //goto exit;
        return WILDDOG_ERR_INVALID;
    }

    wilddog_debug_level(WD_DEBUG_LOG, " ok (%d skipped)\n", ret );

    /*
     * 2. Setup stuff
     */
    wilddog_debug_level(WD_DEBUG_LOG, "  . Setting up the DTLS structure..." );
    fflush( stdout );

    if( ( ret = ssl_init( &ssl ) ) != 0 )
    {
        wilddog_debug_level(WD_DEBUG_ERROR, \
								" failed\n  ! ssl_init returned %x\n\n", ret );
        //goto exit;
        return WILDDOG_ERR_INVALID;
    }

    wilddog_debug_level(WD_DEBUG_LOG, " ok\n" );

    ssl_set_endpoint( &ssl, SSL_IS_CLIENT );
    ssl_set_transport( &ssl, SSL_TRANSPORT_DATAGRAM );

    /* OPTIONAL is usually a bad choice for security, but makes interop easier
     * in this simplified example, in which the ca chain is hardcoded.
     * Production code should set a proper ca chain and use REQUIRED. */
    ssl_set_authmode( &ssl, SSL_VERIFY_OPTIONAL );
    ssl_set_ca_chain( &ssl, &cacert, NULL, SERVER_NAME );

    ssl_set_rng( &ssl, ctr_drbg_random, &ctr_drbg );
    ssl_set_dbg( &ssl, _wilddog_dtls_debug, stdout );

    ssl_set_bio_timeout( &ssl, &ctx,
                         _net_send, _net_recv, _net_recv_timeout,
                         WILDDOG_RECEIVE_TIMEOUT );
                         
    /*
     * 4. Handshake
     */
    wilddog_debug_level(WD_DEBUG_LOG,"  . Performing the SSL/TLS handshake...");
    fflush( stdout );

    do ret = ssl_handshake( &ssl );
    while( ret == POLARSSL_ERR_NET_WANT_READ ||
           ret == POLARSSL_ERR_NET_WANT_WRITE );

    if( ret != 0 )
    {
        wilddog_debug_level(WD_DEBUG_WARN, \
						" failed\n  ! ssl_handshake returned -0x%x\n\n", -ret );
        //goto exit;
        return WILDDOG_ERR_INVALID;
    }

    wilddog_debug_level(WD_DEBUG_LOG, " ok\n" );

    /*
     * 5. Verify the server certificate
     */
    wilddog_debug_level(WD_DEBUG_LOG, " . Verifying peer X.509 certificate...");

    /* In real life, we would have used SSL_VERIFY_REQUIRED so that the
     * handshake would not succeed if the peer's cert is bad.  Even if we used
     * SSL_VERIFY_OPTIONAL, we would bail out here if ret != 0 */
    if( ( ret = ssl_get_verify_result( &ssl ) ) != 0 )
    {
        wilddog_debug_level(WD_DEBUG_LOG, " failed\n" );

        if( ( ret & BADCERT_EXPIRED ) != 0 )
            wilddog_debug_level(WD_DEBUG_LOG, \
            							" ! server certificate has expired\n" );

        if( ( ret & BADCERT_REVOKED ) != 0 )
            wilddog_debug_level(WD_DEBUG_LOG, \
            					"  ! server certificate has been revoked\n" );

        if( ( ret & BADCERT_CN_MISMATCH ) != 0 )
            wilddog_debug_level(WD_DEBUG_LOG, \
            				"  ! CN mismatch (expected CN=%s)\n", SERVER_NAME );

        if( ( ret & BADCERT_NOT_TRUSTED ) != 0 )
            wilddog_debug_level(WD_DEBUG_LOG, \
            				"  ! self-signed or not signed by a trusted CA\n" );

        wilddog_debug_level(WD_DEBUG_LOG, "\n" );
    }
    else
        wilddog_debug_level(WD_DEBUG_LOG, " ok\n" );

	return WILDDOG_ERR_NOERR;
}

/*
 * Function:    _wilddog_sec_deinit
 * Description: Destroy dtls security session
 * Input:       fd: socket id    
 *				addr_in: The address which contains ip and port
 * Output:      N/A
 * Return:      Success: 0
*/
Wilddog_Return_T _wilddog_sec_deinit(int fd, Wilddog_Address_T *addr_in)
{
	int ret;
    /*
     * 8. Done, cleanly close the connection
     */

    wilddog_debug_level(WD_DEBUG_LOG, "  . Closing the connection..." );

    /* No error checking, the connection might be closed already */
    do ret = ssl_close_notify( wilddog_getssl() );
    while( ret == POLARSSL_ERR_NET_WANT_WRITE );
    ret = 0;

    wilddog_debug_level(WD_DEBUG_LOG, " done\n" );

    if( fd != -1 )
        wilddog_closeSocket( fd );

    x509_crt_free( &cacert );
    ssl_free( wilddog_getssl());
    ctr_drbg_free( &ctr_drbg );
    entropy_free( &entropy );

	return WILDDOG_ERR_NOERR;
}

