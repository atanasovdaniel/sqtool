
#include <stdint.h>
#include <malloc.h>
#include <string.h>

#include <squirrel.h>
#include <sqstdstream.h>

#include <env.h>

void env_dump_buffer( const void *buf, int len)
{
    const uint8_t *p = (const uint8_t*)buf;
    while( len > 0)
    {
        printf( " %02X", *p);
        len--;
        p++;
    }
}

// memory

int env_dump_allocs;

void *sq_malloc( SQUnsignedInteger size)
{
    void *r = malloc( size);
    if( env_dump_allocs)
    {
        printf( "MALLOC:%p:%u\n", r, size);
    }
    return r;
}

void *sq_realloc( void* p, SQUnsignedInteger oldsize,SQUnsignedInteger newsize)
{
    void *r = realloc( p, newsize);
    if( env_dump_allocs)
    {
        printf( "REALLOC:%p:%u, %p:%u\n", r, newsize, p, oldsize);
    }
    return realloc( p, newsize);
}

void sq_free(void *p, SQ_UNUSED_ARG(SQUnsignedInteger size))
{
    if( env_dump_allocs)
    {
        printf( "FREE:%p:%u\n", p, size);
    }
    free( p);
}

// stream

static const uint8_t *srd_buff;
static SQInteger srd_len;

void env_set_sread_buffer( const void *buf, SQInteger len)
{
    srd_buff = (const uint8_t*)buf;
    srd_len = len;
}

SQInteger sqstd_sread( void *buf, SQInteger len, SQ_UNUSED_ARG(SQSTREAM stream))
{
    SQInteger left = len;
    if( left > srd_len) left = srd_len;
    if( left)
    {
        memcpy( buf, srd_buff, left);
        srd_buff += left;
        srd_len -= left;
    }
    return left;
}

static uint8_t *swr_buff;
static SQInteger swr_len = -1;
int env_sqstd_swrite_dump;

void env_set_swrite_buffer( void *buf, SQInteger len)
{
    swr_buff = (uint8_t*)buf;
    swr_len = len;
}

SQInteger sqstd_swrite( const void *buf, SQInteger len, SQ_UNUSED_ARG(SQSTREAM stream))
{
    if( swr_len >= 0)
    {
        SQInteger left = len;
        if( left > swr_len) left = swr_len;
        if( left)
        {
            memcpy( swr_buff, buf, left);
            swr_buff += left;
            swr_len -= left;
        }
        return left;
    }
    else
    {
        return len;
    }
}
