#include <string.h>
#include <stdio.h>

#include "wilddog.h"
#include "wilddog_debug.h"
#include "wilddog_config.h"
#include "wilddog_sec.h"

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

typedef struct WILDDOG_SEC_MBEDTLS_T{
    Wilddog_Address_T addr;
    int fd;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
    mbedtls_x509_crt cacert;
    mbedtls_timing_delay_context timer;
}Wilddog_Sec_Mbedtls_T;

extern int mbedtls_debug_get_threshold(void);

/*
 * Function:    _wilddog_dtls_debug
 * Description: DTLS debug printf function
 * Input:       ctx: The context
 *              level: The printing level
 *              str: The printing string
 * Output:      N/A
 * Return:      N/A
*/
STATIC void _wilddog_dtls_debug
    ( 
    void *ctx, 
    int level,
    const char *file, 
    int line,
    const char *str 
    )
{
    ((void) level);

    printf( "%s:%04d: %s", file, line, str );    
    fflush(  stdout  );
}

/*
 * Function:    _net_send
 * Description: The net send function without timeout
 * Input:       ctx: The context   
 *              buf: The buffer which store the sending data
 *              len: The length of the wanting send data             
 * Return:      The length of the actual sending data
*/
int _net_send( void *ctx, const unsigned char *buf, size_t len )
{
    Wilddog_Protocol_T *protocol = (Wilddog_Protocol_T *)ctx;

    wilddog_assert(protocol, WILDDOG_ERR_NULL);
    return wilddog_send(protocol->socketFd, &protocol->addr, (void*)buf, (s32)len);

}

/*
 * Function:    _net_recv
 * Description: The net recv function without timeout
 * Input:       ctx: The context   
 *              len: The length of the wanting receive data
 * Output:      buf: The buffer which store the receiving data
 * Return:      The length of the actual receiving data
*/
int _net_recv( void *ctx, unsigned char *buf, size_t len )
{
    Wilddog_Protocol_T *protocol = (Wilddog_Protocol_T *)ctx;

    wilddog_assert(protocol, WILDDOG_ERR_NULL);

    return wilddog_receive(protocol->socketFd, &protocol->addr, (void*)buf, (s32)len, \
                            WILDDOG_RECEIVE_TIMEOUT);
}

/*
 * Function:    _net_recv_timeout
 * Description: The net recv function with timeout
 * Input:       ctx: The context   
 *              len: The length of the wanting receive data
 *              timeout: Timeout
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
    Wilddog_Protocol_T *protocol = (Wilddog_Protocol_T *)ctx;

    wilddog_assert(protocol, WILDDOG_ERR_NULL);

    return wilddog_receive(protocol->socketFd, &protocol->addr, (void*)buf, (s32)len, timeout);
}

/*
 * Function:    _wilddog_sec_send
 * Description: Dtls security send function
 * Input:       p_data: The buffer which store the sending data
 *              len: The length of the wanting send data
 * Output:      N/A
 * Return:      Success:0
*/
Wilddog_Return_T _wilddog_sec_send
    (
    Wilddog_Protocol_T *protocol,
    void* p_data, 
    s32 len
    )
{
    int ret;
    Wilddog_Sec_Mbedtls_T * mbed = (Wilddog_Sec_Mbedtls_T*)protocol->user_data;
    wilddog_assert(protocol&&mbed, WILDDOG_ERR_NULL);
    do ret = mbedtls_ssl_write(&mbed->ssl,(unsigned char *) p_data, len);
    while( ret == MBEDTLS_ERR_SSL_WANT_READ ||
           ret == MBEDTLS_ERR_SSL_WANT_WRITE );

    return WILDDOG_ERR_NOERR;
}

/*
 * Function:    _wilddog_sec_recv
 * Description: Dtls security recv function
 * Input:       len: The length of the wanting receive data
 * Output:      p_data: The buffer which store the receiving data
 * Return:      The length of the actual receiving data
*/
int _wilddog_sec_recv
    (
    Wilddog_Protocol_T *protocol,
    void* p_data, 
    s32 len
    )
{
    int ret;
    Wilddog_Sec_Mbedtls_T * mbed = (Wilddog_Sec_Mbedtls_T*)protocol->user_data;
    wilddog_assert(protocol&&mbed, WILDDOG_ERR_NULL);

    do
    {
        ret = mbedtls_ssl_read(&mbed->ssl,(unsigned char *)p_data, len );
    }
    while( ret == MBEDTLS_ERR_SSL_WANT_READ ||
           ret == MBEDTLS_ERR_SSL_WANT_WRITE );

    return mbed->ssl.in_left ;
}

