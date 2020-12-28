/*
 * $Id: sal.h,v 1.32 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef _SAL_H_
#define _SAL_H_


/* Debugging */
#if CFG_DEBUGGING_ENABLED
extern void sal_debugf(const char *fmt, ...);
#define SAL_DEBUGF(x) do { sal_debugf x; } while(0)
#else
#define SAL_DEBUGF(x) do { } while(0)
#endif

/* Assertion */
extern void sal_assert(const char *, const char *, uint16) REENTRANT;
#if CFG_ASSERTION_ENABLED
#define SAL_ASSERT(x) (void)((x)? 0 : (sal_assert(#x, __FILE__, __LINE__), 0))
#else
#define SAL_ASSERT(x) do { } while(0)
#endif

/* Memory allocation */
extern void *sal_malloc(uint32 size) REENTRANT;
extern void sal_free(void *p) REENTRANT;

#ifdef __ARM__
extern void *sal_dma_malloc(uint32 size) REENTRANT;
extern void sal_dma_free(void *p) REENTRANT;
#else
#define sal_dma_malloc sal_malloc
#define sal_dma_free sal_free
#endif /* __ARM__ */

/* Timer - Get current ticks */
extern tick_t sal_get_ticks(void) REENTRANT;

/* Timer - Get current seconds */
extern uint32 sal_get_seconds(void) REENTRANT;

/* Timer - Microseconds per tick */
extern uint32 sal_get_us_per_tick(void) REENTRANT;

/* Timer - macro to convert micro seconds to system ticks */
#define SAL_USEC_TO_TICKS(usec) \
    ((tick_t)(((usec) + sal_get_us_per_tick() - 1) / sal_get_us_per_tick()))

/* Timer - macro to check time expiration (in ticks) */
#define SAL_TIME_EXPIRED(start, interval) \
    (sal_get_ticks() - (start) >= (interval))

/* Timer - macro to check time expiration (in seconds) */
#define SAL_TIME_EXPIRED_IN_SECOND(start, interval) \
    (sal_get_seconds() - (start) >= (interval))

/* Timer - sleep for micro seconds (but resolution is still in ticks) */
extern void sal_usleep(uint32 usec) REENTRANT;

/* Timer - sleep for number of ticks (background tasks would be executed) */
extern void sal_sleep(tick_t ticks) REENTRANT;

/* Timer - Adjust timer with system clock change (init = FALSE) */
extern void sal_timer_init(uint32 clk_hz, BOOL init) REENTRANT;

/* Timer - Get current microseconds */
extern uint32 sal_time_usecs(void);

/* Console - printf */
extern void sal_printf(const char *fmt, ...);

/* Console - whether if char is available for input */
extern BOOL sal_char_avail(void) REENTRANT;

/* Console - get input character (may block if no char available) */
extern char sal_getchar(void) REENTRANT;

/* Console - return the last inputed character */
extern char sal_get_last_char(void) REENTRANT;

/* Console - output one character */
extern char sal_putchar(char c) REENTRANT;

/* Checksum calculation */
extern uint16 sal_checksum(uint16 sum, const void *start, uint16 len);

/* string lib related */
#include <string.h>
extern char sal_toupper(char c);
extern char sal_tolower(char c);
extern int sal_stricmp(const char *s1, const char *s2);
extern int sal_strncmp(const char *dest, const char *src, size_t cnt);
extern int sal_strcasecmp(const char *s1, const char *s2);
extern int sal_strncasecmp(const char *dest, const char *src, size_t cnt);
extern size_t sal_strcspn(const char *s1, const char *s2);
extern char *sal_strstr(const char *s1, const char *s2);

#define isdigit(d)      (((d) >= '0') && ((d) <= '9'))
#define isupper(c)      (((c) >= 'A') && ((c) <= 'Z'))
#define islower(c)      (((c) >= 'a') && ((c) <= 'z'))
#define isalpha(c)      ( (isupper(c)) || (islower(c)) )
#define isxdigit(c)      ((((c) >= '0') && ((c) <= '9')) || \
                            (((c) >= 'a') && ((c) <= 'f')) || \
                            (((c) >= 'A') && ((c) <= 'F')))
#define isspace(x)      (((x) == ' ') || ((x) == '\t'))
#define isalnum(c)      ( (isdigit(c)) || (isalpha(c)) )

