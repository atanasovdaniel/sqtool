/*  see copyright notice in squirrel.h */
/* see copyright notice in sqtool.h */
#ifndef _SQSTDIO_H_
#define _SQSTDIO_H_

#ifdef __cplusplus

struct SQStream {
    virtual SQInteger Read(void *buffer, SQInteger size) = 0;
    virtual SQInteger Write(void *buffer, SQInteger size) = 0;
    virtual SQInteger Flush() = 0;
    virtual SQInteger Tell() = 0;
    virtual SQInteger Len() = 0;
    virtual SQInteger Seek(SQInteger offset, SQInteger origin) = 0;
    virtual bool IsValid() = 0;
    virtual bool EOS() = 0;
    virtual void _Release() = 0;
    virtual SQInteger Close() { return 0; };
};

extern "C" {
#endif

extern SQUIRREL_API_VAR const SQTClassDecl std_stream_decl;
#define SQSTD_STREAM_TYPE_TAG ((SQUserPointer)(SQHash)&std_stream_decl)
extern SQUIRREL_API_VAR const SQTClassDecl std_file_decl;
#define SQSTD_FILE_TYPE_TAG ((SQUserPointer)(SQHash)&std_file_decl)
extern SQUIRREL_API_VAR const SQTClassDecl std_popen_decl;
#define SQSTD_POPEN_TYPE_TAG ((SQUserPointer)(SQHash)&std_popen_decl)
extern SQUIRREL_API_VAR const SQTClassDecl std_textreader_decl;
#define SQSTD_TEXTREADER_TYPE_TAG ((SQUserPointer)(SQHash)&std_textreader_decl)
extern SQUIRREL_API_VAR const SQTClassDecl std_textwriter_decl;
#define SQSTD_TEXTWRITER_TYPE_TAG ((SQUserPointer)(SQHash)&std_textwriter_decl)

#define SQ_SEEK_CUR 0
#define SQ_SEEK_END 1
#define SQ_SEEK_SET 2

//typedef void* SQFILE;
typedef struct SQFILE_tag *SQFILE;	// SQFILE represents SQStream

SQUIRREL_API SQInteger sqstd_fread(SQUserPointer, SQInteger, SQFILE);
SQUIRREL_API SQInteger sqstd_fwrite(const SQUserPointer, SQInteger, SQFILE);
SQUIRREL_API SQInteger sqstd_fseek(SQFILE , SQInteger , SQInteger);
SQUIRREL_API SQInteger sqstd_ftell(SQFILE);
SQUIRREL_API SQInteger sqstd_fflush(SQFILE);
SQUIRREL_API SQInteger sqstd_feof(SQFILE);
SQUIRREL_API SQInteger sqstd_fclose(SQFILE);
SQUIRREL_API void sqstd_frelease(SQFILE);

SQUIRREL_API SQInteger __sqstd_stream_releasehook(SQUserPointer p, SQInteger SQ_UNUSED_ARG(size));

SQUIRREL_API SQFILE sqstd_fopen(const SQChar *,const SQChar *);
SQUIRREL_API SQRESULT sqstd_createfile(HSQUIRRELVM v, SQUserPointer file, SQBool own);
SQUIRREL_API SQRESULT sqstd_getfile(HSQUIRRELVM v, SQInteger idx, SQUserPointer *file);

SQUIRREL_API SQFILE sqstd_blob(SQInteger size);

SQUIRREL_API SQFILE sqt_textreader( SQFILE stream, SQBool owns, const SQChar *encoding, SQBool guess);
SQUIRREL_API SQFILE sqt_textwriter( SQFILE stream, SQBool owns, const SQChar *encoding);

//compiler helpers
SQUIRREL_API SQRESULT sqt_compile_stream(HSQUIRRELVM v,SQFILE stream,const SQChar *sourcename,SQBool raiseerror);
SQUIRREL_API SQRESULT sqt_writeclosure_stream(HSQUIRRELVM vm,SQFILE stream);
SQUIRREL_API SQRESULT sqt_readclosure_stream(HSQUIRRELVM vm,SQFILE stream);
SQUIRREL_API SQRESULT sqt_loadfile_stream(HSQUIRRELVM v, SQFILE stream, const SQChar *filename, SQBool printerror,
										  SQInteger buf_size, const SQChar *encoding, SQBool guess);

SQUIRREL_API SQRESULT sqstd_loadfile(HSQUIRRELVM v,const SQChar *filename,SQBool printerror);
SQUIRREL_API SQRESULT sqstd_dofile(HSQUIRRELVM v,const SQChar *filename,SQBool retval,SQBool printerror);
SQUIRREL_API SQRESULT sqstd_writeclosuretofile(HSQUIRRELVM v,const SQChar *filename);

SQUIRREL_API SQRESULT sqstd_register_iolib(HSQUIRRELVM v);
SQUIRREL_API SQRESULT sqstd_register_squirrelio(HSQUIRRELVM v);
SQUIRREL_API SQRESULT sqstd_register_textreader(HSQUIRRELVM v);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*_SQSTDIO_H_*/

