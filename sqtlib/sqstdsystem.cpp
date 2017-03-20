/* see copyright notice in squirrel.h */
/* see copyright notice in sqtool.h */

#ifdef _WIN32
#include <direct.h>  
#else // _WIN32
#include <unistd.h>
#include <string.h>
#include <limits.h>
#endif // _WIN32
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include <squirrel.h>
#include <sqtool.h>
#include <sqstdsystem.h>


static SQInteger _system_getenv(HSQUIRRELVM v)
{
#ifdef _WIN32
	wchar_t *nm;
	sqt_getstring_wc(v,2,&nm);
    sqt_pushstring_wc(v,_wgetenv(nm),-1);
#else // _WIN32
    const SQChar *s;
    sq_getstring(v,2,&s);
    sq_pushstring(v,getenv(s),-1);
#endif // _WIN32
    return 1;
}

static SQInteger _system_setenv(HSQUIRRELVM v)
{
	int r;
#ifdef _WIN32
	SQUnsignedInteger len;
	int p;
    const SQChar *strs[2];
    sq_getstring(v,2,&strs[0]);
    if(sq_gettop(v) >= 3) {
	    sq_getstring(v,3,&strs[1]);
    }
	else {
		strs[1] = _SC("");
	}
	sqt_strtowc( 2, (SQChar**)strs, &len);
	p = wcslen( (wchar_t*)strs[0]);
	((wchar_t*)strs[0])[p] = '=';
	r = _wputenv( (wchar_t*)strs[0]);
	sq_free( (void*)strs[0], len);
#else // _WIN32
    const SQChar *name,*val;
    sq_getstring(v,2,&name);
    if(sq_gettop(v) >= 3) {
		sq_getstring(v,3,&val);
		r = setenv( name, val, 1);
	}
	else {
		r = unsetenv(name);
	}
#endif // _WIN32
    if(r!=0) {
        return sq_throwerror(v,_SC("setenv() failed"));
	}
	return 0;
}

static SQInteger _system_system(HSQUIRRELVM v)
{
	int r;
#ifdef _WIN32
    wchar_t *s;
    sqt_getstring_wc(v,2,&s);
    r = _wsystem(s);
#else // _WIN32
    const SQChar *s;
    sq_getstring(v,2,&s);
    r = system(s);
#endif // _WIN32
	sq_pushinteger(v,r);
    return 1;
}

static SQInteger _system_getcwd(HSQUIRRELVM v)
{
#ifdef _WIN32
	wchar_t *dest, *buff;
	int len = _MAX_PATH;
	buff = (wchar_t*)sq_malloc( len*sizeof(*buff));
	do {
		dest = _wgetcwd( buff, len);
		if( dest || (errno != ERANGE))
			break;
		sq_realloc( buff, len*sizeof(*buff), (len+_MAX_PATH)*sizeof(*buff));
		len += _MAX_PATH;
	} while( len < 10000);	// some fallback to prevent infinite loop
	if( dest) {
		sqt_pushstring_wc(v, dest, -1);
	}
	else {
		sq_pushnull(v);
	}
	sq_free( buff, len*sizeof(*buff));
	return 1;
#else // _WIN32
	SQChar *dest;
	size_t len = PATH_MAX;
	do {
		dest = sq_getscratchpad( v, len);
		dest = getcwd( dest, len);
		if( dest || (errno != ERANGE))
			break;
		len += PATH_MAX;
	} while( len < 10000);	// some fallback to prevent infinite loop
	if( dest) { 
#ifdef __linux__
		if( strncmp( dest, "(unreachable)", 13) == 0)
			dest += 13;
#endif // __linux__
		sq_pushstring(v, dest, -1);
		return 1;
	}
	sq_pushnull(v);
	return 1;
#endif // _WIN32
}

static SQInteger _system_chdir(HSQUIRRELVM v)
{
	SQInteger r;
#ifdef _WIN32
	wchar_t *path;
	sqt_getstring_wc(v,2,&path);
	r = _wchdir( path);
#else // _WIN32
	const SQChar *path;
	sq_getstring(v,2,&path);
	r = chdir( path);
#endif // _WIN32
	sq_pushinteger( v, r);
	return 1;
}

