
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <squirrel.h>
#include <sqstdstream.h>
#include <sqstdtextrw.h>

#include <env.h>

#define ALIGN_2( _x)    _x __attribute__ ((aligned (2)))
#define ALIGN_4( _x)    _x __attribute__ ((aligned (4)))

const char *env_enc_name[] = {
    [SQTEXTENC_UTF8] = "UTF8",
    [SQTEXTENC_N_UTF16] = "NATIVE-UTF-16",
    [SQTEXTENC_UTF16BE] = "UTF-16BE",
    [SQTEXTENC_UTF16LE] = "UTF-16LE",
    [SQTEXTENC_N_UTF32] = "NATIVE-UTF-32",
    [SQTEXTENC_UTF32BE] = "UTF-32BE",
    [SQTEXTENC_UTF32LE] = "UTF-32LE",
    [SQTEXTENC_ASCII] = "ASCII",
};


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
    U2SQ( SQTEXTENC_N_UTF16, str_N16, 2, str_SQ)
    U2SQ( SQTEXTENC_UTF16BE, str_16B, 2, str_SQ)
    U2SQ( SQTEXTENC_UTF16LE, str_16L, 2, str_SQ)
    U2SQ( SQTEXTENC_N_UTF32, str_N32, 4, str_SQ)
    U2SQ( SQTEXTENC_UTF32BE, str_32B, 4, str_SQ)
    U2SQ( SQTEXTENC_UTF32LE, str_32L, 4, str_SQ)
    U2SQ( SQTEXTENC_ASCII, str_8, 1, str_SQ)
#undef U2SQ
};

//sqstd_SQChar_to_UTF( int encoding, const SQChar *str, SQInteger str_len,
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
    SQ2U( SQTEXTENC_N_UTF16, str_SQ, 2, str_N16)
    SQ2U( SQTEXTENC_UTF16BE, str_SQ, 2, str_16B)
    SQ2U( SQTEXTENC_UTF16LE, str_SQ, 2, str_16L)
    SQ2U( SQTEXTENC_N_UTF32, str_SQ, 4, str_N32)
    SQ2U( SQTEXTENC_UTF32BE, str_SQ, 4, str_32B)
    SQ2U( SQTEXTENC_UTF32LE, str_SQ, 4, str_32L)
    SQ2U( SQTEXTENC_ASCII, str_SQ, 1, str_8)
    #undef SQ2U
};

static int test_sqstd_SQChar_to_UTF_len( int enc)
{
    const struct data_SQ_to_U_tag data = data_SQ_to_U[enc];
    const void *out;
    SQUnsignedInteger out_alloc;
    SQInteger out_size;
    int errors = 0;
    SQInteger r;
    int expect_copy = 0;
    int i;

    if( (sqstd_textdefaultenc() == enc) ||
        (((enc == SQTEXTENC_N_UTF16) || (enc == SQTEXTENC_N_UTF32)) && (sqstd_textnativeenc(enc) == sqstd_textdefaultenc())) )
    {
        expect_copy = 1;
    }

    printf( "\n======\tsqstd_SQChar_to_UTF %s\n", env_enc_name[enc]);

// str 0

    r = sqstd_SQChar_to_UTF( enc, 0, 13, &out, &out_alloc, &out_size);
    printf( "r=%d, out=%p, out_alloc=%u, out_size=%d\n", r, out, out_alloc, out_size);
    if( r == 0)
    {
        printf( "NOK - result is 0\n");
        errors++;
    }
    sqstd_textrelease( out, out_alloc);

// str_len 0

    r = sqstd_SQChar_to_UTF( enc, data.str, 0, &out, &out_alloc, &out_size);
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
    sqstd_textrelease( out, out_alloc);

// str_len given - same as strlen

    r = sqstd_SQChar_to_UTF( enc, data.str, data.str_len, &out, &out_alloc, &out_size);
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
    sqstd_textrelease( out, out_alloc);

// str_len not given (use own strlen)

    r = sqstd_SQChar_to_UTF( enc, data.str, -1, &out, &out_alloc, &out_size);
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
    sqstd_textrelease( out, out_alloc);

// str_len given - different from strlen

    r = sqstd_SQChar_to_UTF( enc, data.str, data.str_len - 1 , &out, &out_alloc, &out_size);
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
    sqstd_textrelease( out, out_alloc);

    return errors;
}

