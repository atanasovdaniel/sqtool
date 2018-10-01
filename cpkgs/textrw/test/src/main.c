
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <squirrel.h>
#include <sqstdstream.h>
#include <sqstdtext.h>

#include <env.h>

#define AR(...) __VA_ARGS__

#define ALIGN_2( _x)    _x __attribute__ ((aligned (2)))
#define ALIGN_4( _x)    _x __attribute__ ((aligned (4)))

const char *env_get_enc_name( int encoding)
{
    static const char *env_enc_name[] = {
        [SQTEXTENC_UTF8] = "UTF8",
        [SQTEXTENC_UTF16BE] = "UTF-16BE",
        [SQTEXTENC_UTF16LE] = "UTF-16LE",
        [SQTEXTENC_UTF32BE] = "UTF-32BE",
        [SQTEXTENC_UTF32LE] = "UTF-32LE",
        [SQTEXTENC_ASCII] = "ASCII",
    };

    if( encoding & SQTEXTENC_NATIVE)
    {
        encoding = sqstd_text_nativeenc( encoding);
    }

    return env_enc_name[ encoding & ~SQTEXTENC_FLAGS];
}

const uint8_t str_8[] = { 'H', 'E', 'L', 'L', 'O', 0 };
const uint8_t str_8_1[] = { 'H', 'E', 'L', 'L', 0 };

const uint16_t str_N16[] = { 'H', 'E', 'L', 'L', 'O', 0 };
const uint16_t str_N16_1[] = { 'H', 'E', 'L', 'L', 0 };

const uint32_t str_N32[] = { 'H', 'E', 'L', 'L', 'O', 0 };
const uint32_t str_N32_1[] = { 'H', 'E', 'L', 'L', 0 };

const SQChar str_SQ[] = { 'H', 'E', 'L', 'L', 'O', 0 };
const SQChar str_SQ_1[] = { 'H', 'E', 'L', 'L', 0 };

ALIGN_2( const uint8_t str_16B[]) = { 0,'H', 0,'E', 0,'L', 0,'L', 0,'O', 0,0 };
ALIGN_2( const uint8_t str_16B_1[]) = { 0,'H', 0,'E', 0,'L', 0,'L', 0,0 };

ALIGN_2( const uint8_t str_16L[]) = { 'H',0, 'E',0, 'L',0, 'L',0, 'O',0, 0,0 };
ALIGN_2( const uint8_t str_16L_1[]) = { 'H',0, 'E',0, 'L',0, 'L',0, 0,0 };

ALIGN_4( const uint8_t str_32B[]) = { 0,0,0,'H', 0,0,0,'E', 0,0,0,'L', 0,0,0,'L', 0,0,0,'O', 0,0,0,0 };
ALIGN_4( const uint8_t str_32B_1[]) = { 0,0,0,'H', 0,0,0,'E', 0,0,0,'L', 0,0,0,'L', 0,0,0,0 };

ALIGN_4( const uint8_t str_32L[]) = { 'H',0,0,0, 'E',0,0,0, 'L',0,0,0, 'L',0,0,0, 'O',0,0,0, 0,0,0,0 };
ALIGN_4( const uint8_t str_32L_1[]) = { 'H',0,0,0, 'E',0,0,0, 'L',0,0,0, 'L',0,0,0, 0,0,0,0 };

const struct data_U_to_SQ_tag
{
    const void *inbuf;
    SQInteger inbuf_size;
    SQInteger char_size;
    const SQChar *answer;
    const SQChar *answer_1;
    SQInteger ans_size;
} data_U_to_SQ[] = {
#define U2SQ( _enc, _inb, _incs, _ans)     [_enc] = { _inb, sizeof(_inb), _incs, _ans, _ans##_1, sizeof(_ans)},
    U2SQ( SQTEXTENC_UTF8, str_8, 1, str_SQ)
    U2SQ( SQTEXTENC_UTF16BE, str_16B, 2, str_SQ)
    U2SQ( SQTEXTENC_UTF16LE, str_16L, 2, str_SQ)
    U2SQ( SQTEXTENC_UTF32BE, str_32B, 4, str_SQ)
    U2SQ( SQTEXTENC_UTF32LE, str_32L, 4, str_SQ)
    U2SQ( SQTEXTENC_ASCII, str_8, 1, str_SQ)
};
const struct data_U_to_SQ_tag data_U_to_SQ_N[] = {
    U2SQ( SQTEXTENC_UTF8, str_8, 1, str_SQ)
    U2SQ( SQTEXTENC_UTF16BE, str_N16, 2, str_SQ)
    U2SQ( SQTEXTENC_UTF16LE, str_N16, 2, str_SQ)
    U2SQ( SQTEXTENC_UTF32BE, str_N32, 4, str_SQ)
    U2SQ( SQTEXTENC_UTF32LE, str_N32, 4, str_SQ)
    U2SQ( SQTEXTENC_ASCII, str_8, 1, str_SQ)
};
#undef U2SQ

