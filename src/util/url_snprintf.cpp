/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
 
 
/**
 * @file url_snprintf.cpp
 * @author liuqingjun(com@baidu.com)
 * @date 2012/11/23 11:34:57
 * @brief 
 *  
 **/

#include "util/url_snprintf.h"

int url_snprintf(char *original_buf, size_t size, const char *fmt, ...) {
    int ret;
    va_list vl;
    va_start(vl, fmt);
    ret = url_vsnprintf(original_buf, size, fmt, vl);
    va_end(vl);
    return ret;
}
int url_vsnprintf(char *original_buf, size_t size, const char *fmt, va_list vl) {
    const char hex[] = "0123456789abcdef";
    const char HEX[] = "0123456789ABCDEF";
    char *buf = original_buf;

#define _put(_c) do { \
    char c = (_c);\
    switch(c) { /*unreserved character in rfc 3986*/ \
        case 'A': \
        case 'B': \
        case 'C': \
        case 'D': \
        case 'E': \
        case 'F': \
        case 'G': \
        case 'H': \
        case 'I': \
        case 'J': \
        case 'K': \
        case 'L': \
        case 'M': \
        case 'N': \
        case 'O': \
        case 'P': \
        case 'Q': \
        case 'R': \
        case 'S': \
        case 'T': \
        case 'U': \
        case 'V': \
        case 'W': \
        case 'X': \
        case 'Y': \
        case 'Z': \
        case 'a': \
        case 'b': \
        case 'c': \
        case 'd': \
        case 'e': \
        case 'f': \
        case 'g': \
        case 'h': \
        case 'i': \
        case 'j': \
        case 'k': \
        case 'l': \
        case 'm': \
        case 'n': \
        case 'o': \
        case 'p': \
        case 'q': \
        case 'r': \
        case 's': \
        case 't': \
        case 'u': \
        case 'v': \
        case 'w': \
        case 'x': \
        case 'y': \
        case 'z': \
        case '0': \
        case '1': \
        case '2': \
        case '3': \
        case '4': \
        case '5': \
        case '6': \
        case '7': \
        case '8': \
        case '9': \
        case '-': \
        case '.': \
        case '_': \
        case '~': \
            if (size) {*buf++ = (c); size--;} break;    \
        default:                                        \
            switch(size) {                              \
                case 0:                                 \
                    break;                              \
                case 1:                                 \
                    *buf++ = '%';                       \
                    size = 0;                           \
                    break;                              \
                case 2:                                 \
                    *buf++ = '%';                       \
                    *buf++ = (((c)>>4) & 0xf) + '0';    \
                    size = 0;                           \
                    break;                              \
                default:                                \
                    *buf++ = '%';                       \
                    *buf++ = HEX[(((c)>>4) & 0xf)];     \
                    *buf++ = HEX[((c) & 0xf)];          \
                    size -= 3;                          \
                    break;                              \
            } \
    } \
    } while(0)
