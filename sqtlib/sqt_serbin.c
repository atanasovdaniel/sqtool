#include <stdint.h>
#include <string.h>

#include <squirrel.h>
#include <sqstdaux.h>
#include <sqtool.h>
#include <sqstdio.h>
#include <sqstdblob.h>
#include <sqt_serializer.h>

#define MAGIC_NORMAL    0x01534244L
#define MAGIC_SWAP      0x44425301L

/*
0nnn nnnn	- string    len=N
100n nnnn	- array     len=N
101n nnnn	- table     len=N
1100 0nnn	- blob      len=uint(bytes(2^N))
1100 1nnn	- string    len=uint(bytes(2^N))
1101 0nnn	- table     len=uint(bytes(2^N))
1101 1nnn	- array     len=uint(bytes(2^N))
1110 0nnn	- int       bytes(2^N)
1110 1nnn	- float     bytes(2^N)
1111 0000	- false
1111 0001	- true
1111 0010	- null
1111 0011	- *
.... ....
1111 1111	- *
*/

#define SBF_SWAP    0x01

typedef struct
{
    unsigned int flags;
    SQREADFUNC readfct;
    SQUserPointer user;
    HSQUIRRELVM v;
} read_ctx_t;

typedef struct
{
    unsigned int flags;
    SQWRITEFUNC writefct;
    SQUserPointer user;
    HSQUIRRELVM v;
} write_ctx_t;

static const SQChar ERR_MSG_READ[] = _SC("read failed");
static const SQChar ERR_MSG_WRITE[] = _SC("write failed");
static const SQChar ERR_MSG_MAGIC[] = _SC("bad magic value");
static const SQChar ERR_MSG_OBJECT_TYPE[]   = _SC("non-serializable object type");
static const SQChar ERR_MSG_ARG_NO_STREAM[] = _SC("argument is not a stream");

#define DEF_BSWAP( _T) \
_T bswap_##_T( _T val) { \
    _T retVal; \
    int i; \
    char *pVal = (char*)&val; \
    char *pRetVal = (char*)&retVal; \
    for( i=0; i<sizeof(_T); i++) { \
        pRetVal[sizeof(_T)-1-i] = pVal[i]; \
    } \
    return retVal; \
}

static DEF_BSWAP( uint16_t)
static DEF_BSWAP( uint32_t)
#ifdef _SQ64
static DEF_BSWAP( uint64_t)
#endif // _SQ64
static DEF_BSWAP( float)
static DEF_BSWAP( double)

static SQRESULT read_bytes( const read_ctx_t *ctx, SQUserPointer ptr, SQInteger len)
{
    if( ctx->readfct( ctx->user, ptr, len) != len) {
        return sq_throwerror(ctx->v,ERR_MSG_READ);
    }
    return SQ_OK;
}

static SQRESULT read_unsigned( const read_ctx_t *ctx, SQInteger len, SQUnsignedInteger *pres)
{
	SQRESULT r;
	switch( len) {
		case 1: {
			uint8_t l;
            r = read_bytes(ctx, &l, 1);
			*pres = l;
		} break;
		case 2: {
			uint16_t l;
            r = read_bytes(ctx, &l, 2);
            if( ctx->flags & SBF_SWAP) l = bswap_uint16_t( l);
			*pres = l;
		} break;
		case 4: {
#ifdef _SQ64
			uint32_t l;
#else // _SQ64
			SQUnsignedInteger l;
#endif // _SQ64
            r = read_bytes(ctx, &l, 4);
            if( ctx->flags & SBF_SWAP) l = bswap_uint32_t( l);
			*pres = l;
		} break;
#ifdef _SQ64
		case 8: {
			SQUnsignedInteger l;
            r = read_bytes(ctx, &l, 8);
            if( ctx->flags & SBF_SWAP) l = bswap_uint64_t( l);
			*pres = l;
		} break;
#endif // _SQ64
		default:
			r = SQ_ERROR;
			break;
	}
	return r;
}