//sqstd_text_toutf( int encoding, const SQChar *str, SQInteger str_len,
//                const void **pout, SQUnsignedInteger *pout_alloc, SQInteger *pout_size);
const struct data_SQ_to_U_tag
{
    const SQChar *str;
    SQInteger str_len;
    SQInteger char_size;
    const void *answer;
    const void *answer_1;
    SQInteger ans_size;
} data_SQ_to_U[] = {
#define SQ2U( _enc, _str, _anscs, _ans)   [_enc] = { _str, sizeof(_str)/sizeof(SQChar)-1, _anscs, _ans, _ans##_1, sizeof(_ans)-_anscs},
    SQ2U( SQTEXTENC_UTF8, str_SQ, 1, str_8)
    SQ2U( SQTEXTENC_UTF16BE, str_SQ, 2, str_16B)
    SQ2U( SQTEXTENC_UTF16LE, str_SQ, 2, str_16L)
    SQ2U( SQTEXTENC_UTF32BE, str_SQ, 4, str_32B)
    SQ2U( SQTEXTENC_UTF32LE, str_SQ, 4, str_32L)
    SQ2U( SQTEXTENC_ASCII, str_SQ, 1, str_8)
};
const struct data_SQ_to_U_tag data_SQ_to_U_N[] = {
    SQ2U( SQTEXTENC_UTF8, str_SQ, 1, str_8)
    SQ2U( SQTEXTENC_UTF16BE, str_SQ, 2, str_N16)
    SQ2U( SQTEXTENC_UTF16LE, str_SQ, 2, str_N16)
    SQ2U( SQTEXTENC_UTF32BE, str_SQ, 4, str_N32)
    SQ2U( SQTEXTENC_UTF32LE, str_SQ, 4, str_N32)
    SQ2U( SQTEXTENC_ASCII, str_SQ, 1, str_8)
};
#undef SQ2U

