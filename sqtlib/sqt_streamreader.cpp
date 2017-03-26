/* see copyright notice in sqtool.h */
#include <new>
#include <stddef.h>
#include <string.h>
#include <squirrel.h>
#include <sqtool.h>
#include <sqstdio.h>
#include "sqtstreamreader.h"

static HSQMEMBERHANDLE srdr__stream_handle;

struct SQTStreamReader : public SQStream
{
	SQTStreamReader() { _stream = NULL; _owns = SQFalse; _mark_buff = NULL; };
	SQTStreamReader( SQStream *stream, SQBool owns) { _stream = stream; _owns = owns; _mark_buff = NULL; };
	
    void SetStream( SQStream *stream, SQBool owns) { Close(); _stream = stream; _owns = owns; };
	
    SQInteger Write(void *buffer, SQInteger size) { return -1; };
    SQInteger Flush()	{ return 0; };
    SQInteger Tell()	{ return -1; };
    SQInteger Len()		{ return -1; };
    SQInteger Seek(SQInteger offset, SQInteger origin)	{ return -1; };
    bool IsValid()		{ return (_stream != NULL) && _stream->IsValid(); }
    bool EOS()			{ return ((_mark_buff == NULL) || (_mark_rpos == -1)) && ((_stream == NULL) || _stream->EOS()); }

    SQInteger Close() {
		SQInteger r = 0;
        if( _stream && _owns) {
            r = _stream->Close();
        }
        _stream = NULL;
        _owns = false;
		return r;
    }
	~SQTStreamReader()
	{
		if( _mark_buff != NULL) {
			sq_free( _mark_buff, _mark_len);
			_mark_buff = NULL;
		}
	}
	void _Release() {
		this->~SQTStreamReader();
		sq_free(this,sizeof(SQTStreamReader));
	}
	
	SQInteger Mark( SQInteger readAheadLimit)
	{
		if( (_mark_buff != NULL) && (_mark_rpos != -1)) {
			// mark while reading _mark_buffer
			if( _mark_len < readAheadLimit) {
				_mark_buff = (SQChar*)sq_realloc( _mark_buff, _mark_len, readAheadLimit);
				_mark_len = readAheadLimit;
			}
			memmove( _mark_buff, _mark_buff + _mark_rpos, _mark_wpos - _mark_rpos);
			_mark_wpos -= _mark_rpos;
			_mark_rpos = 0;
		}
		else {
			// new _mark_buffer
			if( _mark_buff != NULL) {
				// free unused _mark_buff
				sq_free( _mark_buff, _mark_len);
			}
			_mark_buff = (SQChar*)sq_malloc( readAheadLimit);
			_mark_len = readAheadLimit;
			_mark_wpos = 0;
			_mark_rpos = -1;
		}
		return SQ_OK;
	}
	
	SQInteger Reset()
	{
		if( _mark_buff != NULL) {
			_mark_rpos = 0;
			return SQ_OK;
		}
		else {
			return SQ_ERROR;
		}
	}

	SQInteger Read(void *buffer, SQInteger size)
	{
		SQInteger r;
	
		if( _mark_buff == NULL) {
			r = _stream->Read( buffer, size);
		}
		else if( _mark_rpos != -1) {
			// read from _mark_buff
			r = size;
			if( size > (_mark_wpos - _mark_rpos))
			{
				r = (_mark_wpos - _mark_rpos);
			}
			memcpy( buffer, _mark_buff + _mark_rpos, r);
			_mark_rpos += r;
			size -= r;
			if( size != 0) {
				// read behind _mark_wpos
				buffer = (SQChar*)buffer + r;
				r += _stream->Read( buffer, size);
				if( r <= _mark_len) {
					// store to _mark_buffer
					memcpy( _mark_buff + _mark_wpos, buffer, size);
					_mark_wpos += size;
					_mark_rpos = -1;
				}
				else {
					// read behind _mark_buffer
					sq_free( _mark_buff, _mark_len);
					_mark_buff = NULL;
				}
			}
		}
		else {
			// read and copy to _mark_buff
			r = _stream->Read( buffer, size);
			if( r <= (_mark_len - _mark_wpos)) {
				memcpy( _mark_buff + _mark_wpos, buffer, r);
				_mark_wpos += r;
			}
			else {
				// read after readAheadLimit
				sq_free( _mark_buff, _mark_len);
				_mark_buff = NULL;
			}
		}
		return r;
	}

protected:
	SQStream *_stream;
	SQBool _owns;
	SQChar *_mark_buff;
	SQInteger _mark_len;
	SQInteger _mark_wpos;
	SQInteger _mark_rpos;
};

SQT_SRDR sqtsrdr_create( SQFILE stream, SQBool owns)
{
	SQTStreamReader *srdr;
	
    srdr = new (sq_malloc(sizeof(SQTStreamReader)))SQTStreamReader( (SQStream*)stream, owns);
	if( srdr->IsValid())
	{
		return (SQT_SRDR)srdr;
	}
	else
	{
		srdr->_Release();
		return NULL;
	}
}

