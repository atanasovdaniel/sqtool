/* see copyright notice in sqtool.h */

static SQInteger _g_fs_fsListDir( HSQUIRRELVM v)
{
    DIR *dir;
	const SQChar *find;

    sq_getstring( v, 2, &find);

    if( !*find)
    {
		find = ".";
    }

    dir = opendir( find);
	if( dir)
    {
        struct dirent *ent;

        sq_newtable( v);

        while( (ent = readdir( dir)) != 0)
        {
			int is_dir = ent->d_type == DT_DIR;

			// skip . and ..
			if( is_dir)
			{
				if( (ent->d_name[0] == '.')
					&&
					(
					 (ent->d_name[1] == '\0')
					 ||
					 (
					  (ent->d_name[1] == '.')
					  &&
					  (ent->d_name[2] == '\0')
					  )
					)
				)
				{
					continue;
				}
			}

			sq_pushstring( v, ent->d_name, -1);
			sq_pushbool( v, is_dir);
            sq_newslot( v, -3, SQFalse);

		}

        closedir(dir);
    }
    else
    {
        sq_pushnull(v);
    }

    return 1;
}
