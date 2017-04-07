/* see copyright notice in sqtool.h */
#include <stddef.h>
#include <squirrel.h>
#include <sqtool.h>
#include <sqstdio.h>
#include <sqtstreamreader.h>

#define IO_BUFFER_SIZE 2048

// typedef SQInteger (*SQLEXREADFUNC)(SQUserPointer);
// typedef SQInteger (*SQWRITEFUNC)(SQUserPointer,SQUserPointer,SQInteger);
// typedef SQInteger (*SQREADFUNC)(SQUserPointer,SQUserPointer,SQInteger);
//
// SQUIRREL_API SQRESULT sq_compile(HSQUIRRELVM v,SQLEXREADFUNC read,SQUserPointer p,const SQChar *sourcename,SQBool raiseerror);
// SQUIRREL_API SQRESULT sq_writeclosure(HSQUIRRELVM vm,SQWRITEFUNC writef,SQUserPointer up);
// SQUIRREL_API SQRESULT sq_readclosure(HSQUIRRELVM vm,SQREADFUNC readf,SQUserPointer up);

static SQInteger sqt_SQLEXREADFUNC( SQUserPointer user)
{
	SQFILE s = (SQFILE)user;
	unsigned char c;
	if( sqstd_fread( &c, 1, s) == 1)
		return c;
	return 0;
}

static SQInteger sqt_SQWRITEFUNC( SQUserPointer user, SQUserPointer buf, SQInteger size)
{
	SQFILE s = (SQFILE)user;
    return sqstd_fwrite( buf, size, s);
}

static SQInteger sqt_SQREADFUNC( SQUserPointer user, SQUserPointer buf, SQInteger size)
{
	SQFILE s = (SQFILE)user;
    SQInteger ret;
    if( (ret = sqstd_fread( buf, size, s))!=0 ) return ret;
    return -1;
}

static SQRESULT sqt_loadfile_srdr(HSQUIRRELVM v, SQT_SRDR srdr, const SQChar *filename, SQBool printerror)
{
    SQInteger ret;
    unsigned short us;

    if( sqtsrdr_mark( srdr, 16) == 0) {
		ret = sqstd_fread(&us,2,(SQFILE)srdr);
		if(ret != 2) {
			//probably an empty file
			us = 0;
		}
		sqtsrdr_reset( srdr);
		if(us == SQ_BYTECODE_STREAM_TAG) { //BYTECODE
			if(SQ_SUCCEEDED(sq_readclosure(v,sqt_SQREADFUNC,srdr))) {
				return SQ_OK;
			}
		}
		else { //SCRIPT
			if(SQ_SUCCEEDED(sq_compile( v, sqt_SQLEXREADFUNC, srdr, filename, printerror))){
				return SQ_OK;
			}
		}
    }
    return sq_throwerror(v,_SC("cannot load the file"));
}

// file io

SQRESULT sqt_loadfile(HSQUIRRELVM v, SQFILE file, const SQChar *filename, SQBool printerror)
{
	SQRESULT r;
	SQT_SRDR srdr;
	
	srdr = sqtsrdr_create( file, SQFalse, IO_BUFFER_SIZE);
	if( srdr != NULL) {
		r = sqt_loadfile_srdr( v, srdr, filename, printerror);
		sqstd_frelease( (SQFILE)srdr);
		return r;
	}
    return sq_throwerror(v,_SC("cannot create reader"));
}

SQRESULT sqt_dofile(HSQUIRRELVM v, SQFILE file, const SQChar *filename,SQBool retval,SQBool printerror)
{
    if(SQ_SUCCEEDED(sqt_loadfile(v,file,filename,printerror))) {
        sq_push(v,-2);
        if(SQ_SUCCEEDED(sq_call(v,1,retval,SQTrue))) {
            sq_remove(v,retval?-2:-1); //removes the closure
            return 1;
        }
        sq_pop(v,1); //removes the closure
    }
    return SQ_ERROR;
}

SQRESULT sqt_writeclosure( HSQUIRRELVM v, SQFILE file)
{
    if(SQ_SUCCEEDED(sq_writeclosure(v,sqt_SQWRITEFUNC,file))) {
        return SQ_OK;
    }
    return SQ_ERROR; //forward the error
}

// sq api

SQRESULT sqstd_loadfile(HSQUIRRELVM v,const SQChar *filename,SQBool printerror)
{
    SQRESULT ret;
    SQFILE file = sqstd_fopen(filename,_SC("rb"));
	if( file) {
		ret = sqt_loadfile( v, file, filename, printerror);
		sqstd_frelease( file);
		return ret;
	}
    return sq_throwerror(v,_SC("cannot open the file"));
}

