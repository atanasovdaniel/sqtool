
failures <- 0;

TESTS <- [
	{ file="./test/src_utf8.nut", text="Здравей, свят!", enc="UTF-8" },
	{ file="./test/src_utf8_bom.nut", text="Здравей, свят!", enc="UTF-8" },
	{ file="./test/src_utf16be_bom.nut", text="Здравей, свят!", enc="UTF-16" },
	{ file="./test/src_utf16le_bom.nut", text="Здравей, свят!", enc="UTF-16" },
];

foreach( t in TESTS)
{
	print( "\n----------------------\nfile = " + t.file + "\n");
	local fct = loadfile( t.file);
	local ret = fct();
	print( ret + "\n");
	local eq = ret == t.text;
	failures += eq ? 0 : 1;
	print( (eq ? "OK" : "NOK") + "\n");
}

foreach( t in TESTS)
{
	print( "\n----------------------\nfile = " + t.file + "\n");
	local ret = dofile( t.file);
	print( ret + "\n");
	local eq = ret == t.text;
	failures += eq ? 0 : 1;
	print( (eq ? "OK" : "NOK") + "\n");
}

function test_fails( encoding)
{
	foreach( t in TESTS)
	{
		print( "\n----------------------\nfile = " + t.file + "\n");
		local fh = file( t.file, "rb");
		local fct;
		try {
			fct = loadfile( fh, true, 0, encoding, false);
		} catch( id) {
			print( "ID:" + id + "\n");
			fct = @() "";
		}
		fh.close();
		local ret = fct();
		print( ret + "\n");
		local eq = ret == t.text;
		eq = (encoding == t.enc) ? eq : !eq;
		failures += eq ? 0 : 1;
		print( (eq ? "OK" : "NOK") + "\n");
	}
}

test_fails("ASCII");
test_fails("UTF-8");
test_fails("UTF-16");

if( failures)
{
	print( "FAIL\n");
}