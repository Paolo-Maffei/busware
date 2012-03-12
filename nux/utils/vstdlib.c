#include <ctype.h>
#include <stdarg.h>

#include <limits.h>

#include "vstdlib.h"

/* we use this so that we can do without the ctype library */
#define is_digit(c)     ((c) >= '0' && (c) <= '9')

#define ZEROPAD 1               /* pad with zero */
#define SIGN    2               /* unsigned/signed long */
#define PLUS    4               /* show plus */
#define SPACE   8               /* space if plus */
#define LEFT    16              /* left justified */
#define SPECIAL 32              /* 0x */
#define LARGE   64              /* use 'ABCDEF' instead of 'abcdef' */


size_t strnlen(const char * s, size_t count) {
        const char *sc;

        for (sc = s; count-- && *sc != '\0'; ++sc)
                /* nothing */;
        return sc - s;
}

static int skip_atoi(const char **s) {
        int i=0;

        while (is_digit(**s))
                i = i*10 + *((*s)++) - '0';
        return i;
}

#define do_div(n,base) ({ int __res; __res = ((unsigned long) n) % (unsigned) base; n = ((unsigned long) n) / (unsigned) base; __res; })

#define EMIT(x)                 ({ if(o<n) {*str++ = (x);} o++; })

static char * number(char * str, long num, int base, int size, int precision,int type, size_t o, size_t n) {
        char c,sign,tmp[66];
        const char *digits="0123456789abcdefghijklmnopqrstuvwxyz";
        int i;

        if (type & LARGE)
                digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        if (type & LEFT)
                type &= ~ZEROPAD;
        if (base < 2 || base > 36)
                return 0;
        c = (type & ZEROPAD) ? '0' : ' ';
        sign = 0;
        if (type & SIGN) {
                if (num < 0) {
                        sign = '-';
                        num = -num;
                        size--;
                } else if (type & PLUS) {
                        sign = '+';
                        size--;
                } else if (type & SPACE) {
                        sign = ' ';
                        size--;
                }
        }
        if (type & SPECIAL) {
                if (base == 16)
                        size -= 2;
                else if (base == 8)
                        size--;
        }
        i = 0;
        if (num == 0)
                tmp[i++]='0';
        else while (num != 0)
                tmp[i++] = digits[do_div(num,base)];
        if (i > precision)
                precision = i;
        size -= precision;
        if (!(type&(ZEROPAD+LEFT)))
                while(size-->0)
                        *str++ = ' ';
        if (sign)
                EMIT(sign);
        if (type & SPECIAL) {
                if (base==8)
                        EMIT('0');
                else if (base==16) {
                        EMIT('0');
                        EMIT(digits[33]);
                }
        }
        if (!(type & LEFT))
                while (size-- > 0)
                        EMIT(c);
        while (i < precision--)
                EMIT('0');
        while (i-- > 0)
                EMIT(tmp[i]);
        while (size-- > 0)
                EMIT(' ');
        return str;
}

int uvsnprintf(char * buf, size_t n, const char * fmt, va_list args) {
	        int len;
	        unsigned long num;
	        int i, base;
	        char * str;
	        const char *s;
			size_t o = 0;
	        int flags;              /* flags to number() */

	        int field_width;        /* width of output field */
	        int precision;          /* min. # of digits for integers; max
	                                   number of chars for from string */
	        int qualifier;          /* 'h', 'l', or 'L' for integer fields */

	        for (str=buf ; *fmt ; ++fmt) {
	                if (*fmt != '%') {
	                        EMIT(*fmt);
	                        continue;
	                }

	                /* process flags */
	                flags = 0;
	                repeat:
	                        ++fmt;          /* this also skips first '%' */
	                        switch (*fmt) {
	                                case '-': flags |= LEFT; goto repeat;
	                                case '+': flags |= PLUS; goto repeat;
	                                case ' ': flags |= SPACE; goto repeat;
	                                case '#': flags |= SPECIAL; goto repeat;
	                                case '0': flags |= ZEROPAD; goto repeat;
	                                }

	                /* get field width */
	                field_width = -1;
	                if (is_digit(*fmt))
	                        field_width = skip_atoi(&fmt);
	                else if (*fmt == '*') {
	                        ++fmt;
	                        /* it's the next argument */
	                        field_width = va_arg(args, int);
	                        if (field_width < 0) {
	                                field_width = -field_width;
	                                flags |= LEFT;
	                        }
	                }

	                /* get the precision */
	                precision = -1;
	                if (*fmt == '.') {
	                        ++fmt;
	                        if (is_digit(*fmt))
	                                precision = skip_atoi(&fmt);
	                        else if (*fmt == '*') {
	                                ++fmt;
	                                /* it's the next argument */
	                                precision = va_arg(args, int);
	                        }
	                        if (precision < 0)
	                                precision = 0;
	                }

	                /* get the conversion qualifier */
	                qualifier = -1;
	                if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L') {
	                        qualifier = *fmt;
	                        ++fmt;
	                }

	                /* default base */
	                base = 10;

	                switch (*fmt) {
	                case 'c':
	                        if (!(flags & LEFT))
	                                while (--field_width > 0)
	                                        *str++ = ' ';
	                        EMIT((unsigned char) va_arg(args, int));
	                        while (--field_width > 0)
	                                EMIT(' ');
	                        continue;

	                case 's':
	                        s = va_arg(args, char *);
	                        if (!s)
	                                s = "<NULL>";

	                        len = strnlen(s, precision);

	                        if (!(flags & LEFT))
	                                while (len < field_width--)
	                                        EMIT(' ');
	                        for (i = 0; i < len; ++i)
	                                EMIT(*s++);
	                        while (len < field_width--)
	                                EMIT(' ');
	                        continue;

	                case 'p':
	                        if (field_width == -1) {
	                                field_width = 2*sizeof(void *);
	                                flags |= ZEROPAD;
	                        }
	                        str = number(str,
	                                (unsigned long) va_arg(args, void *), 16,
	                                field_width, precision, flags, o, n);
	                        continue;


	                case 'n':
	                        if (qualifier == 'l') {
	                                long * ip = va_arg(args, long *);
	                                *ip = (str - buf);
	                        } else {
	                                int * ip = va_arg(args, int *);
	                                *ip = (str - buf);
	                        }
	                        continue;

	                case '%':
	                        EMIT('%');
	                        continue;

	                /* integer number formats - set up the flags and "break" */
	                case 'o':
	                        base = 8;
	                        break;

	                case 'X':
	                        flags |= LARGE;
	                case 'x':
	                        base = 16;
	                        break;

	                case 'd':
	                case 'i':
	                        flags |= SIGN;
	                case 'u':
	                        break;

	                default:
	                        EMIT('%');
	                        if (*fmt)
	                                EMIT(*fmt);
	                        else
	                                --fmt;
	                        continue;
	                }
	                if (qualifier == 'l')
	                        num = va_arg(args, unsigned long);
	                else if (qualifier == 'h') {
	                        num = (unsigned short) va_arg(args, int);
	                        if (flags & SIGN)
	                                num = (short) num;
	                } else if (flags & SIGN)
	                        num = va_arg(args, int);
	                else
	                        num = va_arg(args, unsigned int);
	                str = number(str, num, base, field_width, precision, flags,o,n);
	        }
	        EMIT('\0');
	        return str-buf;
}