static int test_sqstd_SQChar_to_UTF_len( int enc)
{
    const struct data_SQ_to_U_tag data = (enc & SQTEXTENC_NATIVE) ? data_SQ_to_U_N[enc & ~SQTEXTENC_FLAGS] : data_SQ_to_U[enc & ~SQTEXTENC_FLAGS];
    const void *out;
    SQUnsignedInteger out_alloc;
    SQInteger out_size;
    int errors = 0;
    SQInteger r;
    int expect_copy = 0;
    int i;

    if( (sqstd_text_defaultenc() == enc) ||
        //(((enc == SQTEXTENC_N_UTF16) || (enc == SQTEXTENC_N_UTF32)) && (sqstd_text_nativeenc(enc) == sqstd_text_defaultenc())) )
        ((enc & SQTEXTENC_NATIVE) && (sqstd_text_nativeenc(enc) == sqstd_text_defaultenc())) )
    {
        expect_copy = 1;
    }

    printf( "\n======\tsqstd_text_toutf %s%s\n", env_get_enc_name(enc), expect_copy ? " - copy": "");

// str 0

    r = sqstd_text_toutf( enc, 0, 13, &out, &out_alloc, &out_size);
    printf( "r=%d, out=%p, out_alloc=%u, out_size=%d\n", r, out, out_alloc, out_size);
    if( r == 0)
    {
        printf( "NOK - result is 0\n");
        errors++;
    }
    sqstd_text_release( out, out_alloc);

// str_len 0

    r = sqstd_text_toutf( enc, data.str, 0, &out, &out_alloc, &out_size);
    printf( "r=%d, out=%p, out_alloc=%u, out_size=%d\n", r, out, out_alloc, out_size);
    if( r != 0)
    {
        printf( "NOK - result is not 0 - %d\n", r);
        errors++;
    }
    if( out_size == 0)
    {
        for( i=0; i < data.char_size; i++)
        {
            if( ((const uint8_t*)out)[i] != 0)
            {
                printf( "NOK - not empty string\n");
                errors++;
                break;
            }
        }
    }
    else
    {
        printf( "NOK - answer size not match\n");
        errors++;
    }
    sqstd_text_release( out, out_alloc);

// str_len not given (use own strlen)

    r = sqstd_text_toutf( enc, data.str, -1, &out, &out_alloc, &out_size);
    printf( "r=%d, out=%p, out_alloc=%u, out_size=%d\n", r, out, out_alloc, out_size);
    env_dump_buffer( out, out_size + data.char_size); printf( "\n");
    if( r != 0)
    {
        printf( "NOK - result is not 0 - %d\n", r);
        errors++;
    }
    if( expect_copy)
    {
        if( (const void*)data.str != out)
        {
            printf( "NOK - out is a copy\n");
            errors++;
        }
    }
    else
    {
        if( (const void*)data.str == out)
        {
            printf( "NOK - out is not a copy\n");
            errors++;
        }
    }
    if( out_size == data.ans_size)
    {
        if( memcmp( out, data.answer, data.ans_size + data.char_size) != 0)
        {
            printf( "NOK - answer not match\n");
            errors++;
        }
    }
    else
    {
        printf( "NOK - answer size not match\n");
        errors++;
    }
    sqstd_text_release( out, out_alloc);

// str_len given

    r = sqstd_text_toutf( enc, data.str, data.str_len-1, &out, &out_alloc, &out_size);
    printf( "r=%d, out=%p, out_alloc=%u, out_size=%d\n", r, out, out_alloc, out_size);
    env_dump_buffer( out, out_size + data.char_size); printf( "\n");
    if( r != 0)
    {
        printf( "NOK - result is not 0 - %d\n", r);
        errors++;
    }
    if( r != 0)
    {
        printf( "NOK - result is not 0 - %d\n", r);
        errors++;
    }
    if( (const void*)data.str == out)
    {
        printf( "NOK - out is not a copy\n");
        errors++;
    }
    if( out_size == (data.ans_size - data.char_size))
    {
        if( memcmp( out, data.answer_1, data.ans_size) != 0)
        {
            printf( "NOK - answer not match\n");
            errors++;
        }
    }
    else
    {
        printf( "NOK - answer size not match\n");
        errors++;
    }
    sqstd_text_release( out, out_alloc);

    return errors;
}

static int test_sqstd_SQChar_to_UTF( void)
{
    int errors = 0;

    errors += test_sqstd_SQChar_to_UTF_len( SQTEXTENC_UTF8);
    errors += test_sqstd_SQChar_to_UTF_len( SQTEXTENC_UTF16BE | SQTEXTENC_NATIVE);
    errors += test_sqstd_SQChar_to_UTF_len( SQTEXTENC_UTF16BE);
    errors += test_sqstd_SQChar_to_UTF_len( SQTEXTENC_UTF16LE);
    errors += test_sqstd_SQChar_to_UTF_len( SQTEXTENC_UTF32BE | SQTEXTENC_NATIVE);
    errors += test_sqstd_SQChar_to_UTF_len( SQTEXTENC_UTF32BE);
    errors += test_sqstd_SQChar_to_UTF_len( SQTEXTENC_UTF32LE);
    errors += test_sqstd_SQChar_to_UTF_len( SQTEXTENC_ASCII);

    return errors;
}