#define STATE_FLAGS 0
#define STATE_WIDTH 1
#define STATE_PRECISION 2
#define STATE_LENGTH 3
#define STATE_SPECIFIER 4
    char f;
    while ((f = *fmt++) && size > 0) {
        if (f == '%') {
            int state = 0;
            char c;
            unsigned char alternate = 0;
            unsigned char minus = 0;
            unsigned char plus = 0;
            unsigned char space = 0;
            unsigned char zero = 0;
            int width = 0;
            unsigned char haswidth = 0;
            int precision = 0;
            unsigned char hasprecision = 0;
            char length = '\0';
            while((c = *fmt++)) {
                int end = 0;
                switch(c) {
                    //flags
                    case '#':
                        if (state == STATE_FLAGS) {
                            alternate = 1;
                        } else {
                            //error
                        }
                        break;
                    case '-':
                        if (state == STATE_FLAGS) {
                            minus = 1;
                        } else {
                            //error
                        }
                        break;
                    case '+':
                        if (state == STATE_FLAGS) {
                            plus = 1;
                        } else {
                            //error
                        }
                        break;
                    case ' ':
                        if (state == STATE_FLAGS) {
                            space = 1;
                        } else {
                            //error
                        }
                        break;
                    case '\'':
                        break;

                    case '0':
                        if (state == STATE_FLAGS) {
                            zero = 1;
                            break;
                        }
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                        if (state <= STATE_WIDTH) {
                            width = width * 10 + (c - '0');
                            state = STATE_WIDTH;
                            haswidth = 1;
                        } else if (state == STATE_PRECISION) {
                            precision = precision * 10 + (c - '0');
                            hasprecision = 1;
                        } else {
                            //error
                        }
                        break;

                    case '.':
                        if (state <= STATE_PRECISION) {
                            state = STATE_PRECISION;
                            hasprecision = 1;
                        } else {
                            //error
                        }
                        break;
                    case '*':
                        if (state <= STATE_WIDTH) {
                            width = va_arg(vl, int);
                            state = STATE_PRECISION;
                            haswidth = 1;
                        } else if (state == STATE_PRECISION) {
                            precision = va_arg(vl, int);
                            state = STATE_LENGTH;
                            hasprecision = 1;
                        } else {
                            //error
                        }

                    //length
                    case 'h':
                        if (length == 'h') length = 'H';
                        else length = 'h';
                        state = STATE_LENGTH;
                        break;
                    case 'l':
                        if (length == 'l') length = 'L';
                        else length = 'l';
                        state = STATE_LENGTH;
                        break;
                    case 'j':
                        length = 'j';
                        state = STATE_LENGTH;
                        break;
                    case 'z':
                        length = 'z';
                        state = STATE_LENGTH;
                        break;
                    case 't':
                        length = 't';
                        state = STATE_LENGTH;
                        break;
                    case 'L':
                        length = 'D';
                        state = STATE_LENGTH;
                        break;

                    case 'c': //unsigned char
                        {
                            char val = va_arg(vl, int);
                            _put(val);
                            end = 1;
                        }
                        break;

#define _out_int_alt(type, maxlen, hasneg, base, alt, h) do {\
    type val = va_arg(vl, type); \
    unsigned char negative = 0; \
    char tmp[maxlen]; \
    char *cur = tmp; \
    if (val < 0) { \
        negative = 1; \
        val = -val; \
    } \
\
    if (val == 0) { \
        *cur++ = '0'; \
    } else { \
        while (val > 0) { \
            *cur = val % base; \
            if (base <= 10) { \
                *cur = *cur + '0'; \
            } else { \
                *cur = h[(int)*cur]; \
            } \
            cur++; \
            val /= base; \
        } \
    }\
    if (negative) { \
        if (zero && width) { \
            _put('-'); \
            width--; \
        } else { \
            *cur++ = '-'; \
        } \
    } else if (hasneg && plus) { \
        if (zero && width) { \
            _put('+'); \
            width--; \
        } else { \
            *cur++ = '+'; \
        } \
    } else if (hasneg && space) { \
        if (zero && width) { \
            _put(' '); \
            width--; \
        } else { \
            *cur++ = ' '; \
        } \
    } \
    alt; \
\
    if (!minus) { \
        if (!zero && width) { \
            for (int i = cur-tmp; i < width; i++) { \
                _put(' '); \
            } \
        } else if (zero && width) { \
            for (int i = cur-tmp; i < width; i++) { \
                _put('0'); \
            } \
        } \
    } \
    cur--; \
\
    while (cur >= tmp && size > 0) { \
        _put(*cur--); \
        width--; \
    } \
    while (width > 0 && size > 0) { \
        _put(' '); \
        width--; \
    } \
} while(0)
#define _out_int(type, maxlen, hasneg, base) \
    _out_int_alt(type, maxlen, hasneg, base, do{}while(0), hex)
#define _alt_o do { \
    if (alternate) { \
        if (zero && width) { \
            _put('0'); \
            width--; \
        } else { \
            *cur++ = '0'; \
        } \
    } \
} while(0)
#define _alt_x(_x) do { \
    if (alternate) { \
        if (zero && width) { \
            _put('0'); \
            _put(_x); \
            if (width < 2) width = 0; \
            else width -= 2; \
        } else { \
            *cur++ = _x; \
            *cur++ = '0'; \
        } \
    } \
} while(0)
                    case 'i': //signed int
                    case 'd':
                        switch(length) {
                            case 0:
                            case 'h':
                            case 'H':
                                _out_int(int, 16, 1, 10);
                                break;
                            case 'l':
                                _out_int(long, 32, 1, 10);
                                break;
                            case 'L':
                                _out_int(long long, 64, 1, 10);
                                break;
                            case 'z':
                                _out_int(size_t, 32, 1, 10);
                                break;
                            default:
                                ;
                                //error
                        }
                        end = 1;
                        break;

                    case 'u': //unsigned int
                        switch(length) {
                            case 0:
                            case 'h':
                            case 'H':
                                _out_int(unsigned int, 16, 0, 10);
                                break;
                            case 'l':
                                _out_int(unsigned long, 32, 0, 10);
                                break;
                            case 'L':
                                _out_int(unsigned long long, 64, 0, 10);
                                break;
                            case 'z':
                                _out_int(size_t, 32, 0, 10);
                                break;
                            default:
                                ;
                                //error
                        }
                        end = 1;
                        break;
                    case 'o': //unsigned int in octal
                        switch(length) {
                            case 0:
                            case 'h':
                            case 'H':
                                _out_int_alt(unsigned int, 16, 0, 8, _alt_o, hex);
                                break;
                            case 'l':
                                _out_int_alt(unsigned long, 32, 0, 8, _alt_o, hex);
                                break;
                            case 'L':
                                _out_int_alt(unsigned long long, 64, 0, 8, _alt_o, hex);
                                break;
                            case 'z':
                                _out_int_alt(size_t, 32, 0, 8, _alt_o, hex);
                                break;
                            default:
                                ;
                                //error
                        }
                        end = 1;
                        break;
                    case 'x': //hex
                        switch(length) {
                            case 0:
                            case 'h':
                            case 'H':
                                _out_int_alt(unsigned int, 16, 0, 16, _alt_x('x'), hex);
                                break;
                            case 'l':
                                _out_int_alt(unsigned long, 32, 0, 16, _alt_x('x'), hex);
                                break;
                            case 'L':
                                _out_int_alt(unsigned long long, 64, 0, 16, _alt_x('x'), hex);
                                break;
                            case 'z':
                                _out_int_alt(size_t, 32, 0, 16, _alt_x('x'), hex);
                                break;
                            default:
                                ;
                                //error
                        }
                        end = 1;
                        break;
                    case 'X': //HEX
                        switch(length) {
                            case 0:
                            case 'h':
                            case 'H':
                                _out_int_alt(unsigned int, 16, 0, 16, _alt_x('X'), HEX);
                                break;
                            case 'l':
                                _out_int_alt(unsigned long, 32, 0, 16, _alt_x('X'), HEX);
                                break;
                            case 'L':
                                _out_int_alt(unsigned long long, 64, 0, 16, _alt_x('X'), HEX);
                                break;
                            case 'z':
                                _out_int_alt(size_t, 32, 0, 16, _alt_x('X'), HEX);
                                break;
                            default:
                                ;
                                //error
                        }
                        end = 1;
                        break;
                    case 'p': //pointer
                        _out_int_alt(unsigned long, 32, 0, 16, _alt_x('x'), hex);
                        end = 1;
                        break;
                    case 'g': //double
                    case 'G': //g is not implemented for now
                    case 'f': //fixed point
                    case 'e': //double
                    case 'E':
                        {
                            unsigned char fmode = 0;
                            if (c == 'f' || c == 'F')
                                fmode = 1;
                            char capital = 0;
                            if (c == 'E' || c == 'G')
                                capital = 'E'-'e';
                            double val = va_arg(vl, double);
                            unsigned char negative = 0;
                            if (val < 0) {
                                val = -val;
                                negative = 1;
                            }
                            if (!hasprecision) precision = 6;
                            int exp = 0;
                            double scale = 1;
                            double rounder = 0.5;
                            int len = 0;
                            for (int i = precision; i > 0; i--, rounder *= 0.1);

                            if (fmode) {
                                val += rounder;
                            }

                            while( val>=1e100*scale && exp<=350 ){ scale *= 1e100;exp+=100;}
                            while( val>=1e64*scale && exp<=350 ){ scale *= 1e64; exp+=64; }
                            while( val>=1e8*scale && exp<=350 ){ scale *= 1e8; exp+=8; }
                            while( val>=10.0*scale && exp<=350 ){ scale *= 10.0; exp++; }
                            if (!fmode) {
                                val /= scale;
                                while( val<1e-100 && val > 0){ val *= 1e100; exp-=100; }
                                while( val<1e-64 && val > 0){ val *= 1e64; exp-=64; }
                                while( val<1e-8 && val > 0){ val *= 1e8; exp-=8; }
                                while( val<1.0 && val > 0){ val *= 10.0; exp--; }

                                val += rounder;
                                if (val >= 10.0) {
                                    val *= 0.1;
                                    exp++;
                                }
                            }

                            if (width) {
                                //if have width constrain
                                //calculate len first
                                if (negative || plus || space) {
                                    len += 1;
                                }
                                if (fmode) {
                                    len += exp + 1/*>0 part*/ + 1/*dot*/ + precision /*<0 part*/;
                                } else {
                                    len += 1/*integer part*/ + 1/*dot*/ + precision /*fraction part*/;
                                    if (exp >= 100 || exp <= -100) {
                                        len += 1/*e*/ + 1/*sign*/ + 3;
                                    } else {
                                        len += 1/*e*/ + 1/*sign*/ + 2;
                                    }
                                }
                                if (hasprecision && precision == 0 && !alternate) { //should we print the dot?
                                    len -= 1;
                                }
                            }

                            if (width > len && !minus && !zero) {
                                for (int i = width - len; i > 0; i--) {
                                    _put(' ');
                                }
                            }
                            if (negative) {
                                _put('-');
                            } else if (plus) {
                                _put('+');
                            } else if (space) {
                                _put(' ');
                            }
                            if (width > len && !minus && zero) {
                                for (int i = width - len; i > 0; i--) {
                                    _put('0');
                                }
                            }

                            if (fmode) {
                                exp += 1;
                                if (val >= 1) {
                                    for (int i = exp; i > 0; i--, scale *= 0.1) {
                                        _put(((int) (val / scale) % 10) + '0');
                                        val = val - ((double)((int)(val/scale)) * scale);
                                    }
                                } else {
                                    _put('0');
                                }

                                if (!(hasprecision && precision == 0 && !alternate)) {
                                    _put('.');
                                }

                                scale = 10;
                                for (int i = precision; i > 0; i--, scale *= 10) {
                                    _put(((int) (val * scale)%10) + '0');
                                    val = val - ((double)((int)(val*scale)) / scale);
                                }
                            } else {
                                _put((int) val + '0');
                                val = (val - (int)val) * 10.0;
                                if (!(hasprecision && precision == 0 && !alternate)) {
                                    _put('.');
                                }
                                for(int i = precision; i > 0; i--) {
                                    _put((int) val + '0');
                                    val = (val - (int)val) * 10.0;
                                }
                                _put('e' + capital);

                                if (exp < 0) {
                                    _put('-');
                                    exp = -exp;
                                } else {
                                    _put('+');
                                }

                                //print exponent part
                                int maxexp = 99;
                                while(maxexp < exp) maxexp = (maxexp+1)*10 - 1;
                                maxexp = (maxexp + 1) / 10;
                                while(maxexp >= 1) {
                                    _put(((exp/maxexp)%10)+'0');
                                    maxexp /= 10;
                                }
                            }

                            if (width > len && minus) {
                                for (int i = width - len; i > 0; i--) {
                                    _put(' ');
                                }
                            }
                        }
                        end = 1;
                        break;
                    case 's': //string
                        {
                            const char *orival = va_arg(vl, const char *);
                            const char *val = orival;
                            if (width && !minus) {
                                int len = 0;
                                for (const char *tmpchr = val; *tmpchr && len < width; tmpchr++, len++); //strlen
                                if (precision && len > precision) len = precision;
                                if (len < width) {
                                    for (int i = width - len; i > 0; i--) {
                                        _put(' ');
                                    }
                                    for (int i = 0; i < len; i++) {
                                        _put(val[i]);
                                    }
                                } else { //just print as is
                                    if (precision) {
                                        while (*val && size && precision) {
                                            _put(*val);
                                            val++;
                                            precision--;
                                        }
                                    } else {
                                        while (*val && size) {
                                            _put(*val);
                                            val++;
                                        }
                                    }
                                }
                            } else {
                                if (precision) {
                                    while (*val && size && precision) {
                                        _put(*val);
                                        val++;
                                        precision--;
                                    }
                                } else {
                                    while (*val && size) {
                                        _put(*val);
                                        val++;
                                    }
                                }
                                if (width) {
                                    for (width = width - (val - orival) ;width > 0; width--) {
                                        _put(' ');
                                    }
                                }
                            }
                            end = 1;
                        }
                        break;
                    case 'n': //write character so far into integer pointer
                    case '%':
                        {
                            _put('%');
                            end = 1;
                        }
                    default:
                        //error
                        break;
                }
                if (end) break;
            }
        } else {
            _put(f);
        }
    }
    *buf = 0;
    return buf - original_buf;
}


/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
