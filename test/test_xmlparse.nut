


local IVAN = dofile("./test/parsexml.nut");

local parser = IVAN.Parser();
parser.preserveSpaces = true;
parser.mergeCDATA = true;
local xml = parser.parseFile( vargv[1]);
parser = null;
xml.dump();

// function gen1()
// {
//     yield "A";
//     yield "B";
//     yield "C";
// }
//
// foreach( k,v in gen1())
// {
//     print( k + ":" + v + "\n");
// }

function irec(root)
{
    if( root instanceof IVAN.Tag)
    {
        yield root;
        foreach( el in root.childs)
        {
            foreach( ch in irec(el)) yield ch;
        }
    }
}

foreach( n in irec(xml))
{
    print( n.name + "\n");
}
print( "---------\n");

class iter
{
    theiter = null;
    
    constructor( root)
    {
        local function li() { yield root; }
        theiter = li;
    }
    
    function child()
    {
        local it = theiter;
        local function li()
        {
            foreach( v in it())
                if( v instanceof IVAN.Tag)
                    foreach( el in v.childs)
                        yield el;
        }
        theiter = li;
        return this;
    }
    
    function rec()
    {
        local it = theiter;
        local _rec;
        _rec = function (el)
        {
            if( el instanceof IVAN.Tag)
                foreach( ch in el.childs)
                {
                    yield ch;
                    foreach( el in _rec(ch))
                        yield el;
                }
        }
        local function li()
        {
            foreach( v in it())
                if( v instanceof IVAN.Tag)
                    foreach( el in _rec(v))
                        yield el;
        }
        theiter = li;
        return this;
    }
    
    function node()
    {
        local it = theiter;
        local function li()
        {
            foreach( v in it())
                if( v instanceof IVAN.Tag)
                    yield v;
        }
        theiter = li;
        return this;
    }
    
    function name( nm)
    {
        local it = theiter;
        local function li()
        {
            foreach( v in it())
                if( (v instanceof IVAN.Tag) && (v.name == nm))
                    yield v;
        }
        theiter = li;
        return this;
    }
    
    function filt( fct)
    {
        local it = theiter;
        local function li()
        {
            foreach( v in it())
                if( fct( v))
                    yield v;
        }
        theiter = li;
        return this;
    }
    
    function it()
    {
        return theiter();
    }
}

foreach( n in iter(xml).rec().filt( @(v) v instanceof IVAN.Text).it())
{
    print( "\n---\n");
    n.dump();
}