static SQRESULT read_signed( const read_ctx_t *ctx, SQInteger len, SQInteger *pres)
{
	SQRESULT r;
	switch( len) {
		case 1: {
			int8_t l;
            r = read_bytes(ctx, &l, 1);
			*pres = l;
		} break;
		case 2: {
			int16_t l;
            r = read_bytes(ctx, &l, 2);
            if( ctx->flags & SBF_SWAP) l = bswap_uint16_t( l);
			*pres = (SQInteger)l;
		} break;
		case 4: {
#ifdef _SQ64
			int32_t l;
#else // _SQ64
			SQInteger l;
#endif // _SQ64
            r = read_bytes(ctx, &l, 4);
            if( ctx->flags & SBF_SWAP) l = bswap_uint32_t( l);
			*pres = (SQInteger)l;
		} break;
#ifdef _SQ64
		case 8: {
			SQInteger l;
            r = read_bytes(ctx, &l, 8);
            if( ctx->flags & SBF_SWAP) l = bswap_uint64_t( l);
			*pres = (SQInteger)l;
		} break;
#endif // _SQ64
		default:
			r = SQ_ERROR;
			break;
	}
	return r;
}

static SQRESULT read_float( const read_ctx_t *ctx, SQInteger len, SQFloat *pres)
{
	SQRESULT r;
	switch( len) {
        case 4: {
            float l;
            r = read_bytes(ctx, &l, 4);
            if( ctx->flags & SBF_SWAP) l = bswap_float( l);
            *pres = (SQFloat)l;
        } break;
        
		case 8: {
            double l;
            r = read_bytes(ctx, &l, 8);
            if( ctx->flags & SBF_SWAP) l = bswap_double( l);
            *pres = (SQFloat)l;
		} break;
		default:
			r = SQ_ERROR;
			break;
	}
	return r;
}

SQRESULT sqt_serbin_read( const read_ctx_t *ctx)
{
	SQUnsignedInteger len;
	unsigned char b;
    if( SQ_SUCCEEDED( read_bytes(ctx, &b, 1))) {
		if( !(b & 0x80)) {							// 0nnn nnnn	- string N
			// string N
			len = b & 0x7F;
			goto rd_string;
		}
		else if( (b & 0xE0) == 0x80) {				// 100n nnnn	- array N
			// array N
			len = b & 0x1F;
			goto rd_array;
		}
		else if( (b & 0xE0) == 0xA0) {				// 101n nnnn	- table N
			// table N
			len = b & 0x1F;
			goto rd_table;
		}
		else if( (b & 0xC0) == 0xC0) {				// 11xx xnnn	- uint 2^N
			if( b < 0xF0) {
				len = 1 << (b & 0x07);
				b = (b >> 3)  & 0x07;
				if( b < 4) {
					if( SQ_FAILED( read_unsigned( ctx, len, &len)))
						return SQ_ERROR;
					switch( b & 0x03) {
						case 0: goto rd_blob;		// 1100 0nnn	- blob 2^N
						case 1: goto rd_string;		// 1100 1nnn	- string 2^N
						case 2: goto rd_table;		// 1101 0nnn	- table 2^N
						case 3: goto rd_array;		// 1101 1nnn	- array 2^N
						default: break;
					}
				}
				else if( b == 4) {					// 1110 0nnn	- int 2^N
					SQInteger sint;
					if( SQ_FAILED( read_signed( ctx, len, &sint)))
						return SQ_ERROR;
					sq_pushinteger(ctx->v,sint);
					return SQ_OK;
				}
				else if( b == 5) {					// 1110 1nnn	- float 2^N
					SQFloat flt;
					if( SQ_FAILED( read_float( ctx, len, &flt)))
						return SQ_ERROR;
					sq_pushfloat(ctx->v, flt);
					return SQ_OK;
				}
			}
			else {
				switch( b & 0x0F) {
					case 0:	sq_pushbool(ctx->v,SQFalse); return SQ_OK;	// 1111 0000	- false
					case 1:	sq_pushbool(ctx->v,SQTrue); return SQ_OK;	// 1111 0001	- true
					case 2:	sq_pushnull(ctx->v); return SQ_OK;			// 1111 0010	- null
					default: break;
				}
			}
		}
	}
	return SQ_ERROR;

rd_string: {
		SQChar *tmp = sq_getscratchpad(ctx->v,len);
        if( SQ_SUCCEEDED(read_bytes( ctx, tmp, len))) {
            sq_pushstring(ctx->v, tmp, len);
            return SQ_OK;
        }
		return SQ_ERROR;
	}
	
rd_array: {
		SQInteger idx = 0;
		sq_newarray(ctx->v,len);								// array
		while( len) {
			sq_pushinteger(ctx->v,idx);							// array, index
			if( SQ_SUCCEEDED( sqt_serbin_read( ctx))) {         // array, index, value
				if( SQ_SUCCEEDED(sq_set(ctx->v,-3))) {			// array
					len--;
					idx++;
					continue;
				}
			}
			sq_pop(ctx->v,2);									//
			return SQ_ERROR;
		}
		return SQ_OK;
	}

rd_table:
	sq_newtableex(ctx->v, len);                                         // table
	while( len) {
		if( SQ_SUCCEEDED( sqt_serbin_read(ctx))) {                      // table, key
			if( SQ_SUCCEEDED( sqt_serbin_read(ctx))) {                  // table, key, value
				if( SQ_SUCCEEDED(sq_newslot(ctx->v,-3,SQFalse))) {		// table
					len--;
					continue;
				}
			}
			sq_poptop(ctx->v);                                          // table
		}
		sq_poptop(ctx->v);                                              //
		return SQ_ERROR;
	}
	return SQ_OK;

rd_blob: {
		SQUserPointer ptr = sqstd_createblob(ctx->v, len);              // blob
        if( SQ_SUCCEEDED(read_bytes( ctx, ptr, len))) {
            return SQ_OK;
        }
		sq_poptop(ctx->v);                                              //
		return SQ_ERROR;
		
	}
}

