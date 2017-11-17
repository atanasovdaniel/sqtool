
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
	local r = savebinary( b, val);
	print( "savebinary( b, " + val + ") -> " + r + "\n");
	b.dump();
	if( b.len() != (4+exp_size)) pr_err( "!!! Bad size\n");
	b.seek(0);
	r = loadbinary( b);
	print( "loadbinary( b) -> " + r + "\n");
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
    local r = loadbinary( b);
	print( "loadbinary( b) -> " + format( "%.17g", r) + "\n");
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
	local r = savebinary( b, val);
	print( "savebinary( b, " + val + ") -> " + r + "\n");
	b.dump();
	if( b.len() != (4+exp_size)) pr_err( "!!! Bad size\n");
	b.seek(0);
	r = loadbinary( b);
	print( "loadbinary( b) -> " + r + "\n");
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

check_val( 1+_floatsize_,		1.25);

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

// float
if( _floatsize_ == 4) {
check_val_des( [0x44, 0x42, 0x53, 0x01, 0xEA, 0xCD, 0xCC, 0x8C, 0x3F], 1.1);
check_val_des( [0x01, 0x53, 0x42, 0x44, 0xEA, 0x3F, 0x8C, 0xCC, 0xCD], 1.1);
} else {
check_val_des( [0x44, 0x42, 0x53, 0x01, 0xEA, 0xCD, 0xCC, 0x8C, 0x3F], 1.1000000238418579);
check_val_des( [0x01, 0x53, 0x42, 0x44, 0xEA, 0x3F, 0x8C, 0xCC, 0xCD], 1.1000000238418579);
}
check_val_des( [0x44, 0x42, 0x53, 0x01, 0xEA, 0x00, 0x00, 0xA0, 0x3F], 1.25);
check_val_des( [0x01, 0x53, 0x42, 0x44, 0xEA, 0x3F, 0xA0, 0x00, 0x00], 1.25);

//double 
check_val_des( [0x44, 0x42, 0x53, 0x01, 0xEB, 0x9A, 0x99, 0x99, 0x99, 0x99, 0x99, 0xF1, 0x3F], 1.1);
check_val_des( [0x01, 0x53, 0x42, 0x44, 0xEB, 0x3F, 0xF1, 0x99, 0x99, 0x99, 0x99, 0x99, 0x9A], 1.1);
check_val_des( [0x44, 0x42, 0x53, 0x01, 0xEB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF4, 0x3F], 1.25);
check_val_des( [0x01, 0x53, 0x42, 0x44, 0xEB, 0x3F, 0xF4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00], 1.25);

//--------------------

print( "\n-------------\n");
print( errors ? "FAILED\n" : "OK\n" );
