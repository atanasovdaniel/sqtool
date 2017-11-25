
if( vargv.len() > 1)
{
    local fh = file(vargv[1], "rb");
    xmllex(fh);
    fh.close();
}
else
{
    xmllex(stdin);
}
