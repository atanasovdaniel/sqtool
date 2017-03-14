
#include <squirrel.h>
#include <sqtool.h>

/*
0nnn nnnn	- string N
100n nnnn	- array N
101n nnnn	- table N
1100 0nnn	- uint 2^N
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
		case 0: {
			unsigned char l;
			r = (readfct( &l, 1, user) == 1) SQ_OK : SQ_ERROR;
			*pres = l;
		} break;
		case 1: {
			unsigned short l;
			r = (readfct( &l, 2, user) == 2) SQ_OK : SQ_ERROR;
			*pres = l;
		} break;
		case 2: {
			unsigned long l;
			r = (readfct( &l, 4, user) == 4) SQ_OK : SQ_ERROR;
			*pres = l;
		} break;
#ifdef _SQ64
		case 3: {
			SQUnsignedInteger l;
			r = (readfct( &l, 8, user) == 8) SQ_OK : SQ_ERROR;
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
		case 0: {
			signed char l;
			r = (readfct( &l, 1, user) == 1) SQ_OK : SQ_ERROR;
			*pres = l;
		} break;
		case 1: {
			signed short l;
			r = (readfct( &l, 2, user) == 2) SQ_OK : SQ_ERROR;
			*pres = l;
		} break;
		case 2: {
			signed long l;
			r = (readfct( &l, 4, user) == 4) SQ_OK : SQ_ERROR;
			*pres = l;
		} break;
#ifdef _SQ64
		case 3: {
			SQInteger l;
			r = (readfct( &l, 8, user) == 8) SQ_OK : SQ_ERROR;
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
		case sizeof(SQFloat)/8: {
			SQFloat l;
			r = (readfct( &l, sizeof(SQFloat), user) == (sizeof(SQFloat)/8)) SQ_OK : SQ_ERROR;
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
1100 0nnn	- uint 2^N
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

SQRESULT sqt_serbin_read( HSQUIRRELVM v SQREADFUNC readfct, SQUserPointer user)
{
	SQUnsignedInteger len;
	unsigned char b;
	if( read_one( &b, 1, user) == 1) {
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
		else if( (b & 0xE0) == 0xC0) {				// 11xx xnnn	- uint 2^N
			if( b < 0xF0) {
				len = 1 << (b & 0x07);
				b = (b >> 3)  & 0x07;
				if( b < 4) {
					if( SQ_FAILED( read_unsigned( fct, user, len, &len)))
						return SQ_ERROR;
					switch( b & 0x03) {
						case 0: sq_pushinteger(v, len); return 0;	// 1100 0nnn	- uint 2^N
						case 1: goto rd_string;						// 1100 1nnn	- string 2^N
						case 2: goto rd_table;						// 1101 0nnn	- table 2^N
						case 3: goto rd_array;						// 1101 1nnn	- array 2^N
						default: break;
					}
				}
				else if( b == 4) {					// 1110 0nnn	- int 2^N
					SQInteger sint;
					if( SQ_FAILED( read_signed( fct, user, len, &sint)))
						return SQ_ERROR;
					sq_pushInteger(v,sint);
					return SQ_OK;
				}
				else if( b == 5) {					// 1110 1nnn	- float 2^N
					SQFloat flt;
					if( SQ_FAILED( read_float( fct, user, len, &flt)))
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

rd_string:
	SQChar *tmp = sq_getscratchpad(v,len);
	if( readfct( tmp, len, user) == len) {
		sq_pushstring(v, tmp, len);
		return SQ_OK;
	}
	return SQ_ERROR;
	
rd_array:
	{
		SQInteger idx = 0;
		sq_newarray(v,len);										// array
		while( len) {
			sq_pushinteger(v,idx);								// array, index
			if( SQ_SUCCEEDED( sqt_serbin_read( fct, user))) {	// array, index, value
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
		if( SQ_SUCCEEDED( sqt_serbin_read( fct, user))) {		// table, key
			if( SQ_SUCCEEDED( sqt_serbin_read( fct, user))) {	// table, key, value
				if( SQ_SUCCEEDED(sq_set(v,-3))) {				// table
					len--;
					continue;
				}
			}
			pop();												// table
		}
		pop();													//
		return SQ_ERROR;
	}
	return SQ_OK;
}


SQRESULT sqt_serbin_write( HSQUIRRELVM v SQREADFUNC writefct, SQUserPointer user)
{
	
}
