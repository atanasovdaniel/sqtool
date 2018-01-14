/* see copyright notice in sqtool.h */

static SQInteger _g_fs_fsExist( HSQUIRRELVM v)
{
	struct stat st;
	int r;
	const SQChar *name;

	sq_getstring( v, 2, &name);

	r = stat( name, &st);

	sq_pushbool( v, r ? SQFalse : SQTrue);

	return 1;
}

static SQInteger _g_fs_fsIsFile( HSQUIRRELVM v)
{
	struct stat st;
	int r;
	const SQChar *name;

	sq_getstring( v, 2, &name);

	r = stat( name, &st);

	sq_pushbool( v, r ? SQFalse : (S_ISREG(st.st_mode)));

	return 1;
}

static SQInteger _g_fs_fsIsDir( HSQUIRRELVM v)
{
	struct stat st;
	int r;
	const SQChar *name;

	sq_getstring( v, 2, &name);

	r = stat( name, &st);

	sq_pushbool( v, r ? SQFalse : (S_ISDIR(st.st_mode)));

	return 1;
}


