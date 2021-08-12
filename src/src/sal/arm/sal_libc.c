/*
 * $Id: sal_libc.c,v 1.10 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include "system.h"

int sal_xtoi(const char *dest)
{
    int x = 0;
    int digit;

    if ((*dest == '0') && (*(dest+1) == 'x')) dest += 2;

    while (*dest) {
    if ((*dest >= '0') && (*dest <= '9')) {
        digit = *dest - '0';
        }
    else if ((*dest >= 'A') && (*dest <= 'F')) {
        digit = 10 + *dest - 'A';
        }
    else if ((*dest >= 'a') && (*dest <= 'f')) {
        digit = 10 + *dest - 'a';
        }
    else {
        break;
        }
    x *= 16;
    x += digit;
    dest++;
    }

    return x;
}

int sal_memcmp(const void *dest,const void *src,size_t cnt)
{
    const unsigned char *d;
    const unsigned char *s;

    d = (const unsigned char *) dest;
    s = (const unsigned char *) src;

    while (cnt) {
    if (*d < *s) return -1;
    if (*d > *s) return 1;
    d++; s++; cnt--;
    }

    return 0;
}

void *sal_memcpy(void *dest,const void *src,size_t cnt)
{
    unsigned char *d;
    const unsigned char *s;
    d = (unsigned char *) dest;
    s = (const unsigned char *) src;

    while (cnt) {
    *d++ = *s++;
    cnt--;
    }

    return dest;
}

void *memcpy(void *dest,const void *src,size_t cnt)
{
    return sal_memcpy(dest, src, cnt);
}

void *sal_memmove(void *dst, const void *src, size_t n)
{
    register char *dst1 = (char *)dst;

    if (dst > src && dst < src + n) {
        src += n;
        dst1 += n;
        while (n-- > 0) {
            *--dst1 = *(char *)(--src);
        }
    } else {
        while (n-- > 0) {
            *dst1++ = *(char *)(src++);
        }
    }
    return dst;
}



void *sal_memset(void *dest,int c,size_t cnt)
{
    unsigned char *d;

    d = dest;

    while (cnt) {
    *d++ = (unsigned char) c;
    cnt--;
    }

    return d;
}

void* memset( void* buffer, int ch, size_t count ) {

    return sal_memset(buffer, ch, count);

}

size_t sal_strlen(const char *str) __attribute__((section(".2ram")));
size_t sal_strlen(const char *str)
{
    size_t cnt = 0;

    while (*str) {
    str++;
    cnt++;
    }

    return cnt;
}

int sal_strcmp(const char *dest,const char *src)
{
    while (*src && *dest) {
    if (*dest < *src) return -1;
    if (*dest > *src) return 1;
    dest++;
    src++;
    }

    if (*dest && !*src) return 1;
    if (!*dest && *src) return -1;
    return 0;
}


char
sal_toupper(char c)
{
    if ((c >= 'a') && (c <= 'z'))
        c -= 32;
    return c;
}

char
sal_tolower(char c)
{
    if ((c >= 'A') && (c <= 'Z'))
        c += 32;
    return c;
}

int
sal_stricmp(const char *s1, const char *s2)
{
    char dc, sc;

    while(*s2 && *s1) {
        dc = sal_toupper(*s1);
        sc = sal_toupper(*s2);
        if (dc < sc)
            return -1;
        if (dc > sc)
            return 1;
        s1++;
        s2++;
    }

    if (*s1 && !*s2)
        return 1;
    if (!*s1 && *s2)
        return -1;
    return 0;
}

int
sal_strncmp(const char *dest, const char *src, size_t cnt)
{
    const unsigned char *d;
    const unsigned char *s;

    d = (const unsigned char *) dest;
    s = (const unsigned char *) src;

    while (cnt) {
    if (*d < *s) return -1;
    if (*d > *s) return 1;
    if (!*d && !*s) return 0;
    d++; s++; cnt--;
    }

    return 0;
}

size_t
sal_strcspn(const char *s1, const char *s2)
{
    const char *s = s1;
    const char *c;

    while(*s1) {
        for (c = s2; *c; c++) {
            if (*s1 == *c) {
                break;
            }
        }
        if (*c) {
            break;
        }
        s1++;
    }

    return s1 - s;
}

char *sal_strchr(const char *dest,int c) __attribute__((section(".2ram")));
char *sal_strchr(const char *dest,int c)
{
    while (*dest) {
    if (*dest == c) return (char *) dest;
    dest++;
    }
    return NULL;
}

char *sal_strcpy(char *dest,const char *src) __attribute__((section(".2ram")));
char *sal_strcpy(char *dest,const char *src)
{
    char *ptr = dest;

    while (*src) *ptr++ = *src++;
    *ptr = '\0';

    return dest;
}

size_t sal_strlcpy(char *dest, const char *src, size_t cnt)
{
    char *ptr = dest;
    size_t copied = 0;

    while (*src && (cnt > 1)) {
        *ptr++ = *src++;
        cnt--;
        copied++;
    }
    *ptr = '\0';

    return copied;
}

char *sal_strcat(char *dest,const char *src)
{
    char *ptr = dest;

    while (*ptr) ptr++;
    while (*src) *ptr++ = *src++;
    *ptr = '\0';

    return dest;
}

int sal_atoi(const char *dest)
{
    int x = 0;
    int digit;

    while (*dest) {
        if ((*dest >= '0') && (*dest <= '9')) {
            digit = *dest - '0';
        } else {
            break;
        }
        x *= 10;
        x += digit;
        dest++;
    }
    return x;
}

char *sal_strncpy(char *dest,const char *src,size_t cnt)
{
    char *ptr = dest;

    while (*src && (cnt > 0)) {
    *ptr++ = *src++;
    cnt--;
    }

    while (cnt > 0) {
    *ptr++ = 0;
    cnt--;
    }

    return dest;
}

char *
sal_strncat(char *dst, const char *src, size_t cnt)
{
	if (cnt != 0) {
		char *d = dst;
		const char *s = src;

		while (*d != 0)
			d++;
		do {
			if ((*d = *s++) == 0)
				break;
			d++;
		} while (--cnt != 0);
		*d = 0;
	}
	return (dst);
}

char *sal_strnchr(const char *dest,int c,size_t cnt)
{
    while (*dest && (cnt > 0)) {
    if (*dest == c) return (char *) dest;
    dest++;
    cnt--;
    }
    return NULL;
}

char *sal_strrchr(const char *dest,int c)
{
    char *ret = NULL;

    while (*dest) {
    if (*dest == c) ret = (char *) dest;
    dest++;
    }

    return ret;
}

char *
sal_strstr(const char *s1, const char *s2)
{
    if (*s1 == '\0') {
        if (*s2) {
            return (char *) NULL;
        } else {
            return (char *) s1;
        }
    }

    while(*s1) {
        int i;

        for (i=0; ; i++) {
            if (s2[i] == '\0') {
                return (char *) s1;
            }

            if (s2[i] != s1[i]) {
                break;
            }
        }
        s1++;
    }

    return (char *) NULL;
}

char *
sal_strcasestr(const char *s1, const char *s2)
{
    if (*s1 == '\0') {
        if (*s2) {
            return (char *) NULL;
        } else {
            return (char *) s1;
        }
    }

    while(*s1) {
        int i;

        for (i=0; ; i++) {
            if (s2[i] == '\0') {
                return (char *) s1;
            }

            if (sal_toupper(s2[i]) != sal_toupper(s1[i])) {
                break;
            }
        }
        s1++;
    }

    return (char *) NULL;
}


/*
 * A simple random number generator without floating pointer operations
 */
