/*
Copyright (c) 2017-2018 Daniel Atanasov

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <new>
#include <squirrel.h>
#include <sqstdpackage.h>
#include <sqstdaux.h>
#include <sqstdio.h>
#include <sqstdstreamio.h>
#include <sqstdblob.h>
#include "sqstdtext.h"

#define DEFAULT_BAD_CHAR_ASCII  '?'
#define DEFAULT_BAD_CHAR        0xFFFD
#define BOM_CODEPOINT           0xFEFF

#define CV_OK       0
#define CV_ILUNI    100
#define CV_ILSEQ    -100

#define CBUFF_SIZE  16

#define IS_LE( _var) \
    bool _var; \
    { const uint16_t tmp = 1; _var = (*(const uint8_t*)&tmp == 1) ? true : false; }

/*
    returns:
        0 - CV_OK       - Code read from buffer
        <0              - Input buffer is empty (additional bytes needed)
        -100 - CV_ILSEQ - Invalid byte sequence
*/
typedef int (*cvReadFct)( uint32_t *pwc, const uint8_t **pbuf, const uint8_t* const buf_end);

/*
    returns:
        0 - CV_OK   - Code writen to buffer
        100 - CV_ILUNI  - Try to output invalid code
        >0              - Output buffer is full (additional bytes needed)
*/
typedef int (*cvWriteFct)( uint32_t wc, uint8_t **pbuf, const uint8_t* const buf_end);

typedef size_t (*cvStrSizeFct)( const uint8_t *str);

typedef struct cv_tag
{
    cvReadFct read;
    cvWriteFct write;
    cvStrSizeFct strsize;
    uint32_t bad_char;
    unsigned int min_char_size;
    unsigned int mean_char_size;
} cv_t;

typedef enum internal_encoding
{
    ENC_UTF8 = 0,
    ENC_UTF16,
    ENC_UTF16_REV,
    ENC_UTF32,
    ENC_UTF32_REV,
    ENC_ASCII,
    ENC_SQCHAR,
    ENC_SQCHAR_REV,
    ENC_UNKNOWN
} internal_encoding_t;

/* ================
    ASCII
================ */

static int cvReadASCII( uint32_t *pwc, const uint8_t **pbuf, const uint8_t* const buf_end)
{
    const uint8_t *buf = *pbuf;
    if( buf_end > buf) {
        uint8_t c = *buf;
        *pbuf = buf+1;
        if( c < 0x80) {
            *pwc = c;
            return CV_OK;
        }
        else
            return CV_ILSEQ;
    }
    return -1;
}

static int cvWriteASCII( uint32_t wc, uint8_t **pbuf, const uint8_t* const buf_end)
{
    if( wc < 0x80) {
        uint8_t *buf = *pbuf;
        if( buf_end > buf) {
            *buf = wc;
            *pbuf = buf+1;
            return CV_OK;
        }
        return 1;
    }
    return CV_ILUNI;
}

/* ================
    UTF-8
================ */

static const uint8_t utf8_lengths[8] =
{
	0,0,0,0,                /* 1000 to 1011 : not valid */
	2,2,                    /* 1100 to 1101 : 2 bytes */
	3,                      /* 1110 : 3 bytes */
	4                       /* 1111 :4 bytes */
};

static const uint8_t utf8_byte_masks[5] = {0,0,0x1f,0x0f,0x07};
static const uint32_t utf8_min_codept[5] = {0,0,0x80,0x800,0x10000};

static int cvReadUTF8( uint32_t *pwc, const uint8_t **pbuf, const uint8_t* const buf_end)
{
    const uint8_t *buf = *pbuf;
    uint8_t c;
    if( buf == buf_end) { *pbuf = buf; return -1; }
    c = *buf;
    if( c < 0x80) { *pbuf = buf+1; *pwc = c; return CV_OK; }
    else if( (c < 0xC2) || (c > 0xF4)) { *pbuf = buf+1; return CV_ILSEQ; }
    else {
        SQInteger codelen = utf8_lengths[ (c >> 4) - 8];
        if( !codelen) { *pbuf = buf+1; return CV_ILSEQ; }
        if( (buf_end - buf) < codelen) {
            *pbuf = buf;
            int need =  (buf_end - buf) - codelen;
            while( ++buf < buf_end) {
                if( (*buf & 0xC0) != 0x80) {
                    *pbuf = buf;
                    return CV_ILSEQ;
                }
            }
            return need; // need more bytes
        }
        SQInteger n = codelen;
        buf++;
        uint32_t wc = c & utf8_byte_masks[codelen];
        while( --n) {
            c = *buf;
            if( (c & 0xC0) != 0x80) { *pbuf = buf; return CV_ILSEQ; }
            buf++;
            wc <<= 6; wc |= c & 0x3F;
        }
        if( wc < utf8_min_codept[codelen]) { *pbuf = buf; return CV_ILSEQ; }
        *pbuf = buf;
        *pwc = wc;
        return CV_OK;
    }
}

static int cvWriteUTF8( uint32_t wc, uint8_t **pbuf, const uint8_t* const buf_end)
{
    uint8_t *buf = *pbuf;
    if( wc < 0xD800 || (wc >= 0xE000 && wc < 0x110000)) {
        int codelen;

        if( wc <= 0x007F)			{ codelen = 1; }
        else if( wc <= 0x07FF)		{ codelen = 2; }
        else if( wc <= 0xFFFFL)		{ codelen = 3; }
        else						{ codelen = 4; }

        if( (buf_end - buf) < codelen) return codelen - (buf_end - buf);    // need more space

        switch( codelen)
        { /* note: code falls through cases! */
            case 4: buf[3] = 0x80 | (wc & 0x3f); wc = wc >> 6; wc |= 0x10000;
            case 3: buf[2] = 0x80 | (wc & 0x3f); wc = wc >> 6; wc |= 0x800;
            case 2: buf[1] = 0x80 | (wc & 0x3f); wc = wc >> 6; wc |= 0xc0;
            case 1: buf[0] = wc;
        }
        buf += codelen;
        *pbuf = buf;
        return CV_OK;
    }
    return CV_ILUNI;
}

static size_t cvStrSizeUTF8( const uint8_t *str)
{
    const uint8_t *p = str;
    while( *p != '\0') p++;
    return p - str;
}

/* ================
    UTF-16
================ */

static uint16_t CV_SWAP_U16( const uint16_t val)
{
    return ((val & 0xFF) << 8) |
            ((val >> 8) & 0xFF);
}

static int cvReadUTF16_any( uint32_t *pwc, const uint8_t **pbuf, const uint8_t* const buf_end, const bool is_rev)
{
    const uint8_t *buf = *pbuf;
    uint16_t c;
    if( (buf_end-buf) < (int)sizeof(uint16_t)) { *pbuf = buf; return -(sizeof(uint16_t) - (buf_end-buf)); }
    c = *(const uint16_t*)buf;
    if( is_rev) { c = CV_SWAP_U16(c); }
    if( c >= 0xD800 && c < 0xDC00) {
        uint32_t wc = c;
        if( (buf_end-buf) < 2*(int)sizeof(uint16_t)) { *pbuf = buf; return -(2*sizeof(uint16_t) - (buf_end-buf)); }
        buf += sizeof(uint16_t);
        c = *(const uint16_t*)buf;
        if( is_rev) { c = CV_SWAP_U16(c); }
        if( c >= 0xDC00 && c < 0xE000) {
            buf += sizeof(uint16_t);
            wc = 0x10000 + ((wc - 0xD800) << 10) + (c - 0xDC00);
            *pbuf = buf;
            *pwc = wc;
            return CV_OK;
        }
        else { *pbuf = buf; return CV_ILSEQ; }
    }
    else if( c >= 0xDC00 && c < 0xE000) { *pbuf = buf+sizeof(uint16_t); return CV_ILSEQ; }
    else { *pbuf = buf+sizeof(uint16_t); *pwc = c; return SQ_OK; }
}

