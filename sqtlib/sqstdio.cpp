/* see copyright notice in squirrel.h */
/* see copyright notice in sqtool.h */
#include <new>
#include <stdio.h>
#include <squirrel.h>
#include <sqtool.h>
#include <sqstdio.h>
#include "sqstdstream.h"

//#define SQSTD_FILE_TYPE_TAG (SQSTD_STREAM_TYPE_TAG | 0x00000001)

//File
struct SQFile : public SQStream {
    SQFile() { _handle = NULL; _owns = false;}
    SQFile( FILE *file, bool owns) { _handle = file; _owns = owns;}
//    virtual ~SQFile() { Close(); }
	
    bool Open(const SQChar *filename ,const SQChar *mode) {
        Close();
#ifdef _WIN32
		const SQChar *wstr[2] = { filename, mode };
		SQUnsignedInteger wlen;
		sqt_strtowc( 2, (SQChar**)wstr, &wlen);
		_handle = _wfopen( (wchar_t*)wstr[0], (wchar_t*)wstr[1]);
		sq_free( (void*)wstr[0], wlen);
#else
		_handle = fopen( filename, mode);
#endif
        if( _handle) {
            _owns = true;
            return true;
        }
        return false;
    }
    SQInteger Close() {
		SQInteger r = 0;
        if( _handle && _owns) {
            r = fclose( _handle);
        }
        _handle = NULL;
        _owns = false;
		return r;
    }
	void _Release() {
		Close();
		this->~SQFile();
		sq_free(this,sizeof(SQFile));
	}
	
    SQInteger Read(void *buffer,SQInteger size) {
	    return (SQInteger)fread( buffer, 1, size, _handle);
    }
    SQInteger Write(void *buffer,SQInteger size) {
	    return (SQInteger)fwrite( buffer, 1, size, _handle);
    }
    SQInteger Flush() {
	    return fflush( _handle);
    }
    SQInteger Tell() {
	    return ftell( _handle);
    }
    SQInteger Seek(SQInteger offset, SQInteger origin)  {
		int realorigin;
		switch(origin) {
			case SQ_SEEK_CUR: realorigin = SEEK_CUR; break;
			case SQ_SEEK_END: realorigin = SEEK_END; break;
			case SQ_SEEK_SET: realorigin = SEEK_SET; break;
			default: return -1; //failed
		}
		return fseek( _handle, (long)offset, (int)realorigin);
    }
    SQInteger Len() {
        SQInteger prevpos=Tell();
        Seek(0,SQ_SEEK_END);
        SQInteger size=Tell();
        Seek(prevpos,SQ_SEEK_SET);
        return size;
    }
    bool IsValid() { return _handle ? true : false; }
    bool EOS() { return feof( _handle); }
    FILE *GetHandle() {return _handle;}
protected:
    FILE *_handle;
    bool _owns;
};

SQFILE sqstd_fopen(const SQChar *filename ,const SQChar *mode)
{
    SQFile *f;
	
    f = new (sq_malloc(sizeof(SQFile)))SQFile();
	f->Open( filename, mode);
	if( f->IsValid())
	{
		return (SQFILE)f;
	}
	else
	{
		f->_Release();
		return NULL;
	}
}

struct SQFPipe : public SQFile {
    SQFPipe() { _handle = NULL; _owns = false;}
    SQFPipe( FILE *file, bool owns) { _handle = file; _owns = owns;}
    bool Open(const SQChar *command ,const SQChar *mode) {
        Close();
#ifdef _WIN32
		const SQChar *wstr[2] = { command, mode };
		SQUnsignedInteger wlen;
		sqt_strtowc( 2, (SQChar**)wstr, &wlen);
		_handle = _wpopen( (wchar_t*)wstr[0], (wchar_t*)wstr[1]);
		sq_free( (void*)wstr[0], wlen);
#else
		_handle = popen( command, mode);
#endif
        if( _handle) {
            _owns = true;
            return true;
        }
        return false;
    }
	
