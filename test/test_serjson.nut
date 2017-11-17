
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
	::print( r);
}

function dump( val)
{
    switch( typeof val)
    {
        case "array":
            ::print( "[");
            foreach( k,v in val)
            {
                if( k != 0) ::print( ", ");
                dump( v);
            }
            ::print( "]");
            break;
            
        case "table": {
            local first = true;
            ::print( "{");
            foreach( k,v in val)
            {
                if( !first) ::print( ", ");
                dump( k); ::print( "="); dump(v);
                first = false;
            }
            ::print( "}");
        } break;
        
        case "instance":
            if( val instanceof blob)
            {
                val.dump();
            }
            // break; fall to next
        
        default:
            print( val);
            break;
    }
}

function compare( A, B)
{
    if( typeof A != typeof B) return false;
    local res = true;
    switch( typeof A)
    {
        case "array":
            res = A.len() == B.len();
            if( res)
                foreach( k,v in A)
                {
                    res = res && compare( v, B[k]);
                    if( !res) break;
                }
            break;
            
        case "table": {
            res = A.len() == B.len();
            if( res)
                foreach( k,v in A)
                {
                    res = res && (k in B);
                    if( res)
                        res = res && compare( v, B[k]);
                    if( !res) break;
                }
        } break;
        
        case "instance":
            if( A instanceof blob)
            {
                res = A.len() == B.len();
                if( res)
                    foreach( k,v in A)
                    {
                        res = res && compare( v, B[k]);
                        if( !res) break;
                    }
                break;
            }
            // break; fall to next
        
        default:
            res = A == B;
            break;
    }
    return res;
}

function pr_err( str) { errors++; print( str); }

// savejson( stdout, 12345);
// print( "\n");
// savejson( stdout, -12345);
// print( "\n");
// savejson( stdout, 12.345);
// print( "\n");
// savejson( stdout, -12.345e-12);
// print( "\n");
// savejson( stdout, null);
// print( "\n");
// savejson( stdout, true);
// print( "\n");
// savejson( stdout, false);
// print( "\n");
// savejson( stdout, [1]);
// print( "\n");
// savejson( stdout, [1,2,3]);
// print( "\n");
// savejson( stdout, [1,[11,22,33],[44,55],3]);
// print( "\n");
// savejson( stdout, { a=1});
// print( "\n");
// savejson( stdout, { a=1, b=2, c=3}, "l-");
// print( "\n");
// savejson( stdout, "hello");
// print( "\n");
// savejson( stdout, "hel\nlo");
// print( "\n");
// savejson( stdout, "hello\\");
// print( "\n");
// savejson( stdout, "hel\"lo");
// print( "\n");
// savejson( stdout, "hello/");
// print( "\n");
// local t = {};
// t["0"] <- "A";
// t[1] <- "B";
// savejson( stdout, t);
// print( "\n");

function test_value_ok( test_str, test_res)
{
    print( "-------\n");
    print( "str:" + test_str + "\n");
    local b = blob();
    b.print( test_str);
    b.seek(0)
    local res = loadjson( b);
    print( "res=" + res + "\n");
    if( res != test_res)
    {
        pr_err( "exp=" + test_res + "\t(err)\n");
    }
    if( typeof res != typeof test_res)
    {
        pr_err( typeof res + "!=" + typeof test_res + "\t(err)\n");
    }
}

function test_value_fail( test_str, test_res)
{
    try {
        test_value_ok( test_str, test_res);
        pr_err( "No fail\n");
    }
    catch( e) {
        print( "OK: " + e + "\n");
    }
}

function test_array_ok( test_str, test_res)
{
    print( "-------\n");
    print( "str:" + test_str + "\n");
    local b = blob();
    b.print( test_str);
    b.seek(0)
    local res = loadjson( b);
    print( "res="); dump( res); print( "\n");
    local is_ok = compare( res, test_res);
    if( !is_ok)
    {
        pr_err( "exp="); dump( test_res); print( "\n");
    }
    if( typeof res != typeof test_res)
    {
        pr_err( typeof res + "!=" + typeof test_res + "\t(err)\n");
    }
}

