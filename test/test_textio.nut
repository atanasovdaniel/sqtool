
local textio = require("textio");

local textreader = textio.reader;
local textwriter = textio.writer;

fails <- 0;

UTF8_TESTS <- [

	// 1 byte
	// 0xxx.xxxx
	{
		inp = [ 0, 'Z' ],
		out = [ 0, 'Z' ],
	},
	{
		inp = [ 0x7F, 'Z' ],
		out = [ 0x7F, 'Z' ],
	},
	
	// 1 byte - invalid
	{
		inp = [ 0x80, 'Z' ],
		out = [ '?', 'Z' ],
	},
	{
		inp = [ 0xBF, 'Z' ],
		out = [ '?', 'Z' ],
	},

	// 2 byte
	// 110x.xxxx 10xx.xxxx
	
	// 2 bytes - invalid
	{	// 1100.0000 1000.0000 - 000000
		// 1100.0001 1011.1111 - 00007F
		inp = [ 0xC1, 0xBF, 'Z' ],
		out = [ '?', 'Z' ],
	},
	
	// 2 bytes
	{	// 1100.0010 1000.0000 - 000080
		inp = [ 0xC2, 0x80, 'Z' ],
		out = [ 0xC2, 0x80, 'Z' ],
	},
	{	// 1101.1111 1011.1111 - 0007FF
		inp = [ 0xDF, 0xBF, 'Z' ],
		out = [ 0xDF, 0xBF, 'Z' ],
	},
	{
		inp = [ 0xC2, 'x', 'Z' ],
		out = [ '?', 'Z' ],
	},

	// 3 bytes
	// 1110.xxxx 10xx.xxxx 10xx.xxxx
	{	// 1110.0000 1000.0000 1000.0000 - 000000 (1 byte)
		// 1110.0000 1000.0001 1011.1111 - 00007F
		inp = [ 0xE0, 0x81, 0xBF, 'Z' ],
		out = [ '?', 'Z' ],
	},
	{	// 1110.0000 1000.0010 1000.0000 - 000080 (2 bytes)
		// 1110.0000 1001.1111 1011.1111 - 0007FF
		inp = [ 0xE0, 0x9F, 0xBF, 'Z' ],
		out = [ '?', 'Z' ],
	},
	{	// 1110.0000 1010.0000 1000.0000 - 000800
		inp = [ 0xE0, 0xA0, 0x80, 'Z' ],
		out = [ 0xE0, 0xA0, 0x80, 'Z' ],
	},
	{	// 1110.1111 1011.1111 1011.1111 - 00FFFF
		inp = [ 0xEF, 0xBF, 0xBF, 'Z' ],
		out = [ 0xEF, 0xBF, 0xBF, 'Z' ],
	},
	{
		inp = [ 0xE0, 'x', 'Z' ],
		out = [ '?', 'Z' ],
	},
	{
		inp = [ 0xE0, 0x80, 'x', 'Z' ],
		out = [ '?', 'Z' ],
	},

	// 4 bytes
	// 1111.0xxx 10xx.xxxx 10xx.xxxx 10xx.xxxx
	{	// 1111.0000 1000.0000 1000.0000 1000.0000 - 000000 (1 byte)
		// 1111.0000 1000.0000 1000.0001 1011.1111 - 00007F
		inp = [ 0xF0, 0x80, 0x81, 0xBF, 'Z' ],
		out = [ '?', 'Z' ],
	},
	{	// 1111.0000 1000.0000 1000.0010 1000.0000 - 000080 (2 bytes)
		// 1111.0000 1000.0000 1001.1111 1011.1111 - 0007FF
		inp = [ 0xF0, 0x80, 0x9F, 0xBF, 'Z' ],
		out = [ '?', 'Z' ],
	},
	{	// 1111.0000 1000.0000 1010.0000 1000.0000 - 000800 (3 bytes)
		// 1111.0000 1000.1111 1011.1111 1011.1111 - 00FFFF
		inp = [ 0xF0, 0x8F, 0xBF, 0xBF, 'Z' ],
		out = [ '?', 'Z' ],
	},
	{	// 1111.0000 1001.0000 1000.0000 1000.0000 - 010000
		inp = [ 0xF0, 0x90, 0x80, 0x80, 'Z' ],
		out = [ 0xF0, 0x90, 0x80, 0x80, 'Z' ],
	},
	{	// 1111.0100 1000.1111 1011.1111 1011.1111 - 10FFFF
		inp = [ 0xF4, 0x8F, 0xBF, 0xBF, 'Z' ],
		out = [ 0xF4, 0x8F, 0xBF, 0xBF, 'Z' ],
	},
	{	// 1111.0100 1001.0000 1000.0000 1000.0000 - 110000 - invalid
		inp = [ 0xF4, 0x90, 0x80, 0x80, 'Z' ],
		out = [ '?', 'Z' ],
	},
	{	// 1111.0111 1011.1111 1011.1111 1011.1111 - 1FFFFF - invalid
		inp = [ 0xF7, 0xBF, 0xBF, 0xBF, 'Z' ],
		out = [ '?', 'Z' ],
	},

	{
		inp = [ 0xF0, 'x', 'Z' ],
		out = [ '?', 'Z' ],
	},
	{
		inp = [ 0xF0, 0x90, 'x', 'Z' ],
		out = [ '?', 'Z' ],
	},
	{
		inp = [ 0xF0, 0x90, 0x80, 'x', 'Z' ],
		out = [ '?', 'Z' ],
	},

	//+D800 to U+DFFF
	// 1110.xxxx 10xx.xxxx 10xx.xxxx
	{	// 1110.1101 1001.1111 1011.1111 - 00D7FF
		inp = [ 0xED, 0x9F, 0xBF, 'Z' ],
		out = [ 0xED, 0x9F, 0xBF, 'Z' ],
	},
	{	// 1110.1101 1010.0000 1000.0000 - 00D800
		inp = [ 0xED, 0xA0, 0x80, 'Z' ],
		out = [ '?', 'Z' ],
	},
	{	// 1110.1101 1011.1111 1011.1111 - 00DFFF
		inp = [ 0xED, 0xBF, 0xBF, 'Z' ],
		out = [ '?', 'Z' ],
	},
	{	// 1110.1110 1000.0000 1000.0000 - 00E000
		inp = [ 0xEE, 0x80, 0x80, 'Z' ],
		out = [ 0xEE, 0x80, 0x80, 'Z' ],
	},
];