STATIC int rand_is_seeded = 0;
STATIC uint32 rand_c_value, rand_t_value;
#define RAND_MAGIC_1 0x0000444BUL
#define RAND_MAGIC_2 0x88740000UL
#define RAND_MAGIC_3 69069UL

void
sal_srand(uint32 seed)
{
    uint32 time_seed = (seed << 21) + (seed << 14) + (seed << 7);
    rand_c_value = ((time_seed + RAND_MAGIC_1) << 1) + 1;
    rand_t_value = (time_seed << (time_seed & 0xF)) + RAND_MAGIC_2;
    rand_is_seeded = 1;
}

uint16
sal_rand(void)
{
    if (!rand_is_seeded) {
        sal_srand((uint32)sal_get_ticks());
    }
    rand_c_value = rand_c_value * RAND_MAGIC_3;
    rand_t_value ^= rand_t_value >> 15;
    rand_t_value ^= rand_t_value << 17;
    return (uint16)(((rand_t_value ^ rand_c_value) >> 1) & 0x7FFF);
}

int32
sal_strtol(const char *nptr, char **endptr, int base)
{
    int x = 0;
    int digit;
    BOOL negative = FALSE;

    while (isspace(*nptr)){
        nptr++;
    }

    if (*nptr == '-') {
        negative = TRUE;
        nptr++;
    }

    if (base == 0) {
        if ((*nptr == '0') && ((*(nptr+1) == 'x') || (*(nptr+1) == 'X'))) {
            base = 16;
            nptr += 2;
        } else {
            base = 10;
        }
    }

    while (*nptr) {
        if ((*nptr >= '0') && (*nptr <= '9')) {
            digit = *nptr - '0';
        } else if ((*nptr >= 'A') && (*nptr <= 'F')) {
            digit = 10 + *nptr - 'A';
        } else if ((*nptr >= 'a') && (*nptr <= 'f')) {
            digit = 10 + *nptr - 'a';
        } else {
            break;
        }

        if (digit >= base) {
            break;
        }
        x *= base;
        x += digit;
        nptr++;
    }

    *endptr = (char *)nptr;

    if (negative) {
        x = -x;
    }

    return x;
}

