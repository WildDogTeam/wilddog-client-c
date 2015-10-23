#include <string.h>
#include <stdio.h>

#include "wilddog.h"
#include "wilddog_debug.h"
#include "wilddog_config.h"
#include "wilddog_sec_host.h"

#include "wilddog_port.h"
#include "mbedtls/net.h"
#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"
#include "mbedtls/timing.h"
#include "mbedtls/x509_crt.h"

#include "test_lib.h"
#define SERVER_NAME "Li"

STATIC Wilddog_Address_T l_addr_in;
STATIC int l_fd;

extern int mbedtls_debug_get_threshold(void);
typedef struct _ctx
{
	int fd;
	Wilddog_Address_T * addr_in;
}CTX;

CTX ctx;

//ssl_context ssl;
//entropy_context entropy;
//ctr_drbg_context ctr_drbg;    
//x509_crt cacert;

//mbedtls_net_context server_fd;
mbedtls_entropy_context entropy;
mbedtls_ctr_drbg_context ctr_drbg;
mbedtls_ssl_context ssl;
mbedtls_ssl_config conf;
mbedtls_x509_crt cacert;
mbedtls_timing_delay_context timer;

/*
 * Function:    _wilddog_dtls_debug
 * Description: DTLS debug printf function
 * Input:       ctx: The context
 *              level: The printing level
 *              str: The printing string
 * Output:      N/A
 * Return:      N/A
*/
STATIC void _wilddog_dtls_debug( void *ctx, int level,
                                            const char *file, int line,                  
                                            const char *str )
{
    ((void) level);


        printf( "%s:%04d: %s", file, line, str );    
        fflush(  stdout  );
}