/*
 * Function:    _wilddog_sec_init
 * Description: Initialize dtls security session
 * Input:       p_host: url host string  
                d_port: the port want to connect
 * Output:      N/A
 * Return:      Success: 0    Faied: <0
*/
Wilddog_Return_T _wilddog_sec_init(Wilddog_Protocol_T *protocol)
{
    int ret;
    uint32_t flags;
    const char *pers = "dtls_client";
    
    Wilddog_Sec_Mbedtls_T * mbed = NULL;
    
    wilddog_assert(protocol, WILDDOG_ERR_NULL);

    /* open socket
    ** get host by name
    */
    wilddog_openSocket(&protocol->socketFd);
    ret = _wilddog_sec_getHost(&protocol->addr,protocol->host);
    if(ret < 0)
        return ret;

    protocol->user_data = wmalloc(sizeof(Wilddog_Sec_Mbedtls_T));
    if(!protocol->user_data){
        wilddog_debug_level(WD_DEBUG_ERROR,"Malloc failed!");
        return WILDDOG_ERR_NULL;
    }

    mbed = (Wilddog_Sec_Mbedtls_T*)protocol->user_data;

    mbedtls_debug_set_threshold( 0 );
    wilddog_debug_level(WD_DEBUG_LOG, "debug_threshold: %d\n", \
                                        mbedtls_debug_get_threshold());
    /*
     * 0. Initialize the RNG and the session data
     */
//    mbedtls_net_init( &server_fd );
    mbedtls_ssl_init( &mbed->ssl );
    mbedtls_ssl_config_init( &mbed->conf);
    mbedtls_x509_crt_init( &mbed->cacert);
    mbedtls_ctr_drbg_init( &mbed->ctr_drbg );

    wilddog_debug_level(WD_DEBUG_LOG, \
                        "\n  . Seeding the random number generator..." );
    fflush( stdout );

    mbedtls_entropy_init( &mbed->entropy );
    ret = mbedtls_ctr_drbg_seed(&mbed->ctr_drbg, \
                                mbedtls_entropy_func, \
                                &mbed->entropy, \
                                (const unsigned char *) pers, \
                                strlen( pers ));
    if( ret != 0 )
    {
        wilddog_debug_level(WD_DEBUG_ERROR, \
                            " failed\n  ! mbedtls_ctr_drbg_init returned %d\n", 
                            ret );
        
        return WILDDOG_ERR_INVALID;
    }
    
    /*
     * 0. Initialize certificates
     */
    wilddog_debug_level(WD_DEBUG_LOG, \
                        "  . Loading the CA root certificate ..." );
    fflush( stdout );

#ifdef WILDDOG_SELFTEST                            
    ramtest_skipLastmalloc();
#endif 

    ret = mbedtls_x509_crt_parse(&mbed->cacert, \
                                 (const unsigned char *) mbedtls_test_cas_pem, \
                                 mbedtls_test_cas_pem_len );

#ifdef WILDDOG_SELFTEST                            
    ramtest_caculate_x509Ram();
#endif 

    wilddog_debug_level(WD_DEBUG_LOG, \
                        "cacert version:%d\n", \
                        mbed->cacert.version);
    wilddog_debug_level(WD_DEBUG_LOG, \
                        "cacert next version:%d\n", \
                        mbed->cacert.next->version);

    if( ret < 0 )
    {
        wilddog_debug_level(WD_DEBUG_ERROR,  \
                    " failed\n  !  mbedtls_x509_crt_parse returned -0x%x\n\n", 
                    -ret );

        return WILDDOG_ERR_INVALID;
    }

    /*
     * 2. Setup stuff
     */
    wilddog_debug_level(WD_DEBUG_LOG, "  . Setting up the DTLS structure..." );
    fflush( stdout );

    ret = mbedtls_ssl_config_defaults(&mbed->conf,
                                      MBEDTLS_SSL_IS_CLIENT,
                                      MBEDTLS_SSL_TRANSPORT_DATAGRAM,
                                      MBEDTLS_SSL_PRESET_DEFAULT );
    
    if( ret != 0 )
    {
        wilddog_debug_level(WD_DEBUG_ERROR, \
                    " failed\n  ! mbedtls_ssl_config_defaults returned %x\n\n", 
                    ret );

        return WILDDOG_ERR_INVALID;
    }

    /* OPTIONAL is usually a bad choice for security, but makes interop easier
     * in this simplified example, in which the ca chain is hardcoded.
     * Production code should set a proper ca chain and use REQUIRED. */
    mbedtls_ssl_conf_authmode( &mbed->conf, MBEDTLS_SSL_VERIFY_REQUIRED );
    mbedtls_ssl_conf_ca_chain( &mbed->conf, &mbed->cacert, NULL );
    mbedtls_ssl_conf_rng( &mbed->conf, mbedtls_ctr_drbg_random, &mbed->ctr_drbg );
    mbedtls_ssl_conf_dbg( &mbed->conf, _wilddog_dtls_debug, stdout );

    if( ( ret = mbedtls_ssl_setup( &mbed->ssl, &mbed->conf ) ) != 0 )
    {
        wilddog_debug_level(WD_DEBUG_ERROR, 
                            " failed\n  ! mbedtls_ssl_setup returned %d\n\n", 
                            ret );

        return WILDDOG_ERR_INVALID;
    }

    if( ( ret = mbedtls_ssl_set_hostname( &mbed->ssl, SERVER_NAME ) ) != 0 )
    {
        wilddog_debug_level(WD_DEBUG_ERROR, 
                        " failed\n  ! mbedtls_ssl_set_hostname returned %d\n\n",
                        ret );

        return WILDDOG_ERR_INVALID;
    }
    
    mbedtls_ssl_set_bio(&mbed->ssl, 
                        protocol,
                        _net_send, 
                        _net_recv, 
                        _net_recv_timeout );
    
    mbedtls_ssl_conf_read_timeout(&mbed->conf, WILDDOG_RECEIVE_TIMEOUT);

    mbedtls_ssl_set_timer_cb(&mbed->ssl, 
                             &mbed->timer, 
                             mbedtls_timing_set_delay,
                             mbedtls_timing_get_delay );
                             
    /*
     * 4. Handshake
     */
    wilddog_debug_level(WD_DEBUG_LOG,"  . Performing the SSL/TLS handshake...");
    fflush( stdout );
    
    do ret = mbedtls_ssl_handshake( &mbed->ssl );
    while( ret == MBEDTLS_ERR_SSL_WANT_READ ||
           ret == MBEDTLS_ERR_SSL_WANT_WRITE );

    if( ret != 0 )
    {
        wilddog_debug_level(WD_DEBUG_WARN, \
                        " failed\n  ! mbedtls_ssl_handshake returned -0x%x\n\n",
                        -ret );

        return WILDDOG_ERR_INVALID;
    }

    /*
     * 5. Verify the server certificate
     */
    wilddog_debug_level(WD_DEBUG_LOG, " . Verifying peer X.509 certificate...");

    /* In real life, we would have used SSL_VERIFY_REQUIRED so that the
     * handshake would not succeed if the peer's cert is bad.  Even if we used
     * SSL_VERIFY_OPTIONAL, we would bail out here if ret != 0 */

    if( ( flags = mbedtls_ssl_get_verify_result( &mbed->ssl ) ) != 0 )
    {
        char vrfy_buf[512];

        wilddog_debug_level(WD_DEBUG_ERROR, " failed\n" );

        mbedtls_x509_crt_verify_info(vrfy_buf, \
                                     sizeof( vrfy_buf ), \
                                     "  ! ", \
                                     flags );

        wilddog_debug_level(WD_DEBUG_ERROR, "%s\n", vrfy_buf );
    }
    else
        wilddog_debug_level(WD_DEBUG_LOG, " ok\n" );

    return WILDDOG_ERR_NOERR;
}