function test_array_fail( test_str, test_res)
{
    try {
        test_array_ok( test_str, test_res);
        pr_err( "No fail\n");
    }
    catch( e) {
        print( "OK: " + e + "\n");
    }
}

function test_table_ok( test_str, test_res)
{
    print( "-------\n");
    print( "str:" + test_str + "\n");
    local b = blob();
    b.print( test_str);
    b.seek(0)
    local res = loadjson( b);
    print( "res="); dump( res); print( "\n");
    local is_ok = compare( res, test_res);
    if( !is_ok)
    {
        pr_err( "exp="); dump( test_res); print( "\n");
    }
    if( typeof res != typeof test_res)
    {
        pr_err( typeof res + "!=" + typeof test_res + "\t(err)\n");
    }
}

function test_table_fail( test_str, test_res)
{
    try {
        test_table_ok( test_str, test_res);
        pr_err( "No fail\n");
    }
    catch( e) {
        print( "OK: " + e + "\n");
    }
}

local def_save_opts = "itlc";   // indent - tab ('\t'), new line - C ('\n')

function test_save_opt_ok( val, opt, test_res)
{
    print( "-------\n");
    print( "val:"); dump(val); print("\n");
    local b = blob();
    if( opt == null)
    {
        savejson( b, val);
    }
    else
    {
        savejson( b, val, opt);
    }
    b.seek(0);
    local res = "";
    while( !b.eos())
    {
        res += b.readline();
    }
    print( "res:<<" + escape(res) + ">>\n");
    if( res != test_res)
    {
        pr_err( "No match\n");
    }
}

function test_save_ok( val, test_res)
{
    test_save_opt_ok( val, def_save_opts, test_res);
}

function test_save_float_opt_ok( val, opt)
{
    print( "-------\n");
    print( "val:"); dump(val); print("\n");
    local b = blob();
    if( opt == null)
    {
        savejson( b, val);
    }
    else
    {
        savejson( b, val, opt);
    }
    b.seek(0);
    local res = "";
    while( !b.eos())
    {
        res += b.readline();
    }
    print( "res:<<" + escape(res) + ">>\n");
    b.seek(0);
    res = loadjson( b);
    if( res != val)
    {
        pr_err( "No match\n");
    }
}

function test_save_float_ok( val)
{
    test_save_float_opt_ok( val, def_save_opts);
}


