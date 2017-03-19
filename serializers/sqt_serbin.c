#include <stdint.h>
#include <string.h>

#include <squirrel.h>
#include <sqtool.h>
#include <sqstdio.h>
#include <sqstdblob.h>

/*
0nnn nnnn	- string N
100n nnnn	- array N
101n nnnn	- table N
1100 0nnn	- blob 2^N
1100 1nnn	- string 2^N
1101 0nnn	- table 2^N
1101 1nnn	- array 2^N
1110 0nnn	- int 2^N
1110 1nnn	- float 2^N
1111 0000	- false
1111 0001	- true
1111 0010	- null
1111 0011	- *
.... ....
1111 1111	- *
*/

//typedef SQInteger (*SQWRITEFUNC)(SQUserPointer user, SQUserPointer buff, SQInteger size);
//typedef SQInteger (*SQREADFUNC)(SQUserPointer user, SQUserPointer buff, SQInteger size);

static SQRESULT read_unsigned( SQREADFUNC readfct, SQUserPointer user, SQInteger len, SQUnsignedInteger *pres)
{
	SQRESULT r;
	switch( len) {
		case 1: {
			uint8_t l;
			r = (readfct( user, &l, 1) == 1) ? SQ_OK : SQ_ERROR;
			*pres = l;
		} break;
		case 2: {
			uint16_t l;
			r = (readfct( user, &l, 2) == 2) ? SQ_OK : SQ_ERROR;
			*pres = l;
		} break;
		case 4: {
#ifdef _SQ64
			uint32_t l;
#else // _SQ64
			SQUnsignedInteger l;
#endif // _SQ64
			r = (readfct( user, &l, 4) == 4) ? SQ_OK : SQ_ERROR;
			*pres = l;
		} break;
#ifdef _SQ64
		case 8: {
			SQUnsignedInteger l;
			r = (readfct( user, &l, 8) == 8) ? SQ_OK : SQ_ERROR;
			*pres = l;
		} break;
#endif // _SQ64
		default:
			r = SQ_ERROR;
			break;
	}
	return r;
}

static SQRESULT read_signed( SQREADFUNC readfct, SQUserPointer user, SQInteger len, SQInteger *pres)
{
	SQRESULT r;
	switch( len) {
		case 1: {
			int8_t l;
			r = (readfct( user, &l, 1) == 1) ? SQ_OK : SQ_ERROR;
			*pres = l;
		} break;
		case 2: {
			int16_t l;
			r = (readfct( user, &l, 2) == 2) ? SQ_OK : SQ_ERROR;
			*pres = l;
		} break;
		case 4: {
#ifdef _SQ64
			int32_t l;
#else // _SQ64
			SQInteger l;
#endif // _SQ64
			r = (readfct( user, &l, 4) == 4) ? SQ_OK : SQ_ERROR;
			*pres = l;
		} break;
#ifdef _SQ64
		case 8: {
			SQInteger l;
			r = (readfct( user, &l, 8) == 8) ? SQ_OK : SQ_ERROR;
			*pres = l;
		} break;
#endif // _SQ64
		default:
			r = SQ_ERROR;
			break;
	}
	return r;
}

static SQRESULT read_float( SQREADFUNC readfct, SQUserPointer user, SQInteger len, SQFloat *pres)
{
	SQRESULT r;
	switch( len) {
		case sizeof(SQFloat): {
			SQFloat l;
			r = (readfct( user, &l, sizeof(SQFloat)) == sizeof(SQFloat)) ? SQ_OK : SQ_ERROR;
			*pres = l;
		} break;
		default:
			r = SQ_ERROR;
			break;
	}
	return r;
}

/*
0nnn nnnn	- string N
100n nnnn	- array N
101n nnnn	- table N
1100 0nnn	- blob 2^N
1100 1nnn	- string 2^N
1101 0nnn	- table 2^N
1101 1nnn	- array 2^N
1110 0nnn	- int 2^N
1110 1nnn	- float 2^N
1111 0000	- false
1111 0001	- true
1111 0010	- null
1111 0011	- *
.... ....
1111 1111	- *
*/

