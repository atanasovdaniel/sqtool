
dir_name <- vargv[1];
dir <- fsListDir( dir_name);
if( dir != null)
{
	foreach( name, isdir in dir)
	{
		print( (isdir ? "d " : "  ") + name + "\n");
	}
	return 0;
}
else
{
	error( "Cant dir " + dir_name + "\n");
	return 1;
}