uint32
sal_strtoul(const char *nptr, const char **endptr, int base)
{
    unsigned int x = 0;
    int digit;
    BOOL negative = FALSE;

    while (isspace(*nptr)){
        nptr++;
    }

    if (*nptr == '-') {
        negative = TRUE;
        nptr++;
    }

    if (base == 0) {
        if ((*nptr == '0') && ((*(nptr+1) == 'x') || (*(nptr+1) == 'X'))) {
            base = 16;
            nptr += 2;
        } else {
            base = 10;
        }
    }

    while (*nptr) {
        if ((*nptr >= '0') && (*nptr <= '9')) {
            digit = *nptr - '0';
        } else if ((*nptr >= 'A') && (*nptr <= 'F')) {
            digit = 10 + *nptr - 'A';
        } else if ((*nptr >= 'a') && (*nptr <= 'f')) {
            digit = 10 + *nptr - 'a';
        } else {
            break;
        }

        if (digit >= base) {
            break;
        }
        x *= base;
        x += digit;
        nptr++;
    }

    if (endptr) {
        *endptr = (char *)nptr;
    }

    if (negative) {
        x = -x;
    }

    return x;
}

uint64
sal_strtoull(const char *nptr, const char **endptr, int base)
{
    uint64 x = 0;
    int digit;
    BOOL negative = FALSE;

    while (isspace(*nptr)){
        nptr++;
    }

    if (*nptr == '-') {
        negative = TRUE;
        nptr++;
    }

    if (base == 0) {
        if ((*nptr == '0') && ((*(nptr+1) == 'x') || (*(nptr+1) == 'X'))) {
            base = 16;
            nptr += 2;
        } else {
            base = 10;
        }
    }

    while (*nptr) {
        if ((*nptr >= '0') && (*nptr <= '9')) {
            digit = *nptr - '0';
        } else if ((*nptr >= 'A') && (*nptr <= 'F')) {
            digit = 10 + *nptr - 'A';
        } else if ((*nptr >= 'a') && (*nptr <= 'f')) {
            digit = 10 + *nptr - 'a';
        } else {
            break;
        }

        if (digit >= base) {
            break;
        }
        x *= base;
        x += digit;
        nptr++;
    }

    if (endptr) {
        *endptr = (char *)nptr;
    }

    if (negative) {
        x = -x;
    }

    return x;
}

