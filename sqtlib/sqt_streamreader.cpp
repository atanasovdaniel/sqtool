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
//	SQTStreamReader() { _stream = NULL; _owns = SQFalse; _buf = NULL; _is_buffered = SQFlase; _buf = NULL;};
	SQTStreamReader( SQStream *stream, SQBool owns, SQInteger buffer_size)
	{
		_stream = stream;
		_owns = owns;
		_mark_end = -1;
		_buf_pos = 0;
		_buf_len = 0;
		if( buffer_size > 0) {
			_is_buffered = SQTrue;
			_buf = (unsigned char*)sq_malloc( buffer_size);
			_buf_size = buffer_size;
		}
		else {
			_is_buffered = SQFalse;
			_buf = NULL;
			_buf_size = 0;
		}
	};

    void SetStream( SQStream *stream, SQBool owns) { Close(); _stream = stream; _owns = owns; };

    SQInteger Write(const void *buffer, SQInteger size) { return -1; };
    SQInteger Flush()	{ return 0; };
    SQInteger Tell()	{ return -1; };
    SQInteger Len()		{ return -1; };
    SQInteger Seek(SQInteger offset, SQInteger origin)	{ return -1; };
    bool IsValid()		{ return (_stream != NULL) && _stream->IsValid(); }
    bool EOS()			{ return ((_buf == NULL) || (_buf_pos >= _buf_len)) && ((_stream == NULL) || _stream->EOS()); }

	~SQTStreamReader()
	{
		if( _buf != NULL) {
			sq_free( _buf, _buf_size);
			_buf = NULL;
		}
	}

	void _Release()
	{
		this->~SQTStreamReader();
		sq_free(this,sizeof(SQTStreamReader));
	}

    SQInteger Close()
	{
		SQInteger r = 0;
        if( _stream && _owns) {
            r = _stream->Close();
        }
        _stream = NULL;
        _owns = false;
		return r;
    }

	SQInteger Mark( SQInteger readAheadLimit)
	{
		if( _buf_pos != 0) {
			memmove( _buf, _buf + _buf_pos, _buf_len - _buf_pos);
			_buf_len -= _buf_pos;
			_buf_pos = 0;
		}
		_mark_end = readAheadLimit;
		if( _buf_size < _mark_end) {
			_buf = (unsigned char *)sq_realloc( _buf, _buf_size, _mark_end);
			_buf_size = _mark_end;
		}
		return SQ_OK;
	}

	SQInteger Reset()
	{
		if( _buf_pos <= _mark_end) {
			_buf_pos = 0;
			return SQ_OK;
		}
		return SQ_ERROR;
	}

	SQInteger Read(void *buffer, SQInteger size)
	{
		SQInteger total = 0;
		while( size > 0) {
			if( _buf_pos < _buf_len) {	// read from buffer
				SQInteger left = _buf_len - _buf_pos;
				if( left > size)
					left = size;
				memcpy( buffer, _buf + _buf_pos, left);
				_buf_pos += left;
				total += left;
				size -= left;
				buffer = (unsigned char*)buffer + left;
			}
			else {
				if( _buf_pos > _mark_end) {
					_buf_pos = 0;
					_buf_len = 0;
					_mark_end = -1;
					if( !_is_buffered && (_buf != NULL)) {
						sq_free( _buf, _buf_size);
						_buf_size = 0;
						_buf = NULL;
					}
				}
				if( size <= (_buf_size - _buf_len)) {
					SQInteger r = _stream->Read( _buf + _buf_pos, _is_buffered ? _buf_size - _buf_len : size);
					if( r > 0)
						_buf_len += r;
					else
						return total ? total: -1;
				}
				else {
					_buf_pos = 0;
					_buf_len = 0;
					_mark_end = -1;
					SQInteger r = _stream->Read( buffer, size);
					if( r > 0)
						return total + r;
					else
						return total ? total: -1;
				}
			}
		}
		return total;
	}

protected:
	SQStream *_stream;
	SQBool _owns;
	SQInteger _mark_end;
	SQInteger _buf_pos;
	SQInteger _buf_len;
	SQInteger _buf_size;
	SQBool _is_buffered;
	unsigned char *_buf;
};

SQT_SRDR sqtsrdr_create( SQFILE stream, SQBool owns, SQInteger buffer_size)
{
	SQTStreamReader *srdr;

    srdr = new (sq_malloc(sizeof(SQTStreamReader)))SQTStreamReader( (SQStream*)stream, owns, buffer_size);
	if( srdr->IsValid()) {
		return (SQT_SRDR)srdr;
	}
	else {
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
	SQInteger buffer_size = 0;
    if( SQ_FAILED( sq_getinstanceup( v,2,(SQUserPointer*)&stream,(SQUserPointer)SQSTD_STREAM_TYPE_TAG))) {
        return sq_throwerror(v,_SC("invalid argument type"));
	}
	sq_getbool(v,3,&owns);
	if( sq_gettop(v) > 3) {
		sq_getinteger(v, 4, &buffer_size);
	}
	srdr = (SQTStreamReader*)sqtsrdr_create( stream, owns, buffer_size);
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
    _DECL_SRDR_FUNC(constructor,-3,_SC("xxbi")),
    _DECL_SRDR_FUNC(_typeof,1,_SC("x")),
    _DECL_SRDR_FUNC(mark,2,_SC("xi")),
    _DECL_SRDR_FUNC(reset,1,_SC("x")),
    _DECL_SRDR_FUNC(close,1,_SC("x")),
    {NULL,(SQFUNCTION)0,0,NULL}
};

static const SQTMemberDecl _srdr_members[] = {
	{_SC("_stream"), &srdr__stream_handle },
	{NULL,NULL}
};

const SQTClassDecl sqt_streamrdr_decl = {
	&std_stream_decl,	// base_class
    _SC("sqt_streamreader"),	// reg_name
    _SC("streamreader"),		// name
	_srdr_members,			// members
	_srdr_methods,		// methods
	NULL,				// globals
};

SQUIRREL_API SQRESULT sqstd_register_streamrdr(HSQUIRRELVM v)
{
	if(SQ_FAILED(sqt_declareclass(v,&sqt_streamrdr_decl))) {
		return SQ_ERROR;
	}
 	sq_poptop(v);
	return SQ_OK;
}