if( 1) {

// integer
test_value_fail( "-", 0);
test_value_ok( "0", 0);
test_value_ok( "1", 1);
test_value_ok( "-1", -1);
test_value_ok( "123", 123);
test_value_ok( "-123", -123);
//test_value_ok( "1234567890123", 1234567890123);  // may be bug in sqlexer.cpp

// float
test_value_fail( "1.", 0);
test_value_fail( ".1", 0);
test_value_ok( "0.0", 0.0);
test_value_ok( "1.1", 1.1);
test_value_ok( "3.141592653589793238462643383279", 3.141592653589793238462643383279);
test_value_fail( "1e", 0);
test_value_ok( "123x", 123);
test_value_fail( "1.1e", 0);
test_value_fail( "1.1e+", 0);
test_value_fail( "1.1e-", 0);
test_value_fail( "1.1ex", 0);
test_value_fail( "1.1e+x", 0);
test_value_fail( "1.1e-x", 0);
test_value_ok( "1.1e0", 1.1);
test_value_ok( "1.1e1", 11.0);
test_value_ok( "1.1e00", 1.1);
test_value_ok( "1.1e12", 1.1e12);
test_value_ok( "1.1e+01", 11.0);
test_value_ok( "1.1e+12", 1.1e12);
test_value_ok( "1.1e-01", 0.11);
test_value_ok( "1.1e-12", 1.1e-12);

// boolean
test_value_fail( "fals", 0);
test_value_ok( "false", false);
test_value_fail( "falsex", 0);
test_value_fail( "tru", 0);
test_value_ok( "true", true);
test_value_fail( "truex", 0);

// null
test_value_fail( "nul", 0);
test_value_ok( "null", null);
test_value_fail( "nullx", 0);

// string
test_value_ok( "\"\"", "");
test_value_ok( "\"abcd\"", "abcd");
test_value_ok( "\"ab\\\"cd\"", "ab\"cd");
test_value_ok( "\"ab\\\\cd\"", "ab\\cd");
test_value_ok( "\"ab\\/cd\"", "ab/cd");
test_value_ok( "\"ab\\bcd\"", "ab\bcd");
test_value_ok( "\"ab\\fcd\"", "ab\fcd");
test_value_ok( "\"ab\\ncd\"", "ab\ncd");
test_value_ok( "\"ab\\rcd\"", "ab\rcd");
test_value_ok( "\"ab\\tcd\"", "ab\tcd");
//test_value_ok( "\"ab\\u0000xy\"", "ab\u0000xy");  // may be bug in squirrel
test_value_ok( "\"ab\\u0013xy\"", "ab\u0013xy");
test_value_ok( "\"ab\\u0044xy\"", "abDxy");
test_value_ok( "\"ab\\u0414xy\"", "abДxy");
test_value_ok( "\"ab\\u0E5Bxy\"", "ab๛xy");
test_value_fail( "\"abc", 0);  // EOF in string
test_value_fail( "\"\\\"", 0);  // EOF in string
test_value_fail( "\"\n\"", 0);  // New line in string
test_value_fail( "\"\\", 0);    // unexpected EOF
test_value_fail( "\"\\q\"", 0);  // BAD escape sequence
test_value_fail( "\"\\u", 0);  // unexpected EOF
test_value_fail( "\"\\ux", 0);  // unexpected EOF
test_value_fail( "\"\\uxx", 0);  // unexpected EOF
test_value_fail( "\"\\uxxx", 0);  // unexpected EOF
test_value_fail( "\"\\uxxxx", 0);  // BAD escape sequence
test_value_fail( "\"\\ux234", 0);  // BAD escape sequence
test_value_fail( "\"\\u1x34", 0);  // BAD escape sequence
test_value_fail( "\"\\u12x4", 0);  // BAD escape sequence
test_value_fail( "\"\\u123x", 0);  // BAD escape sequence
test_value_fail( "\"ab\\u0000xy\"", 0);  // BAD escape sequence

// array
test_array_ok( "[]", []);
test_array_ok( "[1]", [1]);
test_array_ok( "[1,2,3]", [1,2,3]);
test_array_ok( " \n\r\t[ 1\n, 2\r,\t3 ] ", [1,2,3]);
test_array_fail( "[", 0);   // unexpected EOF
test_array_fail( "[ ", 0);   // unexpected EOF
test_array_fail( "[1", 0);   // unexpected EOF
test_array_fail( "[1,", 0);   // unexpected EOF
test_array_fail( "[1,]", 0);   // bad charecter
test_array_fail( "[1x", 0);   // Expecting array delimer
test_array_fail( "[\"", 0);   // EOF in string
test_array_ok( "[1,[2],3,[4],5]", [1,[2],3,[4],5]);

// table
test_table_ok( "{}", {});
test_table_ok( "{\"a\":1}", {a=1});
test_table_ok( "{\"a\":1,\"b\":2,\"c\":3}", {a=1,b=2,c=3});
test_table_ok( "{\"a\":1,\"b\":2,\"a\":3}", {a=3,b=2});
test_table_fail( "{1:2}", 0);   // table key must be a string
test_table_fail( "{", 0);   // unexpected EOF
test_table_fail( "{\"a\"", 0);   // unexpected EOF
test_table_fail( "{\"a\"x", 0);   // Expecting pair delimer
test_table_fail( "{\"a\":x", 0);   // bad charecter
test_table_fail( "{\"a\":1", 0);   // unexpected EOF
test_table_fail( "{\"a\":1x", 0);   // Expecting table delimer
test_table_fail( "{\"a\":1,", 0);   // unexpected EOF
test_table_fail( "{\"a\":1,x", 0);   // bad charecter

// spaces
test_value_ok( "  123", 123);
test_value_fail( "/123", 0);    // bad charecter
test_value_ok( "//\n123", 123);
test_value_ok( "//line comment\n123", 123);
test_value_ok( "//line comment\r123", 123);
test_value_ok( "//line comment\n\t123", 123);
test_value_ok( "\t//line comment\n123", 123);
test_value_ok( "123//line comment", 123);
test_value_fail( "//line comment 123", 0);  // EOF in comment
test_value_ok( "/*block comment*/123", 123);
test_value_ok( "/**/123", 123);
test_value_ok( "/***/123", 123);
test_value_ok( "/****/123", 123);
test_value_fail( "/*block comment 123", 0); // EOF in comment
test_value_ok( "123/*block comment", 123);

}