static SQRESULT write_bytes( const write_ctx_t *ctx, SQUserPointer ptr, SQInteger len)
{
	if( ctx->writefct( ctx->user, ptr, len) != len) {
        return sq_throwerror(ctx->v,ERR_MSG_WRITE);
    }
    return SQ_OK;
}

static SQRESULT write_signed( const write_ctx_t *ctx, SQInteger val)
{
	SQInteger len;
	int8_t b[1+sizeof(SQInteger)];
	if( (val >= INT8_MIN) && (val <= INT8_MAX)) {
		b[0] = 0xE0 | 0;
		b[1] = (int8_t)val;
		len = 2;
	}
	else if( (val >= INT16_MIN) && (val <= INT16_MAX)) {
		b[0] = 0xE0 | 1;
		*(int16_t*)(b+1) = (int16_t)val;
		len = 3;
	}
#ifndef _SQ64
	else {
		b[0] = 0xE0 | 2;
		*(SQInteger*)(b+1) = val;
		len = 5;
	}
#else // _SQ64
	else if( (val >= INT32_MIN) && (val <= INT32_MAX)) {
		b[0] = 0xE0 | 2;
		*(int32_t*)(b+1) = (int32_t)val;
		len = 5;
	}
	else {
		b[0] = 0xE0 | 3;
		*(SQInteger*)(b+1) = val;
		len = 9;
	}
#endif // _SQ64
    return write_bytes(ctx, b, len);
}

static SQRESULT write_unsigned( const write_ctx_t *ctx, SQInteger pfx, SQUnsignedInteger val)
{
	SQInteger len;
	uint8_t b[1+sizeof(SQUnsignedInteger)];
	if( val <= UINT8_MAX) {
		b[0] = pfx | 0;
		b[1] = (uint8_t)val;
		len = 2;
	}
	else if( val <= UINT16_MAX) {
		b[0] = pfx | 1;
		*(uint16_t*)(b+1) = (uint16_t)val;
		len = 3;
	}
#ifndef _SQ64
	else {
		b[0] = pfx | 2;
		*(SQUnsignedInteger*)(b+1) = val;
		len = 5;
	}
#else // _SQ64
	else if( val <= UINT32_MAX) {
		b[0] = pfx | 2;
		*(uint32_t*)(b+1) = (uint32_t)val;
		len = 5;
	}
	else {
		b[0] = pfx | 3;
		*(SQUnsignedInteger*)(b+1) = val;
		len = 9;
	}
#endif // _SQ64
    return write_bytes(ctx, b, len);
}

static SQRESULT write_float( const write_ctx_t *ctx, SQFloat val)
{
	uint8_t b[1+sizeof(SQFloat)];
	switch( sizeof(SQFloat)) {
		case 2: b[0] = 0xE8 | 1; break;
		case 4: b[0] = 0xE8 | 2; break;
		case 8: b[0] = 0xE8 | 3; break;
		case 16: b[0] = 0xE8 | 4; break;
	}
	*(SQFloat*)(b+1) = val;
    return write_bytes(ctx, b, 1+sizeof(SQFloat));
}


