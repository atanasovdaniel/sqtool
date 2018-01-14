/* see copyright notice in sqtool.h */

static SQInteger _g_fs_fsListDir( HSQUIRRELVM v)
{
	WIN32_FIND_DATAW FindFileData;
	HANDLE hFind;
	LPWSTR str = 0;
	LPWSTR find;
    size_t plen;

    sqt_getstring_wc( v, 2, &str);

    plen = wcslen( str);
    if( plen)
    {
        find = malloc( (plen + 3)*sizeof(*find));
        wcscpy( find, str);
        if( (find[plen-1] != L'\\') && (find[plen-1] != L'/'))
        {
            find[plen] = L'\\';
            plen++;
        }
        find[plen] = L'*';
        find[plen+1] = L'\0';
    }
    else
    {
        find = L"*";
    }

	hFind = FindFirstFileW( find, &FindFileData);

	if( hFind != INVALID_HANDLE_VALUE)
    {
        sq_newtable( v);

        do
        {
			int is_dir = FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;

			// skip . and ..
			if( is_dir)
			{
				if( (FindFileData.cFileName[0] == L'.')
					&&
					(
					 (FindFileData.cFileName[1] == L'\0')
					 ||
					 (
					  (FindFileData.cFileName[1] == L'.')
					  &&
					  (FindFileData.cFileName[2] == L'\0')
					  )
					)
				)
				{
					continue;
				}
			}

			sqt_pushstring_wc( v, FindFileData.cFileName, -1);
			sq_pushbool( v, is_dir);
            sq_newslot( v, -3, SQFalse);

		} while( FindNextFileW( hFind, &FindFileData) != 0);

		FindClose( hFind);
    }
    else
    {
        sq_pushnull(v);
    }

    if( plen)
    {
        free( find);
    }

    return 1;
}