SQRESULT sqt_serbin_read( HSQUIRRELVM v, SQREADFUNC readfct, SQUserPointer user)
{
	SQUnsignedInteger len;
	unsigned char b;
	if( readfct( user, &b, 1) == 1) {
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
					if( SQ_FAILED( read_unsigned( readfct, user, len, &len)))
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
					if( SQ_FAILED( read_signed( readfct, user, len, &sint)))
						return SQ_ERROR;
					sq_pushinteger(v,sint);
					return SQ_OK;
				}
				else if( b == 5) {					// 1110 1nnn	- float 2^N
					SQFloat flt;
					if( SQ_FAILED( read_float( readfct, user, len, &flt)))
						return SQ_ERROR;
					sq_pushfloat(v, flt);
					return SQ_OK;
				}
			}
			else {
				switch( b & 0x0F) {
					case 0:	sq_pushbool(v,SQFalse); return SQ_OK;	// 1111 0000	- false
					case 1:	sq_pushbool(v,SQTrue); return SQ_OK;	// 1111 0001	- true
					case 2:	sq_pushnull(v); return SQ_OK;			// 1111 0010	- null
					default: break;
				}
			}
		}
	}
	return SQ_ERROR;

rd_string: {
		SQChar *tmp = sq_getscratchpad(v,len);
		if( readfct( user, tmp, len) == len) {
			sq_pushstring(v, tmp, len);
			return SQ_OK;
		}
		return SQ_ERROR;
	}
	
rd_array: {
		SQInteger idx = 0;
		sq_newarray(v,len);										// array
		while( len) {
			sq_pushinteger(v,idx);								// array, index
			if( SQ_SUCCEEDED( sqt_serbin_read(v, readfct, user))) {	// array, index, value
				if( SQ_SUCCEEDED(sq_set(v,-3))) {				// array
					len--;
					idx++;
					continue;
				}
			}
			sq_pop(v,2);										//
			return SQ_ERROR;
		}
		return SQ_OK;
	}

rd_table:
	sq_newtableex(v, len);										// table
	while( len) {
		if( SQ_SUCCEEDED( sqt_serbin_read(v, readfct, user))) {		// table, key
			if( SQ_SUCCEEDED( sqt_serbin_read(v, readfct, user))) {	// table, key, value
				if( SQ_SUCCEEDED(sq_newslot(v,-3,SQFalse))) {		// table
					len--;
					continue;
				}
			}
			sq_poptop(v);										// table
		}
		sq_poptop(v);											//
		return SQ_ERROR;
	}
	return SQ_OK;

rd_blob: {
		SQUserPointer ptr = sqstd_createblob(v, len);
		if( readfct( user, ptr, len) == len) {
			return SQ_OK;
		}
		return SQ_ERROR;
		
	}
}

/*
0nnn nnnn	- string N
100n nnnn	- array N
101n nnnn	- table N
1100 0nnn	- blob 2^N
1100 1nnn	- string 2^N
1101 0nnn	- table 2^N
1101 1nnn	- array 2^N
1110 0nnn	- int 2^N
1110 1nnn	- float 2^N
1111 0000	- false
1111 0001	- true
1111 0010	- null
1111 0011	- *
.... ....
1111 1111	- *
*/

static SQRESULT write_signed( SQWRITEFUNC writefct, SQUserPointer user, SQInteger val)
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
	return (writefct( user, b, len) == len) ? SQ_OK : SQ_ERROR;
}

static SQRESULT write_unsigned( SQWRITEFUNC writefct, SQUserPointer user, SQInteger pfx, SQUnsignedInteger val)
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
	return (writefct( user, b, len) == len) ? SQ_OK : SQ_ERROR;
}

