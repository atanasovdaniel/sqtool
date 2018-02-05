/*
Copyright (c) 2017-2018 Daniel Atanasov

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <squirrel.h>
#include <sqstdaux.h>
#include <sqstdpackage.h>
#include <sqstdstream.h>

SQUIRREL_API SQUserPointer _sqt_xml_lexer_type_tag(void);
#define SQT_XML_LEXER_TYPE_TAG _sqt_xml_lexer_type_tag()

/*
    <TAG ATTR_NAME="ATTR-VALUE"/>
    <TAG>TEXT</TAG>
*/

#define BUF_GROW_STEP   1024

typedef enum {
    XML_TOK_EOF         = _SC('\0'),

    XML_TOK_TAG_START   = _SC('<'),     // <`NAME`
    XML_TOK_TAG_END     = _SC('>'),     // </`NAME` > or />

    XML_TOK_ATTR_NAME   = _SC('@'),     // `NAME`="

    XML_TOK_REFERENCE   = _SC('&'),     // &`NAME`;

    XML_TOK_COMMENT     = _SC('!'),     // <!--`...`-->

    XML_TOK_TEXT        = _SC('T'),     // `text` for element content, attribute value and PI body
    XML_TOK_SPACE       = _SC('S'),     // `spaces` for element content
    
    XML_TOK_CDATA       = _SC('C'),     // <![CDATA[`...`]]>`

    XML_TOK_PI_TARGET   = _SC('?'),     // <?`NAME`

    XML_TOK_DOCTYPE     = _SC('D'),     // `<!DOCTYPE ....>`

    XML_TOK_ERROR       = _SC('.'),
    
} xml_tok_t;

enum lex_state {
    //XMLSTATE_PROLOG = 0,
    XMLSTATE_DOCUMENT = 0,
    XMLSTATE_TAG,
    XMLSTATE_ATTR_VALUE,
    XMLSTATE_COMMENT,
    XMLSTATE_REF,
    XMLSTATE_REF2,
    XMLSTATE_TEXT,
    XMLSTATE_CDATA,
    XMLSTATE_PI,
    XMLSTATE_DOCTYPE,
    XMLSTATE_EOF,
    XMLSTATE_ERROR,
};

enum lex_error {
    XMLERR_NONE = 0,
    
    XMLERR_UNEXP_EOF,
    XMLERR_UNEXP_CHAR_SEQ,
    
    XMLERR__COUNT
};

typedef struct
{
    SQREADFUNC readfct;
    SQUserPointer user;
    HSQUIRRELVM v;

    SQChar back_char;

    SQChar *buf;
    SQInteger buf_len;
    SQInteger buf_alloc;

    SQChar quote;
    SQInteger level;
    SQBool preserve_spaces;

    enum lex_state state;
    enum lex_state saved_state;

    enum lex_error error;
    
} read_ctx_t;

static const SQChar ERR_MSG_ARG_NO_STREAM[] = _SC("argument is not a stream");

static const SQChar *xmlerr_text[XMLERR__COUNT] = {
    [XMLERR_NONE] = 0,
    [XMLERR_UNEXP_EOF] = _SC("unexpected EOF"),
    [XMLERR_UNEXP_CHAR_SEQ] = _SC("unexpected char sequence"),
};

#define SQ_EOF (SQChar)-1

// [3]   	S	   ::=   	(#x20 | #x9 | #xD | #xA)+
#define XML_IS_S(_c)   (((_c)==_SC(' '))||((_c)==_SC('\t'))||((_c)==_SC('\n'))||((_c)==_SC('\r')))

// [4]   	NameStartChar	   ::=   	":" | [A-Z] | "_" | [a-z] | [#xC0-#xD6] | [#xD8-#xF6] | [#xF8-#x2FF] | [#x370-#x37D] | [#x37F-#x1FFF] | [#x200C-#x200D] | [#x2070-#x218F] | [#x2C00-#x2FEF] | [#x3001-#xD7FF] | [#xF900-#xFDCF] | [#xFDF0-#xFFFD] | [#x10000-#xEFFFF]
#define XML_IS_NAMESTART(_c) ( \
    (((_c)>=_SC('A'))&&((_c)<=_SC('Z'))) || \
    (((_c)>=_SC('a'))&&((_c)<=_SC('z'))) || \
    ((_c)==_SC('_')) || \
    ((_c)==_SC(':')) || \
    ((unsigned int)(_c)>0x7F) \
)