UTF16BE_TESTS <- [

	// 1 codeunit
	
	// U+0000 to U+D7FF
	{
		inp = [ 0, 0, 0, 'Z' ],
		out = [ 0, 0, 0, 'Z' ],
	},
	{
		inp = [ 0xD7, 0xFF, 0, 'Z' ],
		out = [ 0xD7, 0xFF, 0, 'Z' ],
	},
	
	// 0xDC00..0xDFFF - invalid
	{
		inp = [ 0xDC, 0x00, 0, 'Z' ],
		out = [ 0, '?', 0, 'Z' ],
	},
	
	// U+E000 to U+FFFF
	{
		inp = [ 0xE0, 0x00, 0, 'Z' ],
		out = [ 0xE0, 0x00, 0, 'Z' ],
	},
	{
		inp = [ 0xFF, 0xFF, 0, 'Z' ],
		out = [ 0xFF, 0xFF, 0, 'Z' ],
	},

	// 2 codeunits
	// 0xD800..0xDBFF, 0xDC00..0xDFFF
	{
		inp = [ 0xD8, 0x00, 0xDC, 0x00, 0, 'Z' ],
		out = [ 0xD8, 0x00, 0xDC, 0x00, 0, 'Z' ],
	},
	// 0xD800..0xDBFF, 0xDC00..0xDFFF
	{
		inp = [ 0xDB, 0xFF, 0xDF, 0xFF, 0, 'Z' ],
		out = [ 0xDB, 0xFF, 0xDF, 0xFF, 0, 'Z' ],
	},

	// 0xD800..0xDBFF - invalid seq
	{
		inp = [ 0xD8, 0x00, 0, 0, 0, 'Z' ],
		out = [ 0, '?', 0, 'Z' ],
	},
	{
		inp = [ 0xD8, 0x00, 0xDB, 0xFF, 0, 'Z' ],
		out = [ 0, '?', 0, 'Z' ],
	},
	{
		inp = [ 0xD8, 0x00, 0xE0, 0x00, 0, 'Z' ],
		out = [ 0, '?', 0, 'Z' ],
	},
	{
		inp = [ 0xD8, 0x00, 0xFF, 0xFF, 0, 'Z' ],
		out = [ 0, '?', 0, 'Z' ],
	},
	
	// 0xD800..0xDBFF
	{
		inp = [ 0xD8, 0x00, 0, 'x', 0, 'Z' ],
		out = [ 0, '?', 0, 'Z' ],
	},
];

function blob::dump()
{
	local l = len();
	local i;
	local r = "";
	for( i=0; i < l; i++)
	{
		r += " " + this[i];
	}
	::print( r + "\n");
}