SQRESULT sqt_serbin_write( const write_ctx_t *ctx)
{
	uint8_t b;
	switch(sq_gettype(ctx->v,-1))
	{
		case OT_NULL: {
			b = 0xF2;
            return write_bytes(ctx, &b, 1);
		}
		case OT_INTEGER: {
			SQInteger val;
			sq_getinteger(ctx->v,-1,&val);
			return write_signed( ctx, val);
		}
		case OT_FLOAT: {
			SQFloat val;
			sq_getfloat(ctx->v, -1, &val);
			return write_float( ctx, val);
		}
		case OT_STRING: {
			const SQChar *s;
			SQInteger len;
			sq_getstringandsize(ctx->v,-1,&s,&len);
			if( len <= 0x7F) {
				b = (uint8_t)len;
                if(SQ_FAILED(write_bytes(ctx, &b, 1)))
					return SQ_ERROR;
			}
			else {
				if( SQ_FAILED( write_unsigned( ctx, 0xC8, len)))
					return SQ_ERROR;
			}
            return write_bytes(ctx, (SQUserPointer)s, len);
		}
		case OT_TABLE: {
			SQUnsignedInteger len = sq_getsize(ctx->v,-1);
			if( len <= 0x1F) {
				b = 0xA0 | (uint8_t)len;
                if(SQ_FAILED(write_bytes(ctx, &b, 1)))
					return SQ_ERROR;
			}
			else {
				if( SQ_FAILED( write_unsigned( ctx, 0xD0, len)))
					return SQ_ERROR;
			}
			sq_pushnull(ctx->v);												// table, iterator
			while(SQ_SUCCEEDED(sq_next(ctx->v,-2)))								// table, iterator, key, value
			{
				sq_push(ctx->v,-2);												// table, iterator, key, value, key
				if( SQ_SUCCEEDED( sqt_serbin_write( ctx))) {                    // table, iterator, key, value, key
					sq_poptop(ctx->v);											// table, iterator, key, value
					if( SQ_SUCCEEDED( sqt_serbin_write( ctx))) {                // table, iterator, key, value
						sq_pop(ctx->v,2);										// table, iterator
					}
					else {														// table, iterator, key, value
						sq_pop(ctx->v,3);										// table
						return SQ_ERROR;
					}
				}
				else {															// table, iterator, key, value, key
					sq_pop(ctx->v,4);											// table
					return SQ_ERROR;
				}
			}																	// table, iterator
			sq_pop(ctx->v,1);													// table
			return SQ_OK;
		}
		case OT_ARRAY: {
			SQUnsignedInteger len = sq_getsize(ctx->v,-1);
			if( len <= 0x1F) {
				b = 0x80 | (uint8_t)len;
                if(SQ_FAILED(write_bytes(ctx, &b, 1)))
					return SQ_ERROR;
			}
			else {
				if( SQ_FAILED( write_unsigned( ctx, 0xD8, len)))
					return SQ_ERROR;
			}
			sq_pushnull(ctx->v);												// array, iterator
			while(SQ_SUCCEEDED(sq_next(ctx->v,-2)))								// array, iterator, key, value
			{
				if( SQ_SUCCEEDED(sqt_serbin_write( ctx))) {                     // array, iterator, key, value
					sq_pop(ctx->v,2);											// array, iterator
				}
				else {															// array, iterator, key, value
					sq_pop(ctx->v,3);											// array
					return SQ_ERROR;
				}
			}																	// array, iterator
			sq_pop(ctx->v,1);													// array
			return SQ_OK;
		}
		case OT_BOOL: {
			SQBool bval;
			sq_getbool(ctx->v,-1,&bval);
			b = bval ? 0xF1 : 0xF0;
            return write_bytes(ctx, &b, 1);
		}
		case OT_INSTANCE: {
			SQUserPointer ptr;
			if( SQ_SUCCEEDED( sqstd_getblob(ctx->v, -1, &ptr))) {
				// write blob
				SQUnsignedInteger len = sqstd_getblobsize(ctx->v,-1);
				if( SQ_FAILED( write_unsigned( ctx, 0xC0, len)))
					return SQ_ERROR;
                return write_bytes(ctx, ptr, len);
			}
			//return SQ_ERROR;
		}
		//case OT_CLOSURE:
		//case OT_NATIVECLOSURE:
		//case OT_GENERATOR:
		//case OT_USERPOINTER:
		//case OT_USERDATA:
		//case OT_THREAD:
		//case OT_CLASS:
		//case OT_WEAKREF:
		default:
			return sq_throwerror(ctx->v,ERR_MSG_OBJECT_TYPE);
	}
}