// [4a]   	NameChar	   ::=   	NameStartChar | "-" | "." | [0-9] | #xB7 | [#x0300-#x036F] | [#x203F-#x2040]
#define XML_IS_NAME(_c) ( \
    XML_IS_NAMESTART(c) || \
    (((_c)>=_SC('0'))&&((_c)<=_SC('9'))) || \
    ((_c)==_SC('-')) || \
    ((_c)==_SC('.')) \
)

#define XML_IS_DEC(_c) ( \
    (((_c)>=_SC('0'))&&((_c)<=_SC('9'))) \
)

#define XML_IS_HEX(_c) ( \
    (((_c)>=_SC('0'))&&((_c)<=_SC('9'))) || \
    (((_c)>=_SC('A'))&&((_c)<=_SC('F'))) || \
    (((_c)>=_SC('a'))&&((_c)<=_SC('f'))) \
)

static void buf_append( read_ctx_t *ctx, SQChar c)
{
    if( ctx->buf_alloc <= ctx->buf_len) {
        ctx->buf_alloc += BUF_GROW_STEP;
        ctx->buf = sq_getscratchpad(ctx->v, ctx->buf_alloc*sizeof(SQChar));
    }
    ctx->buf[ctx->buf_len++] = c;
}

static SQChar read_char( read_ctx_t *ctx)
{
    SQChar c;

    if( ctx->back_char) {
        c = ctx->back_char;
        ctx->back_char = 0;
        return c;
    }

    if( ctx->readfct( ctx->user, &c, sizeof(SQChar)) != sizeof(SQChar)) {
        return SQ_EOF;
    }

    return c;
}

static void skip_spaces( read_ctx_t *ctx)
{
    SQChar c;
    while( ((c = read_char(ctx)) != SQ_EOF) && XML_IS_S(c) );
    ctx->back_char = c;
}

static SQBool normalize_EOL( read_ctx_t *ctx)
{
    const SQChar *src;
    SQChar *dst;
    SQBool is_space = SQTrue;
    buf_append(ctx, _SC('\0'));
    src = dst = ctx->buf;
    while( *src != _SC('\0')) {
        is_space = is_space && XML_IS_S(*src);
        if( src[0] == _SC('\r')) {
            if( src[1] == _SC('\n')) {
                src++;
            }
            *dst++ = _SC('\n');
            src++;
        }
        else {
            *dst++ = *src++;
        }
    }
    ctx->buf_len = dst - ctx->buf;
    return is_space;
}

static SQBool prepare_text( read_ctx_t *ctx)
{
    SQBool is_space = normalize_EOL(ctx);
    if( is_space && !ctx->preserve_spaces) {
        ctx->buf_len = 0;
    }
    return is_space;
}

static int read_name( read_ctx_t *ctx)
{
    SQChar c;
    if( ((c = read_char(ctx)) != SQ_EOF) && XML_IS_NAMESTART(c)) {
        buf_append(ctx,c);
        while( ((c = read_char(ctx)) != SQ_EOF) && XML_IS_NAME(c)) {
            buf_append(ctx,c);
        }
        ctx->back_char = c;
        return 0;
    }
    return 1;
}

static void append_unicode( read_ctx_t *ctx, SQUnsignedInteger code)
{
    if( code < 0x80) {
        buf_append(ctx, (SQChar)code);
    }
    else if( code < 0x800) {
        buf_append(ctx, 0xC0 | (code >> 6));
        buf_append(ctx, 0x80 | (code & 0x3F));
    }
    else if( code < 0x10000) {
        buf_append(ctx, 0xE0 | (code >> 12));
        buf_append(ctx, 0x80 | ((code >> 6) & 0x3F));
        buf_append(ctx, 0x80 | (code & 0x3F));
    }
    else if( code < 0x110000) {
        buf_append(ctx, 0xF0 | (code >> 18));
        buf_append(ctx, 0x80 | ((code >> 12) & 0x3F));
        buf_append(ctx, 0x80 | ((code >> 6) & 0x3F));
        buf_append(ctx, 0x80 | (code & 0x3F));
    }
}