static int cvWriteUTF16_any( uint32_t wc, uint8_t **pbuf, const uint8_t* const buf_end, const bool is_rev)
{
    uint8_t *buf = *pbuf;
    if( wc < 0xD800 || (wc >= 0xE000 && wc < 0x110000)) {
        if( wc >= 0x10000) {
            if( (buf_end - buf) < 2*(int)sizeof(uint16_t)) return 2*sizeof(uint16_t) - (buf_end - buf);
            wc -= 0x10000;
            uint16_t hs = 0xD800 | ((wc >> 10) & 0x03FF);
            uint16_t ls = 0xDC00 | (wc & 0x03FF);
            if( is_rev) { hs = CV_SWAP_U16(hs); ls = CV_SWAP_U16(ls); }
            ((uint16_t*)buf)[0] = hs;
            ((uint16_t*)buf)[1] = ls;
            *pbuf = buf + 2*sizeof(uint16_t);
            return CV_OK;
        }
        else {
            if( (buf_end - buf) < (int)sizeof(uint16_t)) return sizeof(uint16_t) - (buf_end - buf);
            uint16_t c = (uint16_t)wc;
            if( is_rev) { c = CV_SWAP_U16(c); }
            *(uint16_t*)buf = c;
            *pbuf = buf + sizeof(uint16_t);
            return CV_OK;
        }
    }
    return CV_ILUNI;
}

static int cvReadUTF16( uint32_t *pwc, const uint8_t **pbuf, const uint8_t* const buf_end)
{
    return cvReadUTF16_any( pwc, pbuf, buf_end, false);
}

static int cvWriteUTF16( uint32_t wc, uint8_t **pbuf, const uint8_t* const buf_end)
{
    return cvWriteUTF16_any( wc, pbuf, buf_end, false);
}

static size_t cvStrSizeUTF16( const uint8_t *str)
{
    const uint8_t *p = str;
    while( *(const uint16_t*)p != '\0') p+=sizeof(uint16_t);
    return p - str;
}

/* ================
    UTF-16 REV
================ */

static int cvReadUTF16REV( uint32_t *pwc, const uint8_t **pbuf, const uint8_t* const buf_end)
{
    return cvReadUTF16_any( pwc, pbuf, buf_end, true);
}

static int cvWriteUTF16REV( uint32_t wc, uint8_t **pbuf, const uint8_t* const buf_end)
{
    return cvWriteUTF16_any( wc, pbuf, buf_end, true);
}

/* ================
    UTF-32
================ */

static uint32_t CV_SWAP_U32( const uint32_t val)
{
    return ((uint32_t)CV_SWAP_U16( val & 0xFFFF) << 16) |
            CV_SWAP_U16( val >> 16);
}

static int cvReadUTF32_any( uint32_t *pwc, const uint8_t **pbuf, const uint8_t* const buf_end, const bool is_rev)
{
    const uint8_t *buf = *pbuf;
    uint32_t wc;
    if( (buf_end-buf) < (int)sizeof(uint32_t)) { *pbuf = buf; return -(sizeof(uint32_t) - (buf_end-buf)); }
    wc = *(const uint32_t*)buf;
    if( is_rev) { wc = CV_SWAP_U32( wc); }
    buf += sizeof(uint32_t);
    if( wc < 0xD800 || (wc >= 0xE000 && wc < 0x110000)) {
        *pbuf = buf;
        *pwc = wc;
        return CV_OK;
    }
    else {
        *pbuf = buf;
        return CV_ILSEQ;
    }
}

static int cvWriteUTF32_any( uint32_t wc, uint8_t **pbuf, const uint8_t* const buf_end, const bool is_rev)
{
    uint8_t *buf = *pbuf;
    if( wc < 0xD800 || (wc >= 0xE000 && wc < 0x110000)) {
        if( (buf_end - buf) < (int)sizeof(uint32_t)) return sizeof(uint32_t) - (buf_end - buf);
        if( is_rev) { wc = CV_SWAP_U32( wc); }
        *(uint32_t*)buf = wc;
        *pbuf = buf + sizeof(uint32_t);
        return CV_OK;
    }
    return CV_ILUNI;
}


static int cvReadUTF32( uint32_t *pwc, const uint8_t **pbuf, const uint8_t* const buf_end)
{
    return cvReadUTF32_any( pwc, pbuf, buf_end, false);
}

static int cvWriteUTF32( uint32_t wc, uint8_t **pbuf, const uint8_t* const buf_end)
{
    return cvWriteUTF32_any( wc, pbuf, buf_end, false);
}

static size_t cvStrSizeUTF32( const uint8_t *str)
{
    const uint8_t *p = str;
    while( *(const uint32_t*)p != '\0') p+=sizeof(uint32_t);
    return p - str;
}

/* ================
    UTF-32 REV
================ */

static int cvReadUTF32REV( uint32_t *pwc, const uint8_t **pbuf, const uint8_t* const buf_end)
{
    return cvReadUTF32_any( pwc, pbuf, buf_end, true);
}

static int cvWriteUTF32REV( uint32_t wc, uint8_t **pbuf, const uint8_t* const buf_end)
{
    return cvWriteUTF32_any( wc, pbuf, buf_end, true);
}

/* ================
 all conversions
================ */

/*
Size of char in bytes for codepoints 0x0000 to 0xFFFF
    UTF8    UTF16   UTF32
min  1      2       4
mean 2.97   2       4

from->to = to_mean / from_min
*/

static const cv_t cvASCII = { cvReadASCII, cvWriteASCII, cvStrSizeUTF8, DEFAULT_BAD_CHAR_ASCII, 1, 1 };
static const cv_t cvUTF8 = { cvReadUTF8, cvWriteUTF8, cvStrSizeUTF8, DEFAULT_BAD_CHAR, 1, 3 };
static const cv_t cvUTF16 = { cvReadUTF16, cvWriteUTF16, cvStrSizeUTF16, DEFAULT_BAD_CHAR, 2, 2 };
static const cv_t cvUTF16REV = { cvReadUTF16REV, cvWriteUTF16REV, cvStrSizeUTF16, DEFAULT_BAD_CHAR, 2, 2 };
static const cv_t cvUTF32 = { cvReadUTF32, cvWriteUTF32, cvStrSizeUTF32, DEFAULT_BAD_CHAR, 4, 4 };
static const cv_t cvUTF32REV = { cvReadUTF32REV, cvWriteUTF32REV, cvStrSizeUTF32, DEFAULT_BAD_CHAR, 4, 4 };

extern "C" {
static const cv_t *cv_list[] = {
    [ENC_UTF8] = &cvUTF8,
    [ENC_UTF16] = &cvUTF16,
    [ENC_UTF16_REV] = &cvUTF16REV,
    [ENC_UTF32] = &cvUTF32,
    [ENC_UTF32_REV] = &cvUTF32REV,
    [ENC_ASCII] = &cvASCII,
};
}

static const cv_t &cvSQChar( void)
{
    if( sizeof(SQChar) == sizeof(uint8_t))
        return cvUTF8;
    else if( sizeof(SQChar) == sizeof(uint16_t))
        return cvUTF16;
    else if( sizeof(SQChar) == sizeof(uint32_t))
        return cvUTF32;
}

/* ====================================
		conversion names
==================================== */

static const SQChar *__NAMES_ASCII[] = { _SC("ASCII"), NULL };
static const SQChar *__NAMES_UTF_8[] = { _SC("UTF-8"), NULL };
static const SQChar *__NAMES_UTF_16BE[] = { _SC("UTF-16BE"), _SC("UTF-16"), _SC("UCS-2BE"), NULL };
static const SQChar *__NAMES_UTF_16LE[] = { _SC("UTF-16LE"), _SC("UCS-2"), _SC("UCS-2LE"), NULL };
static const SQChar *__NAMES_UTF_32BE[] = { _SC("UTF-32BE"), _SC("UTF-32"), _SC("UCS-4BE"), NULL };
static const SQChar *__NAMES_UTF_32LE[] = { _SC("UTF-32LE"), _SC("UCS-4"), _SC("UCS-4LE"), NULL };

static const struct enc_names
{
    int enc_id;
    const SQChar **names;
} encodings_list[] = {
	{ SQTEXTENC_UTF8, __NAMES_UTF_8 },
	{ SQTEXTENC_UTF16BE, __NAMES_UTF_16BE },
	{ SQTEXTENC_UTF16LE, __NAMES_UTF_16LE },
	{ SQTEXTENC_UTF32BE, __NAMES_UTF_32BE },
	{ SQTEXTENC_UTF32LE, __NAMES_UTF_32LE },
	{ SQTEXTENC_ASCII, __NAMES_ASCII },
	{ 0, 0 }
};