static int test_sqstd_UTF_to_SQChar_len( int enc)
{
    const struct data_U_to_SQ_tag data = (enc & SQTEXTENC_NATIVE) ? data_U_to_SQ_N[enc & ~SQTEXTENC_FLAGS] : data_U_to_SQ[enc & ~SQTEXTENC_FLAGS];
    const SQChar *str;
    SQUnsignedInteger str_alloc;
    SQInteger str_len;
    int errors = 0;
    SQInteger r;
    int expect_copy = 0;

    if( (sqstd_text_defaultenc() == enc) ||
        //(((enc == SQTEXTENC_N_UTF16) || (enc == SQTEXTENC_N_UTF32)) && (sqstd_text_nativeenc(enc) == sqstd_text_defaultenc())) )
        ((enc & SQTEXTENC_NATIVE) && (sqstd_text_nativeenc(enc) == sqstd_text_defaultenc())) )
    {
        expect_copy = 1;
    }

    printf( "\n======\tsqstd_text_fromutf %s%s\n", env_get_enc_name(enc), expect_copy ? " - copy" : "");

// inbuf 0

    r = sqstd_text_fromutf( enc, 0, 13, &str, &str_alloc, &str_len);
    printf( "r=%d, str=%p, str_alloc=%u, str_len=%d\n", r, (const void*)str, str_alloc, str_len);
    if( r == 0)
    {
        printf( "NOK - result is 0\n");
        errors++;
    }
    sqstd_text_release( str, str_alloc);

// inbuf_size 0

    r = sqstd_text_fromutf( enc, data.inbuf, 0, &str, &str_alloc, &str_len);
    printf( "r=%d, str=%p, str_alloc=%u, str_len=%d\n", r, (const void*)str, str_alloc, str_len);
    if( r != 0)
    {
        printf( "NOK - result is not 0 - %d\n", r);
        errors++;
    }
    if( str_len == 0)
    {
        if( *str != 0)
        {
            printf( "NOK - not empty string\n");
            errors++;
        }
    }
    else
    {
        printf( "NOK - answer len not match\n");
        errors++;
    }
    sqstd_text_release( str, str_alloc);

// inbuf_size given

    r = sqstd_text_fromutf( enc, data.inbuf, data.inbuf_size - data.char_size, &str, &str_alloc, &str_len);
    printf( "r=%d, str=%p, str_alloc=%u, str_len=%d\n", r, (const void*)str, str_alloc, str_len);
    env_dump_buffer( str, (str_len+1)*sizeof(SQChar)); printf( "\n");
    if( r != 0)
    {
        printf( "NOK - result is not 0 - %d\n", r);
        errors++;
    }
    if( data.inbuf == (const void*)str)
    {
        printf( "NOK - str is not a copy\n");
        errors++;
    }
    if( str_len == (SQInteger)(data.ans_size/sizeof(SQChar) - 1))
    {
        if( memcmp( str, data.answer, data.ans_size) != 0)
        {
            printf( "NOK - answer not match\n");
            errors++;
        }
    }
    else
    {
        printf( "NOK - answer len not match\n");
        errors++;
    }
    sqstd_text_release( str, str_alloc);

// inbuf_size not given (use own strlen)

    r = sqstd_text_fromutf( enc, data.inbuf, -1, &str, &str_alloc, &str_len);
    printf( "r=%d, str=%p, str_alloc=%u, str_len=%d\n", r, (const void*)str, str_alloc, str_len);
    env_dump_buffer( str, (str_len+1)*sizeof(SQChar)); printf( "\n");
    if( r != 0)
    {
        printf( "NOK - result is not 0 - %d\n", r);
        errors++;
    }
    if( expect_copy)
    {
        if( data.inbuf != (const void*)str)
        {
            printf( "NOK - str is a copy\n");
            errors++;
        }
    }
    else
    {
        if( data.inbuf == (const void*)str)
        {
            printf( "NOK - str is not a copy\n");
            errors++;
        }
    }
    if( str_len == (SQInteger)(data.ans_size/sizeof(SQChar) - 1))
    {
        if( memcmp( str, data.answer, data.ans_size) != 0)
        {
            printf( "NOK - answer not match\n");
            errors++;
        }
    }
    else
    {
        printf( "NOK - answer len not match\n");
        errors++;
    }
    sqstd_text_release( str, str_alloc);

    return errors;
}