SQUIRREL_API SQInteger sqtsrdr_mark( SQT_SRDR srdr, SQInteger readAheadLimit)
{
	SQTStreamReader *self = (SQTStreamReader *)srdr;
	return self->Mark(readAheadLimit);
}

SQUIRREL_API SQInteger sqtsrdr_reset( SQT_SRDR srdr)
{
	SQTStreamReader *self = (SQTStreamReader *)srdr;
	return self->Reset();
}

static SQInteger _srdr__typeof(HSQUIRRELVM v)
{
    sq_pushstring(v,sqt_streamrdr_decl.name,-1);
    return 1;
}

static SQInteger _srdr_constructor(HSQUIRRELVM v)
{
	SQTStreamReader *srdr;
	SQFILE stream;
	SQBool owns;
    if( SQ_FAILED( sq_getinstanceup( v,2,(SQUserPointer*)&stream,(SQUserPointer)SQSTD_STREAM_TYPE_TAG))) {
        return sq_throwerror(v,_SC("invalid argument type"));
	}
	sq_getbool(v,3,&owns);
	srdr = (SQTStreamReader*)sqtsrdr_create( stream, owns);
	if( srdr != NULL) {
		if(SQ_FAILED(sq_setinstanceup(v,1,srdr))) {
			srdr->_Release();
			return sq_throwerror(v, _SC("cannot create streamreader instance"));
		}
		sq_setreleasehook(v,1,__sqstd_stream_releasehook);
		// save stream in _stream member
		sq_push(v,2);
		sq_setbyhandle(v,1,&srdr__stream_handle);
		return 0;
	}
	return sq_throwerror(v, _SC("cannot create streamreader"));
}

#define SETUP_STREAMRDR(v) \
    SQTStreamReader *self = NULL; \
    if(SQ_FAILED(sq_getinstanceup(v,1,(SQUserPointer*)&self,(SQUserPointer)SQT_STREAMREADER_TYPE_TAG))) \
        return sq_throwerror(v,_SC("invalid type tag")); \
    if(!self || !self->IsValid())  \
        return sq_throwerror(v,_SC("the streamreader is invalid"));

static SQInteger _srdr_mark(HSQUIRRELVM v)
{
	SETUP_STREAMRDR(v);
	SQInteger readAheadLimit;
	sq_getinteger(v,2,&readAheadLimit);
	sq_pushinteger(v, self->Mark(readAheadLimit));
    return 1;
}

static SQInteger _srdr_reset(HSQUIRRELVM v)
{
	SETUP_STREAMRDR(v);
	sq_pushinteger(v, self->Reset());
    return 1;
}

static SQInteger _srdr_close(HSQUIRRELVM v)
{
	SETUP_STREAMRDR(v);
	
	// release stream from _stream member
	sq_pushnull(v);
	sq_setbyhandle(v,1,&srdr__stream_handle);
	
	sq_pushinteger(v, self->Close());
    return 1;
}

//bindings
#define _DECL_SRDR_FUNC(name,nparams,typecheck) {_SC(#name),_srdr_##name,nparams,typecheck}
static const SQRegFunction _srdr_methods[] = {
    _DECL_SRDR_FUNC(constructor,3,_SC("xxb")),
    _DECL_SRDR_FUNC(_typeof,1,_SC("x")),
    _DECL_SRDR_FUNC(mark,2,_SC("xi")),
    _DECL_SRDR_FUNC(reset,1,_SC("x")),
    _DECL_SRDR_FUNC(close,1,_SC("x")),
    {NULL,(SQFUNCTION)0,0,NULL}
};

const SQTClassDecl sqt_streamrdr_decl = {
	&std_stream_decl,	// base_class
    _SC("sqt_streamreader"),	// reg_name
    _SC("streamreader"),		// name
	_srdr_methods,		// methods
	NULL,				// globals
};

SQUIRREL_API SQRESULT sqstd_register_streamrdr(HSQUIRRELVM v)
{
	if(SQ_FAILED(sqt_declareclass(v,&sqt_streamrdr_decl)))
	{
		return SQ_ERROR;
	}
	//sq_newmember(...)
	sq_pushstring(v,_SC("_stream"),-1);					// root, class, name
	sq_pushnull(v);										// root, class, name, value
	sq_pushnull(v);										// root, class, name, value, attribute
	sq_newmember(v,-4,SQFalse);							// root, class, [name, value, attribute] - bay be a bug (name, value, attribute not poped)
	sq_pop(v,3);										// root, class							 - workaround
	sq_pushstring(v,_SC("_stream"),-1);					// root, class, name
	sq_getmemberhandle(v,-2, &srdr__stream_handle);		// root, class
	sq_poptop(v);										// root
	return SQ_OK;
}


