#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>  // snprintf
#include <math.h>   // isinf, isnan

#include <squirrel.h>
#include <sqstdaux.h>
#include <sqtool.h>
#include <sqstdio.h>
#include <sqt_serializer.h>

#define STR_STEP    128

#ifdef SQUSEDOUBLE
#define _PRINT_FLOAT_FMT _SC("%.17g")
#define MAX_FLOAT_STR_SIZE  50
#else // SQUSEDOUBLE
#define _PRINT_FLOAT_FMT _SC("%.9g")
#define MAX_FLOAT_STR_SIZE  50
#endif // SQUSEDOUBLE

typedef struct
{
    SQREADFUNC readfct;
    SQUserPointer user;
    HSQUIRRELVM v;
    SQChar back_char;
} read_ctx_t;


typedef struct
{
    SQWRITEFUNC writefct;
    SQUserPointer user;
    HSQUIRRELVM v;
    
    SQInteger ind;
    
    const SQChar    *str_nl;
    SQInteger       str_nl_len;
    
    const SQChar    *str_ind;
    SQInteger       str_ind_len;
    
} write_ctx_t;


static const SQChar ERR_MSG_EOF[]           = _SC("unexpected EOF");
static const SQChar ERR_MSG_KEY_TYPE[]      = _SC("table key must be a string");
static const SQChar ERR_MSG_BAD_CONTENT[]   = _SC("bad content");
static const SQChar ERR_MSG_BAD_ESCAPE[]    = _SC("BAD escape sequence");
static const SQChar ERR_MSG_STR_NEWLINE[]   = _SC("newline in string");
static const SQChar ERR_MSG_DELIMER[]       = _SC("expecting delimer");
static const SQChar ERR_MSG_PAIR_DELIMER[]  = _SC("expecting pair delimer");
static const SQChar ERR_MSG_ARG_NO_STREAM[] = _SC("argument is not a stream");
static const SQChar ERR_MSG_OBJECT_TYPE[]   = _SC("non-serializable object type");

// static const SQChar ERR_MSG[] = = _SC("EOF in comment");
#define ERR_MSG_EOF_COMMENT ERR_MSG_EOF
// static const SQChar ERR_MSG[] = _SC("EOF in number");
#define ERR_MSG_EOF_NUMBER  ERR_MSG_EOF
// static const SQChar ERR_MSG[] = _SC("bad number format");
#define ERR_MSG_FMT_NUMBER  ERR_MSG_BAD_CONTENT
// static const SQChar ERR_MSG[] = _SC("Expecting number");
#define ERR_MSG_EXP_NUMBER  ERR_MSG_BAD_CONTENT
// static const SQChar ERR_MSG[] = _SC("EOF in string");
#define ERR_MSG_EOF_STRING  ERR_MSG_EOF

static SQRESULT write_check( const write_ctx_t *ctx, const SQChar *ptr, SQInteger len)
{
    // warning: passing argument 2 of ‘ctx->writefct’ discards ‘const’ qualifier from pointer target type
    if( ctx->writefct( ctx->user, (SQUserPointer)ptr, len) == len) {
        return SQ_OK;
    }
	return SQ_ERROR;
}

static SQRESULT json_escape( const write_ctx_t *ctx, const SQChar *src)
{
    while( *src != _SC('\0'))
    {
        const SQChar *from = src;
        SQChar c;
        
        while( (c = *src) && (c != _SC('"')) && (c != _SC('\\')) && (c != _SC('/')) && (c >= _SC(' ')) )
            src++;
        
        if( src - from) {
            if( SQ_FAILED(write_check(ctx, from, (src - from)*sizeof(SQChar))))
    			return SQ_ERROR;
        }
        
        if( c) {
            src++;
            switch( c) {
                case _SC('"'): case _SC('\\'): case _SC('/'): break;
                case _SC('\b'): c = _SC('b'); break;
                case _SC('\f'): c = _SC('f'); break;
                case _SC('\n'): c = _SC('n'); break;
                case _SC('\r'): c = _SC('r'); break;
                case _SC('\t'): c = _SC('t'); break;
                default: {
                    SQChar esc[6] = { _SC('\\'), _SC('u'), _SC('0'), _SC('0'), _SC('0') + (c >> 4), _SC('0') + (c & 0xF) };
                    if( esc[4] > _SC('9')) esc[4] += _SC('A') - _SC('9') - 1;
                    if( esc[5] > _SC('9')) esc[5] += _SC('A') - _SC('9') - 1;
                    if( SQ_FAILED(write_check(ctx, esc, sizeof(esc))))
                        return SQ_ERROR;
                } continue;
            }
            
            SQChar esc[2] = { _SC('\\'), c };
            if( SQ_FAILED(write_check(ctx, esc, sizeof(esc))))
    			return SQ_ERROR;
        }
    }
    
    return SQ_OK;
}

