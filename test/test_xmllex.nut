
if( vargv.len() > 1)
{
    local fh = file(vargv[1], "rb");
    local lex = xmllex(fh);
    lex.preservespaces(true);
    
    while( lex.next() != '\0')
    {
        print( "[" + format( "%c", lex.token) + "[" + lex.text + "]]\n");
    }
    
    fh.close();
}
else
{
    local lex = xmllex(stdin);
    
    while( lex.next() != '\0')
    {
        print( "[" + format( "%c", lex.token) + "[" + lex.text + "]]\n");
    }
}