#define sal_isspace(_c_) isspace(_c_)
#define sal_isalnum(_c_) isalnum(_c_)
#define sal_isupper(_c_) isupper(_c_)
#define sal_islower(_c_) islower(_c_)
#define sal_isalpha(_c_) isalpha(_c_)
#define sal_isdigit(_c_) isdigit(_c_)
#define sal_isxdigit(_c_) isxdigit(_c_)

#define sal_strcasecmp strcasecmp
#define sal_strncasecmp strncasecmp

extern void
sal_qsort(void *base, int count, int size, int (*compar)(const void *, const void *));

/* C library */
#ifdef __C51__

extern int sal_memcmp(const void *s1, const void *s2, size_t n) REENTRANT;
extern void *sal_memcpy(void *s1, const void *s2, size_t n) USE_INTERNAL_RAM;
extern void *memmove(void *dest, const void *src, int len);
#define sal_memmove memmove
extern void *sal_memset(void *s, int val, size_t n) USE_INTERNAL_RAM;
extern int sal_strlen(const char *s) REENTRANT;
extern char *sal_strcpy(char *, const char *) REENTRANT;
extern char *sal_strncpy(char *, const char *, size_t) REENTRANT;
extern int sal_strcmp(const char *, const char *) REENTRANT;
extern char *sal_strchr(const char *dest,int c) REENTRANT;
extern char *sal_strcat(char *dest,const char *src) REENTRANT;
extern int sal_sprintf(char *buf, const char *fmt, ...);
extern uint32 sal_xtoi(const char *dest) REENTRANT;
extern int32 sal_atoi(const char *dest) REENTRANT;
#ifdef CFG_BANKED_MEMCPY_SUPPORT
extern void *sal_memcpy_bank4(void *s1, const void *s2, size_t n);
extern void *sal_memcpy_bank5(void *s1, const void *s2, size_t n);
#endif /* CFG_BANKED_MEMCPY_SUPPORT */

/* Random number generator (0 ~ 32767 to be uniform for all platforms) */
extern void srand(int seed);    /* Don't use this directly */
extern int rand(void);          /* Don't use this directly */
#define sal_srand(seed)         do { srand((int)(seed)); } while(0)
#define sal_rand()              ((uint16)rand())
#endif /* __C51__ */

#ifdef __MIPS__

#include <stdarg.h>

void APIFUNC(sal_init)(void) REENTRANT;
int xvsprintf(char *outbuf,const char *templat,va_list marker);
int xsprintf(char *buf,const char *templat,...);
int xprintf(const char *templat,...);

extern int (*xprinthook)(const char *);
int xvprintf(const char *template,va_list marker);

int lib_atoi(const char *dest);
int lib_xtoi(const char *dest);
char *lib_strcpy(char *dest,const char *src);
char *lib_strncpy(char *dest,const char *src,size_t cnt);
char *lib_strcat(char *dest,const char *src);
size_t lib_xstrncpy(char *dest,const char *src,size_t cnt);
size_t lib_strlen(const char *str);
char *lib_strnchr(const char *dest,int c,size_t cnt);

int lib_strcmp(const char *dest,const char *src);
char *lib_strchr(const char *dest,int c);
char *lib_strrchr(const char *dest,int c);
int32 lib_strtol(const char *nptr, char **endptr, int base);
uint32 lib_strtoul(const char *nptr, const char **endptr, int base);
int lib_memcmp(const void *dest,const void *src,size_t cnt);
void *lib_memcpy(void *dest,const void *src,size_t cnt);
void *lib_memmove(void *dst, const void *src, size_t n);
void *lib_memset(void *dest,int c,size_t cnt);


/*
 * compatibility macros
 */
#define printf xprintf
#define sprintf xsprintf
#define vsprintf xvsprintf
#define vprintf xvprintf

#define sal_atoi    lib_atoi
#define sal_xtoi    lib_xtoi
#define sal_memcmp  lib_memcmp
#define sal_memcpy  lib_memcpy
#define sal_memmove lib_memmove
#define sal_memset  lib_memset
#define sal_strlen  lib_strlen
#define sal_strcpy  lib_strcpy
#define sal_strncpy lib_strncpy
#define sal_strcmp  lib_strcmp
#define sal_strchr  lib_strchr
#define sal_strcat  lib_strcat
#define sal_sprintf xsprintf
#define sal_strtol  lib_strtol
#define sal_strtoul  lib_strtoul