int usprintf(char *pcBuf, const char *pcString, ...){
    va_list vaArgP;
    int iRet;

    va_start(vaArgP, pcString);
    iRet = uvsnprintf(pcBuf, 0xffff, pcString, vaArgP);
    va_end(vaArgP);
    return(iRet);	
}

int usnprintf(char *pcBuf, unsigned long ulSize, const char *pcString,...) {
    int iRet;
    va_list vaArgP;

    va_start(vaArgP, pcString);
    iRet = uvsnprintf(pcBuf, ulSize, pcString, vaArgP);
    va_end(vaArgP);
    return(iRet);
}

size_t ustrlen(const char * s) {
        const char * sc;

        for (sc = s; *sc != '\0'; ++sc);
        return sc - s;
}

char * ustrncpy(char * dest, const char * src, size_t n) {
        char * tmp = dest;

        while (n)
        {
                if ((*tmp = *src) != 0)
                        src++;
                tmp++;
                n--;
        }
        return dest;
}
/*
 * Convert a string to an unsigned long integer.
 *
 * Ignores 'locale' stuff.  Assumes that the upper and lower case
 * alphabets and digits are each contiguous.
 */
unsigned long ustrtoul(const char * nptr, char ** endptr, int base) {
        const char * s;
        unsigned long acc, cutoff;
        int c;
        int neg, any, cutlim;

        /*
         * See strtol for comments as to the logic used.
         */
        s = nptr;
        do {
                c = (unsigned char) *s++;
        } while (isspace(c));

        if (c == '-')
        {
                neg = 1;
                c = *s++;
        }
        else
        {
                neg = 0;
                if (c == '+')
                        c = *s++;
        }

        if ((base == 0 || base == 16) && c == '0' && (*s == 'x' || *s == 'X'))
        {
                c = s[1];
                s += 2;
                base = 16;
        }

        if (base == 0)
                base = c == '0' ? 8 : 10;

        cutoff = ULONG_MAX / (unsigned long) base;
        cutlim = ULONG_MAX % (unsigned long) base;

        for (acc = 0, any = 0;; c = (unsigned char) *s++)
        {
                if (isdigit(c))
                        c -= '0';
                else if (isalpha(c))
                        c -= isupper(c) ? 'A' - 10 : 'a' - 10;
                else
                        break;

                if (c >= base)
                        break;

                if (any < 0)
                        continue;

                if (acc > cutoff || (acc == cutoff && c > cutlim))
                {
                        any = -1;
                        acc = ULONG_MAX;
						break;
                }
                else
                {
                        any = 1;
                        acc *= (unsigned long) base;
                        acc += c;
                }
        }

        if (neg && any > 0)
                acc = -acc;

        if (endptr != 0)
                *endptr = (char *) (any ? s - 1 : nptr);

        return (acc);
}


int umemcmp(const void * s1, const void * s2, size_t n)
{
        const unsigned char *su1, *su2;
        int res = 0;

        for (su1 = s1, su2 = s2; 0 < n; ++su1, ++su2, n--)
                if ((res = *su1 - *su2) != 0)
                        break;
        return res;
}

char * ustrstr(const char * s1, const char * s2)
{
        size_t l1, l2;

        l2 = ustrlen(s2);
        if (!l2)
                return (char *)s1;

        l1 = ustrlen(s1);
        while (l1 >= l2)
        {
                l1--;
                if (!umemcmp(s1, s2, l2))
                        return (char *)s1;
                s1++;
        }

        return NULL;
}

int ustrncmp(const char * s1, const char * s2, size_t n)
{
        int __res = 0;

        while (n)
        {
                if ((__res = *s1 - *s2++) != 0 || !*s1++)
                        break;
                n--;
        }
        return __res;
}

char * ustrncat(char *s1, const char * s2, size_t n) {
	char *s = s1;
	while (*s != 0)
		s++;
	while (n != 0 && (*s = *s2++) != 0) {
		n--;
		s++;
	}
	if (*s != 0)
		*s = 0;
	return s1;
}