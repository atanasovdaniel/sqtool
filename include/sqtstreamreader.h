#ifndef __SQTSTREAMREADER_H__
#define __SQTSTREAMREADER_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SQT_SRDR_tag *SQT_SRDR;

SQUIRREL_API SQT_SRDR sqtsrdr_create( SQFILE stream, SQBool owns, SQInteger buffer_size);
SQUIRREL_API SQInteger sqtsrdr_mark( SQT_SRDR srdr, SQInteger readAheadLimit);
SQUIRREL_API SQInteger sqtsrdr_reset( SQT_SRDR srdr);

SQUIRREL_API SQRESULT sqstd_register_streamrdr(HSQUIRRELVM v);

extern SQUIRREL_API_VAR const SQTClassDecl sqt_streamrdr_decl;
#define SQT_STREAMREADER_TYPE_TAG ((SQUserPointer)&sqt_streamrdr_decl)

SQUIRREL_API const SQChar *sqt_guessencoding( SQT_SRDR srdr);
SQUIRREL_API SQFILE sqt_textreader_sr( SQT_SRDR srdr, SQBool owns_close, SQBool owns_release, const SQChar *encoding, SQBool guess);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // __SQTSTREAMREADER_H__