static int compare_encoding_name( const SQChar *iname, const SQChar *oname)
{
	SQChar i, o;
	while( ((i = *iname)!=_SC('\0')) & ((o = *oname)!=_SC('\0'))) {		// & - both i and o must be read
		if( !(i==o) && !(i>=_SC('A') && (i^o)==0x20)
		  && !( i==_SC('-') && o==_SC('_')) ) {
			if( i==_SC('-'))
				iname++;
			else
				break;
		}
		else { iname++; oname++; }
	}

	return i-o;
}

SQInteger sqstd_text_encbyname( const SQChar *name)
{
    SQInteger flags = 0;
    while(1) {
        static const SQChar f_strict[] = _SC("strict;");
        static const SQChar f_native[] = _SC("native;");
        if( memcmp( name, f_strict, sizeof(f_strict)-sizeof(SQChar)) == 0) {
            flags |= SQTEXTENC_STRICT;
            name += sizeof(f_strict)/sizeof(SQChar) - 1;
            continue;
        }
        if( memcmp( name, f_native, sizeof(f_native)-sizeof(SQChar)) == 0) {
            flags |= SQTEXTENC_NATIVE;
            name += sizeof(f_native)/sizeof(SQChar) - 1;
            continue;
        }
        break;
    }
    const struct enc_names *enc = encodings_list;
    while( enc->names) {
		const SQChar **pname = enc->names;
		while( *pname) {
			if( compare_encoding_name( *pname, name) == 0) {
				return enc->enc_id | flags;
			}
			pname++;
		}
        enc++;
    }
    return -1;
}

const SQChar* sqstd_text_encname( SQInteger encoding)
{
    int encoding_flags = encoding & SQTEXTENC_FLAGS;
    encoding ^= encoding_flags;
    if( encoding_flags & SQTEXTENC_NATIVE)
    {
        encoding = sqstd_text_nativeenc( encoding);
    }
    switch( encoding)
    {
        case SQTEXTENC_UTF8:    return __NAMES_UTF_8[0];
        case SQTEXTENC_UTF16BE: return __NAMES_UTF_16BE[0];
        case SQTEXTENC_UTF16LE: return __NAMES_UTF_16LE[0];
        case SQTEXTENC_UTF32BE: return __NAMES_UTF_32BE[0];
        case SQTEXTENC_UTF32LE: return __NAMES_UTF_32LE[0];
        case SQTEXTENC_ASCII:   return __NAMES_ASCII[0];
    }
    return 0;
}

SQInteger sqstd_text_defaultenc( void)
{
    IS_LE(is_le);
    if( sizeof(SQChar) == 1)
        return SQTEXTENC_UTF8;
    else if( sizeof(SQChar) == 2) {
        if( is_le)
            return SQTEXTENC_UTF16LE;
        else
            return SQTEXTENC_UTF16BE;
    }
    else if( sizeof(SQChar) == 4) {
        if( is_le)
            return SQTEXTENC_UTF32LE;
        else
            return SQTEXTENC_UTF32BE;
    }
}

SQInteger sqstd_text_nativeenc( SQInteger encoding)
{
    IS_LE(is_le);
    encoding &= ~SQTEXTENC_FLAGS;
    switch(encoding)
    {
        case SQTEXTENC_UTF16BE:
        case SQTEXTENC_UTF16LE:
            return is_le ? SQTEXTENC_UTF16LE : SQTEXTENC_UTF16BE;
        case SQTEXTENC_UTF32BE:
        case SQTEXTENC_UTF32LE:
            return is_le ? SQTEXTENC_UTF32LE : SQTEXTENC_UTF32BE;
        default:
            return encoding;
    }
}

static internal_encoding_t get_int_encoding( int encoding)
{
    int encoding_flags = encoding & SQTEXTENC_FLAGS;
    bool use_sqchar_cv;
    IS_LE(is_le);
    encoding ^= encoding_flags;
    if( encoding_flags & SQTEXTENC_NATIVE)
    {
        encoding = sqstd_text_nativeenc( encoding);
    }
    {
        bool is_not_strict = (encoding_flags & SQTEXTENC_STRICT) ? false : true;
        switch( encoding)
        {
            case SQTEXTENC_UTF8:
                use_sqchar_cv = (sizeof(SQChar) == 1) ? is_not_strict : false;
                break;
            case SQTEXTENC_UTF16BE:
            case SQTEXTENC_UTF16LE:
                use_sqchar_cv = (sizeof(SQChar) == 2) ? is_not_strict : false;
                break;
            case SQTEXTENC_UTF32BE:
            case SQTEXTENC_UTF32LE:
                use_sqchar_cv = (sizeof(SQChar) == 4) ? is_not_strict : false;
                break;
            default:
                use_sqchar_cv = false;
        }
    }
    if( use_sqchar_cv) {
        switch( encoding) {
            default:
            case SQTEXTENC_UTF8:
                return ENC_SQCHAR;
            case SQTEXTENC_UTF16BE:
            case SQTEXTENC_UTF32BE:
                return is_le ? ENC_SQCHAR_REV : ENC_SQCHAR;
            case SQTEXTENC_UTF16LE:
            case SQTEXTENC_UTF32LE:
                return is_le ? ENC_SQCHAR : ENC_SQCHAR_REV;
        }
    }
    else {
        switch( encoding)
        {
            case SQTEXTENC_UTF8:
                return ENC_UTF8;

            case SQTEXTENC_UTF16BE:
                return is_le ? ENC_UTF16_REV : ENC_UTF16;
            case SQTEXTENC_UTF16LE:
                return is_le ? ENC_UTF16 : ENC_UTF16_REV;

            case SQTEXTENC_UTF32BE:
                return is_le ? ENC_UTF32_REV : ENC_UTF32;
            case SQTEXTENC_UTF32LE:
                return is_le ? ENC_UTF32 : ENC_UTF32_REV;

            case SQTEXTENC_ASCII:
                return ENC_ASCII;

            default:
                return ENC_UNKNOWN;
        }
    }
}

/* ====================================
	Convert buffer
==================================== */

class cvTextAlloc
{
private:
    const cvReadFct _read;
    const cvWriteFct _write;
    const uint32_t _bad_char;
    const uint32_t _from_min;
    const uint32_t _to_mean;

    uint8_t *_output;
    size_t _out_alloc;
    uint8_t *_out_ptr;

    const uint8_t *_inbuf;
    const uint8_t *_inbuf_end;

    void Grow( void)
    {
        size_t bytes_left = _inbuf_end - _inbuf;
        size_t prev_out_alloc = _out_alloc;
        _out_alloc += _to_mean + (bytes_left * _to_mean) / _from_min;
        if( _output) {
            size_t out_len = _out_ptr - _output;
            _output = (uint8_t*)sq_realloc( _output, prev_out_alloc, _out_alloc);
            _out_ptr = _output + out_len;
        }
        else {
            _output = _out_ptr = (uint8_t*)sq_malloc( _out_alloc);
        }
    }

    int Append( uint32_t wc)
    {
        int r;
        do {
            r = _write( wc, &_out_ptr, _output + _out_alloc);
            if( r == CV_ILUNI) {
                _errors++;
                //if( _bad_char) {
                    wc = _bad_char;
                    r = _write( wc, &_out_ptr, _output + _out_alloc);
                    if( r == CV_ILUNI)
                        break;
                //}
                //else
                //    break;
            }
            if( r > 0) {
                Grow();
            }
        } while( r > 0);
        return r;
    }

    cvTextAlloc();
public:
    cvTextAlloc( const cv_tag &from, const cv_tag &to) :
        _read( from.read),
        _write( to.write),
        _bad_char( to.bad_char),
        _from_min( from.min_char_size),
        _to_mean( to.mean_char_size),
        _errors( 0)
    {}

    int _errors;