static SQRESULT write_float( SQWRITEFUNC writefct, SQUserPointer user, SQFloat val)
{
	uint8_t b[1+sizeof(SQFloat)];
	switch( sizeof(SQFloat)) {
		case 2: b[0] = 0xE8 | 1; break;
		case 4: b[0] = 0xE8 | 2; break;
		case 8: b[0] = 0xE8 | 3; break;
		case 16: b[0] = 0xE8 | 4; break;
	}
	*(SQFloat*)(b+1) = val;
	return (writefct( user, b, 1+sizeof(SQFloat)) == (1+sizeof(SQFloat))) ? SQ_OK : SQ_ERROR;
}


SQRESULT sqt_serbin_write( HSQUIRRELVM v, SQWRITEFUNC writefct, SQUserPointer user)
{
	uint8_t b;
	switch(sq_gettype(v,-1))
	{
		case OT_NULL: {
			b = 0xF2;
			if( writefct( user, &b, 1) == 1)
				return SQ_OK;
			return SQ_ERROR;
		}
		case OT_INTEGER: {
			SQInteger val;
			sq_getinteger(v,-1,&val);
			return write_signed( writefct, user, val);
		}
		case OT_FLOAT: {
			SQFloat val;
			sq_getfloat(v, -1, &val);
			return write_float( writefct, user, val);
		}
		//case OT_USERPOINTER:
		//	pf(v,_SC("[%s] USERPOINTER\n"),name);
		//	break;
		case OT_STRING: {
			const SQChar *s;
			SQUnsignedInteger len;
			sq_getstring(v,-1,&s);
			len = scstrlen( s);
			if( len <= 0x7F) {
				b = (uint8_t)len;
				if( writefct( user, &b, 1) != 1)
					return SQ_ERROR;
			}
			else {
				if( SQ_FAILED( write_unsigned( writefct, user, 0xC8, len)))
					return SQ_ERROR;
			}
			if( writefct( user, s, len) == len)
				return SQ_OK;
			return SQ_ERROR;
		}
		case OT_TABLE: {
			SQUnsignedInteger len = sq_getsize(v,-1);
			if( len <= 0x1F) {
				b = 0xA0 | (uint8_t)len;
				if( writefct( user, &b, 1) != 1)
					return SQ_ERROR;
			}
			else {
				if( SQ_FAILED( write_unsigned( writefct, user, 0xD0, len)))
					return SQ_ERROR;
			}
			sq_pushnull(v);														// table, iterator
			while(SQ_SUCCEEDED(sq_next(v,-2)))									// table, iterator, key, value
			{
				sq_push(v,-2);													// table, iterator, key, value, key
				if( SQ_SUCCEEDED( sqt_serbin_write( v, writefct, user))) {		// table, iterator, key, value, key
					sq_poptop(v);												// table, iterator, key, value
					if( SQ_SUCCEEDED( sqt_serbin_write( v, writefct, user))) {	// table, iterator, key, value
						sq_pop(v,2);											// table, iterator
					}
					else {														// table, iterator, key, value
						sq_pop(v,3);											// table
						return SQ_ERROR;
					}
				}
				else {															// table, iterator, key, value, key
					sq_pop(v,4);												// table
					return SQ_ERROR;
				}
			}																	// table, iterator
			sq_pop(v,1);														// table
			return SQ_OK;
		}
		case OT_ARRAY: {
			SQUnsignedInteger len = sq_getsize(v,-1);
			if( len <= 0x1F) {
				b = 0x80 | (uint8_t)len;
				if( writefct( user, &b, 1) != 1)
					return SQ_ERROR;
			}
			else {
				if( SQ_FAILED( write_unsigned( writefct, user, 0xD8, len)))
					return SQ_ERROR;
			}
			sq_pushnull(v);														// array, iterator
			while(SQ_SUCCEEDED(sq_next(v,-2)))									// array, iterator, key, value
			{
				if( SQ_SUCCEEDED(sqt_serbin_write( v, writefct, user))) {		// array, iterator, key, value
					sq_pop(v,2);												// array, iterator
				}
				else {															// array, iterator, key, value
					sq_pop(v,3);												// array
					return SQ_ERROR;
				}
			}																	// array, iterator
			sq_pop(v,1);														// array
			return SQ_OK;
		}
		case OT_BOOL: {
			SQBool bval;
			sq_getbool(v,-1,&bval);
			b = bval ? 0xF1 : 0xF0;
			if( writefct( user, &b, 1) == 1)
				return SQ_OK;
			return SQ_ERROR;
		}
		case OT_INSTANCE: {
			SQUserPointer ptr;
			if( SQ_SUCCEEDED( sqstd_getblob(v, -1, &ptr))) {
				// write blob
				SQUnsignedInteger len = sqstd_getblobsize(v,-1);
				if( SQ_FAILED( write_unsigned( writefct, user, 0xC0, len)))
					return SQ_ERROR;
				if( writefct( user, ptr, len) == len)
					return SQ_OK;
			}
			return SQ_ERROR;
		}
		//case OT_CLOSURE:
		//	pf(v,_SC("[%s] CLOSURE\n"),name);
		//	break;
		//case OT_NATIVECLOSURE:
		//	pf(v,_SC("[%s] NATIVECLOSURE\n"),name);
		//	break;
//		case OT_GENERATOR:
//			pf(v,_SC("[%s] GENERATOR\n"),name);
//			break;
		//case OT_USERDATA:
		//	pf(v,_SC("[%s] USERDATA\n"),name);
		//	break;
		//case OT_THREAD:
		//	pf(v,_SC("[%s] THREAD\n"),name);
		//	break;
		//case OT_CLASS:
		//	pf(v,_SC("[%s] CLASS\n"),name);
		//	break;
//		case OT_INSTANCE:
//			pf(v,_SC("[%s] INSTANCE\n"),name);
//			break;
//		case OT_WEAKREF:
//			pf(v,_SC("[%s] WEAKREF\n"),name);
//			break;
		default:
			return SQ_ERROR;
	}
}

