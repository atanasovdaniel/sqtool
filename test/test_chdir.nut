
{
	local cwd;
	local r;
	
	cwd = getcwd();
	print( "getcwd() -> \"" + cwd + "\"\n");
	if( cwd == null) print( "!!! BAD result\n");
	
	r = chdir( "_nowhere_");
	print( "chdir( \"_nowhere_\") -> " + r + "\n");
	if( r != -1) print( "!!! BAD result\n");
	
	r = chdir( "test");
	print( "chdir( \"test\") -> " + r + "\n");
	if( r != 0) {
		print( "!!! BAD result\n");
	}
	else {
		local cdir = getcwd();
		print( "getcwd() -> \"" + cdir + "\"\n");
		if( cdir == null) print( "!!! BAD result\n");
		cdir = cdir.slice(cwd.len()+1);
		print( "Added part is :\"" + cdir + "\"\n");
		
		if( cdir != "test")
			print( "!!! BAD result\n");
	}

	{	
		r = chdir( cwd);
		print( "chdir( cwd) -> " + r + "\n");
		if( r != 0) print( "!!! BAD result\n");
		
		local cdir = getcwd();
		print( "getcwd() -> \"" + cdir + "\"\n");
		if( cdir == null) print( "!!! BAD result\n");
		if( cdir != cwd) print( "!!! BAD result\n");
	}
}