static int test_sqstd_UTF_to_SQChar( void)
{
    int errors = 0;

    errors += test_sqstd_UTF_to_SQChar_len( SQTEXTENC_UTF8);
    errors += test_sqstd_UTF_to_SQChar_len( SQTEXTENC_UTF16BE | SQTEXTENC_NATIVE);
    errors += test_sqstd_UTF_to_SQChar_len( SQTEXTENC_UTF16BE);
    errors += test_sqstd_UTF_to_SQChar_len( SQTEXTENC_UTF16LE);
    errors += test_sqstd_UTF_to_SQChar_len( SQTEXTENC_UTF32BE | SQTEXTENC_NATIVE);
    errors += test_sqstd_UTF_to_SQChar_len( SQTEXTENC_UTF32BE);
    errors += test_sqstd_UTF_to_SQChar_len( SQTEXTENC_UTF32LE);
    errors += test_sqstd_UTF_to_SQChar_len( SQTEXTENC_ASCII);

    return errors;
}

struct utf_test
{
    const char *name;
    const uint8_t *in;
    int in_size;
    const uint8_t *out;
    int out_size;
    SQInteger r1;
    SQInteger r2;
    unsigned int flags;
};

#define F_IS_CUT        1
#define F_IS_BOM        2
#define F_RES_EMPTY     4

// ===== UTF-8 =====================================

#define TEST( _name, _r1, _r2, _fl, _in, _out) \
    static uint8_t data_8_in_##_name[] = _in; \
    static uint8_t data_8_out_##_name[] = _out;

#include "test_utf8.h"

#undef TEST
#define TEST( _name, _r1, _r2, _fl, _in, _out) \
    { \
        "UTF-8 " #_name, \
        data_8_in_##_name, sizeof(data_8_in_##_name), \
        data_8_out_##_name, sizeof(data_8_out_##_name), \
        _r1, _r2, _fl \
    },

static const struct utf_test utf_8_tests[] = {
#include "test_utf8.h"
    { 0,0,0,0,0,0,0,0 }
};

#undef TEST

// ===== UTF-16BE =====================================

#define C16(_a, _b) _a, _b

#define TEST( _name, _r1, _r2, _fl, _in, _out) \
    static uint8_t data_16be_in_##_name[] = _in; \
    static uint8_t data_16be_out_##_name[] = _out;

#include "test_utf16be.h"

#undef TEST
#define TEST( _name, _r1, _r2, _fl, _in, _out) \
    { \
        "UTF-16BE " #_name, \
        data_16be_in_##_name, sizeof(data_16be_in_##_name), \
        data_16be_out_##_name, sizeof(data_16be_out_##_name), \
        _r1, _r2, _fl \
    },

static const struct utf_test utf_16be_tests[] = {
#include "test_utf16be.h"
    { 0,0,0,0,0,0,0,0 }
};

#undef TEST
#undef C16

// ===== UTF-16LE =====================================

#define C16(_a, _b) _b, _a

#define TEST( _name, _r1, _r2, _fl, _in, _out) \
    static uint8_t data_16le_in_##_name[] = _in; \
    static uint8_t data_16le_out_##_name[] = _out;

#include "test_utf16be.h"

#undef TEST
#define TEST( _name, _r1, _r2, _fl, _in, _out) \
    { \
        "UTF-16LE " #_name, \
        data_16le_in_##_name, sizeof(data_16le_in_##_name), \
        data_16le_out_##_name, sizeof(data_16le_out_##_name), \
        _r1, _r2, _fl \
    },

static const struct utf_test utf_16le_tests[] = {
#include "test_utf16be.h"
    { 0,0,0,0,0,0,0,0 }
};

#undef TEST
#undef C16

// ===== UTF-32BE =====================================

#define C32(_a, _b, _c, _d) _a, _b, _c, _d

#define TEST( _name, _r1, _r2, _fl, _in, _out) \
    static uint8_t data_32be_in_##_name[] = _in; \
    static uint8_t data_32be_out_##_name[] = _out;

#include "test_utf32be.h"

#undef TEST
#define TEST( _name, _r1, _r2, _fl, _in, _out) \
    { \
        "UTF-32BE " #_name, \
        data_32be_in_##_name, sizeof(data_32be_in_##_name), \
        data_32be_out_##_name, sizeof(data_32be_out_##_name), \
        _r1, _r2, _fl \
    },

static const struct utf_test utf_32be_tests[] = {
#include "test_utf32be.h"
    { 0,0,0,0,0,0,0,0 }
};