    SQInteger Close() {
		SQInteger r = 0;
        if( _handle && _owns) {
#ifdef _WIN32
            r = _pclose( _handle);
#else
            r = pclose( _handle);
#endif
        }
        _handle = NULL;
        _owns = false;
		return r;
    }
	
	void _Release() {
		Close();
		this->~SQFPipe();
		sq_free(this,sizeof(SQFPipe));
	}
};


SQFILE sqstd_popen(const SQChar *command ,const SQChar *mode)
{
    SQFPipe *f;
	
    f = new (sq_malloc(sizeof(SQFPipe)))SQFPipe();
	f->Open( command, mode);
	if( f->IsValid())
	{
		return (SQFILE)f;
	}
	else
	{
		f->_Release();
		return NULL;
	}
}

static SQInteger _file__typeof(HSQUIRRELVM v)
{
    sq_pushstring(v,std_file_decl.name,-1);
    return 1;
}

static SQInteger _file_constructor(HSQUIRRELVM v)
{
    const SQChar *filename,*mode;
    SQFile *f;
    if(sq_gettype(v,2) == OT_STRING && sq_gettype(v,3) == OT_STRING) {
        sq_getstring(v, 2, &filename);
        sq_getstring(v, 3, &mode);
	    f = new (sq_malloc(sizeof(SQFile)))SQFile();
		f->Open( filename, mode);
		if( !f->IsValid())
		{
	        sq_free(f,sizeof(SQFile));
			return sq_throwerror(v, _SC("cannot open file"));
		}
    } else if(sq_gettype(v,2) == OT_USERPOINTER) {
	    FILE *fh;
	    bool owns = true;
        sq_getuserpointer(v,2,(SQUserPointer*)&fh);
        owns = !(sq_gettype(v,3) == OT_NULL);
	    f = new (sq_malloc(sizeof(SQFile)))SQFile(fh,owns);
    } else {
        return sq_throwerror(v,_SC("wrong parameter"));
    }

    if(SQ_FAILED(sq_setinstanceup(v,1,f))) {
        //f->~SQFile();
        //sq_free(f,sizeof(SQFile));
		f->_Release();
        return sq_throwerror(v, _SC("cannot create file instance"));
    }
    sq_setreleasehook(v,1,__sqstd_stream_releasehook);
    return 0;
}

//bindings
#define _DECL_FILE_FUNC(name,nparams,typecheck) {_SC(#name),_file_##name,nparams,typecheck}
static const SQRegFunction _file_methods[] = {
    _DECL_FILE_FUNC(constructor,3,_SC("x")),
    _DECL_FILE_FUNC(_typeof,1,_SC("x")),
    {NULL,(SQFUNCTION)0,0,NULL}
};

const SQTClassDecl std_file_decl = {
	&std_stream_decl,	// base_class
    _SC("std_file"),	// reg_name
    _SC("file"),		// name
	_file_methods,		// methods
	NULL,				// globals
};

static SQInteger _popen__typeof(HSQUIRRELVM v)
{
    sq_pushstring(v,std_popen_decl.name,-1);
    return 1;
}

static SQInteger _popen_constructor(HSQUIRRELVM v)
{
    const SQChar *command,*mode;
    SQFPipe *f;
    if(sq_gettype(v,2) == OT_STRING && sq_gettype(v,3) == OT_STRING) {
        sq_getstring(v, 2, &command);
        sq_getstring(v, 3, &mode);
	    f = new (sq_malloc(sizeof(SQFPipe)))SQFPipe();
		f->Open( command, mode);
		if( !f->IsValid())
		{
	        sq_free(f,sizeof(SQFile));
			return sq_throwerror(v, _SC("cannot popen command"));
		}
    } else if(sq_gettype(v,2) == OT_USERPOINTER) {
	    FILE *fh;
	    bool owns = true;
        sq_getuserpointer(v,2,(SQUserPointer*)&fh);
        owns = !(sq_gettype(v,3) == OT_NULL);
	    f = new (sq_malloc(sizeof(SQFPipe)))SQFPipe(fh,owns);
    } else {
        return sq_throwerror(v,_SC("wrong parameter"));
    }

    if(SQ_FAILED(sq_setinstanceup(v,1,f))) {
        //f->~SQFile();
        //sq_free(f,sizeof(SQFile));
		f->_Release();
        return sq_throwerror(v, _SC("cannot create popen instance"));
    }
    sq_setreleasehook(v,1,__sqstd_stream_releasehook);
    return 0;
}

