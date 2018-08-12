
#include <stdio.h>
#include <stdint.h>

#include <squirrel.h>
#include <sqstdstream.h>
#include <sqstdtextrw.h>

#include <env.h>


static int test_sqstd_UTF_to_SQChar( void)
{
    const SQChar *str;
    SQUnsignedInteger str_alloc;
    SQInteger str_len;
    SQInteger r;

    printf( "\n======\tsqstd_UTF_to_SQChar UTF8\n");
    {
        uint8_t inb[] = { 'H', 'E', 'L', 'L', 'O', '\0' };

        r = sqstd_UTF_to_SQChar( SQTEXTENC_UTF8, inb, sizeof(inb)-1, &str, &str_alloc, &str_len);
        printf( "r=%d, str=%p, str_alloc=%u, str_len=%d\n", r, (const void*)str, str_alloc, str_len);
        env_dump_buffer( str, (str_len+1)*sizeof(SQChar)); printf( "\n");
        sqstd_textrelease( str, str_alloc);

        r = sqstd_UTF_to_SQChar( SQTEXTENC_UTF8, inb, -1, &str, &str_alloc, &str_len);
        printf( "r=%d, str=%p, str_alloc=%u, str_len=%d\n", r, (const void*)str, str_alloc, str_len);
        env_dump_buffer( str, (str_len+1)*sizeof(SQChar)); printf( "\n");
        sqstd_textrelease( str, str_alloc);

        r = sqstd_UTF_to_SQChar( SQTEXTENC_UTF8, inb, sizeof(inb)-2, &str, &str_alloc, &str_len);
        printf( "r=%d, str=%p, str_alloc=%u, str_len=%d\n", r, (const void*)str, str_alloc, str_len);
        env_dump_buffer( str, (str_len+1)*sizeof(SQChar)); printf( "\n");
        sqstd_textrelease( str, str_alloc);
    }

    printf( "\n======\tsqstd_UTF_to_SQChar NATIVE UTF16\n");
    {
        uint16_t inb[] = { 'H', 'E', 'L', 'L', 'O', '\0' };

        r = sqstd_UTF_to_SQChar( SQTEXTENC_N_UTF16, inb, sizeof(inb)-sizeof(*inb), &str, &str_alloc, &str_len);
        printf( "r=%d, str=%p, str_alloc=%u, str_len=%d\n", r, (const void*)str, str_alloc, str_len);
        env_dump_buffer( str, (str_len+1)*sizeof(SQChar)); printf( "\n");
        sqstd_textrelease( str, str_alloc);

        r = sqstd_UTF_to_SQChar( SQTEXTENC_N_UTF16, inb, -1, &str, &str_alloc, &str_len);
        printf( "r=%d, str=%p, str_alloc=%u, str_len=%d\n", r, (const void*)str, str_alloc, str_len);
        env_dump_buffer( str, (str_len+1)*sizeof(SQChar)); printf( "\n");
        sqstd_textrelease( str, str_alloc);

        r = sqstd_UTF_to_SQChar( SQTEXTENC_N_UTF16, inb, sizeof(inb)-2*sizeof(*inb), &str, &str_alloc, &str_len);
        printf( "r=%d, str=%p, str_alloc=%u, str_len=%d\n", r, (const void*)str, str_alloc, str_len);
        env_dump_buffer( str, (str_len+1)*sizeof(SQChar)); printf( "\n");
        sqstd_textrelease( str, str_alloc);
    }

    printf( "\n======\tsqstd_UTF_to_SQChar NATIVE UTF32\n");
    {
        uint32_t inb[] = { 'H', 'E', 'L', 'L', 'O', '\0' };

        r = sqstd_UTF_to_SQChar( SQTEXTENC_N_UTF32, inb, sizeof(inb)-sizeof(*inb), &str, &str_alloc, &str_len);
        printf( "r=%d, str=%p, str_alloc=%u, str_len=%d\n", r, (const void*)str, str_alloc, str_len);
        env_dump_buffer( str, (str_len+1)*sizeof(SQChar)); printf( "\n");
        sqstd_textrelease( str, str_alloc);

        r = sqstd_UTF_to_SQChar( SQTEXTENC_N_UTF32, inb, -1, &str, &str_alloc, &str_len);
        printf( "r=%d, str=%p, str_alloc=%u, str_len=%d\n", r, (const void*)str, str_alloc, str_len);
        env_dump_buffer( str, (str_len+1)*sizeof(SQChar)); printf( "\n");
        sqstd_textrelease( str, str_alloc);

        r = sqstd_UTF_to_SQChar( SQTEXTENC_N_UTF32, inb, sizeof(inb)-2*sizeof(*inb), &str, &str_alloc, &str_len);
        printf( "r=%d, str=%p, str_alloc=%u, str_len=%d\n", r, (const void*)str, str_alloc, str_len);
        env_dump_buffer( str, (str_len+1)*sizeof(SQChar)); printf( "\n");
        sqstd_textrelease( str, str_alloc);
    }

    return r;
}