static int char_reference( read_ctx_t *ctx)
{
    SQInteger prev_len = ctx->buf_len;
    SQChar c;

    c = read_char(ctx);
    if( c == _SC('x')) {
        while( ((c = read_char(ctx)) != SQ_EOF) && XML_IS_HEX(c)) {
            buf_append(ctx,c);
        }
        if( c == _SC(';')) {
            // &#12AB;
            char *end;
            SQUnsignedInteger code;
            buf_append(ctx,_SC('\0'));
            code = scstrtoul( ctx->buf + prev_len, &end, 16);
            if( code != 0) {
                ctx->buf_len = prev_len;
                append_unicode(ctx,code);
                return 0;
            }
        }
    }
    else if( XML_IS_DEC(c)) {
        buf_append(ctx,c);
        while( ((c = read_char(ctx)) != SQ_EOF) && XML_IS_DEC(c)) {
            buf_append(ctx,c);
        }
        if( c == _SC(';')) {
            // &#1234;
            char *end;
            SQUnsignedInteger code;
            buf_append(ctx,_SC('\0'));
            code = scstrtoul( ctx->buf + prev_len, &end, 10);
            if( code != 0) {
                ctx->buf_len = prev_len;
                append_unicode(ctx,code);
                return 0;
            }
        }
    }
    ctx->error = XMLERR_UNEXP_CHAR_SEQ;
    return 1;
}