#undef TEST
#undef C32

// ===== UTF-32LE =====================================

#define C32(_a, _b, _c, _d) _d, _c, _b, _a

#define TEST( _name, _r1, _r2, _fl, _in, _out) \
    static uint8_t data_32le_in_##_name[] = _in; \
    static uint8_t data_32le_out_##_name[] = _out;

#include "test_utf32be.h"

#undef TEST
#define TEST( _name, _r1, _r2, _fl, _in, _out) \
    { \
        "UTF-32LE " #_name, \
        data_32le_in_##_name, sizeof(data_32le_in_##_name), \
        data_32le_out_##_name, sizeof(data_32le_out_##_name), \
        _r1, _r2, _fl \
    },

static const struct utf_test utf_32le_tests[] = {
#include "test_utf32be.h"
    { 0,0,0,0,0,0,0,0 }
};

#undef TEST
#undef C32

// ==========================================

#define TEST_CONV   0
#define TEST_COPY   1
#define TEST_REV    2

static int get_test_kind( int enc)
{
    int test_kind = TEST_CONV;
    if( enc == sqstd_text_defaultenc())
    {
        test_kind = TEST_COPY;
        printf( "Test type - copy\n");
    }
    else if( sqstd_text_nativeenc(enc) == sqstd_text_defaultenc())
    {
        test_kind = TEST_REV;
        printf( "Test type - rev\n");
    }
    else
    {
        printf( "Test type - convert\n");
    }
    return test_kind;
}

static int revcmp( const uint8_t *pa, const uint8_t *pb, int size, int char_size)
{
    for( int p=0; p < size; p += char_size)
    {
        for( int b=0; b < char_size; b++)
        {
            int r = pa[p+b] - pb[p + char_size-1 - b];
            if( r) return r;
        }
    }
    return 0;
}

static int test_utf( int enc, int char_size, const struct utf_test *test)
{
    //const struct utf_8_test *test = utf_8_tests;
    int errors = 0;
    printf( "\n======\tsqstd_text_fromutf --> sqstd_text_toutf %s\n", env_get_enc_name(enc));
    int test_kind = get_test_kind( enc);
    while( test->name)
    {
        const SQChar *str;
        SQUnsignedInteger str_alloc;
        SQInteger str_len;
        SQInteger r;

        if( test->flags & F_IS_BOM)
        {
            test++;
            continue;
        }
        printf( "test %s\n", test->name);

        r = sqstd_text_fromutf( enc, test->in, test->in_size, &str, &str_alloc, &str_len);
        printf( "r=%d, str=%p, str_alloc=%u, str_len=%d\n", r, (const void*)str, str_alloc, str_len);
        env_dump_buffer( str, (str_len+1)*sizeof(SQChar)); printf( "\n");
        if( test_kind == TEST_CONV)
        {
            const void *out;
            SQUnsignedInteger out_alloc;
            SQInteger out_size;

            if( r != test->r1)
            {
                printf( "NOK - sqstd_text_fromutf returns %d\n", r);
                errors++;
            }
            r = sqstd_text_toutf( enc, str, str_len, &out, &out_alloc, &out_size);
            printf( "r=%d, out=%p, out_alloc=%u, out_size=%d\n", r, out, out_alloc, out_size);
            env_dump_buffer( out, out_size + char_size); printf( "\n");
            if( r != test->r2)
            {
                printf( "NOK - sqstd_text_toutf returns %d\n", r);
                errors++;
            }
            if( out_size == test->out_size)
            {
                if( memcmp( out, test->out, test->out_size) != 0)
                {
                    printf( "NOK - answer not match\n");
                    errors++;
                }
            }
            else
            {
                printf( "NOK - answer size not match\n");
                errors++;
            }
            sqstd_text_release( out, out_alloc);
        }
        else if( (test_kind == TEST_COPY) || (test_kind == TEST_REV))
        {
            int is_cut = test->flags & F_IS_CUT;
            int in_size = test->in_size;
            if( (is_cut && (r != test->r1)) ||      // usualy copy returns SQ_OK
                (!is_cut && r)                      // but for strings with bad size - SQ_ERROR
            )
            {
                printf( "NOK - sqstd_text_fromutf returns %d\n", r);
                errors++;
            }
            if( (const void*)str == test->in)
            {
                printf( "NOK - str is not a copy\n");
                errors++;
            }
            if( is_cut)
            {
                in_size = sizeof(SQChar)*(in_size/sizeof(SQChar));
            }
            if( str_len != (SQInteger)(in_size/sizeof(SQChar)))
            {
                printf( "NOK - str_len not match\n");
                errors++;
            }
            else
            {
                if( test_kind == TEST_COPY)
                {
                    if( memcmp( str, test->in, in_size) != 0)
                    {
                        printf( "NOK - answer not match\n");
                        errors++;
                    }
                }
                else
                {
                    if( revcmp( (const uint8_t*)str, (const uint8_t*)test->in, in_size, char_size) != 0)
                    {
                        printf( "NOK - answer not match\n");
                        errors++;
                    }
                }
                if( str[str_len] != 0)
                {
                    printf( "NOK - answer is not truncated\n");
                    errors++;
                }
            }
        }
        sqstd_text_release( str, str_alloc);

        test++;
    }
    return errors;
}

