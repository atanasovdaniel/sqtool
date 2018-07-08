/*  see copyright notice in sqstdtextrw.cpp */
#ifndef _SQSTDTEXTRW_H_
#define _SQSTDTEXTRW_H_

#ifdef __cplusplus
extern "C" {
#endif

#define SQTEXTENC_UTF8      0
#define SQTEXTENC_UTF16     1
#define SQTEXTENC_UTF16BE   2
#define SQTEXTENC_UTF16LE   3
#define SQTEXTENC_UTF32     4
#define SQTEXTENC_UTF32BE   5
#define SQTEXTENC_UTF32LE   6
#define SQTEXTENC_ASCII     7

SQUIRREL_API SQInteger sqstd_textencbyname( const SQChar *name);

SQUIRREL_API SQInteger sqstd_UTF_to_SQChar( int encoding, const void *inbuf, SQInteger inbuf_size,
                 const SQChar **pstr, SQUnsignedInteger *palloc, SQInteger *plen);
SQUIRREL_API SQInteger sqstd_UTF8_to_SQChar( const uint8_t *inbuf, SQInteger inbuf_size,
                 const SQChar **pstr, SQUnsignedInteger *palloc, SQInteger *plen);
SQUIRREL_API SQInteger sqstd_UTF16_to_SQChar( const uint16_t *inbuf, SQInteger inbuf_size,
                 const SQChar **pstr, SQUnsignedInteger *palloc, SQInteger *plen);
SQUIRREL_API SQInteger sqstd_UTF32_to_SQChar( const uint32_t *inbuf, SQInteger inbuf_size,
                 const SQChar **pstr, SQUnsignedInteger *palloc, SQInteger *plen);


SQUIRREL_API SQInteger sqstd_SQChar_to_UTF( int encoding, const SQChar *str, SQInteger str_len,
                const void **pout, SQUnsignedInteger *pout_alloc, SQInteger *pout_size);
SQUIRREL_API SQInteger sqstd_SQChar_to_UTF8( const SQChar *str, SQInteger str_len,
                const uint8_t **pout, SQUnsignedInteger *pout_alloc, SQInteger *pout_size);
SQUIRREL_API SQInteger sqstd_SQChar_to_UTF16( const SQChar *str, SQInteger str_len,
                const uint16_t **pout, SQUnsignedInteger *pout_alloc, SQInteger *pout_size);
SQUIRREL_API SQInteger sqstd_SQChar_to_UTF32( const SQChar *str, SQInteger str_len,
                const uint32_t **pout, SQUnsignedInteger *pout_alloc, SQInteger *pout_size);

SQUIRREL_API SQInteger sqstd_get_UTF(HSQUIRRELVM v, SQInteger idx, int encoding, const void **pout, SQUnsignedInteger *pout_alloc, SQInteger *pout_size);
SQUIRREL_API SQInteger sqstd_get_UTF8(HSQUIRRELVM v, SQInteger idx, const uint8_t **pout, SQUnsignedInteger *pout_alloc, SQInteger *pout_size);
SQUIRREL_API SQInteger sqstd_get_UTF16(HSQUIRRELVM v, SQInteger idx, const uint16_t **pout, SQUnsignedInteger *pout_alloc, SQInteger *pout_size);
SQUIRREL_API SQInteger sqstd_get_UTF32(HSQUIRRELVM v, SQInteger idx, const uint32_t **pout, SQUnsignedInteger *pout_alloc, SQInteger *pout_size);

SQUIRREL_API void sqstd_push_UTF(HSQUIRRELVM v, int encoding, const void *inbuf, SQInteger inbuf_size);
SQUIRREL_API void sqstd_push_UTF8(HSQUIRRELVM v, const uint8_t *inbuf, SQInteger inbuf_size);
SQUIRREL_API void sqstd_push_UTF16(HSQUIRRELVM v, const uint16_t *inbuf, SQInteger inbuf_size);
SQUIRREL_API void sqstd_push_UTF32(HSQUIRRELVM v, const uint32_t *inbuf, SQInteger inbuf_size);

SQUIRREL_API void sqstd_textrelease( const void *text, SQUnsignedInteger text_alloc_size);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // _SQSTDTEXTRW_H_
