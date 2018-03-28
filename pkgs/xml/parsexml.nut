
local XMLLEX = require("xml.lexer");

local XML = {};

class XML.node
{
    name = null;
    parent = null;
}

class XML.Tag extends XML.node
{
    attrs = null;
    childs = null
    
    constructor( name)
    {
        this.name = name;
        this.attrs = [];
        this.childs = [];
    }
    
    function appendAttr( attr)
    {
        attr.parent = this.weakref();
        attrs.append( attr);
    }

    function appendChild( child)
    {
        child.parent = this.weakref();
        childs.append( child);
    }
    
    function getAttrib(name,defval=null)
    {
        foreach( a in attrs)
            if( a.name == name)
                return a.value;
        return defval;
    }
    
    function itChildNodes()
    {
        local function iter()
        {
            foreach( child in this.childs)
                if( child instanceof XML.Tag)
                    yield child;
        }
        return iter();
    }
    
    function dump()
    {
        print( "<" + name);
        foreach( v in attrs)
        {
            print( " " + v.name + "=\"" + v.value + "\"");
        }
        if( childs.len() != 0)
        {
            print( ">");
            foreach( child in childs) child.dump();
            print( "</" + name + ">");
        }
        else
        {
            print( "/>");
        }
    }
}

class XML.Attr extends XML.node
{
    name = null;
    value = null;
    
    constructor( name, value)
    {
        this.name = name;
        this.value = value;
    }
}

class XML.Text extends XML.node
{
    text = null;
    
    constructor( text)
    {
        this.name = "#TEXT";
        this.text = text;
    }
    
    function dump()
    {
        print( text);
    }
}

class XML.CDATA extends XML.Text
{
    constructor( text)
    {
        base.constructor( text);
        this.name = "#CDATA";
    }
    
    function dump()
    {
        print( "<[CDATA[" + text + "]]>");
    }
}

class XML.Comment extends XML.Text
{
    constructor( text)
    {
        base.constructor( text);
        this.name = "#COMMENT";
    }
    
    function dump()
    {
        print( "<!--" + text + "-->");
    }
}

class XML.PI extends XML.Text
{
    target = null;
    
    constructor( target, text)
    {
        base.constructor( text);
        this.target = target;
        this.name = "#PI";
    }
    
    function dump()
    {
        print( "<?" + target + text + "?>");
    }
}

class XML.DOCTYPE extends XML.node
{
    text = null;
    
    constructor( text)
    {
        this.text = text;
        this.name = "#DOCTYPE";
    }
    
    function dump()
    {
        print( text);
    }
}

class XML.Document extends XML.Tag
{
    XMLDecl = null;
    prolog = null;
    misc = null;
    
    constructor( name)
    {
        prolog = [];
        misc = [];
        base.constructor( name);
    }
    
    function dump()
    {
        print( "<?xml" + XMLDecl + "?>");
        foreach( el in prolog) el.dump();
        base.dump();
        foreach( el in misc) el.dump();
    }
}

class XML.xmllex_dbg extends XMLLEX
{
    function next()
    {
        local r = base.next();
        print( "[" + format( "%c", r) + "[" + text + "]]\n");
        return r;
    }
}

class XML.Parser
{
    lex = null;
    preserveSpaces = false;
    mergeCDATA = false;
    doc = null;
    elstack = null;
    curtag = null;
    tagstack = null;
    entities = null;
    
    function get_entity_ref( ref)
    {
        if( ref in entities)
            return entities[ref];
        else
            throw "unknown entity";
    }
    
    function parse_prolog()
    {
        local tok = lex.token;
        
        if( (tok == '?') && (lex.text == "xml")) {
            lex.next();
            doc.XMLDecl = lex.text;
            tok = lex.next();
        }
        
        while( true)
        switch( tok)
        {
            case 'S': doc.prolog.append( XML.Text( lex.text)); tok = lex.next(); break;
            case '!': doc.prolog.append( XML.Comment( lex.text)); tok = lex.next(); break;
            case 'D': doc.prolog.append( XML.DOCTYPE( lex.text)); tok = lex.next(); break;
            case '?': {
                local tgt = lex.text;
                lex.next();
                doc.prolog.append( XML.PI( tgt, lex.text));
                tok = lex.next();
            } break;
            default: return;
        }
    }
    