static xml_tok_t lex_next( read_ctx_t *ctx)
{
    SQChar c;

    while(1)
    switch( ctx->state)
    {
        //case XMLSTATE_PROLOG:
        case XMLSTATE_DOCUMENT:
            c = read_char(ctx);
            if( c == _SC('<')) {
                c = read_char(ctx);
                if( c == _SC('/')) {
                    if( read_name(ctx) == 0) {
                        skip_spaces(ctx);
                        if( (c = read_char(ctx)) == _SC('>')) {
                            return XML_TOK_TAG_END;
                        }
                    }
                }
                else if( c == _SC('?')) {
                    if( read_name(ctx) == 0) {
                        if( (c = read_char(ctx)) != SQ_EOF) {
                            if( XML_IS_S(c) || (c == _SC('?'))) {
                                ctx->back_char = c;
                                ctx->saved_state = ctx->state;
                                ctx->state = XMLSTATE_PI;
                                if( (ctx->buf_len == 3) &&
                                    ((ctx->buf[0] == _SC('x')) || (ctx->buf[0] == _SC('X'))) &&
                                    ((ctx->buf[1] == _SC('m')) || (ctx->buf[1] == _SC('M'))) &&
                                    ((ctx->buf[2] == _SC('l')) || (ctx->buf[2] == _SC('L'))) )
                                    return XML_TOK_PI_TARGET;
                                else
                                    return XML_TOK_PI_TARGET;
                            }
                        }
                    }
                }
                else if( c == _SC('!')) {
                    c = read_char(ctx);
                    if( c == _SC('-')) {
                        c = read_char(ctx);
                        if( c == _SC('-')) {
                            ctx->saved_state = ctx->state;
                            ctx->state = XMLSTATE_COMMENT;
                            continue;
                        }
                    }
                    else if( c == _SC('[')) {
                        // <![CDATA[ .... ]]>
                        if( read_name(ctx) == 0) {
                            buf_append(ctx,_SC('\0'));
                            if( scstrcmp( ctx->buf, _SC("CDATA")) == 0) {
                                c = read_char(ctx);
                                if( c == _SC('[')) {
                                    ctx->buf_len = 0;
                                    ctx->state = XMLSTATE_CDATA;
                                    continue;
                                }
                            }
                        }
                    }
                    else { // if( ctx->state == XMLSTATE_PROLOG) {
                        // <!DOCTYPE
                        buf_append(ctx,_SC('<'));
                        buf_append(ctx,_SC('!'));
                        ctx->back_char = c;
                        if( read_name(ctx) == 0) {
                            if( ((c = read_char(ctx)) != SQ_EOF) && XML_IS_S(c)) {
                                buf_append(ctx,_SC('\0'));
                                if( scstrcmp( ctx->buf+2, _SC("DOCTYPE")) == 0) {
                                    ctx->buf[ctx->buf_len-1] = c;
                                    ctx->level = 1;
                                    ctx->state = XMLSTATE_DOCTYPE;
                                    continue;
                                }
                            }
                        }
                    }
                }
                else {
                    // <TAG
                    ctx->back_char = c;
                    if( read_name(ctx) == 0) {
                        if( ((c = read_char(ctx)) != SQ_EOF) && (XML_IS_S(c) || (c == _SC('>')) || (c == _SC('/')))) {
                            ctx->back_char = c;
                            ctx->state = XMLSTATE_TAG;
                            return XML_TOK_TAG_START;
                        }
                    }
                }
            }
            else if( c == SQ_EOF) {
                ctx->back_char = c;
                ctx->state = XMLSTATE_EOF;
                return XML_TOK_EOF;
            }
            else {
//                 if( ctx->state == XMLSTATE_DOCUMENT) {
                    // text
                    ctx->back_char = c;
                    ctx->state = XMLSTATE_TEXT;
                    continue;
//                 }
//                 else if( XML_IS_S(c)) { // if( ctx->state == XMLSTATE_PROLOG)
//                     if( !ctx->preserve_spaces) {
//                         skip_spaces(ctx);
//                         continue;
//                     }
//                     else {
//                         buf_append(ctx,c);
//                         while( ((c = read_char(ctx)) != SQ_EOF) && XML_IS_S(c)) {
//                             buf_append(ctx,c);
//                         }
//                         ctx->back_char = c;
//                         normalize_EOL(ctx);
//                         return XML_TOK_TEXT;
//                     }
//                 }
            }
            ctx->error = XMLERR_UNEXP_CHAR_SEQ;
            ctx->state = XMLSTATE_ERROR;
            return XML_TOK_ERROR;

        case XMLSTATE_TAG:
            skip_spaces(ctx);
            if( (c = read_char(ctx)) != SQ_EOF) {
                if( c == _SC('>')) {
                    ctx->state = XMLSTATE_DOCUMENT;
                    continue;
                }
                else if( c == _SC('/')) {
                    if( (c = read_char(ctx)) == _SC('>')) {
                        ctx->state = XMLSTATE_DOCUMENT;
                        return XML_TOK_TAG_END;
                    }
                }
                else {
                    ctx->back_char = c;
                    if( read_name(ctx) == 0) {
                        skip_spaces(ctx);
                        if( (c = read_char(ctx)) == _SC('=')) {
                            skip_spaces(ctx);
                            if( ((c = read_char(ctx)) == _SC('"')) || (c == _SC('\''))) {
                                ctx->quote = c;
                                ctx->state = XMLSTATE_ATTR_VALUE;
                                return XML_TOK_ATTR_NAME;
                            }
                        }
                    }
                }
                ctx->error = XMLERR_UNEXP_CHAR_SEQ;
            }
            else {
                ctx->error = XMLERR_UNEXP_EOF;
            }
            ctx->state = XMLSTATE_ERROR;
            return XML_TOK_ERROR;

        case XMLSTATE_ATTR_VALUE:
            while( ((c = read_char(ctx)) != SQ_EOF) && (c != ctx->quote) && (c != _SC('<')) && (c != _SC('&')) ) {
                buf_append(ctx,c);
            }
            if( c == ctx->quote) {
                ctx->state = XMLSTATE_TAG;
                normalize_EOL(ctx);
                return XML_TOK_TEXT;    // return XML_TOK_ATTR_VALUE;
            }
            else if( c == _SC('&')) {
                ctx->saved_state = ctx->state;
                ctx->state = XMLSTATE_REF;
                continue;   //return XML_TOK_ATTR_VALUE;          // ATTR_VALUE (part)
            }
            else if( c != SQ_EOF) {
                ctx->error = XMLERR_UNEXP_CHAR_SEQ;
            }
            else {
                ctx->error = XMLERR_UNEXP_EOF;
            }
            ctx->state = XMLSTATE_ERROR;
            return XML_TOK_ERROR;

        case XMLSTATE_COMMENT:
            do {
                while( ((c = read_char(ctx)) != SQ_EOF) && (c != _SC('-')) ) {
                    buf_append(ctx,c);
                }
                if( c == _SC('-')) {
                    c = read_char(ctx);
                    if( c == _SC('-')) {
                        c = read_char(ctx);
                        if( c == _SC('>')) {
                            ctx->state = ctx->saved_state;
                            return XML_TOK_COMMENT;
                        }
                        else {
                            ctx->error = XMLERR_UNEXP_CHAR_SEQ; // -- not allowed in comment
                            ctx->state = XMLSTATE_ERROR;
                            return XML_TOK_ERROR;
                        }
                    }
                    buf_append(ctx,_SC('-'));
                    buf_append(ctx,c);
                }
            } while( c != SQ_EOF);
            if( c == SQ_EOF) {
                ctx->error = XMLERR_UNEXP_EOF;
            }
            ctx->state = XMLSTATE_ERROR;
            return XML_TOK_ERROR;

        case XMLSTATE_REF:
            if( ((c = read_char(ctx)) != SQ_EOF)) {
                if( c == _SC('#')) {
                    if( char_reference(ctx) == 0) {
                        ctx->state = ctx->saved_state;
                        continue;
                    }
                }
                else {
                    ctx->back_char = c;
                    ctx->state = XMLSTATE_REF2;
                    if( ctx->saved_state == XMLSTATE_TEXT ) {
                        SQBool is_space = prepare_text(ctx);
                        if( ctx->buf_len) {
                            return is_space ? XML_TOK_SPACE : XML_TOK_TEXT;
                        }
                    }
                    else { // ctx->saved_state == XMLSTATE_ATTR_VALUE
                        normalize_EOL(ctx);
                        if( ctx->buf_len) {
                            return XML_TOK_TEXT;
                        }
                    }
                    continue;
                }
            }
            if( c == SQ_EOF) {
                ctx->error = XMLERR_UNEXP_EOF;
            }
            ctx->state = XMLSTATE_ERROR;
            return XML_TOK_ERROR;
            
        case XMLSTATE_REF2:
            if( read_name(ctx) == 0) {
                if( ((c = read_char(ctx)) == _SC(';'))) {
                    // &amp;
                    ctx->state = ctx->saved_state;
                    return XML_TOK_REFERENCE;
                }
                ctx->error = XMLERR_UNEXP_CHAR_SEQ;
            }
            ctx->state = XMLSTATE_ERROR;
            return XML_TOK_ERROR;

        case XMLSTATE_TEXT:
            while( ((c = read_char(ctx)) != SQ_EOF) && (c != _SC('<')) && (c != _SC('>')) && (c != _SC('&'))) {
                buf_append(ctx,c);
            }
            if( c == _SC('&')) {
                ctx->saved_state = ctx->state;
                ctx->state = XMLSTATE_REF;
                continue;
            }
            else {
                SQBool is_space;
                ctx->back_char = c;
                ctx->state = XMLSTATE_DOCUMENT;
                is_space = prepare_text(ctx);
                if( ctx->buf_len)
                    return is_space ? XML_TOK_SPACE : XML_TOK_TEXT;
                else
                    continue;
            }

        case XMLSTATE_CDATA:
            while( ((c = read_char(ctx)) != SQ_EOF) && (c != _SC('>'))) {
                buf_append(ctx,c);
            }
            if( c == _SC('>')) {
                if( (ctx->buf_len > 1) && (ctx->buf[ctx->buf_len-1] == _SC(']')) && (ctx->buf[ctx->buf_len-2] == _SC(']')) ) {
                    ctx->buf_len -= 2;
                    ctx->state = XMLSTATE_DOCUMENT;
                    return XML_TOK_CDATA;
                }
                buf_append(ctx,c);
                continue;
            }
            ctx->error = XMLERR_UNEXP_EOF;
            ctx->state = XMLSTATE_ERROR;
            return XML_TOK_ERROR;

        case XMLSTATE_PI:
            while( ((c = read_char(ctx)) != SQ_EOF) && (c != _SC('?')) ) {
                buf_append(ctx,c);
            }
            if( c == _SC('?')) {
                if( ((c = read_char(ctx)) != SQ_EOF) && (c == _SC('>')) ) {
                    ctx->state = ctx->saved_state;
                    normalize_EOL(ctx);
                    return XML_TOK_TEXT;    // return XML_TOK_PI_BODY;
                }
                else if( c != SQ_EOF) {
                    buf_append(ctx,_SC('?'));
                    ctx->back_char = c;
                    continue;
                }
            }
            ctx->error = XMLERR_UNEXP_EOF;
            ctx->state = XMLSTATE_ERROR;
            return XML_TOK_ERROR;

        // '<!DOCTYPE' S Name (S ExternalID)? S? ('[' intSubset ']' S?)? '>'
        //  '<!ELEMENT' S Name S contentspec S? '>'
        //  '<!ATTLIST' S Name AttDef* S? '>'
        //  '<!ENTITY' S Name S EntityDef S? '>'
        //  '<!ENTITY' S '%' S Name S PEDef S? '>'
        //  '<!NOTATION' S Name S (ExternalID | PublicID) S? '>'
        //  '<![' S? 'INCLUDE' S? '[' extSubsetDecl ']]>'
        //  '<![' S? 'IGNORE' S? '[' ignoreSectContents* ']]>'
        //  PI
        //  Comment
        case XMLSTATE_DOCTYPE:
            while( ((c = read_char(ctx)) != SQ_EOF) && (c != _SC('>')) && (c != _SC('<')) ) {
                buf_append(ctx,c);
            }
            if( c == _SC('>')) {
                buf_append(ctx,c);
                ctx->level--;
                if( !ctx->level) {
                    ctx->state = XMLSTATE_DOCUMENT;   // XMLSTATE_PROLOG;
                    return XML_TOK_DOCTYPE;
                }
                continue;
            }
            else if( c == _SC('<')) {
                buf_append(ctx,c);
                ctx->level++;
                continue;
            }
            ctx->error = XMLERR_UNEXP_EOF;
            ctx->state = XMLSTATE_ERROR;
            return XML_TOK_ERROR;
        
        case XMLSTATE_EOF:
            return XML_TOK_EOF;

        case XMLSTATE_ERROR:
            return XML_TOK_ERROR;
    }
}