static int test_sqstd_SQChar_to_UTF( void)
{
    const uint8_t *out;
    SQUnsignedInteger out_alloc;
    SQInteger out_size;
    SQInteger r;

    SQChar inb[] = { 'H', 'E', 'L', 'L', 'O', '\0' };

    printf( "\n======\tsqstd_SQChar_to_UTF UTF8\n");
    {
        r = sqstd_SQChar_to_UTF( SQTEXTENC_UTF8, inb, sizeof(inb)/sizeof(*inb) - 1, (const void**)&out, &out_alloc, &out_size);
        printf( "r=%d, out=%p, out_alloc=%u, out_size=%d\n", r, (const void*)out, out_alloc, out_size);
        env_dump_buffer( out, out_size + 1); printf( "\n");
        sqstd_textrelease( out, out_alloc);

        r = sqstd_SQChar_to_UTF( SQTEXTENC_UTF8, inb, -1, (const void**)&out, &out_alloc, &out_size);
        printf( "r=%d, out=%p, out_alloc=%u, out_size=%d\n", r, (const void*)out, out_alloc, out_size);
        env_dump_buffer( out, out_size + 1); printf( "\n");
        sqstd_textrelease( out, out_alloc);

        r = sqstd_SQChar_to_UTF( SQTEXTENC_UTF8, inb, sizeof(inb)/sizeof(*inb) - 2, (const void**)&out, &out_alloc, &out_size);
        printf( "r=%d, out=%p, out_alloc=%u, out_size=%d\n", r, (const void*)out, out_alloc, out_size);
        env_dump_buffer( out, out_size + 1); printf( "\n");
        sqstd_textrelease( out, out_alloc);
    }

    printf( "\n======\tsqstd_SQChar_to_UTF NATIVE UTF16\n");
    {
        r = sqstd_SQChar_to_UTF( SQTEXTENC_N_UTF16, inb, sizeof(inb)/sizeof(*inb) - 1, (const void**)&out, &out_alloc, &out_size);
        printf( "r=%d, out=%p, out_alloc=%u, out_size=%d\n", r, (const void*)out, out_alloc, out_size);
        env_dump_buffer( out, out_size + 2); printf( "\n");
        sqstd_textrelease( out, out_alloc);

        r = sqstd_SQChar_to_UTF( SQTEXTENC_N_UTF16, inb, -1, (const void**)&out, &out_alloc, &out_size);
        printf( "r=%d, out=%p, out_alloc=%u, out_size=%d\n", r, (const void*)out, out_alloc, out_size);
        env_dump_buffer( out, out_size + 2); printf( "\n");
        sqstd_textrelease( out, out_alloc);

        r = sqstd_SQChar_to_UTF( SQTEXTENC_N_UTF16, inb, sizeof(inb)/sizeof(*inb) - 2, (const void**)&out, &out_alloc, &out_size);
        printf( "r=%d, out=%p, out_alloc=%u, out_size=%d\n", r, (const void*)out, out_alloc, out_size);
        env_dump_buffer( out, out_size + 2); printf( "\n");
        sqstd_textrelease( out, out_alloc);
    }

    printf( "\n======\tsqstd_SQChar_to_UTF NATIVE UTF32\n");
    {
        r = sqstd_SQChar_to_UTF( SQTEXTENC_N_UTF32, inb, sizeof(inb)/sizeof(*inb) - 1, (const void**)&out, &out_alloc, &out_size);
        printf( "r=%d, out=%p, out_alloc=%u, out_size=%d\n", r, (const void*)out, out_alloc, out_size);
        env_dump_buffer( out, out_size + 4); printf( "\n");
        sqstd_textrelease( out, out_alloc);

        r = sqstd_SQChar_to_UTF( SQTEXTENC_N_UTF32, inb, -1, (const void**)&out, &out_alloc, &out_size);
        printf( "r=%d, out=%p, out_alloc=%u, out_size=%d\n", r, (const void*)out, out_alloc, out_size);
        env_dump_buffer( out, out_size + 4); printf( "\n");
        sqstd_textrelease( out, out_alloc);

        r = sqstd_SQChar_to_UTF( SQTEXTENC_N_UTF32, inb, sizeof(inb)/sizeof(*inb) - 2, (const void**)&out, &out_alloc, &out_size);
        printf( "r=%d, out=%p, out_alloc=%u, out_size=%d\n", r, (const void*)out, out_alloc, out_size);
        env_dump_buffer( out, out_size + 4); printf( "\n");
        sqstd_textrelease( out, out_alloc);
    }

    return r;
}

int main( void)
{
    printf( "Default encoding :%u\n", sqstd_textdefaultenc());
    //printf( "Default encoding name :%s\n", sqstd_textdefaultencname());

    env_dump_allocs = 1;

    test_sqstd_UTF_to_SQChar();
    test_sqstd_SQChar_to_UTF();

    return 0;
}