function compare( A, B)
{
	local alen = A.len();
	local blen = B.len();
	local len = (alen < blen) ? alen : blen;
	local r = 0;
	local i;
	for( i=0; i < len; i++)
	{
		if( A[i] != B[i])
		{
			::print( " " + A[i] + "/" + B[i]);
			r = 1;
		}
		else
			::print( " " + A[i]);
	}
	if( i < alen)
	{
		r = 1;
		::print( " <");
		for( ; i < alen; i++)
			::print( " " + A[i]);
	}
	if( i < blen)
	{
		r = 1;
		::print( " >");
		for( ; i < blen; i++)
			::print( " " + B[i]);
	}
	::print( "\t - " + ((r==0) ? "OK" : "NOK") + "\n");
	return r;
}

function mk_blob( arr)
{
	local b = blob( arr.len());
	foreach( i,v in arr)
		b[i] = v;
	return b;
}

function mk_blob_sw( arr)
{
	local b = blob( arr.len());
	foreach( i,v in arr)
		b[i^1] = v;
	return b;
}

function do_test_whole( tests, encoding)
{
	foreach( t in tests)
	{
		print( "-------------------\n");
		local bin = mk_blob( t.inp);
		local bout = mk_blob( t.out);
		local r = textreader( bin, true, encoding);
		local brd = r.readblob(1000);
		local bwr = blob();
		local w = textwriter( bwr, false, encoding);
		w.writeblob( brd);
		
		print( "in  "); bin.dump();
		print( "out "); bwr.dump();
		print( "cmp "); fails += compare( bout, bwr);
	}
}

function do_test_one( test, encoding)
{
	print( "-------------------\n");
	local bin = mk_blob( test);
	local rdr = textreader( bin, false, encoding);
	local bout = blob();
	local wrt = textwriter( bout, false, encoding);
	
	while( !rdr.eos())
	{
		local b = rdr.readblob(1);
		wrt.writeblob(b);
	}
	
	wrt.close();
	
	print( "in  "); bin.dump();
	print( "out "); bout.dump();
	print( "cmp "); fails += compare( bin, bout);
}

GUESS_TESTS <- [

	// UTF-8 BOM
	{
		inp = [ 0xef, 0xbb, 0xbf, 'A', 'B', 'C' ],
		out = [ 'A', 'B', 'C' ],
	},
	// UTF-16BE BOM
	{
		inp = [ 0xfe, 0xff, 0x00, 0x41, 0x00, 0x42, 0x00, 0x43 ],
		out = [ 'A', 'B', 'C' ],
	},
	// UTF-16LE BOM
	{
		inp = [ 0xff, 0xfe, 0x41, 0x00, 0x42, 0x00, 0x43, 0x00 ],
		out = [ 'A', 'B', 'C' ],
	},
	// UTF-8 no BOM
	{
		inp = [ 'A', 'B', 'C' ],
		out = [ 'A', 'B', 'C' ],
	},
];

function do_test_guess( test)
{
	print( "-------------------\n");
	local bin = mk_blob( test.inp);
	local rdr = textreader( bin, false, "UTF-8", true);
	local bout = blob();
	local wrt = textwriter( bout, false, "UTF-8");
	
	local b = rdr.readblob(1000);
	wrt.writeblob(b);
	
	wrt.close();
	
	print( "in  "); bin.dump();
	print( "out "); bout.dump();
	print( "cmp "); fails += compare( bout, test.out);
}

AUTO_UTF16 <- [

	// UTF-16BE
	{
		inp = [ 0xfe, 0xff, 0x00, 0x41, 0x00, 0x42, 0x00, 0x43 ],
		out = [ 'A', 'B', 'C' ],
	},
	// UTF-16LE
	{
		inp = [ 0xff, 0xfe, 0x41, 0x00, 0x42, 0x00, 0x43, 0x00 ],
		out = [ 'A', 'B', 'C' ],
	},
];

function do_test_auto_utf16( test)
{
	print( "-------------------\n");
	local bin = mk_blob( test.inp);
	local rdr = textreader( bin, false, "UTF-16");
	local bout = blob();
	local wrt = textwriter( bout, false, "UTF-8");
	
	local b = rdr.readblob(1000);
	wrt.writeblob(b);
	
	wrt.close();
	
	print( "in  "); bin.dump();
	print( "out "); bout.dump();
	print( "cmp "); fails += compare( bout, test.out);
}

do_test_whole( UTF8_TESTS, "UTF-8");
do_test_whole( UTF16BE_TESTS, "UTF-16BE");
do_test_one( [ 0x7F,  0xDF, 0xBF  ,0xEF, 0xBF, 0xBF,  0xF4, 0x8F, 0xBF, 0xBF,  0x7F ], "UTF-8");
do_test_one( [ 0xD7, 0xFF,  0xDB, 0xFF, 0xDF, 0xFF,  0xD7, 0xFF ], "UTF-16BE");

foreach( t in GUESS_TESTS) do_test_guess(t);
foreach( t in AUTO_UTF16) do_test_auto_utf16(t);

if( fails)
{
	print( "-------------------\nFAILED\n");
}