    SQInteger Convert( const uint8_t *inbuf, const uint8_t* const inbuf_end, SQSTDTEXTCV *textcv)
    {
        int r = CV_OK;
        _inbuf = inbuf;
        _inbuf_end = inbuf_end;
        _output = 0;
        _out_alloc = 0;
        Grow();
        while( _inbuf < _inbuf_end) {
            uint32_t wc;
            r = _read( &wc, &_inbuf, _inbuf_end);
            if( r != CV_OK ) {      // CV_ILSEQ or input buffer ends with partial codepoint
                _errors++;
                //if( _bad_char) {    //  both cases are threated as CV_ILSEQ
                    wc = _bad_char;
                    if( r != CV_ILSEQ) { //  but for EOB also input is terminated
                        _inbuf = _inbuf_end;
                    }
                //}
                //else break;
            }
            r = Append( wc);
        }
        size_t out_len = _out_ptr - _output;
        Append( 0);
        textcv->text = _output;
        textcv->allocated = _out_alloc;
        textcv->measure = out_len;
        return ((r == CV_OK) && !_errors) ? SQ_OK : SQ_ERROR;
    }
};

void sqstd_text_release( SQSTDTEXTCV *textcv)
{
    if( textcv->allocated != 0) {
        void *nctext = const_cast<void*>(textcv->text);
        sq_free( nctext, textcv->allocated);
        textcv->allocated = 0;
    }
}

/* ------------------------------------
    SQChar to SQChar
------------------------------------ */

static SQInteger sqstd_SQChar_to_SQChar( const SQChar *str, SQInteger str_size, SQSTDTEXTCV *textcv)
{
    if( str_size == -1) {
        str_size = cvSQChar().strsize( (const uint8_t*)str);
        textcv->text = str;
        textcv->allocated = 0;
        textcv->measure = str_size;
        return SQ_OK;
    }
    else {
        int padding = str_size % sizeof(SQChar);
        str_size -= padding;      // turnicate str_size to whole number of SQChars
        textcv->allocated = str_size + sizeof(SQChar);
        SQChar *out = (SQChar*)sq_malloc( textcv->allocated);
        textcv->text = out;
        memcpy( out, str, str_size);
        out[str_size / sizeof(SQChar)] = _SC('\0');
        textcv->measure = str_size;
        return padding ? SQ_ERROR : SQ_OK;
    }
}

static SQInteger sqstd_SQChar_to_SQCharREV( const SQChar *str, SQInteger str_size, SQSTDTEXTCV *textcv)
{
    int padding = 0;
    if( str_size == -1) {
        str_size = cvSQChar().strsize( (const uint8_t*)str);
    }
    else {
        padding = str_size % sizeof(SQChar);
        str_size -= padding;      // turnicate str_size to whole number of SQChars
    }
    textcv->allocated = str_size + sizeof(SQChar);
    SQChar *out = (SQChar*)sq_malloc( textcv->allocated);
    textcv->text = out;
    textcv->measure = str_size;
    SQInteger str_len = str_size / sizeof(SQChar);
    out[str_len] = _SC('\0');
    while( str_len) {
        if( sizeof(SQChar) == sizeof(uint16_t)) {
            *out = CV_SWAP_U16( *str);
        }
        else if( sizeof(SQChar) == sizeof(uint32_t)) {
            *out = CV_SWAP_U32( *str);
        }
        out++;
        str++;
        str_len--;
    }
    return padding ? SQ_ERROR : SQ_OK;
}

/* ------------------------------------
    UTFx to SQChar
------------------------------------ */

static SQInteger UTF_to_SQChar( const void *inbuf, SQInteger inbuf_size, SQSTDTEXTCV *textcv, const cv_t &cv)
{
    if( inbuf_size == -1) {
        inbuf_size = cv.strsize( (const uint8_t*)inbuf);
    }
    cvTextAlloc cvt( cv, cvSQChar());
    int r = cvt.Convert( (const uint8_t*)inbuf, (const uint8_t*)inbuf + inbuf_size, textcv);
    textcv->measure /= sizeof(SQChar);
    return r;
}

SQInteger sqstd_text_fromutf( int encoding, const void *inbuf, SQInteger inbuf_size, SQSTDTEXTCV *textcv)
{
    if( inbuf) {
        internal_encoding_t enc_id = get_int_encoding( encoding);
        switch( enc_id)
        {
            case ENC_SQCHAR: {
                SQInteger r = sqstd_SQChar_to_SQChar( (const SQChar*)inbuf, inbuf_size, textcv);
                textcv->measure /= sizeof(SQChar);
                return r;
            }
            case ENC_SQCHAR_REV: {
                SQInteger r =  sqstd_SQChar_to_SQCharREV( (const SQChar*)inbuf, inbuf_size, textcv);
                textcv->measure /= sizeof(SQChar);
                return r;
            }
            case ENC_UTF8:
            case ENC_UTF16:
            case ENC_UTF16_REV:
            case ENC_UTF32:
            case ENC_UTF32_REV:
            case ENC_ASCII:
                return UTF_to_SQChar( inbuf, inbuf_size, textcv, *cv_list[enc_id]);
            default:
                break;
        }
    }
    textcv->text = 0;
    textcv->allocated = 0;
    textcv->measure = 0;
    return -1;
}

/* ------------------------------------
    SQChar to UTFx
------------------------------------ */

static SQInteger SQChar_to_UTF( const SQChar *str, SQInteger str_size, SQSTDTEXTCV *textcv, const cv_t &cv)
{
    cvTextAlloc cvt( cvSQChar(), cv);
    int r;
    if( str_size == -1) {
        str_size = cvSQChar().strsize( (const uint8_t*)str);
    }
    r = cvt.Convert( (const uint8_t*)str, (const uint8_t*)str + str_size, textcv);
    return r;
}

SQInteger sqstd_text_toutf( int encoding, const SQChar *str, SQInteger str_len, SQSTDTEXTCV *textcv)
{
    if( str) {
        SQInteger str_size = str_len;
        if( str_size != -1) {
            str_size *= sizeof(SQChar);
        }
        internal_encoding_t enc_id = get_int_encoding( encoding);
        switch( enc_id)
        {
            case ENC_SQCHAR:
                return sqstd_SQChar_to_SQChar( (const SQChar*)str, str_size, textcv);
            case ENC_SQCHAR_REV:
                return sqstd_SQChar_to_SQCharREV( (const SQChar*)str, str_size, textcv);
            case ENC_UTF8:
            case ENC_UTF16:
            case ENC_UTF16_REV:
            case ENC_UTF32:
            case ENC_UTF32_REV:
            case ENC_ASCII:
                return SQChar_to_UTF( str, str_size, textcv, *cv_list[enc_id]);
            default:
                break;
        }
    }
    textcv->text = 0;
    textcv->allocated = 0;
    textcv->measure = 0;
    return -1;
}

/* ====================================
		Conversions
==================================== */

#ifndef TEST_WO_SQAPI
SQInteger sqstd_text_get(HSQUIRRELVM v, SQInteger idx, int encoding, SQSTDTEXTCV *textcv)
{
    const SQChar *str;
    SQInteger str_len;
    sq_getstringandsize(v,idx,&str,&str_len);
    sqstd_text_toutf( encoding, str, str_len, textcv);
    return 0;
}

SQInteger sqstd_text_push(HSQUIRRELVM v, int encoding, const void *inbuf, SQInteger inbuf_size)
{
    SQSTDTEXTCV textcv;
    SQInteger r = sqstd_text_fromutf( encoding, inbuf, inbuf_size, &textcv);
    sq_pushstring(v, (const SQChar*)textcv.text, textcv.measure);
    sqstd_text_release( &textcv);
    return r;
}
#endif // TEST_WO_SQAPI

/* ====================================
		Text reader
==================================== */

uint32_t get_bad_char( const SQChar *bad_char_str, const cv_t *cv)
{
    // null - default
    // "" - 0 - stop on error
    // "x" - x
    uint32_t bad_char = cv->bad_char;
    if( bad_char_str) {
        if( *bad_char_str) {
            const uint8_t *rdbuf = (const uint8_t*)bad_char_str;
            size_t str_size = cvSQChar().strsize( (const uint8_t*)bad_char_str);
            uint32_t wc;
            int r = cvSQChar().read( &wc, &rdbuf, (const uint8_t*)bad_char_str + str_size);
            if( r == CV_OK) {
                uint8_t tmp[16];
                uint8_t *wrbuf = tmp;
                r = cv->write( wc, &wrbuf, wrbuf+sizeof(wrbuf));
                if( r == CV_OK) {
                    bad_char = wc;
                }
            }
        }
        else {
            bad_char = 0;
        }
    }
    return bad_char;
}