static int test_sqstd_SQChar_to_UTF( void)
{
    int errors = 0;

    errors += test_sqstd_SQChar_to_UTF_len( SQTEXTENC_UTF8);
    errors += test_sqstd_SQChar_to_UTF_len( SQTEXTENC_N_UTF16);
    errors += test_sqstd_SQChar_to_UTF_len( SQTEXTENC_UTF16BE);
    errors += test_sqstd_SQChar_to_UTF_len( SQTEXTENC_UTF16LE);
    errors += test_sqstd_SQChar_to_UTF_len( SQTEXTENC_N_UTF32);
    errors += test_sqstd_SQChar_to_UTF_len( SQTEXTENC_UTF32BE);
    errors += test_sqstd_SQChar_to_UTF_len( SQTEXTENC_UTF32LE);
    errors += test_sqstd_SQChar_to_UTF_len( SQTEXTENC_ASCII);

    return errors;
}

static int test_sqstd_UTF_to_SQChar_len( int enc)
{
    const struct data_U_to_SQ_tag data = data_U_to_SQ[enc];
    const SQChar *str;
    SQUnsignedInteger str_alloc;
    SQInteger str_len;
    int errors = 0;
    SQInteger r;
    int expect_copy = 0;

    if( (sqstd_textdefaultenc() == enc) ||
        (((enc == SQTEXTENC_N_UTF16) || (enc == SQTEXTENC_N_UTF32)) && (sqstd_textnativeenc(enc) == sqstd_textdefaultenc())) )
    {
        expect_copy = 1;
    }

    printf( "\n======\tsqstd_UTF_to_SQChar %s\n", env_enc_name[enc]);

// inbuf 0

    r = sqstd_UTF_to_SQChar( enc, 0, 13, &str, &str_alloc, &str_len);
    printf( "r=%d, str=%p, str_alloc=%u, str_len=%d\n", r, (const void*)str, str_alloc, str_len);
    if( r == 0)
    {
        printf( "NOK - result is 0\n");
        errors++;
    }
    sqstd_textrelease( str, str_alloc);

// inbuf_size 0

    r = sqstd_UTF_to_SQChar( enc, data.inbuf, 0, &str, &str_alloc, &str_len);
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
    sqstd_textrelease( str, str_alloc);

// inbuf_size given - same as strlen

    r = sqstd_UTF_to_SQChar( enc, data.inbuf, data.inbuf_size - data.char_size, &str, &str_alloc, &str_len);
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
    sqstd_textrelease( str, str_alloc);

// inbuf_size not given (use own strlen)

    r = sqstd_UTF_to_SQChar( enc, data.inbuf, -1, &str, &str_alloc, &str_len);
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
    sqstd_textrelease( str, str_alloc);

// inbuf_size given - different from strlen

    r = sqstd_UTF_to_SQChar( enc, data.inbuf, data.inbuf_size - 2*data.char_size, &str, &str_alloc, &str_len);
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
    if( str_len == (SQInteger)(data.ans_size/sizeof(SQChar) - 2))
    {
        if( memcmp( str, data.answer_1, data.ans_size - sizeof(SQChar)) != 0)
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
    sqstd_textrelease( str, str_alloc);

// inbuf_size given - not at char boundary

    if( (enc != SQTEXTENC_UTF8) && (enc != SQTEXTENC_ASCII))
    {
        r = sqstd_UTF_to_SQChar( enc, data.inbuf, data.inbuf_size - data.char_size - 1, &str, &str_alloc, &str_len);
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
        if( memcmp( str, data.answer_1, data.ans_size - sizeof(SQChar)) != 0)
        {
            printf( "NOK - answer not match\n");
            errors++;
        }
        sqstd_textrelease( str, str_alloc);
    }

    return errors;
}

static int test_sqstd_UTF_to_SQChar( void)
{
    int errors = 0;

    errors += test_sqstd_UTF_to_SQChar_len( SQTEXTENC_UTF8);
    errors += test_sqstd_UTF_to_SQChar_len( SQTEXTENC_N_UTF16);
    errors += test_sqstd_UTF_to_SQChar_len( SQTEXTENC_UTF16BE);
    errors += test_sqstd_UTF_to_SQChar_len( SQTEXTENC_UTF16LE);
    errors += test_sqstd_UTF_to_SQChar_len( SQTEXTENC_N_UTF32);
    errors += test_sqstd_UTF_to_SQChar_len( SQTEXTENC_UTF32BE);
    errors += test_sqstd_UTF_to_SQChar_len( SQTEXTENC_UTF32LE);
    errors += test_sqstd_UTF_to_SQChar_len( SQTEXTENC_ASCII);

    return errors;
}

struct utf_8_test
{
    const char *name;
    const void *in;
    int in_size;
    const void *out;
    int out_size;
    SQInteger r1;
    SQInteger r2;
};

#define TEST( _name, _r1, _r2, _in, _out) \
    static uint8_t data_8_in_##_name[] = _in; \
    static uint8_t data_8_out_##_name[] = _out;

#include "test_utf8.h"

#undef TEST
#define TEST( _name, _r1, _r2, _in, _out) \
    { \
        #_name, \
        data_8_in_##_name, sizeof(data_8_in_##_name), \
        data_8_out_##_name, sizeof(data_8_out_##_name), \
        _r1, _r2 \
    },

static const struct utf_8_test utf_8_tests[] = {
#include "test_utf8.h"
    { 0,0,0,0,0,0,0 }
};

#undef TEST

static int test_utf8( void)
{
    const struct utf_8_test *test = utf_8_tests;
    int errors = 0;
    while( test->name)
    {
        const SQChar *str;
        SQUnsignedInteger str_alloc;
        SQInteger str_len;
        SQInteger r;

        printf( "test %s\n", test->name);

        r = sqstd_UTF_to_SQChar( SQTEXTENC_UTF8, test->in, test->in_size, &str, &str_alloc, &str_len);
        printf( "r=%d, str=%p, str_alloc=%u, str_len=%d\n", r, (const void*)str, str_alloc, str_len);
        env_dump_buffer( str, (str_len+1)*sizeof(SQChar)); printf( "\n");
        if( sizeof(SQChar) != 1)
        {
            const void *out;
            SQUnsignedInteger out_alloc;
            SQInteger out_size;

            if( r != test->r1)
            {
                printf( "NOK - sqstd_UTF_to_SQChar returns %d\n", r);
                errors++;
            }

            r = sqstd_SQChar_to_UTF( SQTEXTENC_UTF8, str, str_len, &out, &out_alloc, &out_size);
            printf( "r=%d, out=%p, out_alloc=%u, out_size=%d\n", r, out, out_alloc, out_size);
            env_dump_buffer( out, out_size + sizeof(uint8_t)); printf( "\n");
            if( r != test->r2)
            {
                printf( "NOK - sqstd_SQChar_to_UTF returns %d\n", r);
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
            sqstd_textrelease( out, out_alloc);
        }
        else
        {
            if( r != 0)
            {
                printf( "NOK - sqstd_UTF_to_SQChar returns %d\n", r);
                errors++;
            }
            if( (const void*)str != test->in)
            {
                printf( "NOK - str is a copy\n");
                errors++;
            }
        }
        sqstd_textrelease( str, str_alloc);

        test++;
    }
    return errors;
}


int main( void)
{
    int errors = 0;

    printf( "Default encoding :%u - %s\n", sqstd_textdefaultenc(), env_enc_name[sqstd_textdefaultenc()]);

    //env_dump_allocs = 1;

    //errors += test_sqstd_UTF_to_SQChar();
    //errors += test_sqstd_SQChar_to_UTF();

    errors += test_utf8();

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