/*
 * Curt's Printf
 *
 * Reasonably complete subset of ANSI-style printf routines.
 * Needs only sal_strlen and stdarg.
 * Behavior was regressed against Solaris printf(3s) routines (below).
 *
 * Supported format controls:
 *
 *      %%      percent sign
 *      %c      character
 *      %d      integer
 *      %hd     short integer
 *      %ld     long integer
 *      %u      unsigned integer
 *      %o      unsigned octal integer
 *      %x      unsigned hexadecimal integer (lowercase)
 *      %X      unsigned hexadecimal integer (uppercase)
 *      %s      string
 *      %p      pointer
 *      %n      store number of characters output so far
 *      %f      float
 *      %lf     double          (if COMPILER_HAS_DOUBLE is defined)
 *
 * Flag modifiers supported:
 *      Field width, argument field width (*), left justify (-),
 *      zero-fill (0), alternate form (#), always include sign (+),
 *      space before positive numbers (space).
 *
 * Not supported: long long
 *
 * Functions implemented:
 *
 * int sal_vsnprintf(char *buf, size_t bufsize, const char *fmt, va_list ap);
 * int sal_vsprintf(char *buf, const char *fmt, va_list ap);
 * int sal_snprintf(char *buf, size_t bufsize, const char *fmt, ...);
 * int sal_sprintf(char *buf, const char *fmt, ...);
 */

void sal_ltoa(char *buf, unsigned long num, int base, int caps, int prec)
                     __attribute__((section(".2ram")));
 void
 sal_ltoa(char *buf,             /* Large enough result buffer   */
      unsigned long num,         /* Number to convert            */
      int base,                  /* Conversion base (2 to 16)    */
      int caps,                  /* Capitalize letter digits     */
      int prec)                  /* Precision (minimum digits)   */
 {
     char        tmp[68], *s, *digits;

     digits = (caps ? "0123456789ABCDEF" : "0123456789abcdef");

     s = &tmp[sizeof (tmp) - 1];

     for (*s = 0; num || s == &tmp[sizeof (tmp) - 1]; num /= base, prec--)
         *--s = digits[num % base];

     while (prec-- > 0)
         *--s = '0';
     /* coverity[secure_coding] */
     sal_strcpy(buf, s);
 }

 void
 sal_itoa(char *buf,             /* Large enough result buffer   */
      uint32 num,                /* Number to convert            */
      int base,                  /* Conversion base (2 to 16)    */
      int caps,                  /* Capitalize letter digits     */
      int prec)                  /* Precision (minimum digits)   */
 {
     sal_ltoa(buf, num, base, caps, prec);
 }


#define X_STORE(c) { 	\
		 if (PTR_TO_INT(bp) < PTR_TO_INT(be))	 \
			 *bp = (c); 						 \
		 bp++;								 \
 }

#define X_INF		0x7ffffff0