/*
 * Function:    wilddog_getssl
 * Description: get ssl context struct
 * Input:       N/A
 * Output:      N/A
 * Return:      ssl context struct
*/
STATIC mbedtls_ssl_context *wilddog_getssl(void)
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
	void* p_data, 
	s32 len
	)
{
	int ret;

    do ret = mbedtls_ssl_write( wilddog_getssl(), (unsigned char *) p_data, len );
    while( ret == MBEDTLS_ERR_SSL_WANT_READ ||
           ret == MBEDTLS_ERR_SSL_WANT_WRITE );

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
	void* p_data, 
	s32 len
	)
{
	int ret;


    do ret = mbedtls_ssl_read( wilddog_getssl(), (unsigned char *)p_data, len );
    while( ret == MBEDTLS_ERR_SSL_WANT_READ ||
           ret == MBEDTLS_ERR_SSL_WANT_WRITE );

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
Wilddog_Return_T _wilddog_sec_init( Wilddog_Str_T *p_host,u16 d_port)
{
    int ret;
    uint32_t flags;
    const char *pers = "dtls_client";


	/* open socket
	** get host by name
	*/
	wilddog_openSocket(&l_fd);
	if( (ret = _wilddog_sec_getHost(&l_addr_in,p_host,d_port)) <0 )
		return ret;
		
    ctx.fd = l_fd;
    ctx.addr_in = &l_addr_in;

    mbedtls_debug_set_threshold( 0 );
    wilddog_debug_level(WD_DEBUG_LOG, "debug_threshold: %d\n", \
										mbedtls_debug_get_threshold());
    /*
     * 0. Initialize the RNG and the session data
     */
//    mbedtls_net_init( &server_fd );
    mbedtls_ssl_init( &ssl );
    mbedtls_ssl_config_init( &conf );
    mbedtls_x509_crt_init( &cacert );
    mbedtls_ctr_drbg_init( &ctr_drbg );

    wilddog_debug_level(WD_DEBUG_LOG, \
							"\n  . Seeding the random number generator..." );
    fflush( stdout );

    mbedtls_entropy_init( &entropy );
    if( ( ret = mbedtls_ctr_drbg_seed( &ctr_drbg, mbedtls_entropy_func, &entropy,
                               (const unsigned char *) pers,
                               strlen( pers ) ) ) != 0 )
    {
        wilddog_debug_level(WD_DEBUG_ERROR, \
							" failed\n  ! mbedtls_ctr_drbg_init returned %d\n", ret );
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

#ifdef WILDDOG_SELFTEST                            
    ramtest_skipLastmalloc();
#endif 

    ret = mbedtls_x509_crt_parse( &cacert, (const unsigned char *) mbedtls_test_cas_pem,
                          mbedtls_test_cas_pem_len );

#ifdef WILDDOG_SELFTEST                            
    ramtest_caculate_x509Ram();
#endif 

    wilddog_debug_level(WD_DEBUG_LOG, "cacert version:%d\n",cacert.version);
    wilddog_debug_level(WD_DEBUG_LOG, "cacert next version:%d\n", \
														cacert.next->version);


    if( ret < 0 )
    {
        wilddog_debug_level(WD_DEBUG_ERROR,  \
					" failed\n  !  mbedtls_x509_crt_parse returned -0x%x\n\n", -ret );
        //goto exit;
        return WILDDOG_ERR_INVALID;
    }

    wilddog_debug_level(WD_DEBUG_LOG, " ok (%d skipped)\n", ret );

//    server_fd.fd = l_fd;

    /*
     * 2. Setup stuff
     */
    wilddog_debug_level(WD_DEBUG_LOG, "  . Setting up the DTLS structure..." );
    fflush( stdout );


    if( ( ret = mbedtls_ssl_config_defaults( &conf,
                   MBEDTLS_SSL_IS_CLIENT,
                   MBEDTLS_SSL_TRANSPORT_DATAGRAM,
                   MBEDTLS_SSL_PRESET_DEFAULT ) ) != 0 )
    {
        wilddog_debug_level(WD_DEBUG_ERROR, \
								" failed\n  ! mbedtls_ssl_config_defaults returned %x\n\n", ret );
        //goto exit;
        return WILDDOG_ERR_INVALID;
    }

    wilddog_debug_level(WD_DEBUG_LOG, " ok\n" );

    /* OPTIONAL is usually a bad choice for security, but makes interop easier
     * in this simplified example, in which the ca chain is hardcoded.
     * Production code should set a proper ca chain and use REQUIRED. */
    mbedtls_ssl_conf_authmode( &conf, MBEDTLS_SSL_VERIFY_REQUIRED );
    mbedtls_ssl_conf_ca_chain( &conf, &cacert, NULL );
    mbedtls_ssl_conf_rng( &conf, mbedtls_ctr_drbg_random, &ctr_drbg );
    mbedtls_ssl_conf_dbg( &conf, _wilddog_dtls_debug, stdout );

    if( ( ret = mbedtls_ssl_setup( &ssl, &conf ) ) != 0 )
    {
        wilddog_debug_level(WD_DEBUG_ERROR, " failed\n  ! mbedtls_ssl_setup returned %d\n\n", ret );
        //goto exit;
        return WILDDOG_ERR_INVALID;
    }

    if( ( ret = mbedtls_ssl_set_hostname( &ssl, SERVER_NAME ) ) != 0 )
    {
        wilddog_debug_level(WD_DEBUG_ERROR, " failed\n  ! mbedtls_ssl_set_hostname returned %d\n\n", ret );
        //goto exit;
        return WILDDOG_ERR_INVALID;
    }
    
    mbedtls_ssl_set_bio( &ssl, &ctx,
                         _net_send, _net_recv, _net_recv_timeout );
    mbedtls_ssl_conf_read_timeout(&conf, WILDDOG_RECEIVE_TIMEOUT);

    mbedtls_ssl_set_timer_cb( &ssl, &timer, mbedtls_timing_set_delay,
                                            mbedtls_timing_get_delay );
    
    wilddog_debug_level(WD_DEBUG_LOG, " ok\n" );
                         
    /*
     * 4. Handshake
     */
    wilddog_debug_level(WD_DEBUG_LOG,"  . Performing the SSL/TLS handshake...");
    fflush( stdout );
    
    do ret = mbedtls_ssl_handshake( &ssl );
    while( ret == MBEDTLS_ERR_SSL_WANT_READ ||
           ret == MBEDTLS_ERR_SSL_WANT_WRITE );

    if( ret != 0 )
    {
        wilddog_debug_level(WD_DEBUG_WARN, \
						" failed\n  ! mbedtls_ssl_handshake returned -0x%x\n\n", -ret );
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

    if( ( flags = mbedtls_ssl_get_verify_result( &ssl ) ) != 0 )
    {
        char vrfy_buf[512];

        wilddog_debug_level(WD_DEBUG_ERROR, " failed\n" );

        mbedtls_x509_crt_verify_info( vrfy_buf, sizeof( vrfy_buf ), "  ! ", flags );

        wilddog_debug_level(WD_DEBUG_ERROR, "%s\n", vrfy_buf );
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
Wilddog_Return_T _wilddog_sec_deinit(void)
{
	int ret;
    /*
     * 8. Done, cleanly close the connection
     */

    wilddog_debug_level(WD_DEBUG_LOG, "  . Closing the connection..." );

    /* No error checking, the connection might be closed already */
    do ret = mbedtls_ssl_close_notify( wilddog_getssl() );
    while( ret == MBEDTLS_ERR_SSL_WANT_WRITE );
    ret = 0;

    wilddog_debug_level(WD_DEBUG_LOG, " done\n" );

    if( l_fd != -1 )
        wilddog_closeSocket( l_fd );
//    mbedtls_net_free( &server_fd );

    mbedtls_x509_crt_free( &cacert );
    mbedtls_ssl_free( &ssl );
    mbedtls_ssl_config_free( &conf );
    mbedtls_ctr_drbg_free( &ctr_drbg );
    mbedtls_entropy_free( &entropy );


    return WILDDOG_ERR_NOERR;
}