static SQInteger _system_remove(HSQUIRRELVM v)
{
	int r;
#ifdef _WIN32
    wchar_t *s;
    sqt_getstring_wc(v,2,&s);
    r = _wremove(s);
#else // _WIN32
    const SQChar *s;
    sq_getstring(v,2,&s);
    r = remove(s);
#endif // _WIN32
    if(r!=0)
        return sq_throwerror(v,_SC("remove() failed"));
    return 0;
}

static SQInteger _system_rename(HSQUIRRELVM v)
{
	int r;
#ifdef _WIN32
	SQUnsignedInteger len;
    const SQChar *names[2];
    sq_getstring(v,2,&names[0]);
    sq_getstring(v,3,&names[1]);
	sqt_strtowc( 2, (SQChar**)names, &len);
	r = _wrename( (wchar_t*)names[0], (wchar_t*)names[1]);
	sq_free( (void*)names[0], len);
#else // _WIN32
    const SQChar *oldn,*newn;
    sq_getstring(v,2,&oldn);
    sq_getstring(v,3,&newn);
	r = rename( oldn, newn);
#endif // _WIN32
    if(r!=0)
        return sq_throwerror(v,_SC("rename() failed"));
    return 0;
}

static SQInteger _system_clock(HSQUIRRELVM v)
{
    sq_pushfloat(v,((SQFloat)clock())/(SQFloat)CLOCKS_PER_SEC);
    return 1;
}

static SQInteger _system_time(HSQUIRRELVM v)
{
    SQInteger t = (SQInteger)time(NULL);
    sq_pushinteger(v,t);
    return 1;
}

static void _set_integer_slot(HSQUIRRELVM v,const SQChar *name,SQInteger val)
{
    sq_pushstring(v,name,-1);
    sq_pushinteger(v,val);
    sq_rawset(v,-3);
}

static SQInteger _system_date(HSQUIRRELVM v)
{
    time_t t;
    SQInteger it;
    SQInteger format = 'l';
    if(sq_gettop(v) > 1) {
        sq_getinteger(v,2,&it);
        t = it;
        if(sq_gettop(v) > 2) {
            sq_getinteger(v,3,(SQInteger*)&format);
        }
    }
    else {
        time(&t);
    }
    tm *date;
    if(format == 'u')
        date = gmtime(&t);
    else
        date = localtime(&t);
    if(!date)
        return sq_throwerror(v,_SC("crt api failure"));
    sq_newtable(v);
    _set_integer_slot(v, _SC("sec"), date->tm_sec);
    _set_integer_slot(v, _SC("min"), date->tm_min);
    _set_integer_slot(v, _SC("hour"), date->tm_hour);
    _set_integer_slot(v, _SC("day"), date->tm_mday);
    _set_integer_slot(v, _SC("month"), date->tm_mon);
    _set_integer_slot(v, _SC("year"), date->tm_year+1900);
    _set_integer_slot(v, _SC("wday"), date->tm_wday);
    _set_integer_slot(v, _SC("yday"), date->tm_yday);
    return 1;
}

#define _DECL_FUNC(name,nparams,pmask) {_SC(#name),_system_##name,nparams,pmask}
static const SQRegFunction systemlib_funcs[]={
    _DECL_FUNC(getenv,2,_SC(".s")),
    _DECL_FUNC(setenv,-2,_SC(".ss")),
    _DECL_FUNC(system,2,_SC(".s")),
    _DECL_FUNC(clock,0,NULL),
    _DECL_FUNC(time,1,NULL),
    _DECL_FUNC(date,-1,_SC(".nn")),
    _DECL_FUNC(remove,2,_SC(".s")),
    _DECL_FUNC(rename,3,_SC(".ss")),
    _DECL_FUNC(getcwd,1,NULL),
    _DECL_FUNC(chdir,2,_SC(".s")),
    {NULL,(SQFUNCTION)0,0,NULL}
};
#undef _DECL_FUNC

SQInteger sqstd_register_systemlib(HSQUIRRELVM v)
{
	sqt_declarefunctions(v, systemlib_funcs);
    return 1;
}