int sal_vsnprintf(char *buf, size_t bufsize, const char *fmt, va_list ap)
                    __attribute__((section(".2ram")));
 int sal_vsnprintf(char *buf, size_t bufsize, const char *fmt, va_list ap)
 {
	 char		 c, *bp, *be;
	 char				 *p_null = NULL;
	 char		 *b_inf = p_null - 1;

	 bp = buf;
	 be = (bufsize == X_INF) ? b_inf : &buf[bufsize - 1];

	 while ((c = *fmt++) != 0) {
	 int		 width = 0, ljust = 0, plus = 0, space = 0;
	 int	 altform = 0, prec = 0, byte = 0, half = 0, base = 0;
	 int	 tlong = 0, fillz = 0, plen, pad;
	 long		 num = 0;
	 char		 tmp[36], *p = tmp;
#ifdef COMPILER_HAS_DOUBLE
	 int prec_given = 0;
#endif

	 if (c != '%') {
		 X_STORE(c);
		 continue;
	 }

	 for (c = *fmt++; ; c = *fmt++)
		 switch (c) {
         case 'b': byte = 1;	 break;
		 case 'h': half = 1;	 break;
		 case 'l': tlong = 1;	 break;
		 case '-': ljust = 1;	 break;
		 case '+': plus = 1;	 break;
		 case ' ': space = 1;	 break;
		 case '0': fillz = 1;	 break;
		 case '#': altform = 1;  break;
		 case '*': width = -1;	 break;  /* Mark as need-to-fetch */
		 case '.':
		 if ((c = *fmt++) == '*')
			 prec = -1; 		 /* Mark as need-to-fetch */
		 else {
			 for (prec = 0; c >= '0' && c <= '9'; c = *fmt++)
			 prec = prec * 10 + (c - '0');
			 fmt--;
		 }
#ifdef COMPILER_HAS_DOUBLE
		 prec_given = 1;
#endif
		 break;
		 default:
		 if (c >= '1' && c <= '9') {
			 for (width = 0; c >= '0' && c <= '9'; c = *fmt++)
			 width = width * 10 + (c - '0');
			 fmt--;
		 } else
			 goto break_for;
		 break;
		 }
	 break_for:

	 if (width == -1)
		 width = va_arg(ap,int);
	 if (prec == -1)
		 prec = va_arg(ap,int);

	 if (c == 0)
		 break;

	 switch (c) {
	 case 'd':
	 case 'i':
		 num = tlong ? va_arg(ap, long) : va_arg(ap, int);
		 if (half)
		 num = (int) (short) num;
			 else if (!tlong)
				 num = (int) num;
		 /* For zero-fill, the sign must be to the left of the zeroes */
		 if (fillz && (num < 0 || plus || space)) {
		 X_STORE(num < 0 ? '-' : space ? ' ' : '+');
		 if (width > 0)
			 width--;
		 if (num < 0)
			 num = -num;
		 }
		 if (! fillz) {
		 if (num < 0) {
			 *p++ = '-';
			 num = -num;
		 } else if (plus)
			 *p++ = '+';
		 else if (space)
			 *p++ = ' ';
		 }
		 base = 10;
		 break;
	 case 'u':
		 num = tlong ? va_arg(ap, long) : va_arg(ap, int);
		 if (half)
		 num = (int) (short) num;
			 else if (!tlong)
				 num = (long) (unsigned int) num;
		 base = 10;
		 break;
	 case 'p':
			 *p++ = '0';
			 *p++ = 'x';
			 tlong = 1;
			 altform = 0;
		 /* Fall through */
	 case 'x':
	 case 'X':
		 num = tlong ? va_arg(ap, long) : va_arg(ap, int);
         if (byte)
            num = (int) (unsigned char) num;
         else if (half)
		 num = (int) (unsigned short) num;
			 else if (!tlong)
				 num = (long) (unsigned int) num;
		 if (altform) {
		 prec += 2;
		 *p++ = '0';
		 *p++ = c;
		 }
		 base = 16;
		 break;
	 case 'o':
	 case 'O':
		 num = tlong ? va_arg(ap, long) : va_arg(ap, int);
		 if (half)
		 num = (int) (unsigned short) num;
			 else if (!tlong)
				 num = (long) (unsigned int) num;
		 if (altform) {
		 prec++;
		 *p++ = '0';
		 }
		 base = 8;
		 break;
#ifdef COMPILER_HAS_DOUBLE
	 case 'f':
		 {
		 double 	 f;

		 f = va_arg(ap, double);
		 if (! prec_given)
			 prec = 6;
		 sal_ftoa(p, f, prec);
		 fillz = 0;
		 p = tmp;
		 prec = X_INF;
		 }
		 break;
#endif /* COMPILER_HAS_DOUBLE */
	 case 's':
		 p = va_arg(ap,char *);
		 if (prec == 0)
		 prec = X_INF;
		 break;
	 case 'c':
		 p[0] = va_arg(ap,int);
		 p[1] = 0;
		 prec = 1;
		 break;
	 case 'n':
		 *va_arg(ap,int *) = bp - buf;
		 p[0] = 0;
		 break;
	 case '%':
		 p[0] = '%';
		 p[1] = 0;
		 prec = 1;
		 break;
	 default:
		 X_STORE(c);
		 continue;
	 }

	 if (base != 0) {
		 sal_ltoa(p, num, base, (c == 'X'), prec);
		 if (prec)
		 fillz = 0;
		 p = tmp;
		 prec = X_INF;
	 }

	 if ((plen = sal_strlen(p)) > prec)
		 plen = prec;

	 if (width < plen)
		 width = plen;

	 pad = width - plen;

	 while (! ljust && pad-- > 0)
		 X_STORE(fillz ? '0' : ' ');
	 for (; plen-- > 0 && width-- > 0; p++)
		 X_STORE(*p);
	 while (pad-- > 0)
		 X_STORE(' ');
	 }

	 if ((be == b_inf) || (bp < be))
	 *bp = 0;
	 else
	 /*    coverity[var_deref_op : FALSE]	 */
	 *be = 0;
     if (bp > be) {
         sal_printf("out of range buf %s bufsize %d\n", buf, bufsize);
     }
	 return (bp - buf);
 }


 int sal_snprintf(char *buf, size_t bufsize, const char *fmt, ...)
 {
	 va_list	 ap;
	 int		 r;

	 va_start(ap,fmt);
	 r = sal_vsnprintf(buf, bufsize, fmt, ap);
	 va_end(ap);

	 return r;
 }

