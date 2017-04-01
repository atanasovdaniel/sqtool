
{
	local wr = textwriter( stdout, false);
	wr.print( "здравей!\n");
	wr.close();
}

{
	//Здравей, свят
	local UTF8=[0xd0, 0x97, 0xd0, 0xb4, 0xd1, 0x80, 0xd0, 0xb0, 0xd0, 0xb2, 0xd0, 0xb5, 0xd0, 0xb9, 0x2c, 0x20, 0xd1, 0x81, 0xd0, 0xb2, 0xd1, 0x8f, 0xd1, 0x82, '\n'];
	local b = blob(1);
	local wr = textwriter( stdout, false);
	foreach( c in UTF8)
	{
		b[0] = c;
		wr.writeblob( b);
	}
	wr.close();
}

{
	// A{U+4E12}З{U+4E12}B\n
	local UTF8=['A', 0xE4 0xB8 0x92, 0xd0, 0x97, 0xE4 0xB8 0x92, 'B', '\n'];
	local b = blob(1);
	local wr = textwriter( stdout, false);
	foreach( c in UTF8)
	{
		b[0] = c;
		wr.writeblob( b);
	}
	wr.close();
}
