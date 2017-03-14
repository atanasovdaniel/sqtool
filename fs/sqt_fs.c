/* see copyright notice in sqtool.h */
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include <squirrel.h>
#include <sqtool.h>

#include "sqt_fs.h"

#define _DECL_GLOBALFS_FUNC(name,nparams,typecheck) {_SC(#name),_g_fs_##name,nparams,typecheck}

#ifdef _WIN32

#include <windows.h>
#include <wchar.h>

#include "listDir_win_impl.h"
#include "stat_win_impl.h"

#else // _WIN32

#include <sys/stat.h>
#include <dirent.h>

#include "listDir_linux_impl.h"
#include "stat_linux_impl.h"

#endif // _WIN32

static SQRegFunction ezxml_funcs[]={
	_DECL_GLOBALFS_FUNC( fsListDir, 2, _SC(".s")),
	_DECL_GLOBALFS_FUNC( fsExist, 2, _SC(".s")),
	_DECL_GLOBALFS_FUNC( fsIsFile, 2, _SC(".s")),
	_DECL_GLOBALFS_FUNC( fsIsDir, 2, _SC(".s")),
	{0,0}
};

SQRESULT sqstd_register_fs(HSQUIRRELVM v)
{
	return sqt_declarefunctions( v, ezxml_funcs);
}
