
function dump_sa( ind, sa)
{
    local t = type(sa);
    if( (t == "instance") || (t == "array"))
    {
        print( "[\n");
        foreach( k,v in sa)
        {
            print( ind + "\t");
            dump_sa( ind + "\t", v);
            print( ",\n");
        }
        print( ind + "]");
    }
    else if( t == "table")
    {
        print( "{\n");
        foreach( k,v in sa)
        {
            print( ind + "\t" + k + "=");
            dump_sa( ind + "\t", v);
            print( ",\n");
        }
        print( ind + "}");
    }
    else
    {
        print( sa);
    }
}

function ctypes::basictype::dump(ind)
{
    ::print( (typeof this) + "(" + name() + ",s:" + size() + ",a:" + align() + ")");
}

function ctypes::basicarray::dump(ind)
{
    ::print( (typeof this) + "(" + name() + ",s:" + size() + ",a:" + align() + ",l:" + len() + ",\n");
    ::print( ind + "\tt:");
    type().dump(ind+"\t");
    ::print( "\n" + ind + ")");
}

function ctypes::basicmember::dump(ind)
{
    ::print( (typeof this) + "(" + name() + ",o:" + offset() + ",\n")
    ::print( ind + "\tt:");
    type().dump(ind+"\t");
    ::print( "\n" + ind + ")");
}

function ctypes::basicstruct::dump(ind)
{
    ::print( (typeof this) + "(" + name() + ",s:" + size() + ",a:" + align() + ",[\n");
    foreach( m in this["$members"])
    {
        ::print( ind + "\t");
        m.dump(ind+"\t");
        ::print( ",\n");
    }
    ::print( ind + "])");
}

// {
//     local val = ctypes.basicvalue( ctypes.uint32_t);
//     print( "val.get() -> " + val.get() + "\n");
//     val.clear();
//     print( "val.get() -> " + val.get() + "\n");
//     val.set( 12345);
//     print( "val.get() -> " + val.get() + "\n");
// }

// {
//     local val1 = ctypes.basicvalue( ctypes.uint32_t);
//     local val2 = ctypes.basicvalue( ctypes.uint32_t);
//     
//     print( "val1.get() -> " + val1.get() + "\n");
//     print( "val2.get() -> " + val2.get() + "\n");
//     
//     val1.set(12345);
//     
//     print( "val1.get() -> " + val1.get() + "\n");
//     print( "val2.get() -> " + val2.get() + "\n");
//
//     val2.set(val1);
//     
//     print( "val1.get() -> " + val1.get() + "\n");
//     print( "val2.get() -> " + val2.get() + "\n");
// }

// {
//     local val = ctypes.basicvalue( ctypes.basicarray( ctypes.uint32_t, 5));
//     print( "val[2] -> " + val[2] + "\n");
//     print( "val[2] -> " + typeof val[2] + "\n");
//     print( "val[2].get() -> " + val[2].get() + "\n");
//     for( local i=0; i < 5; i++) print( "val[" + i + "].get() --> " + val[i].get() + "\n");
//     val.clear();
//     for( local i=0; i < 5; i++) print( "val[" + i + "].get() --> " + val[i].get() + "\n");
//     val[2] = 12345;
//     for( local i=0; i < 5; i++) print( "val[" + i + "].get() --> " + val[i].get() + "\n");
//     val[4] = val[2];
//     for( local i=0; i < 5; i++) print( "val[" + i + "].get() --> " + val[i].get() + "\n");
// }

// {
//     local val = ctypes.basicvalue( ctypes.basicarray( ctypes.uint32_t, 5));
//     val[0] = 11; val[1] = 22; val[2] = 33; val[3] = 44; val[4] = 55;
//     for( local i=0; i < 5; i++) print( "val[" + i + "].get() --> " + val[i].get() + "\n");
//     local ar = val.get();
//     print( typeof ar + "\n");
//     foreach( k,v in ar) print( "ar[" + k + "] --> " + v + "\n");
// }

// {
//     local val = ctypes.basicvalue( ctypes.basicarray( ctypes.uint32_t, 5));
//     for( local i=0; i < 5; i++) print( "val[" + i + "].get() --> " + val[i].get() + "\n");
//     val.set( [111,222,333,444,555]);
//     for( local i=0; i < 5; i++) print( "val[" + i + "].get() --> " + val[i].get() + "\n");
// }

// {
//     class matrix extends ctypes.basicvalue
//     {
//         static _type = ctypes.basicarray( ctypes.uint32_t, 5);
//         constructor()
//         {
//             base.constructor( _type);
//         }
//     }
//     
//     local val = matrix();
//     for( local i=0; i < 5; i++) print( "val[" + i + "].get() --> " + val[i].get() + "\n");
// }

{
    local st = ctypes.basicstruct("qaz");
    print( "size=" + st.size() + ", align=" + st.align() + "\n");
    
    st.setmembers( [
        ctypes.basicmember( "aa", ctypes.uint32_t),
        ctypes.uint8_t.member("bb")
        ctypes.uint8_t.array(5).member("cc")
        ctypes.uint8_t.member("dd")
        ]);
    
    print( "size=" + st.size() + ", align=" + st.align() + "\n");
    
    local val = ctypes.basicvalue( st);
    print( "val.aa=" + val.aa.get() + "\n");
    print( "val.bb=" + val.bb.get() + "\n");
    print( "val.cc=" + val.cc.get() + "\n");
    print( "val.dd=" + val.dd.get() + "\n");
    
    val.clear();
    print( "val.aa=" + val.aa.get() + "\n");
    print( "val.bb=" + val.bb.get() + "\n");
    print( "val.cc=" + val.cc.get() + "\n");
    print( "val.dd=" + val.dd.get() + "\n");
    
    val.set( { aa=11, bb=22, dd=44});
    print( "val.aa=" + val.aa.get() + "\n");
    print( "val.bb=" + val.bb.get() + "\n");
    print( "val.cc=" + val.cc.get() + "\n");
    print( "val.dd=" + val.dd.get() + "\n");
    
    //print( val.get() + "\n");
    dump_sa( "", val.get());
    print( "\n");
    st.dump("");
    print( "\n");
    
    local st2 = ctypes.basicstruct("wsx");
    st2.setmembers( [
        st.member( "aa"),
//        ctypes.uint8_t.member("bb")
        st.array(5).member("cc")
        ctypes.uint8_t.member("dd")
        ]);
    
    st2.dump("");
    print( "\n");
}