SQRESULT sqt_xmllex_load_cb(HSQUIRRELVM v, SQ_UNUSED_ARG(const SQChar *opts), SQREADFUNC readfct, SQUserPointer user)
{
    read_ctx_t ctx;
    xml_tok_t tok;

    ctx.v = v;
    ctx.readfct = readfct;
    ctx.user = user;

    ctx.back_char = 0;

    ctx.buf_len = 0;
    ctx.buf_alloc = BUF_GROW_STEP;
    ctx.buf = sq_getscratchpad(v, ctx.buf_alloc*sizeof(SQChar));

    ctx.quote = 0;
    ctx.level = 0;
    ctx.preserve_spaces = SQFalse;

    ctx.state = XMLSTATE_DOCUMENT;    // XMLSTATE_PROLOG;
    ctx.saved_state = XMLSTATE_DOCUMENT;    // XMLSTATE_PROLOG;
    
    ctx.error = XMLERR_NONE;

    while( ((tok = lex_next(&ctx)) != XML_TOK_EOF) && (tok != XML_TOK_ERROR))
    {
        ctx.buf[ctx.buf_len] = '\0';
        printf( "[%c[%s]]", tok, ctx.buf);
        ctx.buf_len = 0;
    }
    if( tok == XML_TOK_ERROR) {
        return sq_throwerror(v, xmlerr_text[ctx.error]);
    }

	return 0;
}