SQRESULT sqt_serbin_load_cb(HSQUIRRELVM v, const SQChar *opts, SQREADFUNC readfct, SQUserPointer user)
{
    read_ctx_t ctx;
    uint32_t magic;
    
    ctx.flags = 0;
    ctx.v = v;
    ctx.readfct = readfct;
    ctx.user = user;
    
    if( SQ_FAILED(read_bytes(&ctx, &magic, sizeof(magic)))) {
        return SQ_ERROR;
    }
    
    if( magic == MAGIC_NORMAL) {
        // ok
    }
    else if( magic == MAGIC_SWAP) {
        ctx.flags |= SBF_SWAP;
    }
    else {
        return sq_throwerror(v,ERR_MSG_MAGIC);
    }
    
	return sqt_serbin_read( &ctx);
}

SQRESULT sqt_serbin_load(HSQUIRRELVM v, const SQChar *opts, SQFILE file)
{
    return sqt_serbin_load_cb(v, opts, sqstd_FILEREADFUNC, file);
}

static SQRESULT _g_serbin_loadbinary(HSQUIRRELVM v)
{
                                // this stream opts...
	SQFILE file;
    const SQChar *opts = 0;
    
    if( SQ_FAILED( sq_getinstanceup( v,2,(SQUserPointer*)&file,(SQUserPointer)SQSTD_STREAM_TYPE_TAG))) {
        return sq_throwerror(v,ERR_MSG_ARG_NO_STREAM);
	}
    
    if( sq_gettop(v) > 2)
    {
        sq_getstring(v,3,&opts);
    }

	if(SQ_SUCCEEDED(sqt_serbin_load(v, opts, file))) {
                                // this stream opts... data
		return 1;   // return loaded data
    }
    return SQ_ERROR; //propagates the error
}


SQRESULT sqt_serbin_save_cb( HSQUIRRELVM v, const SQChar *opts, SQWRITEFUNC writefct, SQUserPointer user)
{
    write_ctx_t ctx;
    uint32_t magic;
    
    ctx.flags = 0;
    ctx.v = v;
    ctx.writefct = writefct;
    ctx.user = user;
    
    magic = MAGIC_NORMAL;
    if(SQ_FAILED(write_bytes(&ctx, &magic, sizeof(magic)))) {
        return SQ_ERROR;
    }
    
	return sqt_serbin_write( &ctx);
}

SQRESULT sqt_serbin_save( HSQUIRRELVM v, const SQChar *opts, SQFILE file)
{
    return sqt_serbin_save_cb(v, opts, sqstd_FILEWRITEFUNC, file);
}

static SQRESULT _g_serbin_savebinary(HSQUIRRELVM v)
{
                                // this stream data opts...
	SQFILE file;
    const SQChar *opts = _SC("");
    
    if( SQ_FAILED( sq_getinstanceup( v,2,(SQUserPointer*)&file,(SQUserPointer)SQSTD_STREAM_TYPE_TAG))) {
        return sq_throwerror(v,ERR_MSG_ARG_NO_STREAM);
	}
    
    if( sq_gettop(v) > 3)
    {
        sq_push(v,3);           // this stream data opts.. data
        sq_remove(v,3);         // this stream opts.. data
        sq_getstring(v,3,&opts);
    }
    
	if(SQ_SUCCEEDED(sqt_serbin_save(v, opts, file))) {
		return 0;   // no return value
    }
    return SQ_ERROR; //propagates the error
}

#define _DECL_GLOBALSERBIN_FUNC(name,nparams,typecheck) {_SC(#name),_g_serbin_##name,nparams,typecheck}
static const SQRegFunction _serbin_funcs[]={
    _DECL_GLOBALSERBIN_FUNC(loadbinary,-2,_SC(".xs")),
    _DECL_GLOBALSERBIN_FUNC(savebinary,-3,_SC(".x.s")),
    {NULL,(SQFUNCTION)0,0,NULL}
};

SQRESULT sqstd_register_serbin(HSQUIRRELVM v)
{
	return sqstd_registerfunctions(v, _serbin_funcs);
}