static int test_utf_rw( int enc, int char_size, const struct utf_test *test)
{
    int errors = 0;
    SQSTREAM stream = (void*)0x1234;
    printf( "\n======\tsqstd_text_reader --> sqstd_text_writer %s\n", env_get_enc_name(enc));
    while( test->name)
    {
        SQChar buff[1024];
        uint8_t wr_buff[1024];
        SQInteger buff_pos;
        SQInteger r_errs;
        SQInteger w_errs;
        int r = 0;
        int test_kind;
        int bom_size = 0;
        int bom_char_size = char_size;
        SQBool read_bom = (test->flags & F_IS_BOM) ? SQTrue : SQFalse;

        printf( "test %s\n", test->name);

        if( !read_bom)
        {
            test_kind = get_test_kind( enc);
        }
        else
        {
            if( test->r2 >= 0)
            {
                test_kind = get_test_kind( test->r2);
                switch( test->r2 & ~SQTEXTENC_FLAGS)
                {
                    case SQTEXTENC_UTF8:
                        bom_size = 3;
                        bom_char_size = 1;
                        break;
                    case SQTEXTENC_UTF16BE:
                    case SQTEXTENC_UTF16LE:
                        bom_size = 2;
                        bom_char_size = 2;
                        break;
                    case SQTEXTENC_UTF32BE:
                    case SQTEXTENC_UTF32LE:
                        bom_size = 4;
                        bom_char_size = 4;
                    default:
                        break;
                }
            }
            else
            {
                test_kind = get_test_kind( enc);
            }
        }

        {
            SQSTDTEXTRD rdr;
            env_set_sread_buffer( test->in, test->in_size);
            env_stream_eof = 0;
            rdr = sqstd_text_reader( stream, enc, read_bom, 0);
            buff_pos=0;
            while( 1)
            {
                r = sqstd_text_readchar( rdr, buff + buff_pos);
                if( !r)
                {
                    buff_pos++;
                }
                else
                {
                    if( !sqstd_text_rdeof( rdr))
                    {
                        printf( "NOK - sqstd_text_readchar() return %d for char %u not at EOF\n", r, buff_pos);
                        errors++;
                    }
                    break;
                }
            }
            r_errs = sqstd_text_rderrors( rdr);
            if( r_errs)
            {
                printf( "read errors %u\n", r_errs);
            }
            sqstd_text_readerrelease( rdr);
            printf( "buf: "); env_dump_buffer( buff, buff_pos*sizeof(SQChar)); printf( "\n");
        }
        if( test->flags & F_RES_EMPTY)
        {
            if( buff_pos != 0)
            {
                printf( "NOK - buff is not empty\n");
                errors++;
            }
        }
        else
        {
            {
                SQSTDTEXTWR wrt;
                env_set_swrite_buffer( wr_buff, sizeof(wr_buff));
                wrt = sqstd_text_writer( stream, enc, SQFalse, 0);
                r = sqstd_text_writestring( wrt, buff, buff_pos);
                w_errs = sqstd_text_wrerrors( wrt);
                if( w_errs)
                {
                    printf( "NOK - write errors %u\n", w_errs);
                    errors++;
                }
                sqstd_text_writerrelease( wrt);
                printf( "ans: "); env_dump_buffer( wr_buff, env_get_swrite_len()); printf( "\n");
            }

            if( test_kind == TEST_CONV)
            {
                if( env_get_swrite_len() == test->out_size)
                {
                    if( memcmp( wr_buff, test->out, test->out_size) != 0)
                    {
                        printf( "NOK - answer not match :");
                        env_dump_buffer( test->out, test->out_size); printf( "\n");
                        errors++;
                    }
                }
                else
                {
                    printf( "NOK - answer size not match\n");
                    errors++;
                }
            }
            else if( (test_kind == TEST_COPY) || (test_kind == TEST_REV))
            {
                int is_cut = test->flags & F_IS_CUT;
                int in_size = test->in_size;
                const uint8_t *test_in = test->in;
                if( bom_size )
                {
                    in_size = test->in_size - bom_size;
                    test_in = test->in + bom_size;
                }
                if( is_cut)
                {
                    if( (int)(buff_pos*sizeof(SQChar)) >= in_size)
                    {
                        printf( "NOK - no cut chars\n");
                        errors++;
                    }
                    in_size = sizeof(SQChar)*(in_size/sizeof(SQChar));
                    if( r_errs == 0)
                    {
                        printf( "NOK - no read error for cut\n");
                        errors++;
                    }
                }
                if( (int)(buff_pos*sizeof(SQChar)) == in_size)
                {
                    if( test_kind == TEST_COPY)
                    {
                        if( memcmp( (uint8_t*)buff, test_in, in_size) != 0)
                        {
                            printf( "NOK - buffer is not a copy :");
                            env_dump_buffer( test_in, in_size); printf( "\n");
                            errors++;
                        }
                    }
                    else
                    {
                        if( revcmp( (uint8_t*)buff, test_in, in_size, bom_char_size) != 0)
                        {
                            printf( "NOK - buffer is not a reverse copy :");
                            env_dump_buffer( test_in, in_size); printf( "\n");
                            errors++;
                        }
                    }
                }
                else
                {
                    printf( "NOK - buffer size not match %u vs %u\n", in_size, buff_pos*sizeof(SQChar));
                    errors++;
                }
            }
        }

        test++;
    }
    return errors;
}