/*
 * Function:    _wilddog_sec_deinit
 * Description: Destroy dtls security session
 * Input:       N/A
 * Output:      N/A
 * Return:      Success: 0
*/
Wilddog_Return_T _wilddog_sec_deinit(Wilddog_Protocol_T *protocol)
{
    int ret;
    Wilddog_Sec_Mbedtls_T * mbed = (Wilddog_Sec_Mbedtls_T*)protocol->user_data;
    /*
     * 8. Done, cleanly close the connection
     */
    wilddog_assert(protocol&&mbed, WILDDOG_ERR_NULL);
    
    wilddog_debug_level(WD_DEBUG_LOG, "  . Closing the connection..." );

    /* No error checking, the connection might be closed already */
    do ret = mbedtls_ssl_close_notify(&mbed->ssl);
    while( ret == MBEDTLS_ERR_SSL_WANT_WRITE );
    ret = 0;

    wilddog_debug_level(WD_DEBUG_LOG, " done\n" );

    if(protocol->socketFd != -1)
        wilddog_closeSocket(protocol->socketFd);
    protocol->socketFd = -1;
    memset(&protocol->addr,0,sizeof(protocol->addr));

    mbedtls_x509_crt_free( &mbed->cacert );
    mbedtls_ssl_free( &mbed->ssl );
    mbedtls_ssl_config_free( &mbed->conf );
    mbedtls_ctr_drbg_free( &mbed->ctr_drbg );
    mbedtls_entropy_free( &mbed->entropy );

    wfree(mbed);
    protocol->user_data = NULL;
    return WILDDOG_ERR_NOERR;
}
/*
 * Function:    _wilddog_sec_init
 * Description: Initialize dtls security session
 * Input:       p_host: url host string  
 *              d_port: the port want to connect
 *              retryNum: Max retry time
 * Output:      N/A
 * Return:      Success: 0    Faied: < 0
*/
Wilddog_Return_T _wilddog_sec_reconnect(Wilddog_Protocol_T *protocol,int retryNum)
{
    int i;
    Wilddog_Return_T ret = WILDDOG_ERR_INVALID;
    for(i = 0; i < retryNum; i++)
    {
        _wilddog_sec_deinit(protocol);
        ret = _wilddog_sec_init(protocol);
        if(WILDDOG_ERR_NOERR == ret)
            return ret;
    }
    return ret;
}

