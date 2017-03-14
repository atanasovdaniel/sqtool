
errors <- 0;

function check_readline_two_lines( s, line_1, line_2)
{
	// read first line
	//	
	local l = s.readline();
	if( l != line_1)
	{
		print( "! Bad line [[" + escape(l) + "]]\n");
		errors++;
	}
	// read second line
	//	
	l = s.readline();
	if( l != line_2)
	{
		print( "! Bad line [[" + escape(l) + "]]\n");
		errors++;
	}
	// blob EOS reached
	//	
	if( (typeof s == "blob") && !s.eos())
	{
		print( "! Wrong eos()\n");
		errors++;
	}
	// read after end
	//	
	l = s.readline();
	if( l != "")
	{
		print( "! Bad line [[" + escape(l) + "]]\n");
		errors++;
	}
	// EOS reached
	//	
	if( !s.eos())
	{
		print( "! Wrong eos()\n");
		errors++;
	}
}

function check_readline_non_terminated_line( s, line_1)
{
	local l = s.readline();
	if( l != line_1)
	{
		print( "! Bad line [[" + l + "]]\n");
		errors++;
	}
	// EOS reached
	//	
	if( !s.eos())
	{
		print( "! Wrong eos()\n");
		errors++;
	}
	// read after end
	//	
	l = s.readline();
	if( l != "")
	{
		print( "! Bad line [[" + l + "]]\n");
		errors++;
	}
}


if( "readline" in stream)
{
	print( "Checking stream.readline()\n");
	
	print( " blob\n");
	// blob
	{
		local b = blob();
		
		b.writen( 'h', 'b');
		b.writen( 'e', 'b');
		b.writen( 'l', 'b');
		b.writen( 'l', 'b');
		b.writen( 'o', 'b');
		b.writen( '\n', 'b');
		b.writen( 'w', 'b');
		b.writen( 'o', 'b');
		b.writen( 'r', 'b');
		b.writen( 'l', 'b');
		b.writen( 'd', 'b');
		b.writen( '\n', 'b');
		
		b.seek( 0);
		
		check_readline_two_lines( b, "hello\n", "world\n");
	}
	{
		// check blob
		
		local b = blob();
		
		b.writen( 'a', 'b');
		b.writen( 'g', 'b');
		b.writen( 'a', 'b');
		b.writen( 'i', 'b');
		b.writen( 'n', 'b');
		
		b.seek( 0);
		
		check_readline_non_terminated_line( b, "again");
	}
	
	print( " file\n");
	// file - lf
	print( "file - lf\n");
	{
		local f = file( "./test/file_tl_lf.txt", "r");
		
		check_readline_two_lines( f, "hello\n", "world\n");
		
		f.close();
	}
	// file - crlf
	print( "file - crlf\n");
	{
		local f = file( "./test/file_tl_crlf.txt", "r");
		
		check_readline_two_lines( f, "hello\r\n", "world\r\n");
		
		f.close();
	}
	// file - non terminated
	print( "file - non terminated\n");
	{
		local f = file( "./test/file_nt.txt", "r");
		
		check_readline_non_terminated_line( f, "again");
		
		f.close();
	}
	
	if( "popen" in this)
	{
		//print( "Have stream.readline() and popen()\n");
		
		local cmd;
		if( _osname_ == "windows")
			cmd = "cat" /*"type"*/;
		else
			cmd = "cat";
		
		print( " popen " + cmd + "\n");
		// popen - lf
		print( "popen - lf\n");
		{
			local f = popen( cmd + " ./test/file_tl_lf.txt", "r");
		
			check_readline_two_lines( f, "hello\n", "world\n");
		
			f.close();
		}
		// file - crlf
		print( "popen - crlf\n");
		{
			local f = popen( cmd + " ./test/file_tl_crlf.txt", "r");
		
			check_readline_two_lines( f, "hello\r\n", "world\r\n");
		
			f.close();
		}
		// file - non terminated
		print( "popen - non terminated\n");
		{
			local f = popen( cmd + " ./test/file_nt.txt", "r");
		
			check_readline_non_terminated_line( f, "again");
		
			f.close();
		}
	}
}
else
{
	print( "Don't have stream.readline()\n");
}

if( "print" in stream)
{
	print( "Checking stream.print()\n");
	
	
}
else
{
	print( "Don't have stream.print()\n");
}


if( errors) print( "! Errors " + errors + "\n");

return errors;

