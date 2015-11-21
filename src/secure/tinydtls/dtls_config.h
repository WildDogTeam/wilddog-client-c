/* dtls_config.h.  Generated from dtls_config.h.in by configure.  */
/* dtls_config.h.in.  Generated from configure.in by autoheader.  */
#include "wilddog.h"
/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* Define to 1 if building with X509 support. */
#undef DTLS_X509
#define DTLS_X509 1 


/* Define to 1 if building with ECC support. */
#undef DTLS_ECC
#define DTLS_ECC 1 

/* Define to 1 if building with PSK support */
#undef DTLS_PSK
#define DTLS_PSK 1

/* Define to 1 if you have the <arpa/inet.h> header file. */
#if defined(WILDDOG_PORT_TYPE_WICED) || defined(WILDDOG_PORT_TYPE_QUCETEL) || defined(WILDDOG_PORT_TYPE_MXCHIP)
#define HAVE_ARPA_INET_H 0
#else
#define HAVE_ARPA_INET_H 1
#endif
/* Define to 1 if you have the <assert.h> header file. */
#if !defined(WILDDOG_PORT_TYPE_QUCETEL) && !defined(WILDDOG_PORT_TYPE_MXCHIP)
#define HAVE_ASSERT_H 1
#endif
/* Define to 1 if you have the <fcntl.h> header file. */
#undef HAVE_FCNTL_H
#define HAVE_FCNTL_H 1

/* Define to 1 if you have the `fls' function. */
/* #undef HAVE_FLS */

/* Define to 1 if you have the <inttypes.h> header file. */
#undef HAVE_INTTYPES_H
#define HAVE_INTTYPES_H 1

/* Define to 1 if your system has a GNU libc compatible `malloc' function, and
   to 0 otherwise. */
#undef HAVE_MALLOC
#define HAVE_MALLOC 1

/* Define to 1 if you have the <memory.h> header file. */
#undef HAVE_MEMORY_H
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `memset' function. */
#undef HAVE_MEMSET
#define HAVE_MEMSET 1

/* Define to 1 if you have the <netdb.h> header file. */
#undef HAVE_NETDB_H
#define HAVE_NETDB_H 1

/* Define to 1 if you have the <netinet/in.h> header file. */
#undef HAVE_NETINET_IN_H
#define HAVE_NETINET_IN_H 1

/* Define to 1 if you have the `select' function. */
#undef HAVE_SELECT
#define HAVE_SELECT 1

/* Define to 1 if struct sockaddr_in6 has a member sin6_len. */
/* #undef HAVE_SOCKADDR_IN6_SIN6_LEN */

/* Define to 1 if you have the `socket' function. */
#undef HAVE_SOCKET
#define HAVE_SOCKET 1

/* Define to 1 if you have the <stddef.h> header file. */
#undef HAVE_STDDEF_H
#define HAVE_STDDEF_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#undef HAVE_STDINT_H
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#undef HAVE_STDLIB_H
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strdup' function. */
#undef HAVE_STRDUP
#define HAVE_STRDUP 1

/* Define to 1 if you have the `strerror' function. */
#undef HAVE_STRERROR
#define HAVE_STRERROR 1

/* Define to 1 if you have the <strings.h> header file. */
#undef HAVE_STRINGS_H
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#undef HAVE_STRING_H
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strnlen' function. */
#undef HAVE_STRNLEN
#define HAVE_STRNLEN 1

/* Define to 1 if you have the <sys/param.h> header file. */
#undef HAVE_SYS_PARAM_H
#define HAVE_SYS_PARAM_H 1

/* Define to 1 if you have the <sys/socket.h> header file. */
#undef HAVE_SYS_SOCKET_H
#define HAVE_SYS_SOCKET_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#undef HAVE_SYS_STAT_H
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#undef HAVE_SYS_TIME_H
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#undef HAVE_SYS_TYPES_H
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <time.h> header file. */
#if defined(WILDDOG_PORT_TYPE_WICED) || defined(WILDDOG_PORT_TYPE_QUCETEL) || defined(WILDDOG_PORT_TYPE_MXCHIP)
#undef HAVE_TIME_H
#else
#define HAVE_TIME_H 1
#endif
/* Define to 1 if you have the <unistd.h> header file. */
#undef HAVE_UNISTD_H
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the `vprintf' function. */
#if defined(WILDDOG_PORT_TYPE_WICED) || defined(WILDDOG_PORT_TYPE_QUCETEL) || defined(WILDDOG_PORT_TYPE_MXCHIP)
#define HAVE_VPRINTF 0
#else
#define HAVE_VPRINTF 1
#endif
/* Define to the address where bug reports for this package should be sent. */
#undef PACKAGE_BUGREPORT
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#undef PACKAGE_NAME
#define PACKAGE_NAME "tinydtls"

/* Define to the full name and version of this package. */
#undef PACKAGE_STRING
#define PACKAGE_STRING "tinydtls 0.8.2"

/* Define to the one symbol short name of this package. */
#undef PACKAGE_TARNAME
#define PACKAGE_TARNAME "tinydtls"

/* Define to the home page for this package. */
#undef PACKAGE_URL
#define PACKAGE_URL ""

/* Define to the version of this package. */
#undef PACKAGE_VERSION
#define PACKAGE_VERSION "0.8.2"

/* Define to 1 if you have the ANSI C header files. */
#undef STDC_HEADERS
#define STDC_HEADERS 1

/* Define to 1 if building for Contiki. */
/* #undef WITH_CONTIKI */

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif


#undef WORDS_BIGENDIAN

#if WILDDOG_LITTLE_ENDIAN == 1
//#undef WORDS_BIGENDIAN
#else
#define WORDS_BIGENDIAN  (1)
#endif

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
/* #undef inline */
#endif

/* Define to rpl_malloc if the replacement function should be used. */
/* #undef malloc */

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */



#ifdef WITH_CONTIKI
#include "platform-specific/platform.h"
#endif
