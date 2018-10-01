/*  see copyright notice in sqstdtextrw.cpp */
#ifndef _SQSTDTEXT_H_
#define _SQSTDTEXT_H_

#ifdef __cplusplus
extern "C" {
#endif

#define SQTEXTENC_UTF8      0
#define SQTEXTENC_UTF16BE   1
#define SQTEXTENC_UTF16LE   2
#define SQTEXTENC_UTF32BE   3
#define SQTEXTENC_UTF32LE   4
#define SQTEXTENC_ASCII     5

#define SQTEXTENC_NATIVE    0x40
#define SQTEXTENC_STRICT    0x80
#define SQTEXTENC_FLAGS     (SQTEXTENC_NATIVE | SQTEXTENC_STRICT)

//#define SQTEXTENC_N_UTF16   (SQTEXTENC_UTF16BE | SQTEXTENC_NATIVE)   // Native
//#define SQTEXTENC_N_UTF32   (SQTEXTENC_UTF32BE | SQTEXTENC_NATIVE)   // Native

typedef struct SQSTDTEXTCV_tag {
    const void *text;
    SQUnsignedInteger allocated;
    SQInteger measure;
} SQSTDTEXTCV;

SQUIRREL_API SQInteger sqstd_text_encbyname( const SQChar *name);
SQUIRREL_API const SQChar* sqstd_text_encname( SQInteger enc);
SQUIRREL_API SQInteger sqstd_text_defaultenc( void);
SQUIRREL_API SQInteger sqstd_text_nativeenc( SQInteger enc);

SQUIRREL_API SQInteger sqstd_text_fromutf( int encoding, const void *inbuf, SQInteger inbuf_size, SQSTDTEXTCV *textcv);
SQUIRREL_API SQInteger sqstd_text_toutf( int encoding, const SQChar *str, SQInteger str_len, SQSTDTEXTCV *textcv);
SQUIRREL_API void sqstd_text_release( SQSTDTEXTCV *textcv);

SQUIRREL_API SQInteger sqstd_text_get(HSQUIRRELVM v, SQInteger idx, int encoding, SQSTDTEXTCV *textcv);
SQUIRREL_API SQInteger sqstd_text_push(HSQUIRRELVM v, int encoding, const void *inbuf, SQInteger inbuf_size);

typedef void *SQSTDTEXTRD;
SQUIRREL_API SQSTDTEXTRD sqstd_text_reader( SQSTREAM stream, int encoding, SQBool read_bom, const SQChar *bad_char);
SQUIRREL_API void sqstd_text_readerrelease( SQSTDTEXTRD reader);
SQUIRREL_API SQInteger sqstd_text_peekchar( SQSTDTEXTRD reader, SQChar *pc);
SQUIRREL_API SQInteger sqstd_text_readchar( SQSTDTEXTRD reader, SQChar *pc);
SQUIRREL_API SQInteger sqstd_text_readline( SQSTDTEXTRD reader, SQChar **pbuff, SQChar * const buff_end);
SQUIRREL_API SQInteger sqstd_text_rderrors( SQSTDTEXTRD reader);
SQUIRREL_API SQInteger sqstd_text_rdeof( SQSTDTEXTRD reader);

typedef void *SQSTDTEXTWR;
SQSTDTEXTWR sqstd_text_writer( SQSTREAM stream, int encoding, SQBool write_bom, const SQChar *bad_char);
SQUIRREL_API void sqstd_text_writerrelease( SQSTDTEXTWR writer);
SQUIRREL_API SQInteger sqstd_text_writechar( SQSTDTEXTWR writer, SQChar c);
SQUIRREL_API SQInteger sqstd_text_writestring( SQSTDTEXTWR writer, const SQChar *str, SQInteger len);
SQUIRREL_API SQInteger sqstd_text_wrerrors( SQSTDTEXTWR reader);

SQUIRREL_API SQUserPointer _sqstd_textrd_type_tag(void);
#define SQSTD_TEXTRD_TYPE_TAG _sqstd_textrd_type_tag()

SQUIRREL_API SQUserPointer _sqstd_textwr_type_tag(void);
#define SQSTD_TEXTWR_TYPE_TAG _sqstd_textwr_type_tag()

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // _SQSTDTEXT_H_