    function parse_attr_value()
    {
        local val = "";
        local is_ok = false;
        local tok = lex.token;
        while( true)
        switch( tok)
        {
            case 'T': is_ok = true; val += lex.text; tok = lex.next(); break;
            case '&': is_ok = true; val += get_entity_ref( lex.text); tok = lex.next(); break;
            default: if( is_ok) return val; else throw "attribute value expected";
        }
    }
    
    function parse_text()
    {
        local val = "";
        local tok = lex.token;
        while( true)
        switch( tok)
        {
            case 'S':
            case 'T': val += lex.text; tok = lex.next(); break;
            case '&': val += get_entity_ref( lex.text); tok = lex.next(); break;
            
            case 'C': if( mergeCDATA) { val += lex.text; tok = lex.next(); break; }
                    // else fall trough
            default: return val;
        }
    }
    
    function parse_element()
    {
        local tok = lex.token;
        while( true)
        switch( tok)
        {
            case '@': {
                local name = lex.text;
                tok = lex.next();
                local value = parse_attr_value();
                curtag.appendAttr( XML.Attr( name, value));
                tok = lex.token;
            } break;
            
            case '>':
                if( (lex.text != "") && (curtag.name != lex.text))
                    throw "open and close tag names not match";
                else
                {
                    tok = lex.next();
                    return;
                }
            
            case '<': {
                tagstack.push( curtag);
                curtag = XML.Tag( lex.text);
                lex.next();
                parse_element();
                tok = lex.token;
                local par = tagstack.pop();
                par.appendChild( curtag);
                curtag = par;
            } break;
            
            case 'C': if( !mergeCDATA) { curtag.appendChild( XML.CDATA( lex.text)); tok = lex.next(); break; }
                    // else fall trough
            case 'S':
            case 'T': curtag.appendChild( XML.Text( parse_text())); tok = lex.token; break;
            
            case '!': curtag.appendChild( XML.Comment( lex.text)); tok = lex.next(); break;
            case '?': {
                local tgt = lex.text;
                lex.next();
                curtag.appendChild( XML.PI( tgt, lex.text));
                tok = lex.next();
            } break;
            
            default: return;
        }
    }
    
    function parseStream( fh, path)
    {
        path = path;
        //lex = XML.xmllex_dbg(fh);
        lex = XMLLEX(fh);
        lex.preservespaces( preserveSpaces);
        
        doc = XML.Document("");

        lex.next();
        
        parse_prolog();
        
        if( lex.token == '<')
        {
            doc.name = lex.text;
            curtag = doc
            tagstack = [];
            lex.next();
            
            parse_element();
            
            if( tagstack.len() != 0)
                throw "tags left open";
            
            local tok = lex.token;
            while( tok != 0)
            switch( tok)
            {
                case 'S': doc.misc.append( XML.Text( parse_text())); tok = lex.token; break;
                case '!': doc.misc.append( XML.Comment( lex.text)); tok = lex.next(); break;
                case '?': doc.misc.append( XML.PI( lex.text)); tok = lex.next(); break;
                default: ::print( "bad: " + tok + "\n"); throw "junk after root node"
            }
            
            return doc;
        }
        else
            throw "root element expected";
    }
    
    function parseFile( path)
    {
        local fh = file( path, "rb");
        local xml;
        try
        {
            parseStream( fh, path);
        }
        catch( e)
        {
            fh.close();
            throw e;
        }
        fh.close();
        return doc;
    }
    
    constructor()
    {
        entities = { amp="&", lt="<", gt=">", apos="'", quot="\"" };
    }
}

return XML;