struct SQTextReader
{
    SQTextReader( SQSTREAM stream) : _stream(stream), _buf_len(0), _buf_pos(0), _errors(0) {}
    virtual int read_next( void) = 0;
    virtual int init_from_buffer( uint8_t *tmp_buf, int tmp_buf_len) = 0;
    virtual int EOF( void) const { return (_buf_pos >= _buf_len) ? sqstd_seof(_stream) : 0; }
    int ClearErrors( void) { int r = _errors; _errors = 0; return r; }
    int PeekChar( SQChar *pc)
    {
        int r = CV_OK;
        if( _buf_pos >= _buf_len) {
            _buf_pos = 0;
            _buf_len = 0;
            r = read_next();
        }
        if( r == CV_OK) {
            *pc = _buf[_buf_pos];
        }
        return r;
    }

    int ReadChar( SQChar *pc)
    {
        int r = PeekChar( pc);
        if( r == CV_OK) {
            _buf_pos++;
        }
        return r;
    }
    int ReadLine( SQChar **pbuff, const SQChar *const buff_end)
    {
        SQChar *buff = *pbuff;
        int r = CV_OK;
        while( (buff < buff_end) && ((r = ReadChar( buff)) == CV_OK)) {
            if( *buff == _SC('\n'))
                break;
            buff++;
        }
        //if( (r == CV_OK) && (*buff != _SC('\n'))) {
        //    r = 1;
        //}
        *pbuff = buff;
        return r;
    }

    SQSTREAM _stream;
	size_t _buf_len;
	size_t _buf_pos;
    int _errors;
	SQChar _buf[CBUFF_SIZE];
};

struct SQTextReaderSQ : SQTextReader
{
    SQTextReaderSQ( SQSTREAM stream) : SQTextReader(stream) {}
    int read_next( void)
    {
        SQInteger rd = sqstd_sread( _buf, sizeof(SQChar), _stream);
        if( rd == sizeof(SQChar)) {
            _buf_len = 1;
            return CV_OK;
        }
        else if( rd) {
            _errors++;
        }
        return -1;  // EOF
    }
    int init_from_buffer( uint8_t *tmp_buf, int tmp_buf_len)
    {
        int r = CV_OK;
        int to_append = tmp_buf_len % sizeof(SQChar);
        if( to_append) {
            to_append = sizeof(SQChar) - to_append;
            SQInteger rd = sqstd_sread( tmp_buf+tmp_buf_len, to_append, _stream);
            if( rd == to_append) {
                tmp_buf_len += to_append;
            }
            else {
                _errors++;
                r = -1; // EOF
            }
        }
        memcpy( _buf, tmp_buf, tmp_buf_len);
        _buf_pos = 0;
        _buf_len = tmp_buf_len / sizeof(SQChar);
        return r;
    }
};

struct SQTextReaderSQREV : SQTextReaderSQ
{
    SQTextReaderSQREV( SQSTREAM stream) : SQTextReaderSQ(stream) {}
    int read_next( void)
    {
        int r = SQTextReaderSQ::read_next();
        if( r == CV_OK) {
            if( sizeof(SQChar) == 2) {
                _buf[0] = CV_SWAP_U16( _buf[0]);
            }
            else if( sizeof(SQChar) == 4) {
                _buf[0] = CV_SWAP_U32( _buf[0]);
            }
        }
        return r;
    }

    int init_from_buffer( uint8_t *tmp_buf, int tmp_buf_len)
    {
        int r = SQTextReaderSQ::init_from_buffer( tmp_buf, tmp_buf_len);
        for( size_t i=0; i < _buf_len; i++) {
            if( sizeof(SQChar) == 2) {
                _buf[i] = CV_SWAP_U16( _buf[i]);
            }
            else if( sizeof(SQChar) == 4) {
                _buf[i] = CV_SWAP_U32( _buf[i]);
            }
        }
        return r;
    }
};

struct SQTextReaderCV : SQTextReader
{
    SQTextReaderCV( SQSTREAM stream, const cv_t *cv, const SQChar *bad_char_str) : SQTextReader(stream), _cv(cv), _inbuf_len(0)
    {
        _bad_char = get_bad_char( bad_char_str, &cvSQChar());
    }

    int EOF( void) const { return !_inbuf_len && SQTextReader::EOF(); }

    int read_next( void)
    {
        const uint8_t *rdbuf = _inbuf;
        uint32_t wc;
        while(1) {
            SQInteger r;
            if( _inbuf_len) {
                r = _cv->read( &wc, &rdbuf, _inbuf+_inbuf_len);
                if( (r == CV_OK) || (r == CV_ILSEQ)) {
                    _inbuf_len = _inbuf + _inbuf_len - rdbuf;
                    if( _inbuf_len) {
                        memmove( _inbuf, rdbuf, _inbuf_len);
                    }
                    if( r == CV_ILSEQ) {
                        if( _bad_char) {
                            wc = _bad_char;
                            _errors++;
                        }
                        else
                            return r;
                    }
                    return append_wchar( wc);
                }
            }
            else {
                r = - _cv->min_char_size;
            }
            SQInteger rd = sqstd_sread( _inbuf+_inbuf_len, -r, _stream);
            if( rd) {
                _inbuf_len += rd;
            }
            else if( _inbuf_len && _bad_char) {
                _inbuf_len = 0;
                _errors++;
                return append_wchar( _bad_char);
            }
            else
                return r; // EOF
        }
    }

    int init_from_buffer( uint8_t *tmp_buf, int tmp_buf_len)
    {
        memcpy( _inbuf, tmp_buf, tmp_buf_len);
        _inbuf_len = tmp_buf_len;
        return CV_OK;
    }

private:
    int append_wchar( uint32_t wc)
    {
        uint8_t *tmp = (uint8_t*)_buf + _buf_pos;
        int r;
        r = cvSQChar().write( wc, &tmp, (uint8_t*)(_buf + CBUFF_SIZE));
        if( (r == CV_ILUNI)  && (_bad_char != 0)) {
            wc = _bad_char;
            _errors++;
            r = cvSQChar().write( wc, &tmp, (uint8_t*)(_buf + CBUFF_SIZE));
        }
        if( r == CV_OK) {
            _buf_len += (tmp - (uint8_t*)_buf) / sizeof(SQChar);
        }
        return r;
    }

    const cv_t *_cv;
    uint32_t _bad_char;
    size_t _inbuf_len;
    uint8_t _inbuf[16];
};

static SQTextReader *create_reader_by_enc( int encoding, SQSTREAM stream, const SQChar *bad_char_str)
{
    internal_encoding_t enc_id = get_int_encoding( encoding);
    switch( enc_id)
    {
        case ENC_SQCHAR:
            return new (sq_malloc(sizeof(SQTextReaderSQ)))SQTextReaderSQ( stream);
        case ENC_SQCHAR_REV:
            return new (sq_malloc(sizeof(SQTextReaderSQREV)))SQTextReaderSQREV( stream);
        case ENC_UTF8:
        case ENC_UTF16:
        case ENC_UTF16_REV:
        case ENC_UTF32:
        case ENC_UTF32_REV:
        case ENC_ASCII:
            return new (sq_malloc(sizeof(SQTextReaderCV)))SQTextReaderCV( stream, cv_list[enc_id], bad_char_str);
        default:
            return 0;
    }
}