SQRESULT sqt_xmllex_load(HSQUIRRELVM v, const SQChar *opts, SQSTREAM stream)
{
    return sqt_xmllex_load_cb(v, opts, sqstd_STREAMREADFUNC, stream);
}

extern const SQRegClass _sqt_xmllex_decl;
#define SQT_XMLLEX_TYPE_TAG ((SQUserPointer)(SQHash)&_sqt_xmllex_decl)

static HSQMEMBERHANDLE xmllex__stream_handle;
static HSQMEMBERHANDLE xmllex_token_handle;
static HSQMEMBERHANDLE xmllex_text_handle;

#define SETUP_XMLLEX(v) \
    read_ctx_t *ctx = NULL; \
    if(SQ_FAILED(sq_getinstanceup(v,1,(SQUserPointer*)&ctx,(SQUserPointer)SQT_XMLLEX_TYPE_TAG))) \
        return sq_throwerror(v,_SC("invalid type tag"));

static SQInteger __xmllex_releasehook(SQUserPointer p, SQInteger size)
{
    sq_free( p, size);
    return 1;
}

static SQInteger _xmllex__typeof(HSQUIRRELVM v)
{
    sq_pushstring(v,_sqt_xmllex_decl.name,-1);
    return 1;
}

static SQInteger _xmllex_constructor(HSQUIRRELVM v)
{
	SQSTREAM stream;
    const SQChar *opts = _SC("");
    read_ctx_t *ctx;

    if( SQ_FAILED( sq_getinstanceup( v,2,(SQUserPointer*)&stream,(SQUserPointer)SQSTD_STREAM_TYPE_TAG))) {
        return sq_throwerror(v,ERR_MSG_ARG_NO_STREAM);
	}
    
    if( sq_gettop(v) > 2)
    {
        sq_getstring(v,3,&opts);
    }
    
    ctx = sq_malloc( sizeof(read_ctx_t));
    if( ctx) {
        ctx->v = v;
        ctx->readfct = sqstd_STREAMREADFUNC;
        ctx->user = stream;

        ctx->back_char = 0;

        ctx->buf_len = 0;
        ctx->buf_alloc = 0;
        ctx->buf = 0;

        ctx->quote = 0;
        ctx->level = 0;
        ctx->preserve_spaces = SQFalse;

        ctx->state = XMLSTATE_DOCUMENT; // XMLSTATE_PROLOG;
        ctx->saved_state = XMLSTATE_DOCUMENT;   // XMLSTATE_PROLOG;
    
        ctx->error = XMLERR_NONE;
    
        if(SQ_FAILED(sq_setinstanceup(v,1,ctx))) {
            sq_free( ctx, sizeof(read_ctx_t));
            return sq_throwerror(v, _SC("cannot create xmllex instance"));
        }
        
		sq_setreleasehook(v,1,__xmllex_releasehook);

        // save stream in _stream member
        sq_push(v,2);
        sq_setbyhandle(v,1,&xmllex__stream_handle);

        sq_pushinteger(v,-1);
        sq_setbyhandle(v,1,&xmllex_token_handle);
    
        return 0;
    }
	return sq_throwerror(v, _SC("cannot create xmllex"));
}

