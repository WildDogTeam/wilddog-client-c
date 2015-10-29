
#ifndef _WILDDOG_ENDIAN_H_
#define _WILDDOG_ENDIAN_H_

#ifdef __cplusplus
extern "C"
{
#endif

 

#if WILDDOG_LITTLE_ENDIAN == 1
#define __WD_SWAP32__(val) ( (u32) ((((val) & 0xFF000000) >> 24 ) | \
    (((val) & 0x00FF0000) >> 8) \
             | (((val) & 0x0000FF00) << 8) | (((val) & 0x000000FF) << 24)) )

#define __WD_SWAP16__(val) ( (u16) ((((val) & 0xFF00) >> 8) | \
    (((val) & 0x00FF) << 8)))


#ifndef wilddog_htonl
#define wilddog_htonl(val)  __WD_SWAP32__(val)
#endif /*htonl */
#ifndef wilddog_ntohl
#define wilddog_ntohl(val)  __WD_SWAP32__(val)
#endif /* htonl */

#ifndef wilddog_htons
#define wilddog_htons(val)  __WD_SWAP16__(val)
#endif /*htons */

#ifndef wilddog_ntohs
#define wilddog_ntohs(val)  __WD_SWAP16__(val)
#endif /*htons */

#else
    
#ifndef wilddog_htonl
#define wilddog_htonl(val) (val) 
#endif /* htonl */
#ifndef wilddog_ntohl
#define wilddog_ntohl(val)  (val)
#endif /* htonl */

#ifndef wilddog_htons
#define wilddog_htons(val)  (val)
#endif /*htons */

#ifndef wilddog_ntohs
#define wilddog_ntohs(val)  (val)
#endif /*htons */

#endif

#ifdef __cplusplus
}
#endif

#endif /*_WILDDOG_ENDIAN_H_*/