static int read_BOM( SQSTREAM stream, uint8_t *tmp_buf, int *penc_id)
{
    SQInteger tmp_buf_len;
    tmp_buf_len = sqstd_sread( tmp_buf, 4, stream);
    switch( tmp_buf_len)
    {
        case 4:
            if( (tmp_buf[0] == 0x00) && (tmp_buf[1] == 0x00) && (tmp_buf[2] == 0xFE) && (tmp_buf[3] == 0xFF)) {
                // UTF32BE
                tmp_buf_len = 0;
                *penc_id = SQTEXTENC_UTF32BE;
                break;
            }
            else if( (tmp_buf[0] == 0xFF) && (tmp_buf[1] == 0xFE) && (tmp_buf[2] == 0x00) && (tmp_buf[3] == 0x00)) {
                // UTF32LE
                tmp_buf_len = 0;
                *penc_id = SQTEXTENC_UTF32LE;
                break;
            }
        case 3:
            if( (tmp_buf[0] == 0xEF) && (tmp_buf[1] == 0xBB) && (tmp_buf[2] == 0xBF)) {
                // UTF8
                tmp_buf[0] = tmp_buf[3];
                tmp_buf_len = (tmp_buf_len > 3) ? tmp_buf_len - 3 : 0;
                *penc_id = SQTEXTENC_UTF8;
                break;
            }
        case 2:
            if( (tmp_buf[0] == 0xFE) && (tmp_buf[1] == 0xFF)) {
                // UTF16BE
                *(uint16_t*)tmp_buf = *(uint16_t*)(tmp_buf+2);
                tmp_buf_len = (tmp_buf_len > 2) ? tmp_buf_len - 2 : 0;
                *penc_id = SQTEXTENC_UTF16BE;
                break;
            }
            else if( (tmp_buf[0] == 0xFF) && (tmp_buf[1] == 0xFE)) {
                // UTF16LE
                *(uint16_t*)tmp_buf = *(uint16_t*)(tmp_buf+2);
                tmp_buf_len = (tmp_buf_len > 2) ? tmp_buf_len - 2 : 0;
                *penc_id = SQTEXTENC_UTF16LE;
                break;
            }
        default:
            break;
    }
    return tmp_buf_len;
}

SQSTDTEXTRD sqstd_text_reader( SQSTREAM stream, int encoding, SQBool read_bom, const SQChar *bad_char_str)
{
    uint8_t tmp_buf[8];
    SQInteger tmp_buf_len = 0;
	SQTextReader *rd;

	if( read_bom) {
        tmp_buf_len = read_BOM( stream, tmp_buf, &encoding);
	}
	rd = create_reader_by_enc( encoding, stream, bad_char_str);
	if( tmp_buf_len) {
        rd->init_from_buffer( tmp_buf, tmp_buf_len);
	}
    return (SQSTDTEXTRD)rd;
}

void sqstd_text_readerrelease( SQSTDTEXTRD reader)
{
    SQTextReader *rd = (SQTextReader*)reader;
    rd->~SQTextReader();
    sq_free(rd,sizeof(SQTextReader));
}

SQInteger sqstd_text_peekchar( SQSTDTEXTRD reader, SQChar *pc)
{
    SQTextReader *rd = (SQTextReader*)reader;
    return rd->PeekChar( pc);
}

SQInteger sqstd_text_readchar( SQSTDTEXTRD reader, SQChar *pc)
{
    SQTextReader *rd = (SQTextReader*)reader;
    return rd->ReadChar( pc);
}

SQInteger sqstd_text_readline( SQSTDTEXTRD reader, SQChar **pbuff, SQChar * const buff_end)
{
    SQTextReader *rd = (SQTextReader*)reader;
    SQInteger r = rd->ReadLine( pbuff, buff_end);
    return r == CV_OK ? SQ_OK : SQ_ERROR;
}

SQInteger sqstd_text_rderrors( SQSTDTEXTRD reader)
{
    SQTextReader *rd = (SQTextReader*)reader;
    return rd->ClearErrors();
}

SQUIRREL_API SQInteger sqstd_text_rdeof( SQSTDTEXTRD reader)
{
    SQTextReader *rd = (SQTextReader*)reader;
    return rd->EOF();
}

/* ====================================
		Text Writer
==================================== */

struct SQTextWriter
{
    SQTextWriter( SQSTREAM stream) : _stream(stream), _errors(0) {}
    virtual int WriteChar( SQChar c) = 0;
    virtual int WriteString( const SQChar *str, SQInteger str_len) = 0;
    int ClearErrors( void) { int r = _errors; _errors = 0; return r; }
    SQSTREAM _stream;
	int _errors;
};

struct SQTextWriterSQ : SQTextWriter
{
    SQTextWriterSQ( SQSTREAM stream) : SQTextWriter(stream) {}
    int WriteChar( SQChar c)
    {
        if( sqstd_swrite( &c, sizeof(SQChar), _stream) == sizeof(SQChar))
            return 0;
        else
            return 1;
    }
    int WriteString( const SQChar *str, SQInteger str_len)
    {
        SQInteger str_size;
        if( str_len == -1) {
            str_size = cvSQChar().strsize( (const uint8_t*)str);
        }
        else {
            str_size = str_len * sizeof(SQChar);
        }
        if( sqstd_swrite( str, str_size, _stream) == str_size)
            return 0;
        else
            return 1;
    }
};

struct SQTextWriterSQREV : SQTextWriterSQ
{
    SQTextWriterSQREV( SQSTREAM stream) : SQTextWriterSQ(stream) {}
    int WriteChar( SQChar c)
    {
        if( sizeof(SQChar) == 2)
            return SQTextWriterSQ::WriteChar( CV_SWAP_U16( c));
        else if( sizeof(SQChar) == 4)
            return SQTextWriterSQ::WriteChar( CV_SWAP_U32( c));
        else
            return 1;
    }
    int WriteString( const SQChar *str, SQInteger str_len)
    {
        int r = 0;
        if( str_len == -1) {
            str_len = cvSQChar().strsize( (const uint8_t*)str) / sizeof(SQChar);
        }
        while( str_len && !r) {
            r = WriteChar( *str);
            str++;
            str_len--;
        }
        return r;
    }
};

struct SQTextWriterCV : SQTextWriter
{
    SQTextWriterCV( SQSTREAM stream, const cv_t *cv, const SQChar *bad_char_str) : SQTextWriter(stream), _cv(cv), _buf_len(0)
    {
        _bad_char = get_bad_char( bad_char_str, cv);
    }

    int WriteChar( SQChar c)
    {
        const uint8_t *rdbuf = (uint8_t*)_buf;
        uint32_t wc;
        int r;

        _buf[_buf_len++] = c;
        r = cvSQChar().read( &wc, &rdbuf, (uint8_t*)(_buf + _buf_len));
        if( (r == CV_ILSEQ) && (_bad_char != 0)) {
            r = CV_OK;
            _errors++;
            wc = _bad_char;
        }
        if( (r != CV_ILSEQ) && (r < 0) ) {
            return CV_OK;
        }
        else if( r == CV_OK) {
            _buf_len = 0;
            return WriteCodepoint( wc);
        }
        else {
            _buf_len = 0;
            return r;
        }
    }

    int WriteString( const SQChar *str, SQInteger str_len)
    {
        const uint8_t *rdbuf = (const uint8_t*)str;
        uint32_t wc;
        int r = CV_OK;
        if( str_len == -1) {
            str_len = cvSQChar().strsize( (const uint8_t*)str) / sizeof(SQChar);
        }
        while(rdbuf < (const uint8_t*)(str + str_len)) {
            r = cvSQChar().read( &wc, &rdbuf, (const uint8_t*)(str + str_len));
            if( (r == CV_ILSEQ) && (_bad_char != 0)) {
                r = CV_OK;
                _errors++;
                wc = _bad_char;
            }
            if( r == CV_OK) {
                r = WriteCodepoint( wc);
            }
            if( r != CV_OK)
                break;
        }
        return r;
    }

private:
    int WriteCodepoint( uint32_t wc)
    {
        uint8_t tmp[8];
        uint8_t *wrbuf = tmp;
        int r;

        r = _cv->write( wc, &wrbuf, tmp + sizeof(tmp));
        if( (r == CV_ILUNI) && (_bad_char != 0)) {
            wrbuf = tmp;
            _errors++;
            r = _cv->write( _bad_char, &wrbuf, tmp + sizeof(tmp));
        }
        if( r == CV_OK) {
            SQInteger wrbuf_len = wrbuf - tmp;
            if( sqstd_swrite( tmp, wrbuf_len, _stream) != wrbuf_len) {
                r = 1;
            }
        }
        return r;
    }

    const cv_t *_cv;
	size_t _buf_len;
    uint32_t _bad_char;
	SQChar _buf[CBUFF_SIZE];
};