/* Random number generator (0 ~ 32767 to be uniform for all platforms) */
extern void sal_srand(uint32 seed);
extern uint16 sal_rand(void);
#endif /* __MIPS__ */

#ifdef __ARM__

#include <stdarg.h>

extern void sal_init(void);

extern int vsprintf(char *buf, const char *fmt, va_list args);
/* int vsprintf(char *outbuf,const char *templat,va_list marker); */
extern int sprintf(char *buf,const char *templat,...);
extern int printf(const char *templat,...);

extern int sal_atoi(const char *dest);
int sal_ctoi(const char *s, char **end);
extern int sal_xtoi(const char *dest);
extern char *sal_strcpy(char *dest,const char *src);
extern char *sal_strncpy(char *dest,const char *src,size_t cnt);
extern size_t sal_strlcpy(char *dst, const char *src, size_t size);
extern char *sal_strcat(char *dest,const char *src);
extern size_t sal_strlen(const char *str);
extern char * sal_strnchr(const char *dest,int c,size_t cnt);
extern int sal_snprintf(char *buf, size_t bufsize, const char *fmt, ...);
extern int sal_sprintf(char *buf,const char *templat,...);
extern int sal_vsnprintf(char *buf, size_t bufsize, const char *fmt, va_list ap);
extern char *sal_strdup(const char *s);
extern int sal_strcmp(const char *dest,const char *src);
extern char *sal_strchr(const char *dest,int c);
extern char *sal_strrchr(const char *dest,int c);
extern int32 sal_strtol(const char *nptr, char **endptr, int base);
extern uint32 sal_strtoul(const char *nptr, const char **endptr, int base);
extern uint64 sal_strtoull(const char *nptr, const char **endptr, int base);
extern int sal_memcmp(const void *dest,const void *src,size_t cnt);
extern void *sal_memcpy(void *dest,const void *src,size_t cnt);
extern void *sal_memset(void *dest,int c,size_t cnt);
extern void *sal_memmove(void *dst, const void *src, size_t n);
extern char *sal_strchr(const char *dest,int c);
extern size_t sal_strlen(const char *str);
extern char *sal_strtok_r(char *s1, const char *delim, char **s2);

#define SAL_PRIx32 "x"
#define SAL_PRIu32 "u"
#define SAL_PRIx64 "llx"
#define SAL_PRIu64 "llu"


/* Random number generator (0 ~ 32767 to be uniform for all platforms) */
extern void sal_srand(uint32 seed);
extern uint16 sal_rand(void);

#endif /* __ARM__ */

#define SAL_UPORT_BASE 1
#define SAL_ZUPORT_TO_UPORT(zuport) ((zuport) + SAL_UPORT_BASE)
#define SAL_UPORT_TO_ZUPORT(zuport) ((zuport) - SAL_UPORT_BASE)
#define SAL_UPORT_TO_NZUPORT(uport) ((uport) + 1 - SAL_UPORT_BASE)
#define SAL_NZUPORT_TO_UPORT(nzuport) ((nzuport) + SAL_UPORT_BASE - 1)


/* User port iteration */
#define SAL_UPORT_ITER(_p)        \
        for ((_p) = SAL_UPORT_BASE; (_p) < (board_uport_count() + SAL_UPORT_BASE); (_p)++)

/* User port sanity check */
#define SAL_UPORT_IS_NOT_VALID(_p)  \
        (((_p) < SAL_UPORT_BASE) || ((_p) >= (board_uport_count() + SAL_UPORT_BASE)))


#define SAL_IS_UMDUMB()   \
        (sal_strcmp(target, "umdumb")==0)

#define SAL_IS_UMPLUS()   \
        (sal_strcmp(target, "umplus")==0)

#define SAL_IS_UMWEB()   \
        (sal_strcmp(target, "umweb")==0)

#define SAL_IS_LOADER()   \
        (sal_strcmp(target, "loader")==0)

#define SAL_IS_ROMCODE()   \
        (sal_strcmp(target, "rom")==0)

#define SAL_IS_RAMAPP()   \
        (sal_strcmp(target, "ramapp")==0)

#define SAL_IS_QT()   \
        (sal_strcmp(target, "qt")==0)

#endif /* _SAL_H_ */