static SQRESULT _xmllex_next(HSQUIRRELVM v)
{
    xml_tok_t tok;
    SETUP_XMLLEX(v);
    
    ctx->v = v;
    ctx->buf_len = 0;
    ctx->buf_alloc = 0;
    ctx->buf = 0;
    
    tok = lex_next(ctx);
    if( tok == XML_TOK_ERROR) {
        return sq_throwerror(v, xmlerr_text[ctx->error]);
    }

    if( ctx->buf_len) {
        sq_pushstring(v, ctx->buf, ctx->buf_len);
    }
    else {
        sq_pushstring(v, _SC(""), 0);
    }
    sq_setbyhandle(v,1,&xmllex_text_handle);
    
    sq_pushinteger(v, (SQInteger)tok);
    sq_push(v,-1);
    sq_setbyhandle(v,1,&xmllex_token_handle);
    
    return 1;
}

static SQRESULT _xmllex_preservespaces(HSQUIRRELVM v)
{
    SQBool val;
    SETUP_XMLLEX(v);
    sq_getbool(v,2,&val);
    ctx->preserve_spaces = val;
    return 0;
}

#define _DECL_XMLLEX_FUNC(name,nparams,typecheck) {_SC(#name),_xmllex_##name,nparams,typecheck}
static const SQRegFunction _xmllex_methods[]={
    _DECL_XMLLEX_FUNC(constructor,-2,_SC("xxs")),
    _DECL_XMLLEX_FUNC(_typeof,1,_SC("x")),
    _DECL_XMLLEX_FUNC(next,1,_SC(".")),
    _DECL_XMLLEX_FUNC(preservespaces,2,_SC(".b")),
    {NULL,(SQFUNCTION)0,0,NULL}
};

static const SQRegMember _xmllex_members[] = {
	{_SC("text"), &xmllex_text_handle },
	{_SC("token"), &xmllex_token_handle },
	{_SC("_stream"), &xmllex__stream_handle },
	{NULL,NULL}
};

SQUserPointer _sqt_xml_lexer_type_tag(void)
{
    return (SQUserPointer)_sqt_xml_lexer_type_tag;
}

const SQRegClass _sqt_xmllex_decl = {
//	NULL,               // base_class
//    _SC("sqt_xml_lexer"),  // reg_name
    _SC("lexer"),		// name
	_xmllex_members,	// members
	_xmllex_methods,	// methods
};

SQUIRREL_API SQInteger SQPACKAGE_LOADFCT(HSQUIRRELVM v);
SQInteger SQPACKAGE_LOADFCT(HSQUIRRELVM v)
{
	if(SQ_FAILED(sqstd_registerclass(v,SQT_XML_LEXER_TYPE_TAG,&_sqt_xmllex_decl,0))) {
		return SQ_ERROR;
	}
    return 1;
}

SQRESULT sqstd_register_xml_lexer(HSQUIRRELVM v)
{
    if(SQ_SUCCEEDED(sqstd_package_registerfct(v,_SC("xml.lexer"),SQPACKAGE_LOADFCT))) {
        sq_poptop(v);
        return SQ_OK;
    }
    return SQ_ERROR;
}