static SQTextWriter *create_writer_by_enc( int encoding, SQSTREAM stream, const SQChar *bad_char_str)
{
    internal_encoding_t enc_id = get_int_encoding( encoding);
    switch( enc_id)
    {
        case ENC_SQCHAR:
            return new (sq_malloc(sizeof(SQTextWriterSQ)))SQTextWriterSQ( stream);
        case ENC_SQCHAR_REV:
            return new (sq_malloc(sizeof(SQTextWriterSQREV)))SQTextWriterSQREV( stream);
        case ENC_UTF8:
        case ENC_UTF16:
        case ENC_UTF16_REV:
        case ENC_UTF32:
        case ENC_UTF32_REV:
        case ENC_ASCII:
            return new (sq_malloc(sizeof(SQTextWriterCV)))SQTextWriterCV( stream, cv_list[enc_id], bad_char_str);
        default:
            return 0;
    }
}

static int write_BOM( SQSTREAM stream, int encoding)
{
    int encoding_flags = encoding & SQTEXTENC_FLAGS;
    encoding ^= encoding_flags;
    if( encoding_flags & SQTEXTENC_NATIVE)
    {
        encoding = sqstd_text_nativeenc( encoding);
    }
    switch( encoding)
    {
        case SQTEXTENC_UTF8: {
            static const uint8_t bom[3] = { 0xEF, 0xBB, 0xBF };
            return sqstd_swrite( bom, sizeof(bom), stream) != sizeof(bom);
        }
        case SQTEXTENC_UTF16BE: {
            static const uint8_t bom[2] = { 0xFE, 0xFF };
            return sqstd_swrite( bom, sizeof(bom), stream) != sizeof(bom);
        }
        case SQTEXTENC_UTF16LE: {
            static const uint8_t bom[2] = { 0xFF, 0xFE };
            return sqstd_swrite( bom, sizeof(bom), stream) != sizeof(bom);
        }
        case SQTEXTENC_UTF32BE: {
            static const uint8_t bom[4] = { 0x00, 0x00, 0xFE, 0xFF };
            return sqstd_swrite( bom, sizeof(bom), stream) != sizeof(bom);
        }
        case SQTEXTENC_UTF32LE: {
            static const uint8_t bom[4] = { 0xFF, 0xFE, 0x00, 0x00 };
            return sqstd_swrite( bom, sizeof(bom), stream) != sizeof(bom);
        }
        case SQTEXTENC_ASCII:
        default:
            return 0;
    }
}

SQSTDTEXTWR sqstd_text_writer( SQSTREAM stream, int encoding, SQBool write_bom, const SQChar *bad_char_str)
{
    SQSTDTEXTWR wr = (SQSTDTEXTWR)create_writer_by_enc( encoding, stream, bad_char_str);
    if( write_bom) {
        write_BOM( stream, encoding);
    }
    return wr;
}

void sqstd_text_writerrelease( SQSTDTEXTWR writer)
{
    SQTextWriter *wr = (SQTextWriter*)writer;
    //wr->~SQTextWriter();
    sq_free(wr,sizeof(SQTextWriter));
}

SQInteger sqstd_text_writechar( SQSTDTEXTWR writer, SQChar c)
{
    SQTextWriter *wr = (SQTextWriter*)writer;
    return wr->WriteChar( c);
}

SQInteger sqstd_text_writestring( SQSTDTEXTWR writer, const SQChar *str, SQInteger len)
{
    SQTextWriter *wr = (SQTextWriter*)writer;
    return wr->WriteString( str, len);
}

SQInteger sqstd_text_wrerrors( SQSTDTEXTWR writer)
{
    SQTextWriter *wr = (SQTextWriter*)writer;
    return wr->ClearErrors();
}

/* ====================================
==================================== */
#ifndef TEST_WO_SQAPI

/*
    function blobtostr( blob, enc)
    function strtoblob( str, enc)

    class reader
    {
        constructor( stream, encoding, read_bom, bad_char)
        function readchar()
        function peekchar()
        function readline( max_chars)
    }

    class writer
    {
        constructor( stream, encoding, write_bom, bad_char)
        function print( string)
    }

    function textfile( name, flags, encoding, use_bom)

*/

static SQRESULT _textrw_blobtostr(HSQUIRRELVM v)
{
    SQUserPointer ptr;
    SQInteger size;
    const SQChar *enc;
    int enc_id;
    SQBool is_strict = SQFalse;
    if( SQ_FAILED(sqstd_getblob(v,2,&ptr))) return SQ_ERROR;
    size = sqstd_getblobsize(v,2);
    sq_getstring(v,3,&enc);
    enc_id = sqstd_text_encbyname( enc);
    if( enc_id < 0) {
        return sq_throwerror(v,_SC("Unknown encoding name"));
    }
    if( sq_gettop(v) > 3) {
        sq_getbool(v,4,&is_strict);
    }
    SQInteger r = sqstd_text_push(v,enc_id,ptr,size);
    if( is_strict && r) {
        return sq_throwerror(v,_SC("Text conversion failed"));
    }
    return 1;
}

static SQRESULT _textrw_strtoblob(HSQUIRRELVM v)
{
    const SQChar *enc;
    int enc_id;
    SQSTDTEXTCV out;
    SQBool is_strict = SQFalse;
    sq_getstring(v,3,&enc);
    enc_id = sqstd_text_encbyname( enc);
    if( enc_id < 0) {
        return sq_throwerror(v,_SC("Unknown encoding name"));
    }
    if( sq_gettop(v) > 3) {
        sq_getbool(v,4,&is_strict);
    }
    SQInteger r = sqstd_text_get(v,2,enc_id,&out);
    if( is_strict && r) {
        sqstd_text_release(&out);
        return sq_throwerror(v,_SC("Text conversion failed"));
    }
    else {
        SQUserPointer blob = sqstd_createblob(v,out.measure);
        memcpy(blob,out.text,out.measure);
        sqstd_text_release(&out);
        return 1;
    }
}

#define _DECL_TEXTRW_FUNC(name,nparams,typecheck) {_SC(#name),_textrw_##name,nparams,typecheck}
static const SQRegFunction _textrw_globals[]={
    _DECL_TEXTRW_FUNC(blobtostr,-3,_SC(".xsb")),
    _DECL_TEXTRW_FUNC(strtoblob,-3,_SC(".ssb")),
    {NULL,(SQFUNCTION)0,0,NULL}
};

/* ====================================
	Text reader
==================================== */

static HSQMEMBERHANDLE textrd__stream_handle;

static SQInteger _textrd_releasehook( SQUserPointer p, SQInteger SQ_UNUSED_ARG(size))
{
    SQSTDTEXTRD rdr = (SQSTDTEXTRD)p;
    sqstd_text_readerrelease(rdr);
    return 1;
}

static SQInteger _textrd_constructor(HSQUIRRELVM v)
{
	SQSTREAM stream;
	const SQChar *encoding;
	SQInteger enc_id;
    SQBool read_bom = SQFalse;
    const SQChar *bad_char = 0;

    if( SQ_FAILED( sq_getinstanceup( v,2,(SQUserPointer*)&stream,SQSTD_STREAM_TYPE_TAG))) {
        return sq_throwerror(v,_SC("argument is not a stream"));
	}
    sq_getstring(v, 3, &encoding);
    SQInteger top = sq_gettop(v);
    if( top > 3) {
        sq_getbool(v, 4, &read_bom);
        if( top > 4) {
            sq_getstring(v, 5, &bad_char);
        }
    }

    enc_id = sqstd_text_encbyname( encoding);
    if( enc_id == -1) {
        return sq_throwerror(v,_SC("unknown encoding"));
    }

    SQSTDTEXTRD rdr = sqstd_text_reader( stream, enc_id, read_bom, bad_char);
    if( rdr) {
        if(SQ_FAILED(sq_setinstanceup(v,1,rdr))) {
            sqstd_text_readerrelease(rdr);
            return sq_throwerror(v, _SC("cannot create text reader instance"));
        }

        sq_setreleasehook(v,1,_textrd_releasehook);

        // save stream in _stream member
        sq_push(v,2);
        sq_setbyhandle(v,1,&textrd__stream_handle);

        return 0;
    }
    else {
        return sq_throwerror(v, _SC("failed to create text reader"));
    }
}

#define SETUP_TEXTRD(v) \
    SQSTDTEXTRD rdr = NULL; \
    if(SQ_FAILED(sq_getinstanceup(v,1,(SQUserPointer*)&rdr,SQSTD_TEXTRD_TYPE_TAG))) \
        return sq_throwerror(v,_SC("invalid type tag"));