int sal_sprintf(char *buf, const char *fmt, ...)
{
    va_list             ap;
    int                 r;

    va_start(ap,fmt);
    r = sal_vsnprintf(buf, (size_t) X_INF, fmt, ap);
    va_end(ap);

    return r;
}


#if 0
 /*
  * _shr_ctoi
  *
  *   Converts a C-style constant integer to unsigned int
  */

 unsigned int
 _shr_ctoi(const char *s)
 {
     unsigned int        n, neg, base = 10;

     s += (neg = (*s == '-'));

     if (*s == '0') {
         s++;

         if (*s == 'x' || *s == 'X') {
             base = 16;
             s++;
         } else if (*s == 'b' || *s == 'B') {
             base = 2;
             s++;
         } else {
             base = 8;
         }
     }

     for (n = 0; ((*s >= 'a' && *s <= 'z' && base > 10) ||
                  (*s >= 'A' && *s <= 'Z' && base > 10) ||
                  (*s >= '0' && *s <= '9')); s++) {
         n = n * base +
             (*s >= 'a' ? *s - 'a' + 10 :
              *s >= 'A' ? *s - 'A' + 10 :
              *s - '0');
     }

     return (neg ? -n : n);
 }
#endif
#if 0
 int
shr_bitop_str_decode(char *str_value,
                     unsigned int *dst_ptr,
                     int max_words)
{
    char        *e;
    uint32      v;
    int         bit;

    sal_memset(dst_ptr, 0,  sizeof(uint32) * max_words);

    if (str_value[0] == '0' && (str_value[1] == 'x' || str_value[1] == 'X')) {
        /* get end of string */
        str_value += 2;
        for (e = str_value; *e; e++)
            ;
        e -= 1;
        /* back up to beginning of string, setting ports as we go */
        bit = 0;
        while (e >= str_value) {
            if (*e >= '0' && *e <= '9') {
            v = *e - '0';
            } else if (*e >= 'a' && *e <= 'f') {
            v = *e - 'a' + 10;
            } else if (*e >= 'A' && *e <= 'F') {
            v = *e - 'A' + 10;
            } else {
            return -1;          /* error: invalid hex digits */
            }
            e -= 1;
            /* now set a nibble's worth of ports */
            if ((v & 1) && bit <  sizeof(uint32)  * max_words) {
                 dst_ptr[bit/32] |= (1 << (bit%32));
            }
            bit += 1;
            if ((v & 2) && bit < sizeof(uint32) * max_words) {
                dst_ptr[bit/32] |= (1 << (bit%32));
            }
            bit += 1;
            if ((v & 4) && bit < sizeof(uint32) * max_words) {
                dst_ptr[bit/32] |= (1 << (bit%32));
            }
            bit += 1;
            if ((v & 8) && bit < sizeof(uint32) * max_words) {
                dst_ptr[bit/32] |= (1 << (bit%32));
            }
            bit += 1;
        }
    } else {
        v = 0;
        while (*dst_ptr >= '0' && *dst_ptr <= '9') {
            v = v * 10 + (*dst_ptr++ - '0');
        }
        if (*dst_ptr != '\0') {
                        return -1;                  /* error: invalid decimal digits */
                    }
                    bit = 0;
                    while (v) {
                        if ((v & 1) && bit < sizeof(uint32) * max_words) {
                        dst_ptr[bit/32] |= (1 << (bit%32));
                        }
                        v >>= 1;
                        bit += 1;
                    }
                }
                return 0;
}
#endif

#if 1
/*
 * sal_ctoi
 *
 *   Converts a C-style constant to integer.
 *   Also supports '0b' prefix for binary.
 */

