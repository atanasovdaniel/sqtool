
local trw = require( "textrw");
local sys = require( "system");

local encodings = [
    "utf8",
//    "utf16",
    "utf16be",
    "utf16le",
//    "utf32",
    "utf32be",
    "utf32le",
    "ascii"
];

function blob::dump()
{
	local l = len();
	local i;
	local r = "";
	for( i=0; i < l; i++)
	{
		r += " " + format( "%02X", this[i]);
	}
    return r;
}

function write_random( strm, enc, count)
{
    local w = trw.writer( strm, enc);
    for( local i=0; i < count; i++)
    {
        //w.writecodepoint( (0x20 + (0x10FFDF / RAND_MAX.tofloat()) * rand()).tointeger());
        w.writecodepoint( (0x20 + (0xFFDF / RAND_MAX.tofloat()) * rand()).tointeger());
    }
}

function time_cv_strtoblob( str)
{
    local count = 1000;
    foreach( _, enc in encodings)
    {
        local t_start = sys.time();
        for( local i=0; i < count; i++)
        {
            trw.strtoblob( str, enc);
        }
        local t_end = sys.time();
        print( "strtoblob( " + enc + "):\t" + ((t_end - t_start).tofloat() / count) + "\n");
    }
}

function time_readchar( str)
{
    local count = 100;
    foreach( _, enc in encodings)
    {
        local blb = trw.strtoblob( str, enc);
        local t_start = sys.time();
        for( local i=0; i < count; i++)
        {
            blb.seek(0);
            local rdr = trw.reader( blb, enc);
            while( rdr.readchar() != null);
        }
        local t_end = sys.time();
        print( "readchar( " + enc + "):\t" + ((t_end - t_start).tofloat() / count) + "\t" + blb.tell() + "\n");
    }
}

srand( PI * RAND_MAX);
local b = blob();
write_random( b, "utf8", 1024*1024);
local s = trw.blobtostr( b, "utf8");

time_cv_strtoblob( s);

time_readchar( s);

// str=1024*1024 cnt=1000 (release) char
// strtoblob( utf8):	0.004
// strtoblob( utf16be):	0.033
// strtoblob( utf16le):	0.03
// strtoblob( utf32be):	0.039
// strtoblob( utf32le):	0.036
// strtoblob( ascii):	0.03

// str=1024*1024 cnt=100 (release) char
// readchar( utf8):	1.06	3014314
// readchar( utf16be):	1.06	2032050
// readchar( utf16le):	1.04	2032050
// readchar( utf32be):	1.05	4064100
// readchar( utf32le):	1.03	4064100
// readchar( ascii):	0.27	1016025