//bindings
#define _DECL_POPEN_FUNC(name,nparams,typecheck) {_SC(#name),_popen_##name,nparams,typecheck}
static const SQRegFunction _popen_methods[] = {
    _DECL_POPEN_FUNC(constructor,3,_SC("x")),
    _DECL_POPEN_FUNC(_typeof,1,_SC("x")),
    {NULL,(SQFUNCTION)0,0,NULL}
};

const SQTClassDecl std_popen_decl = {
	&std_file_decl,	// base_class
    _SC("std_popen"),	// reg_name
    _SC("popen"),		// name
	_popen_methods,		// methods
	NULL,				// globals
};

SQRESULT sqstd_createfile(HSQUIRRELVM v, FILE *file, SQBool own)
{
    SQInteger top = sq_gettop(v);
    sq_pushregistrytable(v);
    sq_pushstring(v,std_file_decl.reg_name,-1);
    if(SQ_SUCCEEDED(sq_get(v,-2))) {
        sq_remove(v,-2); //removes the registry
        sq_pushroottable(v); // push the this
        sq_pushuserpointer(v,file); //file
        if(own){
            sq_pushinteger(v,1); //true
        }
        else{
            sq_pushnull(v); //false
        }
        if(SQ_SUCCEEDED( sq_call(v,3,SQTrue,SQFalse) )) {
            sq_remove(v,-2);
            return SQ_OK;
        }
    }
    sq_settop(v,top);
    return SQ_ERROR;
}

SQRESULT sqstd_getfile(HSQUIRRELVM v, SQInteger idx, FILE **file)
{
    SQFile *fileobj = NULL;
    if(SQ_SUCCEEDED(sq_getinstanceup(v,idx,(SQUserPointer*)&fileobj,(SQUserPointer)SQSTD_FILE_TYPE_TAG))) {
        *file = fileobj->GetHandle();
        return SQ_OK;
    }
    return sq_throwerror(v,_SC("not a file"));
}

SQRESULT sqstd_register_iolib(HSQUIRRELVM v)
{
    SQInteger top = sq_gettop(v);
	if(SQ_FAILED(sqt_declareclass(v,&std_file_decl)))
	{
		return SQ_ERROR;
	}
	sq_poptop(v);
	if(SQ_FAILED(sqt_declareclass(v,&std_popen_decl)))
	{
		return SQ_ERROR;
	}
	sq_poptop(v);
    //create delegate
    //declare_stream(v,_SC("file"),(SQUserPointer)SQSTD_FILE_TYPE_TAG,_SC("std_file"),_file_methods,iolib_funcs);
    //declare_stream(v,_SC("popen"),(SQUserPointer)SQSTD_FILE_TYPE_TAG,_SC("std_popen"),_popen_methods,NULL);
    sq_pushstring(v,_SC("stdout"),-1);
    sqstd_createfile(v,stdout,SQFalse);
    sq_newslot(v,-3,SQFalse);
	
    sq_pushstring(v,_SC("stdin"),-1);
    sqstd_createfile(v,stdin,SQFalse);
    sq_newslot(v,-3,SQFalse);
	
    sq_pushstring(v,_SC("stderr"),-1);
    sqstd_createfile(v,stderr,SQFalse);
    sq_newslot(v,-3,SQFalse);
	
    sq_settop(v,top);
    return SQ_OK;
}