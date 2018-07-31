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
#include <stdio.h>  // !!! DEBUG

#include <new>
#include <squirrel.h>
#include <sqstdpackage.h>
#include <sqstdaux.h>
#include <sqstdio.h>
#include <sqstdstreamio.h>
#include <sqstdblob.h>
#include "sqstdtextrw.h"

#define DEFAULT_BAD_CHAR_ASCII  '?'
#define DEFAULT_BAD_CHAR        0xFFFD
#define BOM_CODEPOINT           0xFEFF

#define CV_OK       0
#define CV_ILUNI    100
#define CV_ILSEQ    -100

#define CBUFF_SIZE  16

/*
#ifdef SQUNICODE
    #define cvSQChar        cvUTF16
    #define cvReadSQChar    cvReadUTF16
    #define cvWrteSQChar    cvWriteUTF16
#else // SQUNICODE
    #define cvSQChar        cvUTF8
    #define cvReadSQChar    cvReadUTF8
    #define cvWrteSQChar    cvWriteUTF8
#endif // SQUNICODE
*/

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

/*
Size of char in bytes for codepoints 0x0000 to 0xFFFF
    UTF8    UTF16   UTF32
min  1      2       4
mean 2.97   2       4

from->to = to_mean / from_min
*/


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

static uint8_t utf8_byte_masks[5] = {0,0,0x1f,0x0f,0x07};
static uint32_t utf8_min_codept[5] = {0,0,0x80,0x800,0x10000};

static int cvReadUTF8( uint32_t *pwc, const uint8_t **pbuf, const uint8_t* const buf_end)
{
    const uint8_t *buf = *pbuf;
    while(1) {
        uint8_t c;
        if( buf == buf_end) { *pbuf = buf; return -1; }
        c = *buf;
        if( c < 0x80) { *pbuf = buf+1; *pwc = c; return CV_OK; }
        else if( c < 0xC0) { *pbuf = buf+1; return CV_ILSEQ; }
        else {
            SQInteger codelen = utf8_lengths[ (c >> 4) - 8];
            if( !codelen) { *pbuf = buf+1; return CV_ILSEQ; }
            if( (buf_end - buf) < codelen) { *pbuf = buf; return (buf_end - buf) - codelen; } // need more bytes
            SQInteger n = codelen;
            buf++;
            uint32_t wc = c & utf8_byte_masks[codelen];
            while( --n) {
                c = *buf++;
                if( (c & 0xC0) != 0x80) { *pbuf = buf; return CV_ILSEQ; }
                wc <<= 6; wc |= c & 0x3F;
            }
            if( wc < utf8_min_codept[codelen]) { *pbuf = buf; return CV_ILSEQ; }
            if( wc != BOM_CODEPOINT) {
                *pbuf = buf;
                *pwc = wc;
                return CV_OK;
            }
        }
    }
}

