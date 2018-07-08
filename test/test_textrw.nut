
local trw = require( "textrw");

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

function array2blob( type, arr)
{
    local b = blob();
    foreach( _, v in arr)
    {
        b.writen( v, type);
    }
    return b;
}

local encodings = [
    "utf8",
    "utf16",
    "utf16be",
    "utf16le",
    "utf32",
    "utf32be",
    "utf32le",
    "ascii"
];

foreach( _, enc in encodings)
{
    local b = trw.strtoblob( "hellou свят", enc);
    print( enc + "\t");
    print( b.dump())
    local s = trw.blobtostr( b, enc);
    print( "\t\"" + s + "\"\n");
}

{
    print( "EOB\t");
    local b = array2blob( 'w', [ 0x0041, 0x0042, 0x0043]);
    b.resize( b.len() -1);
    print( b.dump())
    local s = trw.blobtostr( b, "utf16");
    print( "\t\"" + s + "\"\n");
}

{
    print( "EOB seq\t");
    local b = array2blob( 'w', [ 0x0041, 0xD800]);
    print( b.dump())
    local s = trw.blobtostr( b, "utf16");
    print( "\t\"" + s + "\"\n");
}

{
    print( "ILSEQ\t");
    local b = array2blob( 'w', [ 0x0044, 0xDC00, 0x0041]);
    print( b.dump())
    local s = trw.blobtostr( b, "utf16");
    print( "\t\"" + s + "\"\n");
}

{
    print( "U16 seq\t");
    local b = array2blob( 'w', [ 0x0041, 0xD800, 0xDC00]);
    print( b.dump())
    local s = trw.blobtostr( b, "utf16");
    print( "\t\"" + s + "\"\t");
    local b2 = trw.strtoblob( s, "utf32be");
    print( b2.dump())
    print( "\n");
}

