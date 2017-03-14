/* see copyright notice in sqtool.h */
#include <squirrel.h>
#include <sqtool.h>

#ifdef _WIN32

#include <windows.h>

void sqt_pushstring_wc( HSQUIRRELVM v, const wchar_t *src, SQInteger len)
{
	// !!! len is not used
    SQChar *dest;
    SQInteger allocated;

    allocated = WideCharToMultiByte( CP_UTF8, 0 /*MB_ERR_INVALID_CHARS*/, src, -1, NULL, 0, NULL, NULL);

	dest = sq_getscratchpad(v,allocated);

    if( WideCharToMultiByte( CP_UTF8, 0, src, -1, dest, allocated, NULL, NULL) != 0)
    {
        sq_pushstring( v, dest, allocated-1);
    }
    else
    {
        sq_pushstring( v, _SC(""), -1);
    }
}

SQInteger sqt_getstring_wc( HSQUIRRELVM v, SQInteger idx, wchar_t **dst)
{
    const SQChar *src;
    LPWSTR dest;
    SQInteger allocated;

    sq_getstring( v, idx, &src);

    allocated = MultiByteToWideChar( CP_UTF8, 0 /*MB_ERR_INVALID_CHARS*/, src, -1, NULL, 0);

	dest = (LPWSTR)sq_getscratchpad( v, sizeof(*dest)*allocated);

    if( MultiByteToWideChar( CP_UTF8, 0, src, -1, dest, allocated) != 0)
    {
        *dst = dest;
        return SQ_OK;
    }
    else
    {
        *dst = 0;
        return SQ_ERROR;
    }
}

SQInteger sqt_strtowc( SQInteger n, SQChar **strs, SQUnsignedInteger *ptotal_len)
{
    LPWSTR dest = 0;
	SQInteger i;
	SQUnsignedInteger total_len = 0;
	
	for( i=0; i < n; i++)
	{
	    SQInteger l = MultiByteToWideChar( CP_UTF8, 0, strs[i], -1, NULL, 0);
		total_len += l;
	}
	
	total_len *= sizeof(*dest);
	
	dest = (LPWSTR)sq_malloc( total_len);
	*ptotal_len = total_len;
	total_len /= sizeof(*dest);
	
	for( i=0; i < n; i++)
	{
	    SQInteger l = MultiByteToWideChar( CP_UTF8, 0, strs[i], -1, dest, total_len);
		
		strs[i] = (SQChar*)dest;
		dest += l;
		total_len -= l;
	}
	
	return 0;
}

SQInteger sqt_wctostr( SQInteger n, SQChar **strs, SQUnsignedInteger *ptotal_len)
{
	SQChar *dest = 0;
	SQInteger i;
	SQUnsignedInteger total_len = 0;
	
	for( i=0; i < n; i++)
	{
		SQInteger l = WideCharToMultiByte( CP_UTF8, 0 /*MB_ERR_INVALID_CHARS*/, (wchar_t*)strs[i], -1, NULL, 0, NULL, NULL);
		total_len += l;
	}
	
	total_len *= sizeof(*dest);
	
	dest = (SQChar*)sq_malloc( total_len);
	*ptotal_len = total_len;
	total_len /= sizeof(*dest);
	
	for( i=0; i < n; i++)
	{
		SQInteger l = WideCharToMultiByte( CP_UTF8, 0, (wchar_t*)strs[i], -1, dest, total_len, NULL, NULL);
		
		strs[i] = (SQChar*)dest;
		dest += l;
		total_len -= l;
	}
	
	return 0;
}

#endif // _WIN32