static int cvWriteUTF8( uint32_t wc, uint8_t **pbuf, const uint8_t* const buf_end)
{
    uint8_t *buf = *pbuf;
    if( wc < 0xD800 || (wc >= 0xE000 && wc != 0xFFFE && wc < 0x110000)) {
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

static int cvReadUTF16( uint32_t *pwc, const uint8_t **pbuf, const uint8_t* const buf_end)
{
    const uint8_t *buf = *pbuf;
    while(1) {
        uint16_t c;
        if( (buf_end-buf) < (int)sizeof(uint16_t)) { *pbuf = buf; return -(sizeof(uint16_t) - (buf_end-buf)); }
        c = *(const uint16_t*)buf;
        if( c == BOM_CODEPOINT)		{ buf += sizeof(uint16_t); }
        else if( c >= 0xD800 && c < 0xDC00) {
            uint32_t wc = c;
            if( (buf_end-buf) < 2*(int)sizeof(uint16_t)) { *pbuf = buf; return -(2*sizeof(uint16_t) - (buf_end-buf)); }
            buf += sizeof(uint16_t);
            c = *(const uint16_t*)buf;
            buf += sizeof(uint16_t);
            if( c >= 0xDC00 && c < 0xE000) {
                wc = 0x10000 + ((wc - 0xD800) << 10) + (c - 0xDC00);
                *pbuf = buf;
                *pwc = wc;
                return CV_OK;
            }
            else { *pbuf = buf; return CV_ILSEQ; }
        }
        else if( (c >= 0xDC00 && c < 0xE000) || (c == 0xFFFE)) { *pbuf = buf+sizeof(uint16_t); return CV_ILSEQ; }
        else { *pbuf = buf+sizeof(uint16_t); *pwc = c; return SQ_OK; }
    }
}

static int cvWriteUTF16( uint32_t wc, uint8_t **pbuf, const uint8_t* const buf_end)
{
    uint8_t *buf = *pbuf;
    if( wc < 0xD800 || (wc >= 0xE000 && wc != 0xFFFE && wc < 0x110000)) {
        if( wc >= 0x10000) {
            if( (buf_end - buf) < 2*(int)sizeof(uint16_t)) return 2*sizeof(uint16_t) - (buf_end - buf);
            wc -= 0x10000;
            ((uint16_t*)buf)[0] = 0xD800 | ((wc >> 10) & 0x03FF);
            ((uint16_t*)buf)[1] = 0xDC00 | (wc & 0x03FF);
            *pbuf = buf + 2*sizeof(uint16_t);
            return CV_OK;
        }
        else {
            if( (buf_end - buf) < (int)sizeof(uint16_t)) return sizeof(uint16_t) - (buf_end - buf);
            *(uint16_t*)buf = wc;
            *pbuf = buf + sizeof(uint16_t);
            return CV_OK;
        }
    }
    return CV_ILUNI;
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

static uint16_t CV_SWAP_U16( const uint16_t val)
{
    return ((val & 0xFF) << 8) |
            ((val >> 8) & 0xFF);
}

static int cvReadUTF16REV( uint32_t *pwc, const uint8_t **pbuf, const uint8_t* const buf_end)
{
    const uint8_t *buf = *pbuf;
    while(1) {
        uint16_t c;
        if( (buf_end-buf) < (int)sizeof(uint16_t)) { *pbuf = buf; return -(sizeof(uint16_t) - (buf_end-buf)); }
        c = *(const uint16_t*)buf;
        c = CV_SWAP_U16(c);
        if( c == BOM_CODEPOINT)		{ buf += sizeof(uint16_t); }
        else if( c >= 0xD800 && c < 0xDC00) {
            uint32_t wc = c;
            if( (buf_end-buf) < 2*(int)sizeof(uint16_t)) { *pbuf = buf; return -(2*sizeof(uint16_t) - (buf_end-buf)); }
            buf += sizeof(uint16_t);
            c = *(const uint16_t*)buf;
            c = CV_SWAP_U16( c);
            buf += sizeof(uint16_t);
            if( c >= 0xDC00 && c < 0xE000) {
                wc = 0x10000 + ((wc - 0xD800) << 10) + (c - 0xDC00);
                *pbuf = buf;
                *pwc = wc;
                return CV_OK;
            }
            else { *pbuf = buf; return CV_ILSEQ; }
        }
        else if( (c >= 0xDC00 && c < 0xE000) || (c == 0xFFFE)) { *pbuf = buf+sizeof(uint16_t); return CV_ILSEQ; }
        else { *pbuf = buf+sizeof(uint16_t); *pwc = c; return SQ_OK; }
    }
}

static int cvWriteUTF16REV( uint32_t wc, uint8_t **pbuf, const uint8_t* const buf_end)
{
    uint8_t *buf = *pbuf;
    if( wc < 0xD800 || (wc >= 0xE000 && wc != 0xFFFE && wc < 0x110000)) {
        uint16_t c;
        if( wc >= 0x10000) {
            if( (buf_end - buf) < 2*(int)sizeof(uint16_t)) return 2*sizeof(uint16_t) - (buf_end - buf);
            wc -= 0x10000;
            c = 0xD800 | ((wc >> 10) & 0x03FF);
            ((uint16_t*)buf)[0] = CV_SWAP_U16(c);
            c = 0xDC00 | (wc & 0x03FF);
            ((uint16_t*)buf)[1] = CV_SWAP_U16(c);
            *pbuf = buf + 2*sizeof(uint16_t);
            return CV_OK;
        }
        else {
            if( (buf_end - buf) < (int)sizeof(uint16_t)) return sizeof(uint16_t) - (buf_end - buf);
            c = (uint16_t)wc;
            *(uint16_t*)buf = CV_SWAP_U16(c);
            *pbuf = buf + sizeof(uint16_t);
            return CV_OK;
        }
    }
    return CV_ILUNI;
}

/* ================
    UTF-32
================ */

static int cvReadUTF32( uint32_t *pwc, const uint8_t **pbuf, const uint8_t* const buf_end)
{
    const uint8_t *buf = *pbuf;
    while(1) {
        uint32_t wc;
        if( (buf_end-buf) < (int)sizeof(uint32_t)) { *pbuf = buf; return -(sizeof(uint32_t) - (buf_end-buf)); }
        wc = *(const uint32_t*)buf;
        buf += sizeof(uint32_t);
        if( wc < 0xD800 || (wc >= 0xE000 && wc != 0xFFFE && wc < 0x110000)) {
            *pbuf = buf;
            *pwc = wc;
            return CV_OK;
        }
        else {
            *pbuf = buf;
            return CV_ILSEQ;
        }
    }
}

static int cvWriteUTF32( uint32_t wc, uint8_t **pbuf, const uint8_t* const buf_end)
{
    uint8_t *buf = *pbuf;
    if( wc < 0xD800 || (wc >= 0xE000 && wc != 0xFFFE && wc < 0x110000)) {
        if( (buf_end - buf) < (int)sizeof(uint32_t)) return sizeof(uint32_t) - (buf_end - buf);
        *(uint32_t*)buf = wc;
        *pbuf = buf + sizeof(uint32_t);
        return CV_OK;
    }
    return CV_ILUNI;
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

static uint32_t CV_SWAP_U32( const uint32_t val)
{
    return ((uint32_t)CV_SWAP_U16( val & 0xFFFF) << 16) |
            CV_SWAP_U16( val >> 16);
}

static int cvReadUTF32REV( uint32_t *pwc, const uint8_t **pbuf, const uint8_t* const buf_end)
{
    const uint8_t *buf = *pbuf;
    while(1) {
        uint32_t wc;
        if( (buf_end-buf) < (int)sizeof(uint32_t)) { *pbuf = buf; return -(sizeof(uint32_t) - (buf_end-buf)); }
        wc = *(const uint32_t*)buf;
        wc = CV_SWAP_U32( wc);
        buf += sizeof(uint32_t);
        if( wc < 0xD800 || (wc >= 0xE000 && wc != 0xFFFE && wc < 0x110000)) {
            *pbuf = buf;
            *pwc = wc;
            return CV_OK;
        }
        else {
            *pbuf = buf;
            return CV_ILSEQ;
        }
    }
}

static int cvWriteUTF32REV( uint32_t wc, uint8_t **pbuf, const uint8_t* const buf_end)
{
    uint8_t *buf = *pbuf;
    if( wc < 0xD800 || (wc >= 0xE000 && wc != 0xFFFE && wc < 0x110000)) {
        if( (buf_end - buf) < (int)sizeof(uint32_t)) return sizeof(uint32_t) - (buf_end - buf);
        wc = CV_SWAP_U32( wc);
        *(uint32_t*)buf = wc;
        *pbuf = buf + sizeof(uint32_t);
        return CV_OK;
    }
    return CV_ILUNI;
}

/* ================
 all conversions
================ */

static const cv_t cvASCII = { cvReadASCII, cvWriteASCII, cvStrSizeUTF8, DEFAULT_BAD_CHAR_ASCII, 1, 1 };
static const cv_t cvUTF8 = { cvReadUTF8, cvWriteUTF8, cvStrSizeUTF8, DEFAULT_BAD_CHAR, 1, 3 };
static const cv_t cvUTF16 = { cvReadUTF16, cvWriteUTF16, cvStrSizeUTF16, DEFAULT_BAD_CHAR, 2, 2 };
static const cv_t cvUTF16REV = { cvReadUTF16REV, cvWriteUTF16REV, cvStrSizeUTF16, DEFAULT_BAD_CHAR, 2, 2 };
static const cv_t cvUTF32 = { cvReadUTF32, cvWriteUTF32, cvStrSizeUTF32, DEFAULT_BAD_CHAR, 4, 4 };
static const cv_t cvUTF32REV = { cvReadUTF32REV, cvWriteUTF32REV, cvStrSizeUTF32, DEFAULT_BAD_CHAR, 4, 4 };

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
static const SQChar *__NAMES_UTF_16[] = { _SC("UTF-16"), _SC("UCS-2"), NULL };
static const SQChar *__NAMES_UTF_16BE[] = { _SC("UTF-16BE"), _SC("UCS-2BE"), NULL };
static const SQChar *__NAMES_UTF_16LE[] = { _SC("UTF-16LE"), _SC("UCS-2LE"), NULL };
static const SQChar *__NAMES_UTF_32[] = { _SC("UTF-32"), _SC("UCS-4"), NULL };
static const SQChar *__NAMES_UTF_32BE[] = { _SC("UTF-32BE"), _SC("UCS-4BE"), NULL };
static const SQChar *__NAMES_UTF_32LE[] = { _SC("UTF-32LE"), _SC("UCS-4LE"), NULL };

static const struct enc_names
{
    int enc_id;
    const SQChar **names;
} encodings_list[] = {
	{ SQTEXTENC_UTF8, __NAMES_UTF_8 },
	{ SQTEXTENC_UTF16, __NAMES_UTF_16 },
	{ SQTEXTENC_UTF16BE, __NAMES_UTF_16BE },
	{ SQTEXTENC_UTF16LE, __NAMES_UTF_16LE },
	{ SQTEXTENC_UTF32, __NAMES_UTF_32 },
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

SQInteger sqstd_textencbyname( const SQChar *name)
{
    const struct enc_names *enc = encodings_list;
    while( enc->names) {
		const SQChar **pname = enc->names;
		while( *pname) {
			if( compare_encoding_name( *pname, name) == 0) {
				return enc->enc_id;
			}
			pname++;
		}
        enc++;
    }
    return -1;
}

static const cv_t *cv_by_id( int enc_id)
{
    const uint16_t tmp = 1;
    switch( enc_id)
    {
        case SQTEXTENC_UTF8:
            return &cvUTF8;

        case SQTEXTENC_UTF16:
            return &cvUTF16;
        case SQTEXTENC_UTF16BE:
            if( *(const uint8_t*)&tmp == 1)
                // localy little endian
                return &cvUTF16REV;
            else
                return &cvUTF16;
        case SQTEXTENC_UTF16LE:
            if( *(const uint8_t*)&tmp == 1)
                return &cvUTF16;
            else
                return &cvUTF16REV;

        case SQTEXTENC_UTF32:
            return &cvUTF32;
        case SQTEXTENC_UTF32BE:
            if( *(const uint8_t*)&tmp == 1)
                return &cvUTF32REV;
            else
                return &cvUTF32;
        case SQTEXTENC_UTF32LE:
            if( *(const uint8_t*)&tmp == 1)
                return &cvUTF32;
            else
                return &cvUTF32REV;

        case SQTEXTENC_ASCII:
            return &cvASCII;

        default:
            return NULL;
    }
}

/* ====================================
		Check char
==================================== */

static int cvCheckChar( const SQChar *str, SQInteger len, cvWriteFct write, uint32_t *pwc)
{
    const uint8_t *rdbuf = (const uint8_t*)str;
    uint32_t wc;
    int r;

    if( len == -1) {
        len = scstrlen( str);
    }

    r = cvSQChar().read( &wc, &rdbuf, (const uint8_t*)(str + len));
    if( r == CV_OK) {
        uint8_t tmp[8];
        uint8_t *wrbuf = tmp;
        r = write( wc, &wrbuf, tmp+sizeof(tmp));
    }
    if( r == CV_OK) {
        *pwc = wc;
    }
    return r;
}

static int cvCheckBadChar( const SQChar *str, SQInteger len, const cv_t &cv, uint32_t *bad_char)
{
    if( str != 0) {
        if( *str != _SC('\0')) {
            return cvCheckChar( str, len, cv.write, bad_char);
        }
        else {
            *bad_char = 0;
        }
    }
    else {
        *bad_char = cv.bad_char;
    }
    return CV_OK;
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
//        _output(0),
//        _out_alloc(0)
    {}

    int _errors;

    SQInteger Convert( const uint8_t *inbuf, const uint8_t* const inbuf_end,
                 uint8_t **poutbuf, uint8_t **poutbuf_end, size_t *poutbuf_alloc)
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
        *poutbuf_end = _output + out_len;
        *poutbuf = _output;
        *poutbuf_alloc = _out_alloc;
        return ((r == CV_OK) && !_errors) ? SQ_OK : SQ_ERROR;
    }
};

/* ------------------------------------
    SQChar to SQChar
------------------------------------ */

static SQInteger sqstd_SQChar_to_SQChar( const SQChar *str, SQInteger str_len, const SQChar **pout, SQUnsignedInteger *pout_alloc, SQInteger *pout_size)
{
    if( str_len != -1) {
        if( str[str_len] != _SC('\0')) {
            size_t out_size = str_len * sizeof(SQChar);
            *pout_alloc = out_size + sizeof(SQChar);
            SQChar *out = (SQChar*)sq_malloc( *pout_alloc);
            memcpy( out, str, out_size);
            out[str_len] = _SC('\0');
            *pout = out;
            if( pout_size) *pout_size = out_size;
            return SQ_OK;
        }
    }
    else {
        str_len = scstrlen( str);
    }
    *pout = str;
    *pout_alloc = 0;
    if( pout_size) *pout_size = str_len;
    return SQ_OK;
}

static SQInteger sqstd_UTF16_to_UTF16REV( const uint16_t *inbuf, SQInteger inbuf_size, const uint16_t **pout, SQUnsignedInteger *pout_alloc, SQInteger *pout_size)
{
    if( inbuf_size == -1) {
        inbuf_size = cvStrSizeUTF16( (const uint8_t*)inbuf);
    }
    size_t out_size = inbuf_size;
    *pout_alloc = out_size + sizeof(uint16_t);
    uint16_t *out = (uint16_t*)sq_malloc( *pout_alloc);
    *pout = out;
    while( out_size >= sizeof(uint16_t)) {
        uint16_t v = *inbuf++;
        *out++ = CV_SWAP_U16( v);
        out_size -= sizeof(uint16_t);
    }
    if( pout_size) *pout_size = out - *pout;
    return SQ_OK;
}

static SQInteger sqstd_UTF32_to_UTF32REV( const uint32_t *inbuf, SQInteger inbuf_size, const uint32_t **pout, SQUnsignedInteger *pout_alloc, SQInteger *pout_size)
{
    if( inbuf_size == -1) {
        inbuf_size = cvStrSizeUTF32( (const uint8_t*)inbuf);
    }
    size_t out_size = inbuf_size;
    *pout_alloc = out_size + sizeof(uint32_t);
    uint32_t *out = (uint32_t*)sq_malloc( *pout_alloc);
    *pout = out;
    while( out_size >= sizeof(uint32_t)) {
        uint32_t v = *inbuf++;
        *out++ = CV_SWAP_U16( v);
        out_size -= sizeof(uint32_t);
    }
    if( pout_size) *pout_size = out - *pout;
    return SQ_OK;
}

void sqstd_textrelease( const void *text, SQUnsignedInteger text_alloc_size)
{
    if( text_alloc_size != 0) {
        void *nctext = const_cast<void*>(text);
        sq_free( nctext, text_alloc_size);
    }
}

/* ------------------------------------
    UTFx to SQChar
------------------------------------ */

class cvTextToQSChar : protected cvTextAlloc
{
    cvTextToQSChar();
public:
    cvTextToQSChar( const cv_tag &from) :
        cvTextAlloc( from, cvSQChar())
    {}
    SQInteger Convert( const uint8_t *inbuf, const uint8_t* const inbuf_end,
                 SQChar **pstr, SQUnsignedInteger *palloc, SQInteger *plen)
    {
        SQChar *outbuf_end;
        size_t  outbuf_alloc;
        int r = cvTextAlloc::Convert( inbuf, inbuf_end, (uint8_t**)pstr, (uint8_t**)&outbuf_end, &outbuf_alloc);
        if( plen) *plen = outbuf_end - *pstr;
        *palloc = outbuf_alloc;
        return r;
    }
};

SQInteger sqstd_ASCII_to_SQChar( const uint8_t *inbuf, SQInteger inbuf_size,
                 const SQChar **pstr, SQUnsignedInteger *palloc, SQInteger *plen)
{
    if( inbuf_size == -1) {
        inbuf_size = cvStrSizeUTF8( inbuf);
    }
    cvTextToQSChar cv( cvASCII);
    SQChar *str;
    int r = cv.Convert( inbuf, inbuf + inbuf_size, &str, palloc, plen);
    *pstr = str;
    return r;
}

SQInteger sqstd_UTF8_to_SQChar( const uint8_t *inbuf, SQInteger inbuf_size,
                 const SQChar **pstr, SQUnsignedInteger *palloc, SQInteger *plen)
{
    if( inbuf_size == -1) {
        inbuf_size = cvStrSizeUTF8( inbuf);
    }
    if( sizeof(SQChar) == sizeof(uint8_t)) {
        return sqstd_SQChar_to_SQChar( (const SQChar*)inbuf, inbuf_size, pstr, palloc, plen);
    }
    else {
        cvTextToQSChar cv( cvUTF8);
        SQChar *str;
        int r = cv.Convert( inbuf, inbuf + inbuf_size, &str, palloc, plen);
        *pstr = str;
        return r;
    }
}

SQInteger sqstd_UTF16_to_SQChar( const uint16_t *inbuf, SQInteger inbuf_size,
                 const SQChar **pstr, SQUnsignedInteger *palloc, SQInteger *plen)
{
    if( inbuf_size == -1) {
        inbuf_size = cvStrSizeUTF16( (const uint8_t*)inbuf);
    }
    if( sizeof(SQChar) == sizeof(uint16_t)) {
        return sqstd_SQChar_to_SQChar( (const SQChar*)inbuf, inbuf_size, pstr, palloc, plen);
    }
    else {
        cvTextToQSChar cv( cvUTF16);
        SQChar *str;
        int r = cv.Convert( (const uint8_t*)inbuf, (const uint8_t*)inbuf + inbuf_size, &str, palloc, plen);
        *pstr = str;
        return r;
    }
}

static SQInteger sqstd_UTF16REV_to_SQChar( const uint16_t *inbuf, SQInteger inbuf_size,
                 const SQChar **pstr, SQUnsignedInteger *palloc, SQInteger *plen)
{
    if( inbuf_size == -1) {
        inbuf_size = cvStrSizeUTF16( (const uint8_t*)inbuf);
    }
    if( sizeof(SQChar) == sizeof(uint16_t)) {
        return sqstd_UTF16_to_UTF16REV( inbuf, inbuf_size, (const uint16_t**)pstr, palloc, plen);
    }
    else {
        cvTextToQSChar cv( cvUTF16REV);
        SQChar *str;
        int r = cv.Convert( (const uint8_t*)inbuf, (const uint8_t*)inbuf + inbuf_size, &str, palloc, plen);
        *pstr = str;
        return r;
    }
}

static SQInteger sqstd_UTF16LE_to_SQChar( const uint16_t *inbuf, SQInteger inbuf_size,
                 const SQChar **pstr, SQUnsignedInteger *palloc, SQInteger *plen)
{
    const uint16_t tmp = 1;
    if( *(const uint8_t*)&tmp == 1) {
        // localy little endian
        return sqstd_UTF16_to_SQChar( inbuf, inbuf_size, pstr, palloc, plen);
    }
    else {
        // localy big endian
        return sqstd_UTF16REV_to_SQChar( inbuf, inbuf_size, pstr, palloc, plen);
    }
}

static SQInteger sqstd_UTF16BE_to_SQChar( const uint16_t *inbuf, SQInteger inbuf_size,
                 const SQChar **pstr, SQUnsignedInteger *palloc, SQInteger *plen)
{
    const uint16_t tmp = 1;
    if( *(const uint8_t*)&tmp == 1) {
        // localy little endian
        return sqstd_UTF16REV_to_SQChar( inbuf, inbuf_size, pstr, palloc, plen);
    }
    else {
        // localy big endian
        return sqstd_UTF16_to_SQChar( inbuf, inbuf_size, pstr, palloc, plen);
    }
}

SQInteger sqstd_UTF32_to_SQChar( const uint32_t *inbuf, SQInteger inbuf_size,
                 const SQChar **pstr, SQUnsignedInteger *palloc, SQInteger *plen)
{
    if( inbuf_size == -1) {
        inbuf_size = cvStrSizeUTF32( (const uint8_t*)inbuf);
    }
    if( sizeof(SQChar) == sizeof(uint32_t)) {
        return sqstd_SQChar_to_SQChar( (const SQChar*)inbuf, inbuf_size, pstr, palloc, plen);
    }
    else {
        cvTextToQSChar cv( cvUTF32);
        SQChar *str;
        int r = cv.Convert( (const uint8_t*)inbuf, (const uint8_t*)inbuf + inbuf_size, &str, palloc, plen);
        *pstr = str;
        return r;
    }
}

static SQInteger sqstd_UTF32REV_to_SQChar( const uint32_t *inbuf, SQInteger inbuf_size,
                 const SQChar **pstr, SQUnsignedInteger *palloc, SQInteger *plen)
{
    if( inbuf_size == -1) {
        inbuf_size = cvStrSizeUTF32( (const uint8_t*)inbuf);
    }
    if( sizeof(SQChar) == sizeof(uint32_t)) {
        return sqstd_UTF32_to_UTF32REV( inbuf, inbuf_size, (const uint32_t**)pstr, palloc, plen);
    }
    else {
        cvTextToQSChar cv( cvUTF32REV);
        SQChar *str;
        int r = cv.Convert( (const uint8_t*)inbuf, (const uint8_t*)inbuf + inbuf_size, &str, palloc, plen);
        *pstr = str;
        return r;
    }
}

static SQInteger sqstd_UTF32LE_to_SQChar( const uint32_t *inbuf, SQInteger inbuf_size,
                 const SQChar **pstr, SQUnsignedInteger *palloc, SQInteger *plen)
{
    const uint16_t tmp = 1;
    if( *(const uint8_t*)&tmp == 1) {
        // localy little endian
        return sqstd_UTF32_to_SQChar( inbuf, inbuf_size, pstr, palloc, plen);
    }
    else {
        // localy big endian
        return sqstd_UTF32REV_to_SQChar( inbuf, inbuf_size, pstr, palloc, plen);
    }
}

static SQInteger sqstd_UTF32BE_to_SQChar( const uint32_t *inbuf, SQInteger inbuf_size,
                 const SQChar **pstr, SQUnsignedInteger *palloc, SQInteger *plen)
{
    const uint16_t tmp = 1;
    if( *(const uint8_t*)&tmp == 1) {
        // localy little endian
        return sqstd_UTF32REV_to_SQChar( inbuf, inbuf_size, pstr, palloc, plen);
    }
    else {
        // localy big endian
        return sqstd_UTF32_to_SQChar( inbuf, inbuf_size, pstr, palloc, plen);
    }
}

SQInteger sqstd_UTF_to_SQChar( int encoding, const void *inbuf, SQInteger inbuf_size,
                 const SQChar **pstr, SQUnsignedInteger *palloc, SQInteger *plen)
{
    switch( encoding)
    {
        case SQTEXTENC_UTF8:
            return sqstd_UTF8_to_SQChar( (const uint8_t*)inbuf, inbuf_size, pstr, palloc, plen);

        case SQTEXTENC_UTF16:
            return sqstd_UTF16_to_SQChar( (const uint16_t*)inbuf, inbuf_size, pstr, palloc, plen);
        case SQTEXTENC_UTF16BE:
            return sqstd_UTF16BE_to_SQChar( (const uint16_t*)inbuf, inbuf_size, pstr, palloc, plen);
        case SQTEXTENC_UTF16LE:
            return sqstd_UTF16LE_to_SQChar( (const uint16_t*)inbuf, inbuf_size, pstr, palloc, plen);

        case SQTEXTENC_UTF32:
            return sqstd_UTF32_to_SQChar( (const uint32_t*)inbuf, inbuf_size, pstr, palloc, plen);
        case SQTEXTENC_UTF32BE:
            return sqstd_UTF32BE_to_SQChar( (const uint32_t*)inbuf, inbuf_size, pstr, palloc, plen);
        case SQTEXTENC_UTF32LE:
            return sqstd_UTF32LE_to_SQChar( (const uint32_t*)inbuf, inbuf_size, pstr, palloc, plen);

        case SQTEXTENC_ASCII:
            return sqstd_ASCII_to_SQChar( (const uint8_t*)inbuf, inbuf_size, pstr, palloc, plen);

        default:
            return -1; // !!!
    }
}

/* ------------------------------------
    SQChar to UTFx
------------------------------------ */

class cvTextFromQSChar : protected cvTextAlloc
{
    cvTextFromQSChar();
public:
    cvTextFromQSChar( const cv_tag &to) :
        cvTextAlloc( cvSQChar(), to)
    {}
    SQInteger Convert( const SQChar *str, SQInteger str_len,
                 uint8_t **pout, SQInteger *pout_size, SQUnsignedInteger *pout_alloc)
    {
        uint8_t *outbuf_end;
        size_t  outbuf_alloc;
        int r;
        if( str_len == -1) {
            str_len = scstrlen( str);
        }
        r = cvTextAlloc::Convert( (const uint8_t*)str, (const uint8_t*)(str+str_len), pout, &outbuf_end, &outbuf_alloc);
        if( pout_size) *pout_size = outbuf_end - *pout;
        *pout_alloc = outbuf_alloc;
        return r;
    }
};

static SQInteger sqstd_SQChar_to_ASCII( const SQChar *str, SQInteger str_len, const uint8_t **pout, SQUnsignedInteger *pout_alloc, SQInteger *pout_size)
{
    cvTextFromQSChar cv( cvASCII);
    uint8_t *out;
    int r = cv.Convert( str, str_len, &out, pout_size, pout_alloc);
    *pout = out;
    return r;
}

SQInteger sqstd_SQChar_to_UTF8( const SQChar *str, SQInteger str_len, const uint8_t **pout, SQUnsignedInteger *pout_alloc, SQInteger *pout_size)
{
    if( sizeof(SQChar) == sizeof(uint8_t)) {
        return sqstd_SQChar_to_SQChar( str, str_len, (const SQChar**)pout, pout_alloc, pout_size);
    }
    else {
        cvTextFromQSChar cv( cvUTF8);
        uint8_t *out;
        int r = cv.Convert( str, str_len, &out, pout_size, pout_alloc);
        *pout = out;
        return r;
    }
}

SQInteger sqstd_SQChar_to_UTF16( const SQChar *str, SQInteger str_len, const uint16_t **pout, SQUnsignedInteger *pout_alloc, SQInteger *pout_size)
{
    if( sizeof(SQChar) == sizeof(uint16_t)) {
        return sqstd_SQChar_to_SQChar( str, str_len, (const SQChar**)pout, pout_alloc, pout_size);
    }
    else {
        cvTextFromQSChar cv( cvUTF16);
        uint16_t *out;
        SQInteger r = cv.Convert( str, str_len, (uint8_t**)&out, pout_size, pout_alloc);
        *pout = out;
        return r;
    }
}

static SQInteger sqstd_SQChar_to_UTF16REV( const SQChar *str, SQInteger str_len, const uint16_t **pout, SQUnsignedInteger *pout_alloc, SQInteger *pout_size)
{
    if( sizeof(SQChar) == sizeof(uint16_t)) {
        return sqstd_UTF16_to_UTF16REV( (const uint16_t*)str, str_len, pout, pout_alloc, pout_size);
    }
    else {
        cvTextFromQSChar cv( cvUTF16REV);
        uint16_t *out;
        SQInteger r = cv.Convert( str, str_len, (uint8_t**)&out, pout_size, pout_alloc);
        *pout = out;
        return r;
    }
}

static SQInteger sqstd_SQChar_to_UTF16LE( const SQChar *str, SQInteger str_len, const uint16_t **pout, SQUnsignedInteger *pout_alloc, SQInteger *pout_size)
{
    const uint16_t tmp = 1;
    if( *(const uint8_t*)&tmp == 1) {
        // localy little endian
        return sqstd_SQChar_to_UTF16( str, str_len, pout, pout_alloc, pout_size);
    }
    else {
        // localy big endian
        return sqstd_SQChar_to_UTF16REV( str, str_len, pout, pout_alloc, pout_size);
    }
}

static SQInteger sqstd_SQChar_to_UTF16BE( const SQChar *str, SQInteger str_len, const uint16_t **pout, SQUnsignedInteger *pout_alloc, SQInteger *pout_size)
{
    const uint16_t tmp = 1;
    if( *(const uint8_t*)&tmp == 1) {
        // localy little endian
        return sqstd_SQChar_to_UTF16REV( str, str_len, pout, pout_alloc, pout_size);
    }
    else {
        // localy big endian
        return sqstd_SQChar_to_UTF16( str, str_len, pout, pout_alloc, pout_size);
    }
}

SQInteger sqstd_SQChar_to_UTF32( const SQChar *str, SQInteger str_len, const uint32_t **pout, SQUnsignedInteger *pout_alloc, SQInteger *pout_size)
{
    if( sizeof(SQChar) == sizeof(uint32_t)) {
        return sqstd_SQChar_to_SQChar( str, str_len, (const SQChar**)pout, pout_alloc, pout_size);
    }
    else {
        cvTextFromQSChar cv( cvUTF32);
        uint32_t *out;
        SQInteger r = cv.Convert( str, str_len, (uint8_t**)&out, pout_size, pout_alloc);
        *pout = out;
        return r;
    }
}

static SQInteger sqstd_SQChar_to_UTF32REV( const SQChar *str, SQInteger str_len, const uint32_t **pout, SQUnsignedInteger *pout_alloc, SQInteger *pout_size)
{
    if( sizeof(SQChar) == sizeof(uint32_t)) {
        return sqstd_UTF32_to_UTF32REV( (const uint32_t*)str, str_len, pout, pout_alloc, pout_size);
    }
    else {
        cvTextFromQSChar cv( cvUTF32REV);
        uint32_t *out;
        SQInteger r = cv.Convert( str, str_len, (uint8_t**)&out, pout_size, pout_alloc);
        *pout = out;
        return r;
    }
}

static SQInteger sqstd_SQChar_to_UTF32LE( const SQChar *str, SQInteger str_len, const uint32_t **pout, SQUnsignedInteger *pout_alloc, SQInteger *pout_size)
{
    const uint16_t tmp = 1;
    if( *(const uint8_t*)&tmp == 1) {
        // localy little endian
        return sqstd_SQChar_to_UTF32( str, str_len, pout, pout_alloc, pout_size);
    }
    else {
        // localy big endian
        return sqstd_SQChar_to_UTF32REV( str, str_len, pout, pout_alloc, pout_size);
    }
}

static SQInteger sqstd_SQChar_to_UTF32BE( const SQChar *str, SQInteger str_len, const uint32_t **pout, SQUnsignedInteger *pout_alloc, SQInteger *pout_size)
{
    const uint16_t tmp = 1;
    if( *(const uint8_t*)&tmp == 1) {
        // localy little endian
        return sqstd_SQChar_to_UTF32REV( str, str_len, pout, pout_alloc, pout_size);
    }
    else {
        // localy big endian
        return sqstd_SQChar_to_UTF32( str, str_len, pout, pout_alloc, pout_size);
    }
}

SQInteger sqstd_SQChar_to_UTF( int encoding, const SQChar *str, SQInteger str_len, const void **pout, SQUnsignedInteger *pout_alloc, SQInteger *pout_size)
{
    switch( encoding)
    {
        case SQTEXTENC_UTF8:
            return sqstd_SQChar_to_UTF8( str, str_len, (const uint8_t**)pout, pout_alloc, pout_size);

        case SQTEXTENC_UTF16:
            return sqstd_SQChar_to_UTF16( str, str_len, (const uint16_t**)pout, pout_alloc, pout_size);
        case SQTEXTENC_UTF16BE:
            return sqstd_SQChar_to_UTF16BE( str, str_len, (const uint16_t**)pout, pout_alloc, pout_size);
        case SQTEXTENC_UTF16LE:
            return sqstd_SQChar_to_UTF16LE( str, str_len, (const uint16_t**)pout, pout_alloc, pout_size);

        case SQTEXTENC_UTF32:
            return sqstd_SQChar_to_UTF32( str, str_len, (const uint32_t**)pout, pout_alloc, pout_size);
        case SQTEXTENC_UTF32BE:
            return sqstd_SQChar_to_UTF32BE( str, str_len, (const uint32_t**)pout, pout_alloc, pout_size);
        case SQTEXTENC_UTF32LE:
            return sqstd_SQChar_to_UTF32LE( str, str_len, (const uint32_t**)pout, pout_alloc, pout_size);

        case SQTEXTENC_ASCII:
            return sqstd_SQChar_to_ASCII( str, str_len, (const uint8_t**)pout, pout_alloc, pout_size);

        default:
            return -1;  // !!!
    }
}


/* ====================================
		Conversions
==================================== */

// stack --> UTF_X
SQInteger sqstd_get_UTF8(HSQUIRRELVM v, SQInteger idx, const uint8_t **pout, SQUnsignedInteger *pout_alloc, SQInteger *pout_size)
{
    const SQChar *str;
    SQInteger str_len;
    sq_getstringandsize(v,idx,&str,&str_len);
    sqstd_SQChar_to_UTF8( str, str_len, pout, pout_alloc, pout_size);
    return 0;
}

SQInteger sqstd_get_UTF16(HSQUIRRELVM v, SQInteger idx, const uint16_t **pout, SQUnsignedInteger *pout_alloc, SQInteger *pout_size)
{
    const SQChar *str;
    SQInteger str_len;
    sq_getstringandsize(v,idx,&str,&str_len);
    sqstd_SQChar_to_UTF16( str, str_len, pout, pout_alloc, pout_size);
    return 0;
}

SQInteger sqstd_get_UTF32(HSQUIRRELVM v, SQInteger idx, const uint32_t **pout, SQUnsignedInteger *pout_alloc, SQInteger *pout_size)
{
    const SQChar *str;
    SQInteger str_len;
    sq_getstringandsize(v,idx,&str,&str_len);
    sqstd_SQChar_to_UTF32( str, str_len, pout, pout_alloc, pout_size);
    return 0;
}

SQInteger sqstd_get_UTF(HSQUIRRELVM v, SQInteger idx, int encoding, const void **pout, SQUnsignedInteger *pout_alloc, SQInteger *pout_size)
{
    const SQChar *str;
    SQInteger str_len;
    sq_getstringandsize(v,idx,&str,&str_len);
    sqstd_SQChar_to_UTF( encoding, str, str_len, pout, pout_alloc, pout_size);
    return 0;
}


// UTF_X --> stack
SQInteger sqstd_push_UTF8(HSQUIRRELVM v, const uint8_t *inbuf, SQInteger inbuf_size)
{
    const SQChar *str;
    SQUnsignedInteger str_alloc;
    SQInteger str_len;
    SQInteger r = sqstd_UTF8_to_SQChar( inbuf, inbuf_size, &str, &str_alloc, &str_len);
    sq_pushstring(v, str, str_len);
    sqstd_textrelease( str, str_alloc);
    return r;
}

SQInteger sqstd_push_UTF16(HSQUIRRELVM v, const uint16_t *inbuf, SQInteger inbuf_size)
{
    const SQChar *str;
    SQUnsignedInteger str_alloc;
    SQInteger str_len;
    SQInteger r = sqstd_UTF16_to_SQChar( inbuf, inbuf_size, &str, &str_alloc, &str_len);
    sq_pushstring(v, str, str_len);
    sqstd_textrelease( str, str_alloc);
    return r;
}

SQInteger sqstd_push_UTF32(HSQUIRRELVM v, const uint32_t *inbuf, SQInteger inbuf_size)
{
    const SQChar *str;
    SQUnsignedInteger str_alloc;
    SQInteger str_len;
    SQInteger r = sqstd_UTF32_to_SQChar( inbuf, inbuf_size, &str, &str_alloc, &str_len);
    sq_pushstring(v, str, str_len);
    sqstd_textrelease( str, str_alloc);
    return r;
}

SQInteger sqstd_push_UTF(HSQUIRRELVM v, int encoding, const void *inbuf, SQInteger inbuf_size)
{
    const SQChar *str;
    SQUnsignedInteger str_alloc;
    SQInteger str_len;
    SQInteger r = sqstd_UTF_to_SQChar( encoding, inbuf, inbuf_size, &str, &str_alloc, &str_len);
    sq_pushstring(v, str, str_len);
    sqstd_textrelease( str, str_alloc);
    return r;
}

/* ====================================
		Text reader
==================================== */

struct SQTextReader
{
    SQTextReader( SQStream *stream, const cv_t *cv) : _stream(stream), _cv(cv),
        _bad_char(0), _buf_len(0), _buf_pos(0), _wc_buf_pos(0), _wc_buf_len(0), _errors(0)
    {
    }

    int SetBadChar( const SQChar *str, SQInteger len)
    {
        return cvCheckBadChar( str, len, cvSQChar(), &_bad_char);
    }

    int ClearErrors( void) { int r = _errors; _errors = 0; return r; }

    int ReadBOM(void)
    {
        uint8_t tmp_buf[8];
        SQInteger tmp_buf_len;
        int r = CV_OK;
        tmp_buf_len = sqstd_sread( tmp_buf, 4, (SQSTREAM)_stream);
        switch( tmp_buf_len)
        {
            case 4:
                if( *(uint32_t*)tmp_buf == 0x0000FEFFL) {
                    // UTF32
                    tmp_buf_len = 0;
                    _cv = &cvUTF32;
                }
                else if( *(uint32_t*)tmp_buf == 0xFFFE0000L) {
                    // UTF32_REV
                    tmp_buf_len = 0;
                    _cv = &cvUTF32REV;
                }
            case 3:
                if( (tmp_buf[0] == 0xEF) && (tmp_buf[1] == 0xBB) && (tmp_buf[2] == 0xBF)) {
                    // UTF8
                    tmp_buf[0] = tmp_buf[3];
                    tmp_buf_len = (tmp_buf_len > 3) ? tmp_buf_len - 3 : 0;
                    _cv = &cvUTF8;
                }
            case 2:
                if( *(uint16_t*)tmp_buf == 0xFEFF) {
                    // UTF16
                    *(uint16_t*)tmp_buf = *(uint16_t*)(tmp_buf+2);
                    tmp_buf_len = (tmp_buf_len > 2) ? tmp_buf_len - 2 : 0;
                    _cv = &cvUTF16;
                }
                else if( *(uint16_t*)tmp_buf == 0xFFFE) {
                    // UTF16_REV
                    *(uint16_t*)tmp_buf = *(uint16_t*)(tmp_buf+2);
                    tmp_buf_len = (tmp_buf_len > 2) ? tmp_buf_len - 2 : 0;
                    _cv = &cvUTF16REV;
                }
            default:
                ;
        }
        if( tmp_buf_len) {
            const uint8_t *pbuf = tmp_buf;
            while( pbuf < tmp_buf+tmp_buf_len) {
                uint32_t wc;
                r = _cv->read( &wc, &pbuf, tmp_buf+tmp_buf_len);
                if( r == CV_ILSEQ) {
                    if( _bad_char) {
                        wc = _bad_char;
                        _errors++;
                    }
                    else
                        break;
                }
                else if( r < 0) {
                    r = sqstd_sread( tmp_buf+tmp_buf_len, 1, _stream);
                    if( r == 1) {
                        tmp_buf_len++;
                        continue;
                    }
                }
                _wc_buf[_wc_buf_len++] = wc;
            }
        }
        return r;
    }

    int ReadCodepoint( uint32_t *pwc)
    {
        if( _wc_buf_pos < _wc_buf_len) {
            *pwc = _wc_buf[_wc_buf_pos++];
            return CV_OK;
        }
        else {
            uint8_t tmp[8];
            const uint8_t *rdbuf = tmp;
            size_t rdbuf_len = 0;
            uint32_t wc;
            int r;

            int step = _cv->min_char_size;
            while(1) {
                r = sqstd_sread( tmp+rdbuf_len, step, _stream);
                if( r == step) {
                    rdbuf_len += step;
                    r = _cv->read( &wc, &rdbuf, tmp+rdbuf_len);
                    if( (r == CV_OK) || (r == CV_ILSEQ)) {
                        break;
                    }
                    else {
                        step = -r;
                    }
                }
                else {
                    r = -1; // EOF
                    break;
                }
            }
            if( (r == CV_ILSEQ) && (_bad_char != 0)) {
                wc = _bad_char;
                _errors++;
                r = CV_OK;
            }
            *pwc = wc;
            return r;
        }
    }

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

    int read_next( void)
    {
        uint32_t wc;
        int r;

        r = ReadCodepoint( &wc);
        if( r == CV_OK) {
            _buf_len = 0;
            _buf_pos = 0;
            r = append_wchar( wc);
//            uint8_t *tmp = (uint8_t*)_buf;
//            r = cvSQChar().write( wc, &tmp, (uint8_t*)(_buf + CBUFF_SIZE));
//            if( (r == CV_ILUNI)  && (_bad_char != 0)) {
//                wc = _bad_char;
//                r = cvSQChar().write( wc, &tmp, (uint8_t*)(_buf + CBUFF_SIZE));
//            }
//            if( r == CV_OK) {
//                _buf_len = (tmp - (uint8_t*)_buf) / sizeof(SQChar);
//                _buf_pos = 0;
//            }
        }
        return r;
    }

    int PeekChar( SQChar *pc)
    {
        int r = CV_OK;
        if( _buf_pos >= _buf_len) {
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

    SQStream *_stream;
    const cv_t *_cv;
    uint32_t _bad_char;

	size_t _buf_len;
	size_t _buf_pos;
	SQChar _buf[CBUFF_SIZE];
    size_t _wc_buf_pos;
    size_t _wc_buf_len;
    uint32_t _wc_buf[CBUFF_SIZE];
    int _errors;
};

SQSTDTEXTRD sqstd_text_reader( SQSTREAM stream, int encoding, SQBool read_bom, const SQChar *bad_char)
{
    const cv_t *cv = cv_by_id( encoding);
    if( cv == NULL) return NULL;
	SQTextReader *rd = new (sq_malloc(sizeof(SQTextReader)))SQTextReader( (SQStream*)stream, cv);
	if( bad_char) {
        rd->SetBadChar( bad_char, -1);
	}
	if( read_bom) {
        rd->ReadBOM();
    }
    return (SQSTDTEXTRD)rd;
}

void sqstd_text_releasereader( SQSTDTEXTRD reader)
{
    SQTextReader *rd = (SQTextReader*)reader;
    rd->~SQTextReader();
    sq_free(rd,sizeof(SQTextReader));
}

SQInteger sqstd_text_readcodepoint( SQSTDTEXTRD reader, SQInteger *pwc)
{
    SQTextReader *rd = (SQTextReader*)reader;
    uint32_t wc;
    int r = rd->ReadCodepoint( &wc);
    *pwc = wc;
    return r;
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

/* ====================================
		Text Writer
==================================== */

struct SQTextWriter
{
    SQTextWriter( SQStream *stream, const cv_t *cv) : _stream(stream), _cv(cv), _buf_len(0), _bad_char(0), _errors(0)
    {
    }

    int SetBadChar( const SQChar *str, SQInteger len)
    {
        return cvCheckBadChar( str, len, *_cv, &_bad_char);
    }

    int ClearErrors( void) { int r = _errors; _errors = 0; return r; }

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
            size_t wrbuf_len = wrbuf - tmp;
            if( sqstd_swrite( tmp, wrbuf_len, _stream) != wrbuf_len) {
                r = 1;
            }
        }
        return r;
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

    int WriteString( const SQChar * const str, size_t str_len)
    {
        const uint8_t *rdbuf = (const uint8_t*)str;
        uint32_t wc;
        int r;
        if( str_len == -1) {
            str_len = scstrlen( str);
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

    SQStream *_stream;
    const cv_t *_cv;
	size_t _buf_len;
    uint32_t _bad_char;
	SQChar _buf[CBUFF_SIZE];
	int _errors;
};

typedef void *SQSTDTEXTWR;

SQSTDTEXTWR sqstd_text_writer( SQSTREAM stream, int encoding, SQBool write_bom, const SQChar *bad_char)
{
    const cv_t *cv = cv_by_id( encoding);
    if( cv == NULL) return NULL;
	SQTextWriter *wr = new (sq_malloc(sizeof(SQTextWriter)))SQTextWriter( (SQStream*)stream, cv);
	if( bad_char) {
        wr->SetBadChar( bad_char, -1);
	}
	if( write_bom && (encoding != SQTEXTENC_ASCII)) {
        wr->WriteCodepoint( BOM_CODEPOINT);
	}
    return (SQSTDTEXTWR)wr;
}

void sqstd_text_releasewriter( SQSTDTEXTWR writer)
{
    SQTextWriter *wr = (SQTextWriter*)writer;
    wr->~SQTextWriter();
    sq_free(wr,sizeof(SQTextWriter));
}

SQInteger sqstd_text_writecodepoint( SQSTDTEXTWR writer, SQInteger wc)
{
    SQTextWriter *wr = (SQTextWriter*)writer;
    return wr->WriteCodepoint( wc);
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

/* ====================================
==================================== */

/*
    function blobtostr( blob, enc)
    {
        SQUIRREL_API SQInteger sqstd_UTF_to_SQChar( int encoding, const void *inbuf, SQInteger inbuf_size,
                         const SQChar **pstr, SQUnsignedInteger *palloc, SQInteger *plen);

        void sqstd_push_UTF(HSQUIRRELVM v, int encoding, const void *inbuf, SQInteger inbuf_size)
    }

    function strtoblob( str, enc)
    {
        SQUIRREL_API SQInteger sqstd_SQChar_to_UTF( int encoding, const SQChar *str, SQInteger str_len,
                        const void **pout, SQUnsignedInteger *pout_alloc, SQInteger *pout_size);

        SQInteger sqstd_get_UTF(HSQUIRRELVM v, SQInteger idx, int encoding, const void **pout, SQUnsignedInteger *pout_alloc, SQInteger *pout_size)
    }

    class reader
    {
        constructor( stream, encoding, read_bom)

        function readChar()
        function peekChar()

        function readLine( max_chars)

        function setBadChar( char)
        function getBadChar()
    }

    class writer
    {
        constructor( stream, encoding, write_bom)

        function write( string)

        function setBadChar( char)
        function getBadChar()
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
    enc_id = sqstd_textencbyname( enc);
    if( enc_id < 0) {
        return sq_throwerror(v,_SC("Unknown encoding name"));
    }
    if( sq_gettop(v) > 3) {
        sq_getbool(v,4,&is_strict);
    }
    SQInteger r = sqstd_push_UTF(v,enc_id,ptr,size);
    if( is_strict && r) {
        return sq_throwerror(v,_SC("Text conversion failed"));
    }
    return 1;
}

static SQRESULT _textrw_strtoblob(HSQUIRRELVM v)
{
    const SQChar *enc;
    int enc_id;
    const void *out;
    SQUnsignedInteger out_alloc;
    SQInteger out_size;
    SQBool is_strict = SQFalse;
    sq_getstring(v,3,&enc);
    enc_id = sqstd_textencbyname( enc);
    if( enc_id < 0) {
        return sq_throwerror(v,_SC("Unknown encoding name"));
    }
    if( sq_gettop(v) > 3) {
        sq_getbool(v,4,&is_strict);
    }
    SQInteger r = sqstd_get_UTF(v,2,enc_id,&out,&out_alloc,&out_size);
    if( is_strict && r) {
        sqstd_textrelease(out,out_alloc);
        return sq_throwerror(v,_SC("Text conversion failed"));
    }
    else {
        SQUserPointer blob = sqstd_createblob(v,out_size);
        memcpy(blob,out,out_size);
        sqstd_textrelease(out,out_alloc);
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
    sqstd_text_releasereader(rdr);
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

    enc_id = sqstd_textencbyname( encoding);
    if( enc_id == -1) {
        return sq_throwerror(v,_SC("unknown encoding"));
    }

    SQSTDTEXTRD rdr = sqstd_text_reader( stream, enc_id, read_bom, bad_char);
    if( rdr) {
        if(SQ_FAILED(sq_setinstanceup(v,1,rdr))) {
            sqstd_text_releasereader(rdr);
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

static SQInteger _textrd_readcodepoint(HSQUIRRELVM v)
{
    SQInteger r;
    SQInteger wc;
    SETUP_TEXTRD(v)
    r = sqstd_text_readcodepoint( rdr, &wc);
    if( r == SQ_OK) {
        sq_pushinteger(v, wc);
    }
    else {
        sq_pushnull(v);
    }
    return 1;
}

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
    _DECL_TEXTRD_FUNC(readcodepoint,1,_SC("x")),
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
    sqstd_text_releasewriter(wrt);
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

    enc_id = sqstd_textencbyname( encoding);
    if( enc_id == -1) {
        return sq_throwerror(v,_SC("unknown encoding"));
    }

    SQSTDTEXTWR wrt = sqstd_text_writer( stream, enc_id, write_bom, 0);
    if( wrt) {
        if(SQ_FAILED(sq_setinstanceup(v,1,wrt))) {
            sqstd_text_releasewriter(wrt);
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

static SQInteger _textwr_writecodepoint(HSQUIRRELVM v)
{
    SQInteger r;
    SQInteger wc;
    SETUP_TEXTWR(v)
    sq_getinteger(v, 2, &wc);
    r = sqstd_text_writecodepoint( wrt, wc);
    sq_pushinteger(v,r);
    return 1;
}

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
    _DECL_TEXTWR_FUNC(writecodepoint,2,_SC("xi")),
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
