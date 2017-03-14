
{
	local p = popen( "some_unknown_command", "r");
	print( "p=" + p + "\n");
	local l = p.readline();
	print( "[[" + l + "]]\n");
	local r = p.close();
	print( "r=" + r + "\n");
}

{
	local p = popen( "ls -la", "r");
	print( "p=" + p + "\n");
	while( !p.eos())
	{
		local l = p.readline();
		print( "[[" + l + "]]\n");
	}
	local r = p.close();
	print( "r=" + r + "\n");
}

{
	local p = popen( "some_unknown_command", "w");
	print( "p=" + p + "\n");
	p.print( "hello world\n");
	local r = p.close();
	print( "r=" + r + "\n");
}


{
	local p = popen( "cat", "w");
	print( "p=" + p + "\n");
	p.print( "line_1\n");
	p.print( "line");
	p.print( "_2\n");
	p.print( "line_3\n");
	p.print( "line_4");
	local r = p.close();
	print( "r=" + r + "\n");
}
