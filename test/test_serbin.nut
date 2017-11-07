
errors <- 0;

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

function pr_err( str) { errors++; print( str); }

function check_val( exp_size, val)
{
	print( "--------\n");
	local b = blob();
	local r = savedata( b, val);
	print( "savedata( b, " + val + ") -> " + r + "\n");
	b.dump();
	if( b.len() != (4+exp_size)) pr_err( "!!! Bad size\n");
	b.seek(0);
	r = loaddata( b);
	print( "loaddata( b) -> " + r + "\n");
	if( r != val) pr_err( "!!! Bad value\n");
	if( b.tell() != (4+exp_size)) pr_err( "!!! Bad position\n");
}

function check_val_des( arr, val)
{
	print( "--------\n");
    local b = blob();
    foreach( v in arr)
    {
        b.writen( v, 'b');
    }
    b.dump()
    b.seek(0);
    local r = loaddata( b);
	print( "loaddata( b) -> " + r + "\n");
	if( r != val) pr_err( "!!! Bad value\n");
	if( b.tell() != b.len()) pr_err( "!!! Bad position\n");
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
	if( b.len() != (4+exp_size)) pr_err( "!!! Bad size\n");
	b.seek(0);
	r = loaddata( b);
	print( "loaddata( b) -> " + r + "\n");
	dump_cont(r);
	local is_ok = val.len() == r.len();
	if( !is_ok) pr_err( "!!! Bad len()\n");
	foreach(k,v in val) is_ok = is_ok && (r[k] == v);
	if( !is_ok) pr_err( "!!! Bad value\n");
	if( b.tell() != (4+exp_size)) pr_err( "!!! Bad position\n");
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

//--------

check_val_des( [0x01, 0x53, 0x42, 0x44, 0xE0, 0x01], 1);
check_val_des( [0x01, 0x53, 0x42, 0x44, 0xE0, 0x7F], 127);
check_val_des( [0x01, 0x53, 0x42, 0x44, 0xE0, 0x80], -128);

check_val_des( [0x01, 0x53, 0x42, 0x44, 0xE1, 0x00, 0x80], 128);
check_val_des( [0x01, 0x53, 0x42, 0x44, 0xE1, 0xFF, 0x7F], -129);
check_val_des( [0x01, 0x53, 0x42, 0x44, 0xE1, 0x7F, 0xFF], 32767);
check_val_des( [0x01, 0x53, 0x42, 0x44, 0xE1, 0x80, 0x00], -32768);

check_val_des( [0x01, 0x53, 0x42, 0x44, 0xE2, 0x00, 0x00, 0x80, 0x00], 32768);
check_val_des( [0x01, 0x53, 0x42, 0x44, 0xE2, 0xFF, 0xFF, 0x7F, 0xFF], -32769);
check_val_des( [0x01, 0x53, 0x42, 0x44, 0xE2, 0x7F, 0xFF, 0xFF, 0xFF], 2147483647);
check_val_des( [0x01, 0x53, 0x42, 0x44, 0xE2, 0x80, 0x00, 0x00, 0x00], -2147483648);

check_val_des( [0x01, 0x53, 0x42, 0x44, 0xEA, 0x3F, 0x8C, 0xCC, 0xCD], 1.1);

/*
--------
savedata( b, 1.1) -> 1.1
 44 42 53 01 EA CD CC 8C 3F
loaddata( b) -> 1.1
--------
savedata( b, true) -> true
 44 42 53 01 F1
loaddata( b) -> true
--------
savedata( b, false) -> false
 44 42 53 01 F0
loaddata( b) -> false
--------
savedata( b, (null : 0x(nil))) -> (null : 0x(nil))
 44 42 53 01 F2
loaddata( b) -> (null : 0x(nil))
--------
savedata( b, abc) -> abc
 44 42 53 01 03 61 62 63
loaddata( b) -> abc
--------
savedata( b, (array : 0x0x6eab78)) -> (array : 0x0x6eab78)
 44 42 53 01 83 E0 01 E0 02 E0 03
loaddata( b) -> (array : 0x0x6f0b80)
[ 0=1 1=2 2=3]
--------
savedata( b, (table : 0x0x6ea230)) -> (table : 0x0x6ea230)
 44 42 53 01 A3 01 61 E0 01 01 63 E0 03 01 62 E0 02
loaddata( b) -> (table : 0x0x6f12f8)
[ a=1 c=3 b=2]
--------
savedata( b, (instance : 0x0x6f0a58)) -> (instance : 0x0x6f0a58)
 44 42 53 01 C0 03 01 02 03
loaddata( b) -> (instance : 0x0x6f2288)
[ 0=1 1=2 2=3]
*/
//--------------------
print( "\n-------------\n");
print( errors ? "FAILED\n" : "OK\n" );