int main( void)
{
    int errors = 0;

    printf( "Default encoding :%u - %s\n", sqstd_text_defaultenc(), env_get_enc_name(sqstd_text_defaultenc()));

    //env_dump_allocs = 1;

    printf( "\n==========\n");
    errors += test_sqstd_UTF_to_SQChar();
    printf( "\n==========\n");
    errors += test_sqstd_SQChar_to_UTF();

    printf( "\n==========\n");
    errors += test_utf( SQTEXTENC_UTF8, 1, utf_8_tests);

    printf( "\n==========\n");
    errors += test_utf( SQTEXTENC_UTF16BE, 2, utf_16be_tests);
    printf( "\n==========\n");
    errors += test_utf( SQTEXTENC_UTF16LE, 2, utf_16le_tests);

    printf( "\n==========\n");
    errors += test_utf( SQTEXTENC_UTF32BE, 4, utf_32be_tests);
    printf( "\n==========\n");
    errors += test_utf( SQTEXTENC_UTF32LE, 4, utf_32le_tests);

    printf( "\n==========\n");
    errors += test_utf_rw( SQTEXTENC_UTF8, 1, utf_8_tests);

    printf( "\n==========\n");
    errors += test_utf_rw( SQTEXTENC_UTF16BE, 2, utf_16be_tests);
    printf( "\n==========\n");
    errors += test_utf_rw( SQTEXTENC_UTF16LE, 2, utf_16le_tests);

    printf( "\n==========\n");
    errors += test_utf_rw( SQTEXTENC_UTF32BE, 4, utf_32be_tests);
    printf( "\n==========\n");
    errors += test_utf_rw( SQTEXTENC_UTF32LE, 4, utf_32le_tests);

    if( errors)
    {
        printf( "\nERRORS %d\n", errors);
    }
    else
    {
        printf( "\nOK\n");
    }

    return 0; //errors;
}