static SQInteger _g_serbin_loaddata(HSQUIRRELVM v)
{
	SQFILE file;
    SQBool printerror = SQFalse;
    if(sq_gettop(v) >= 3) {
        sq_getbool(v,3,&printerror);
    }
    if( SQ_FAILED( sq_getinstanceup( v,2,(SQUserPointer*)&file,(SQUserPointer)SQSTD_STREAM_TYPE_TAG))) {
        return sq_throwerror(v,_SC("invalid argument type"));
	}
	if(SQ_SUCCEEDED(sqt_serbin_read(v,sqt_SQFILEREADFUNC,file)))
		return 1;
    return SQ_ERROR; //propagates the error
}

static SQInteger _g_serbin_savedata(HSQUIRRELVM v)
{
	SQFILE file;
    if( SQ_FAILED( sq_getinstanceup( v,2,(SQUserPointer*)&file,(SQUserPointer)SQSTD_STREAM_TYPE_TAG))) {
        return sq_throwerror(v,_SC("invalid argument type"));
	}
	if(SQ_SUCCEEDED(sqt_serbin_write(v,sqt_SQFILEWRITEFUNC, file)))
		return 1;
    return SQ_ERROR; //propagates the error
}


#define _DECL_GLOBALSERBIN_FUNC(name,nparams,typecheck) {_SC(#name),_g_serbin_##name,nparams,typecheck}
static const SQRegFunction _serbin_funcs[]={
    _DECL_GLOBALSERBIN_FUNC(loaddata,-2,_SC(".xb")),
    _DECL_GLOBALSERBIN_FUNC(savedata,3,_SC(".x.")),
    {NULL,(SQFUNCTION)0,0,NULL}
};

SQRESULT sqstd_register_serbin(HSQUIRRELVM v)
{
	return sqt_declarefunctions(v, _serbin_funcs);
}
