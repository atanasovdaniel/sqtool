/* see copyright notice in sqtool.h */

static SQInteger _g_fs_fsExist( HSQUIRRELVM v)
{
	DWORD r;
	LPWSTR name;

    sqt_getstring_wc( v, 2, &name);

    r = GetFileAttributesW( name);

	sq_pushbool( v, (r == INVALID_FILE_ATTRIBUTES) ? SQFalse : SQTrue);

	return 1;
}

static SQInteger _g_fs_fsIsFile( HSQUIRRELVM v)
{
	DWORD r;
	LPWSTR name;

    sqt_getstring_wc( v, 2, &name);

    r = GetFileAttributesW( name);

	sq_pushbool( v, (r == INVALID_FILE_ATTRIBUTES) ? SQFalse : (r & FILE_ATTRIBUTE_DIRECTORY) ? SQFalse : SQTrue);

	return 1;
}

static SQInteger _g_fs_fsIsDir( HSQUIRRELVM v)
{
	DWORD r;
	LPWSTR name;

    sqt_getstring_wc( v, 2, &name);

    r = GetFileAttributesW( name);

	sq_pushbool( v, (r == INVALID_FILE_ATTRIBUTES) ? SQFalse : (r & FILE_ATTRIBUTE_DIRECTORY) ? SQTrue : SQFalse);

	return 1;
}