static SQRESULT nl_indent( const write_ctx_t *ctx)
{
    if( ctx->str_nl_len && ctx->str_ind_len) {
        if( SQ_FAILED(write_check(ctx, ctx->str_nl, ctx->str_nl_len)))
            return SQ_ERROR;
    
        if( ctx->str_ind_len) {
            SQInteger ind = ctx->ind;
            while( ind--) {
                if( SQ_FAILED(write_check(ctx, ctx->str_ind, ctx->str_ind_len)))
                    return SQ_ERROR;
            }
        }
    }
        
    return SQ_OK;
}

static SQRESULT sqt_serjson_write( write_ctx_t *ctx)
{
	switch(sq_gettype(ctx->v,-1))
	{
		case OT_NULL: {
            return write_check(ctx, _SC("null"), 4*sizeof(SQChar));
		}
		case OT_BOOL: {
			SQBool bval;
			sq_getbool(ctx->v,-1,&bval);
            if( bval) {
                return write_check(ctx, _SC("true"), 4*sizeof(SQChar));
            }
            else {
                return write_check(ctx, _SC("false"), 5*sizeof(SQChar));
            }
		}
		case OT_INTEGER: {
			const SQChar *s;
            SQInteger size;
            SQRESULT r;
            sq_tostring(ctx->v,-1);
			sq_getstringandsize(ctx->v,-1,&s,&size);
            r = write_check(ctx, s, size);
            sq_poptop(ctx->v);
            return r;
		}
		case OT_FLOAT: {
            SQFloat val;
            SQChar *buf = sq_getscratchpad(ctx->v,MAX_FLOAT_STR_SIZE);
            SQChar *p = buf;
            int is_flt = 0;
            sq_getfloat(ctx->v,-1, &val);
            if( !isinf(val)) {
                if( isnan(val)) val = 0.0;  // NaN --> 0
                scsprintf( buf, MAX_FLOAT_STR_SIZE-3, _PRINT_FLOAT_FMT, val);
                while( *p != _SC('\0')) {
                     is_flt = is_flt || (*p == _SC('.')) || (*p == _SC('e')) || (*p == _SC('E'));
                     p++;
                }
                if( !is_flt) {
                    *(p++) = _SC('.');
                    *(p++) = _SC('0');
                    *p = _SC('\0');
                }
                return write_check(ctx, buf, (p - buf)*sizeof(SQChar));
            }
            else {
                if( isinf(val) > 0)
                    return write_check(ctx, _SC("1e999"), 5*sizeof(SQChar));    // Inf --> 1e999
                else
                    return write_check(ctx, _SC("-1e999"), 6*sizeof(SQChar));   // -Inf --> -1e999
            }
        }
		case OT_STRING: {
			const SQChar *s;
            SQChar q = _SC('"');
			sq_getstring(ctx->v,-1,&s);
            if( SQ_FAILED(write_check(ctx, &q, sizeof(q))))
                return SQ_ERROR;
            if( SQ_FAILED( json_escape( ctx, s)))
				return SQ_ERROR;
            return write_check(ctx, &q, sizeof(q));
		}
		case OT_TABLE: {
			SQUnsignedInteger left = sq_getsize(ctx->v,-1);
            SQChar q = _SC('{');
            if( SQ_FAILED(write_check(ctx, &q, sizeof(q))))
                return SQ_ERROR;
            ctx->ind++;
            if( SQ_FAILED( nl_indent( ctx)))
				return SQ_ERROR;
			sq_pushnull(ctx->v);												// table, iterator
			while(SQ_SUCCEEDED(sq_next(ctx->v,-2)))								// table, iterator, key, value
			{
                left--;
                if( sq_gettype(ctx->v,-2) != OT_STRING) {
                    return sq_throwerror(ctx->v,ERR_MSG_KEY_TYPE);
                }
				sq_push(ctx->v,-2);												// table, iterator, key, value, key
				if( SQ_SUCCEEDED( sqt_serjson_write( ctx))) {                   // table, iterator, key, value, key
					sq_poptop(ctx->v);											// table, iterator, key, value
                    q = _SC(':');
                    if( SQ_FAILED(write_check(ctx, &q, sizeof(q)))) {
						sq_pop(ctx->v,3);										// table
						return SQ_ERROR;
                    }
					if( SQ_SUCCEEDED( sqt_serjson_write( ctx))) {               // table, iterator, key, value
						sq_pop(ctx->v,2);										// table, iterator
                        if( left) {
                            q = _SC(',');
                            if( SQ_FAILED(write_check(ctx, &q, sizeof(q)))) {
                                sq_pop(ctx->v,1);								// table
                                return SQ_ERROR;
                            }
                            if( SQ_FAILED( nl_indent( ctx))) {
                                sq_pop(ctx->v,1);                               // table
                                return SQ_ERROR;
                            }
                        }
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
            ctx->ind--;
            if( SQ_FAILED( nl_indent( ctx)))
				return SQ_ERROR;
            q = _SC('}');
            return write_check(ctx, &q, sizeof(q));
		}
		case OT_ARRAY: {
			SQUnsignedInteger left = sq_getsize(ctx->v,-1);
            SQChar q = _SC('[');
			if( SQ_FAILED(write_check(ctx, &q, sizeof(q))))
				return SQ_ERROR;
            ctx->ind++;
            if( SQ_FAILED( nl_indent( ctx)))
				return SQ_ERROR;
			sq_pushnull(ctx->v);												// array, iterator
			while(SQ_SUCCEEDED(sq_next(ctx->v,-2)))								// array, iterator, key, value
			{
                left--;
				if( SQ_SUCCEEDED(sqt_serjson_write( ctx))) {                    // array, iterator, key, value
					sq_pop(ctx->v,2);											// array, iterator
                    if( left) {
                        q = _SC(',');
                        if( SQ_FAILED(write_check(ctx, &q, sizeof(q)))) {
                            sq_pop(ctx->v,1);                                   // table
                            return SQ_ERROR;
                        }
                        if( SQ_FAILED( nl_indent( ctx))) {
                            sq_pop(ctx->v,1);                                   // table
                            return SQ_ERROR;
                        }
                    }
				}
				else {															// array, iterator, key, value
					sq_pop(ctx->v,3);											// array
					return SQ_ERROR;
				}
			}																	// array, iterator
			sq_pop(ctx->v,1);													// array
            ctx->ind--;
            if( SQ_FAILED( nl_indent( ctx)))
				return SQ_ERROR;
            q = _SC(']');
            return write_check(ctx, &q, sizeof(q));
		}
		//case OT_INSTANCE:
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

#define SQ_EOF (SQChar)-1

static SQChar read_char( read_ctx_t *ctx)
{
    SQChar c;
    
    if( ctx->back_char) {
        c = ctx->back_char;
        ctx->back_char = 0;
        return c;
    }
    
    if( ctx->readfct( ctx->user, &c, sizeof(SQChar)) != sizeof(SQChar)) {
        return SQ_EOF;
    }
    
    return c;
}

static SQRESULT skip_spaces( read_ctx_t *ctx, SQChar *pc)
{
    SQChar c;
    
    // skip spaces
    while(1) {
        while( ((c = read_char(ctx)) != SQ_EOF) && scisspace(c));

        if( c == _SC('/')) {
            c = read_char(ctx);
            if( c == _SC('/')) {
                // line comment
                while( ((c = read_char(ctx)) != SQ_EOF) && (c != _SC('\r')) && (c != _SC('\n')) );
                if( c == SQ_EOF) {
                    return sq_throwerror(ctx->v,ERR_MSG_EOF_COMMENT);
                }
            }
            else if( c == _SC('*')) {
                /* block comment */
                while(1) {
                    while( ((c = read_char(ctx)) != SQ_EOF) && (c != _SC('*')) );
                
                    if( c != SQ_EOF)
                        while( ((c = read_char(ctx)) != SQ_EOF) && (c == _SC('*')) );
                
                    if( c == _SC('/')) {
                        break;
                    }
                    else if( c == SQ_EOF) {
                        return sq_throwerror(ctx->v,ERR_MSG_EOF_COMMENT);
                    }
                }
            }
            else {
                return sq_throwerror(ctx->v,ERR_MSG_BAD_CONTENT);
            }
        }
        else if( c == SQ_EOF) {
            return sq_throwerror(ctx->v,ERR_MSG_EOF);
        }
        else break;
    }
    
    *pc = c;
    return SQ_OK;
}

static SQRESULT check_kwd( read_ctx_t *ctx, const SQChar *kwd, SQChar *pc)
{
    SQChar c;
    while( *kwd) {
        if( (c = read_char(ctx)) == *kwd) {
            kwd++;
        }
        else {
            break;
        }
    }
    if( !*kwd) {
        c = read_char(ctx);
        if( !scisalnum(c)) {
            *pc = c;
            return SQ_OK;
        }
    }
    return sq_throwerror(ctx->v,ERR_MSG_BAD_CONTENT);
}

#define STORE(_c) \
do { \
    if( str_len >= str_aloc) { \
        str_aloc += STR_STEP; \
        str = sq_getscratchpad(ctx->v,str_aloc); \
    } \
    str[str_len++] = (_c); \
} while(0)

static SQRESULT read_number( read_ctx_t *ctx, SQChar *pc)
{
    SQChar c = *pc;
    SQInteger str_len = 0;
    SQInteger str_aloc = STR_STEP;
    int is_float = 0;
    int have_num = 0;
    SQChar *str = sq_getscratchpad(ctx->v,str_aloc);

    str[str_len++] = c;
    if( scisdigit(c) ) {
        have_num = 1;
    }
//     else if( c == _SC('-') ) {
//     }
    
    while( ((c = read_char(ctx)) != SQ_EOF) && scisdigit(c)) {
        STORE( c);
        have_num = 1;
    }
    if( have_num && ((c == _SC('.')) || (c == _SC('e')) || (c == _SC('E'))) ) {
        is_float = 1;
        if( c == _SC('.')) {
            STORE( c);
            have_num = 0;
            while( ((c = read_char(ctx)) != SQ_EOF) && scisdigit(c)) {
                STORE( c);
                have_num = 1;
            }
        }
        if( have_num && ((c == _SC('e')) || (c == _SC('E'))) ) {
            STORE( c);
            if( (c = read_char(ctx)) == SQ_EOF) {
                return sq_throwerror(ctx->v,ERR_MSG_EOF_NUMBER);
            }
            have_num = 0;
            if( scisdigit(c) ) {
                have_num = 1;
            }
            else if( (c == _SC('-')) || (c == _SC('+'))) { /* nothing */ }
            else {
                return sq_throwerror(ctx->v,ERR_MSG_FMT_NUMBER);
            }
            STORE( c);
            while( ((c = read_char(ctx)) != SQ_EOF) && scisdigit(c)) {
                STORE( c);
                have_num = 1;
            }
        }
    }
    if( !have_num) {
        return sq_throwerror(ctx->v,ERR_MSG_EXP_NUMBER);
    }
    *pc = c;
    STORE( _SC('\0'));
    if( !is_float) {
        SQChar *end;
        SQInteger val = scstrtol( str, &end, 10);
        sq_pushinteger( ctx->v, val);
    }
    else {
        SQChar *end;
        SQFloat val = (SQFloat)scstrtod( str, &end);
        sq_pushfloat( ctx->v, val);
    }
    return SQ_OK;
}

static SQRESULT sqt_serjson_read( read_ctx_t *ctx)
{
    SQChar c;
    
    if(SQ_FAILED(skip_spaces(ctx,&c)))
        return SQ_ERROR;
    
    if( c == _SC('"')) {
        // string
        SQInteger str_aloc = STR_STEP;
        SQInteger str_len = 0;
        SQChar *str = sq_getscratchpad(ctx->v,str_aloc);
        while( ((c = read_char(ctx)) != SQ_EOF) && (c != _SC('"'))) {
            if( c == _SC('\\')) {
                if( (c = read_char(ctx)) == SQ_EOF) {
                    return sq_throwerror(ctx->v,ERR_MSG_EOF_STRING);
                }
                if( (c == _SC('"')) || (c == _SC('\\')) || (c == _SC('/')) ) { /* nothing */ }
                else if( c == _SC('u')) {
                    // unicode
                    SQChar hex[4];
                    int i;
                    SQUnsignedInteger code;
                    if( ctx->readfct( ctx->user, &hex, sizeof(hex)) != sizeof(hex)) {
                        return sq_throwerror(ctx->v,ERR_MSG_EOF_STRING);
                    }
                    for( i=0; i < sizeof(hex); i++) {
                        if( scisxdigit(hex[i])) {
                            if( hex[i] >= _SC('a')) { hex[i] -= _SC('a') - 10; }
                            else if( hex[i] >= _SC('A')) { hex[i] -= _SC('A') - 10; }
                            else { hex[i] -= _SC('0'); }
                        }
                        else {
                            return sq_throwerror(ctx->v,ERR_MSG_BAD_ESCAPE);
                        }
                    }
                    code = (hex[0] << 12) | (hex[1] << 8) | (hex[2] << 4) | hex[3];
                    if( code == 0)
                    {
                        return sq_throwerror(ctx->v,ERR_MSG_BAD_ESCAPE);
                    }
#ifdef SQUNICODE
                    c = (SQChar)code;
#else // SQUNICODE
                    if( code < 0x80) {
                        c = (SQChar)code;
                    }
                    else {
                        if( (str_len + 2) > str_aloc) {
                            str_aloc += STR_STEP;
                            str = sq_getscratchpad(ctx->v,str_aloc);
                        }
                        if( code < 0x800) {
                            str[str_len++] = 0xC0 | (code >> 6);
                            c = 0x80 | (code & 0x3F);
                        }
                        else {
                            str[str_len++] = 0xE0 | (code >> 12);
                            str[str_len++] = 0x80 | ((code >> 6) & 0x3F);
                            c = 0x80 | (code & 0x3F);
                        }
                    }
#endif // SQUNICODE
                }
                else {
                    switch( c) {
                        case _SC('b'): c = _SC('\b'); break;
                        case _SC('f'): c = _SC('\f'); break;
                        case _SC('n'): c = _SC('\n'); break;
                        case _SC('r'): c = _SC('\r'); break;
                        case _SC('t'): c = _SC('\t'); break;
                        default: return sq_throwerror(ctx->v,ERR_MSG_BAD_ESCAPE);
                    }
                }
            }
            else if( (c == _SC('\n')) || (c == _SC('\r'))) {
                return sq_throwerror(ctx->v,ERR_MSG_STR_NEWLINE);
            }
            STORE(c);
        }
        if( c == _SC('"')) {
            sq_pushstring(ctx->v, str, str_len);
        }
        else {
            return sq_throwerror(ctx->v,ERR_MSG_EOF_STRING);
        }
    }
    else if( c == _SC('[')) {
        // array
		sq_newarray(ctx->v,0);                              // array
        if(SQ_FAILED(skip_spaces(ctx,&c)))
            return SQ_ERROR;
        if( c != _SC(']')) {
            ctx->back_char = c;
            while(1) {
                if(SQ_FAILED(sqt_serjson_read(ctx)))        // array value
                    return SQ_ERROR;
                if( SQ_FAILED(sq_arrayappend(ctx->v,-2))) {	// array
                    return SQ_ERROR;
                }
                if(SQ_FAILED(skip_spaces(ctx,&c)))
                    return SQ_ERROR;
                if( c == _SC(',')) { /* nothing */ }
                else if( c == _SC(']')) {
                    break;
                }
                else {
                    return sq_throwerror(ctx->v,ERR_MSG_DELIMER);
                }
            }
        }
    }
    else if( c == _SC('{')) {
        // object
    	sq_newtable(ctx->v);    // table
        if(SQ_FAILED(skip_spaces(ctx,&c)))
            return SQ_ERROR;
        if( c != _SC('}')) {
            ctx->back_char = c;
            while(1) {
                if(SQ_FAILED(sqt_serjson_read(ctx)))    // table key
                    return SQ_ERROR;
                if( sq_gettype(ctx->v,-1) != OT_STRING) {
                    return sq_throwerror(ctx->v,ERR_MSG_KEY_TYPE);
                }
                if(SQ_FAILED(skip_spaces(ctx,&c))) {
                    sq_pop(ctx->v,2);                   //
                    return SQ_ERROR;
                }
                if( c != _SC(':')) {
                    sq_pop(ctx->v,2);                   //
                    return sq_throwerror(ctx->v,ERR_MSG_PAIR_DELIMER);
                }
                if(SQ_FAILED(sqt_serjson_read(ctx))) {  // table key vlaue
                    sq_pop(ctx->v,2);                   //
                    return SQ_ERROR;
                }
                if( SQ_FAILED(sq_newslot(ctx->v,-3,SQFalse))) {		// table
                    return SQ_ERROR;
                }
                if(SQ_FAILED(skip_spaces(ctx,&c))) {
                    sq_poptop(ctx->v);                   //
                    return SQ_ERROR;
                }
                if( c == _SC(',')) { /* nothing */ }
                else if( c == _SC('}')) {
                    break;
                }
                else {
                    return sq_throwerror(ctx->v,ERR_MSG_DELIMER);
                }
            }
        }
    }
    else if( scisdigit(c) || (c == _SC('-')) ) {
        // number
        if(SQ_FAILED( read_number( ctx, &c))) {
            return SQ_ERROR;
        }
        ctx->back_char = c;
    }
    else if( c == _SC('t') && SQ_SUCCEEDED(check_kwd(ctx, _SC("rue"), &c))) {
        // true
        ctx->back_char = c;
        sq_pushbool(ctx->v, SQTrue);
    }
    else if( c == _SC('f') && SQ_SUCCEEDED(check_kwd(ctx, _SC("alse"), &c))) {
        // false
        ctx->back_char = c;
        sq_pushbool(ctx->v, SQFalse);
    }
    else if( c == _SC('n') && SQ_SUCCEEDED(check_kwd(ctx, _SC("ull"), &c))) {
        // null
        ctx->back_char = c;
        sq_pushnull(ctx->v);
    }
    else {
        return sq_throwerror(ctx->v,ERR_MSG_BAD_CONTENT);
    }
    
    return SQ_OK;
}


SQRESULT sqt_serjson_load_cb(HSQUIRRELVM v, const SQChar *opts, SQREADFUNC readfct, SQUserPointer user)
{
    read_ctx_t ctx;
    
    ctx.v = v;
    ctx.readfct = readfct;
    ctx.user = user;
    ctx.back_char = 0;
    
	return sqt_serjson_read( &ctx);
}

SQRESULT sqt_serjson_load(HSQUIRRELVM v, const SQChar *opts, SQFILE file)
{
    return sqt_serjson_load_cb(v, opts, sqstd_FILEREADFUNC, file);
}

static SQRESULT _g_serjson_loadjson(HSQUIRRELVM v)
{
                                        // this stream opts...
	SQFILE file;
    const SQChar *opts = _SC("");

    if( SQ_FAILED( sq_getinstanceup( v,2,(SQUserPointer*)&file,(SQUserPointer)SQSTD_STREAM_TYPE_TAG))) {
        return sq_throwerror(v,ERR_MSG_ARG_NO_STREAM);
	}
    
    if( sq_gettop(v) > 2)
    {
        sq_getstring(v,3,&opts);
    }
    
	if(SQ_SUCCEEDED(sqt_serjson_load(v, opts, file))) {
                                        // this stream opts... data
		return 1;
    }
    return SQ_ERROR; //propagates the error
}


SQRESULT sqt_serjson_save_cb( HSQUIRRELVM v, const SQChar *opts, SQWRITEFUNC writefct, SQUserPointer user)
{
    write_ctx_t ctx;
    const SQChar *c = opts;
    
    ctx.v = v;
    ctx.writefct = writefct;
    ctx.user = user;
    
    ctx.ind = 0;
    
    ctx.str_nl = _SC("\n");
    ctx.str_nl_len = 1;
    
    ctx.str_ind = _SC("\t");
    ctx.str_ind_len = 1;
    
    while( *c)
    {
        if( (c[0] == _SC('i')) && c[1]) {   // Indentation char
            c++;
            if( *c == _SC('t')) {
                ctx.str_ind = _SC("\t");    // use Tab
                ctx.str_ind_len = 1;
            }
            else if( *c == _SC('s')) {      // use Space
                ctx.str_ind = _SC(" ");
                ctx.str_ind_len = 1;
            }
            else if( *c == _SC('-')) {      // do not indent
                ctx.str_ind = _SC("");
                ctx.str_ind_len = 0;
            }
        }
        else if( (c[0] == _SC('l')) && c[1]) {  // New Line style
            c++;
            if( *c == _SC('c')) {       // C style - \n
                ctx.str_nl = _SC("\n");
                ctx.str_nl_len = 1;
            }
            else if( *c == _SC('p')) {  // Printer style \r\n
                ctx.str_nl = _SC("\r\n");
                ctx.str_nl_len = 2;
            }
            else if( *c == _SC('-')) {  // no new line
                ctx.str_nl = _SC("");
                ctx.str_nl_len = 0;
            }
        }
        c++;
    }
    
	if(SQ_FAILED(sqt_serjson_write( &ctx))) {
            return SQ_ERROR;
    }
    return write_check(&ctx, ctx.str_nl, ctx.str_nl_len);
}

SQRESULT sqt_serjson_save( HSQUIRRELVM v, const SQChar *opts, SQFILE file)
{
    return sqt_serjson_save_cb(v, opts, sqstd_FILEWRITEFUNC, file);
}

static SQRESULT _g_serjson_savejson(HSQUIRRELVM v)
{
                                // this stream data opts...
	SQFILE file;
    const SQChar *opts = _SC("");

    if( SQ_FAILED( sq_getinstanceup( v,2,(SQUserPointer*)&file,(SQUserPointer)SQSTD_STREAM_TYPE_TAG))) {
        return sq_throwerror(v,ERR_MSG_ARG_NO_STREAM);
	}
    
    if( sq_gettop(v) > 3)
    {
        sq_push(v,3);           // this stream data opts... data
        sq_remove(v,3);         // this stream opts... data
        sq_getstring(v,3,&opts);
    }
    
	if(SQ_SUCCEEDED(sqt_serjson_save(v,opts,file))) {
		return 0;   // no return value
    }
    return SQ_ERROR; //propagates the error
}

#define _DECL_GLOBALSERJSON_FUNC(name,nparams,typecheck) {_SC(#name),_g_serjson_##name,nparams,typecheck}
static const SQRegFunction _serjson_funcs[]={
    _DECL_GLOBALSERJSON_FUNC(loadjson,-2,_SC(".xs")),
    _DECL_GLOBALSERJSON_FUNC(savejson,-3,_SC(".x.s")),
    {NULL,(SQFUNCTION)0,0,NULL}
};

SQRESULT sqstd_register_serjson(HSQUIRRELVM v)
{
	return sqstd_registerfunctions(v, _serjson_funcs);
}