int
sal_ctoi(const char *s, char **end)
{
    unsigned int        n, neg;
    int base = 10;

    if (s == 0) {
        if (end != 0) {
            end = 0;
        }
        return 0;
    }

    s += (neg = (*s == '-'));

    if (*s == '0') {
        s++;
        if (*s == 'x' || *s == 'X') {
            base = 16;
            s++;
        } else if (*s == 'b' || *s == 'B') {
            base = 2;
            s++;
        } else {
            base = 8;
        }
    }

    for (n = 0; ((*s >= 'a' && *s < 'a' + base - 10) ||
                 (*s >= 'A' && *s < 'A' + base - 10) ||
                 (*s >= '0' && *s <= '9')); s++) {
        n = n * base + ((*s <= '9' ? *s : *s + 9) & 15);
    }

    if (end != 0) {
        *end = (char *) s;
    }

    return (int) (neg ? -n : n);
}
#endif



/* src/soc/common/util.c */
#if 0

int soc_phy_fw_get(char *dev_name, uint8 **fw, int *fw_len)
{

    int i = 0;

    while(i < MAX_FW_TYPES) {
        if (fw_desc[i].fw == NULL) {
            /* empty slot */
            break;
        }
        if ( !sal_strcmp(dev_name, fw_desc[i].dev_name) ) {
            if (fw_desc[i].fw == NO_FW) {
                /* f/w unavailable */
                return SOC_E_UNAVAIL;
            }
            /* matching f/w found */
            *fw = fw_desc[i].fw;
            *fw_len = fw_desc[i].fw_len;
            return SOC_E_NONE;
        }
        i++;
    }
    if (i == MAX_FW_TYPES) {
        /* no more room */
        return SOC_E_UNAVAIL;
    }

    fw_desc[i].dev_name = dev_name;

    if (soc_phy_fw_acquire && ((*soc_phy_fw_acquire)(dev_name, fw, fw_len) == SOC_E_NONE)) {
        fw_desc[i].fw = *fw;
        fw_desc[i].fw_len = *fw_len;
        return SOC_E_NONE;
    } else {
        /* This type of f/w is not found. So add a blacklist entry. */
        fw_desc[i].fw = NO_FW;
    }

    return SOC_E_UNAVAIL;
}
#endif

uint32
sal_mutex_create(char *desc)
{
     return 0xFF;
}

void
sal_mutex_destroy(uint32 m)
{

}


int
sal_sem_take(uint32 b, int usec) {
   return 1;

}


int
sal_mutex_take(uint32 b, int usec) {
   return 1;

}
int
sal_sem_give(uint32 b)
{
  return 1;
}
int
sal_mutex_give(uint32 b, int usec) {
   return 1;

}
void * sal_sem_create(char *desc, int binary, int initial_count) {
   return (void *) 1;
}

#define BOOT_F_QUICKTURN 0x10000

uint32
sal_boot_flags_get(void)
{
#if CONFIG_EMULATION
    return BOOT_F_QUICKTURN;
#else
    return 0;
#endif
}
uint32
sal_thread_self(void)
{
    return 0x1;
}

unsigned int
sal_thread_create(char *a, int b, int c,void *f, void *t )
{
    return 0x1;
}

void
sal_udelay(uint32 usec) {
   sal_usleep(usec);
}

#define A(i)    ((void *) &((char *)(base))[(i) * (size)])

void
sal_qsort(void *base, int count, int size, int (*compar)(const void *, const void *))
{
    int         h = 1, i, j;
    char        tmp[256];

    SAL_ASSERT(size < (int)sizeof(tmp));

    while (h * 3 + 1 < count) {
        h = 3 * h + 1;
    }

    while (h > 0) {
        for (i = h - 1; i < count; i++) {
            sal_memcpy(tmp, A(i), size);

            for (j = i; j >= h && (*compar)(A(j - h), tmp) > 0; j -= h) {
                sal_memcpy(A(j), A(j - h), size);
            }

            sal_memcpy(A(j), tmp, size);
        }

        h /= 3;
    }
}

int
sal_strcasecmp(const char *dest, const char *src)
{
    char dc,sc;
    int rv = 0;

    while (1) {
        dc = sal_toupper(*dest++);
        sc = sal_toupper(*src++);
        if ((rv = dc - sc) != 0 || !dc) {
            break;
        }
    }
    return rv;
}

int
sal_strncasecmp(const char *dest, const char *src, size_t cnt)
{
    char dc, sc;
    int rv = 0;

    while (cnt) {
        dc = sal_toupper(*dest++);
        sc = sal_toupper(*src++);
        if ((rv = dc - sc) != 0 || !dc) {
            break;
        }
        cnt--;
    }
    return rv;
}
