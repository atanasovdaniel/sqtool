
function blob::dump()
{
	local l = len();
	local i;
	local r = "";
	for( i=0; i < l; i++)
	{
		r += ::format( " %02X", this[i]);
	}
	::print( r + "\n");
}

function check_val( exp_size, val)
{
	print( "--------\n");
	local b = blob();
	local r = savedata( b, val);
	print( "savedata( b, " + val + ") -> " + r + "\n");
	b.dump();
	if( b.len() != exp_size) print( "!!! Bad size\n");
	b.seek(0);
	r = loaddata( b);
	print( "loaddata( b) -> " + r + "\n");
	if( r != val) print( "!!! Bad value\n");
	if( b.tell() != exp_size) print( "!!! Bad position\n");
}

function dump_cont( val)
{
	print( "[");
	foreach( k,v in val)	print( " " + k + "=" + v)
	print( "]\n");
}

function check_cont( exp_size, val)
{
	print( "--------\n");
	local b = blob();
	local r = savedata( b, val);
	print( "savedata( b, " + val + ") -> " + r + "\n");
	b.dump();
	if( b.len() != exp_size) print( "!!! Bad size\n");
	b.seek(0);
	r = loaddata( b);
	print( "loaddata( b) -> " + r + "\n");
	dump_cont(r);
	local is_ok = val.len() == r.len();
	if( !is_ok) print( "!!! Bad len()\n");
	foreach(k,v in val) is_ok = is_ok && (r[k] == v);
	if( !is_ok) print( "!!! Bad value\n");
	if( b.tell() != exp_size) print( "!!! Bad position\n");
}


check_val( 1+1,		1);
check_val( 1+1,		127);
check_val( 1+1,		-128);

check_val( 1+2,		128);
check_val( 1+2,		-129);
check_val( 1+2,		32767);
check_val( 1+2,		-32768);

check_val( 1+4,		32768);
check_val( 1+4,		-32769);
check_val( 1+4,		0x7FFFFFFF);
check_val( 1+4,		0x80000000);

check_val( 1+_floatsize_,		1.1);

check_val( 1,		true);
check_val( 1,		false);

check_val( 1,		null);

check_val( 1+3,		"abc");

check_cont( 1+3*(1+1),		[1,2,3]);

check_cont( 1+3*(1+1 +1+1),		{a=1,b=2,c=3});

{
	local b = blob(3);
	b[0] = 1;
	b[1] = 2;
	b[2] = 3;
	check_cont( 1+1+3,		b);
}

