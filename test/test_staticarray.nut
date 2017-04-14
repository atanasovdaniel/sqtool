
function dump_sa( ind, sa)
{
    local t = type(sa);
    if( (t == "instance") || (t == "array"))
    {
        print( "\n" + ind + "[");
        foreach( k,v in sa)
        {
            if( k != 0) print( ", ");
            dump_sa( ind + " ", v);
        }
        print( ind + "]\n");
    }
    else
    {
        print( sa);
    }
}

{
    local a = staticarray( 'i', 2, 3, 4).clear();
    
    a[1][2][3] = 13;
    print( "a[1][2][3]=" + a[1][2][3] + "\n");
    dump_sa( "", a);
    print("\n");
    
    print("--------------\n");
    a[0][1] = a[1][2];
    dump_sa( "", a);
    print("\n");
    
    print("--------------\n");
    local aa = a.toarray();
    dump_sa( "", aa);
    print("\n");
    
}