print( "======================================================\n");

if( 1) {

// identation
// new line
test_save_opt_ok( 123, "lc", "123\n");
test_save_opt_ok( 123, "lp", "123\r\n");
test_save_opt_ok( 123, "l-", "123");
//  array
test_save_opt_ok( [123], "lcit", "[\n\t123\n]\n");
test_save_opt_ok( [123,456], "lcit", "[\n\t123,\n\t456\n]\n");
test_save_opt_ok( [123], "lcis", "[\n 123\n]\n");
test_save_opt_ok( [123,456], "lcis", "[\n 123,\n 456\n]\n");
test_save_opt_ok( [123], "lci-", "[123]\n");
test_save_opt_ok( [123,456], "lci-", "[123,456]\n");
//  table
test_save_opt_ok( {a=123}, "lcit", "{\n\t\"a\":123\n}\n");
test_save_opt_ok( {a=123,b=456}, "lcit", "{\n\t\"a\":123,\n\t\"b\":456\n}\n");
test_save_opt_ok( {a=123}, "lcis", "{\n \"a\":123\n}\n");
test_save_opt_ok( {a=123,b=456}, "lcis", "{\n \"a\":123,\n \"b\":456\n}\n");
test_save_opt_ok( {a=123}, "lci-", "{\"a\":123}\n");
test_save_opt_ok( {a=123,b=456}, "lci-", "{\"a\":123,\"b\":456}\n");

// escaping
test_save_ok( "ab\"cd", "\"ab\\\"cd\"\n");
test_save_ok( "ab\\cd", "\"ab\\\\cd\"\n");
test_save_ok( "ab/cd", "\"ab\\/cd\"\n");
test_save_ok( "ab\x13cd", "\"ab\\u0013cd\"\n");
test_save_ok( "ab\bcd", "\"ab\\bcd\"\n");
test_save_ok( "ab\fcd", "\"ab\\fcd\"\n");
test_save_ok( "ab\ncd", "\"ab\\ncd\"\n");
test_save_ok( "ab\rcd", "\"ab\\rcd\"\n");
test_save_ok( "ab\tcd", "\"ab\\tcd\"\n");

// keywords
test_save_ok( true, "true\n");
test_save_ok( false, "false\n");
test_save_ok( null, "null\n");

// integer
test_save_ok( 123, "123\n");
test_save_ok( -123, "-123\n");

// float
test_save_ok( 123.0, "123.0\n");
test_save_ok( -123.0, "-123.0\n");
test_save_float_ok( 12.3);
test_save_float_ok( 12.3);
test_save_float_ok( -12.3);
test_save_float_ok( 1.2e5);
test_save_float_ok( 1.2e15);
test_save_float_ok( 1.2e-15);
test_save_float_ok( 123456789012345.0);
test_save_float_ok( 1e999);
test_save_float_ok( -1e999);
test_save_float_ok( PI);

}

print( "\n-------------\n");
print( errors ? "FAILED\n" : "OK\n" );