static SQInteger _textrd_peekchar(HSQUIRRELVM v)
{
    SQInteger r;
    SQChar c;
    SETUP_TEXTRD(v)
    r = sqstd_text_peekchar( rdr, &c);
    if( r == SQ_OK) {
        sq_pushstring(v, &c, 1);
    }
    else {
        sq_pushnull(v);
    }
    return 1;
}

static SQInteger _textrd_readchar(HSQUIRRELVM v)
{
    SQInteger r;
    SQChar c;
    SETUP_TEXTRD(v)
    r = sqstd_text_readchar( rdr, &c);
    if( r == SQ_OK) {
        sq_pushstring(v, &c, 1);
    }
    else {
        sq_pushnull(v);
    }
    return 1;
}

#define READLINE_DEFAULT_MAX    8192
#define READLINE_STEP_SIZE      256

static SQInteger _textrd_readline(HSQUIRRELVM v)
{
    SQInteger max_len = READLINE_DEFAULT_MAX;
    SQInteger len = 0;
    SQInteger allocated = 0;
    SQChar *buff;
    SETUP_TEXTRD(v)
    if(sq_gettop(v) > 1) {
        sq_getinteger(v,2,&max_len);
        if( max_len < 0) max_len = (1U << (8*sizeof(SQInteger)-1)) - 1U;
    }
    do {
        if( allocated >= max_len) break;
        allocated+=READLINE_STEP_SIZE;
        if( allocated > max_len) allocated = max_len;
        buff = sq_getscratchpad(v,allocated);
        SQChar *end = buff + len;
        sqstd_text_readline( rdr, &end, buff + allocated);
        len = end - buff;
    } while( len < allocated);
    if( len) {
        sq_pushstring(v, buff, len);
    }
    else {
        sq_pushnull(v);
    }
    return 1;
}

static SQInteger _textrd__typeof(HSQUIRRELVM v);

#define _DECL_TEXTRD_FUNC(name,nparams,typecheck) {_SC(#name),_textrd_##name,nparams,typecheck}
static const SQRegFunction _textrd_methods[]={
    _DECL_TEXTRD_FUNC(constructor,-3,_SC("xxsbs")),
    _DECL_TEXTRD_FUNC(peekchar,1,_SC("x")),
    _DECL_TEXTRD_FUNC(readchar,1,_SC("x")),
    _DECL_TEXTRD_FUNC(readline,-2,_SC("xi")),
    _DECL_TEXTRD_FUNC(_typeof,1,_SC("x")),
    {NULL,(SQFUNCTION)0,0,NULL}
};

static const SQRegMember _textrd_members[] = {
	{_SC("_stream"), &textrd__stream_handle },
	{NULL,NULL}
};

SQUserPointer _sqstd_textrd_type_tag(void)
{
    return (SQUserPointer)_sqstd_textrd_type_tag;
}

static const SQRegClass _sqstd_textrd_decl = {
    _SC("reader"),		// name
	_textrd_members,	// members
	_textrd_methods,	// methods
};

static SQInteger _textrd__typeof(HSQUIRRELVM v)
{
    sq_pushstring(v,_sqstd_textrd_decl.name,-1);
    return 1;
}

/* ====================================
	Text writer
==================================== */

static HSQMEMBERHANDLE textwr__stream_handle;

static SQInteger _textwr_releasehook( SQUserPointer p, SQInteger SQ_UNUSED_ARG(size))
{
    SQSTDTEXTWR wrt = (SQSTDTEXTWR)p;
    sqstd_text_writerrelease(wrt);
    return 1;
}

static SQInteger _textwr_constructor(HSQUIRRELVM v)
{
	SQSTREAM stream;
	const SQChar *encoding;
	SQInteger enc_id;
    SQBool write_bom = SQFalse;
    const SQChar *bad_char = 0;

    if( SQ_FAILED( sq_getinstanceup( v,2,(SQUserPointer*)&stream,SQSTD_STREAM_TYPE_TAG))) {
        return sq_throwerror(v,_SC("argument is not a stream"));
	}
    sq_getstring(v, 3, &encoding);
    SQInteger top = sq_gettop(v);
    if( sq_gettop(v) > 3) {
        sq_getbool(v, 4, &write_bom);
        if( top > 4) {
            sq_getstring(v, 5, &bad_char);
        }
    }

    enc_id = sqstd_text_encbyname( encoding);
    if( enc_id == -1) {
        return sq_throwerror(v,_SC("unknown encoding"));
    }

    SQSTDTEXTWR wrt = sqstd_text_writer( stream, enc_id, write_bom, 0);
    if( wrt) {
        if(SQ_FAILED(sq_setinstanceup(v,1,wrt))) {
            sqstd_text_writerrelease(wrt);
            return sq_throwerror(v, _SC("cannot create text writer instance"));
        }

        sq_setreleasehook(v,1,_textwr_releasehook);

        // save stream in _stream member
        sq_push(v,2);
        sq_setbyhandle(v,1,&textwr__stream_handle);

        return 0;
    }
    else {
        return sq_throwerror(v, _SC("failed to create text writer"));
    }
}

#define SETUP_TEXTWR(v) \
    SQSTDTEXTWR wrt = NULL; \
    if(SQ_FAILED(sq_getinstanceup(v,1,(SQUserPointer*)&wrt,SQSTD_TEXTWR_TYPE_TAG))) \
        return sq_throwerror(v,_SC("invalid type tag"));

static SQInteger _textwr_print(HSQUIRRELVM v)
{
    SQInteger r;
    const SQChar *str;
    SQInteger len;
    SETUP_TEXTWR(v)
    sq_getstringandsize(v,2,&str,&len);
    r = sqstd_text_writestring( wrt, str, len);
    sq_pushinteger(v,r);
    return 1;
}

static SQInteger _textwr__typeof(HSQUIRRELVM v);

#define _DECL_TEXTWR_FUNC(name,nparams,typecheck) {_SC(#name),_textwr_##name,nparams,typecheck}
static const SQRegFunction _textwr_methods[]={
    _DECL_TEXTWR_FUNC(constructor,-3,_SC("xxsbs")),
    _DECL_TEXTWR_FUNC(print,2,_SC("xs")),
    _DECL_TEXTWR_FUNC(_typeof,1,_SC("x")),
    {NULL,(SQFUNCTION)0,0,NULL}
};

static const SQRegMember _textwr_members[] = {
	{_SC("_stream"), &textwr__stream_handle },
	{NULL,NULL}
};

SQUserPointer _sqstd_textwr_type_tag(void)
{
    return (SQUserPointer)_sqstd_textwr_type_tag;
}

static const SQRegClass _sqstd_textwr_decl = {
    _SC("writer"),		// name
	_textwr_members,	// members
	_textwr_methods,	// methods
};

static SQInteger _textwr__typeof(HSQUIRRELVM v)
{
    sq_pushstring(v,_sqstd_textwr_decl.name,-1);
    return 1;
}

/* ====================================
		Register
==================================== */
extern "C" {
    SQUIRREL_API SQInteger SQPACKAGE_LOADFCT(HSQUIRRELVM v);
}
SQInteger SQPACKAGE_LOADFCT(HSQUIRRELVM v)
{
    sq_newtable(v);

	if(SQ_FAILED(sqstd_registerclass(v,SQSTD_TEXTRD_TYPE_TAG,&_sqstd_textrd_decl,0))) {
		return SQ_ERROR;
	}
	sq_poptop(v);

	if(SQ_FAILED(sqstd_registerclass(v,SQSTD_TEXTWR_TYPE_TAG,&_sqstd_textwr_decl,0))) {
		return SQ_ERROR;
	}
	sq_poptop(v);

    if(SQ_FAILED(sqstd_registerfunctions(v,_textrw_globals))) {
        return SQ_ERROR;
    }

	return 1;
}

/*
SQUIRREL_API SQRESULT sqstd_register_textrwlib(HSQUIRRELVM v)
{
    if(SQ_SUCCEEDED(sqstd_package_registerfct(v,_SC("textrw"),SQPACKAGE_LOADFCT))) {
        sq_poptop(v);
        return SQ_OK;
    }
    return SQ_ERROR;
}
*/
#endif // TEST_WO_SQAPI