SQRESULT sqstd_dofile(HSQUIRRELVM v,const SQChar *filename, SQBool retval,SQBool printerror)
{
    SQRESULT ret;
    SQFILE file = sqstd_fopen(filename,_SC("rb"));
	if( file) {
		ret = sqt_dofile( v, file, filename, retval, printerror);
		sqstd_frelease( file);
		return ret;
	}
    return sq_throwerror(v,_SC("cannot open the file"));
}

SQRESULT sqstd_writeclosuretofile(HSQUIRRELVM v,const SQChar *filename)
{
    SQFILE file = sqstd_fopen(filename,_SC("wb+"));
    if(!file) return sq_throwerror(v,_SC("cannot open the file"));
    if(SQ_SUCCEEDED(sqt_writeclosure(v,file))) {
        sqstd_frelease(file);
        return SQ_OK;
    }
    sqstd_frelease(file);
    return SQ_ERROR; //forward the error
}

// bindings

static SQInteger _g_stream_loadfile(HSQUIRRELVM v)
{
    SQBool printerror = SQFalse;
    if(sq_gettop(v) >= 3) {
        sq_getbool(v,3,&printerror);
    }
	if( sq_gettype(v,2) == OT_INSTANCE) {
		SQFILE file;
	    if( SQ_FAILED( sq_getinstanceup( v,2,(SQUserPointer*)&file,(SQUserPointer)SQSTD_STREAM_TYPE_TAG))) {
	        return sq_throwerror(v,_SC("invalid argument type"));
		}
		
		if(SQ_SUCCEEDED(sqt_loadfile(v,file,_SC("stream"),printerror)))
			return 1;
	}
	else {
	    const SQChar *filename;
		sq_getstring(v,2,&filename);
		if(SQ_SUCCEEDED(sqstd_loadfile(v,filename,printerror)))
			return 1;
	}
    return SQ_ERROR; //propagates the error
}

static SQInteger _g_stream_dofile(HSQUIRRELVM v)
{
    SQBool printerror = SQFalse;
    if(sq_gettop(v) >= 3) {
        sq_getbool(v,3,&printerror);
    }
    sq_push(v,1); //repush the this
	
	if( sq_gettype(v,2) == OT_INSTANCE) {
		SQFILE file;
	    if( SQ_FAILED( sq_getinstanceup( v,2,(SQUserPointer*)&file,(SQUserPointer)SQSTD_STREAM_TYPE_TAG))) {
	        return sq_throwerror(v,_SC("invalid argument type"));
		}
		if(SQ_SUCCEEDED(sqt_dofile(v,file,_SC("stream"),SQTrue,printerror)))
			return 1;
	}
	else {
		const SQChar *filename;
		sq_getstring(v,2,&filename);
		if(SQ_SUCCEEDED(sqstd_dofile(v,filename,SQTrue,printerror)))
			return 1;
	}
    return SQ_ERROR; //propagates the error
}

static SQInteger _g_stream_writeclosuretofile(HSQUIRRELVM v)
{
	if( sq_gettype(v,2) == OT_INSTANCE) {
		SQFILE file;
	    if( SQ_FAILED( sq_getinstanceup( v,2,(SQUserPointer*)&file,(SQUserPointer)SQSTD_STREAM_TYPE_TAG))) {
	        return sq_throwerror(v,_SC("invalid argument type"));
		}
		if(SQ_SUCCEEDED(sqt_writeclosure(v,file)))
			return 1;
	}
	else {
		const SQChar *filename;
		sq_getstring(v,2,&filename);
		if(SQ_SUCCEEDED(sqstd_writeclosuretofile(v,filename)))
			return 1;
	}
    return SQ_ERROR; //propagates the error
}

#define _DECL_GLOBALSTREAM_FUNC(name,nparams,typecheck) {_SC(#name),_g_stream_##name,nparams,typecheck}
static const SQRegFunction _stream_funcs[]={
    _DECL_GLOBALSTREAM_FUNC(loadfile,-2,_SC(".s|xb")),
    _DECL_GLOBALSTREAM_FUNC(dofile,-2,_SC(".s|xb")),
    _DECL_GLOBALSTREAM_FUNC(writeclosuretofile,3,_SC(".s|xc")),
    {NULL,(SQFUNCTION)0,0,NULL}
};

SQRESULT sqstd_register_squirrelio(HSQUIRRELVM v)
{
	return sqt_declarefunctions(v, _stream_funcs);
}
