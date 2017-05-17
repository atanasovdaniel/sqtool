#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <squirrel.h>
#include <sqtool.h>
#include <sqt_basictypes.h>

//#define SQTSA_IS_FLOAT  0x200
//#define SQTSA_IS_SIGNED 0x100
//#define SQTSA_TYPE_MASK (SQTSA_IS_FLOAT | SQTSA_IS_SIGNED)

extern SQUIRREL_API_VAR const SQTClassDecl sqt_basictype_decl;
#define SQT_BASICTYPE_TYPE_TAG ((SQUserPointer)(SQHash)&sqt_basictype_decl)
extern SQUIRREL_API_VAR const SQTClassDecl sqt_basicvalue_decl;
#define SQT_BASICVALUE_TYPE_TAG ((SQUserPointer)(SQHash)&sqt_basicvalue_decl)
extern SQUIRREL_API_VAR const SQTClassDecl sqt_basicpointer_decl;
#define SQT_BASICPOINTER_TYPE_TAG ((SQUserPointer)(SQHash)&sqt_basicpointer_decl)
extern SQUIRREL_API_VAR const SQTClassDecl sqt_basicarray_decl;
#define SQT_BASICARRAY_TYPE_TAG ((SQUserPointer)(SQHash)&sqt_basicarray_decl)
extern SQUIRREL_API_VAR const SQTClassDecl sqt_basicmember_decl;
#define SQT_BASICMEMBER_TYPE_TAG ((SQUserPointer)(SQHash)&sqt_basicmember_decl)
extern SQUIRREL_API_VAR const SQTClassDecl sqt_basicstruct_decl;
#define SQT_BASICSTRUCT_TYPE_TAG ((SQUserPointer)(SQHash)&sqt_basicstruct_decl)

typedef void (*sqt_basicpush_fct)(const SQTBasicTypeDef *bt, HSQUIRRELVM v, SQUserPointer p);
typedef SQRESULT (*sqt_basicget_fct)(const SQTBasicTypeDef *bt, HSQUIRRELVM v, SQInteger idx, SQUserPointer p);

struct tagSQTBasicTypeDef
{
    sqt_basicpush_fct push;
    sqt_basicget_fct get;
    SQInteger size;
    SQInteger align;
    const SQChar *name;
};

SQInteger sqt_basicgetsize( const SQTBasicTypeDef *bt)
{
    return bt->size;
}
SQInteger sqt_basicgetalign( const SQTBasicTypeDef *bt)
{
    return bt->align;
}
void sqt_basicpush( const SQTBasicTypeDef *bt, HSQUIRRELVM v, SQUserPointer p)
{
    bt->push( bt, v, p);
}
SQRESULT sqt_basicget( const SQTBasicTypeDef *bt, HSQUIRRELVM v, SQInteger idx, SQUserPointer p)
{
    return bt->get( bt, v, idx, p);
}

static SQInteger sqt_basicalign( SQInteger align, SQInteger offset)
{
    SQInteger rem = offset % align;
    if( rem) {
        offset += align - rem;
    }
    return offset;
}

const SQTBasicTypeDef *sqt_basictypebyid( SQInteger id)
{
    switch(id) {
    case 'l': return &SQ_Basic_SQInteger;
    case 'i': return &SQ_Basic_int32_t;
    case 's': return &SQ_Basic_int16_t;
    case 'w': return &SQ_Basic_uint16_t;
    case 'c': return &SQ_Basic_int8_t;
    case 'b': return &SQ_Basic_uint8_t;
    case 'f': return &SQ_Basic_float;
    case 'd': return &SQ_Basic_double;
    default: return NULL;
    }
}

#define MK_BASE(_type, _sqtype, _push, _get) \
static void sq_pushbasic_##_type( const SQTBasicTypeDef *bt, HSQUIRRELVM v,const SQUserPointer p) \
{ \
    _sqtype val = (_sqtype)*(const _type*)p; \
    _push(v,val); \
} \
static SQRESULT sq_getbasic_##_type( const SQTBasicTypeDef *bt, HSQUIRRELVM v, SQInteger idx, SQUserPointer p) \
{ \
    _sqtype val; \
    if( SQ_SUCCEEDED(_get(v,idx,&val))) { \
        *(_type*)p = (_type)val; \
        return SQ_OK; \
    } \
    return SQ_ERROR; \
} \
const SQTBasicTypeDef SQ_Basic_##_type = { \
    sq_pushbasic_##_type, \
    sq_getbasic_##_type, \
    sizeof(_type), \
    offsetof( struct { uint8_t b; _type me; }, me), \
    _SC(#_type), \
};

MK_BASE( uint8_t, SQInteger, sq_pushinteger, sq_getinteger)
MK_BASE( int8_t, SQInteger, sq_pushinteger, sq_getinteger)
MK_BASE( uint16_t, SQInteger, sq_pushinteger, sq_getinteger)
MK_BASE( int16_t, SQInteger, sq_pushinteger, sq_getinteger)
MK_BASE( uint32_t, SQInteger, sq_pushinteger, sq_getinteger)
MK_BASE( int32_t, SQInteger, sq_pushinteger, sq_getinteger)
#ifdef _SQ64
MK_BASE( uint64_t, SQInteger, sq_pushinteger, sq_getinteger)
MK_BASE( int64_t, SQInteger, sq_pushinteger, sq_getinteger)
#endif // _SQ64

MK_BASE( float, SQFloat, sq_pushfloat, sq_getfloat)
MK_BASE( double, SQFloat, sq_pushfloat, sq_getfloat)

// void
static void sq_pushbasic_void( const SQTBasicTypeDef *bt, HSQUIRRELVM v,const SQUserPointer p)
{
	sq_throwerror(v,_SC("can't push C type void"));
}
static SQRESULT sq_getbasic_void( const SQTBasicTypeDef *bt, HSQUIRRELVM v, SQInteger idx, SQUserPointer p)
{
	return sq_throwerror(v,_SC("can't get C type void"));
}
const SQTBasicTypeDef SQ_Basic_void = {
    sq_pushbasic_void,
    sq_getbasic_void,
    0,
    1,
    _SC("void"),
};

// // void *
// static void sq_pushbasic_voidptr( const SQTBasicTypeDef *bt, HSQUIRRELVM v,const SQUserPointer p)
// {
//     SQUserPointer val = (SQUserPointer)*(const void**)p;
// 	sq_pushuserpointer(v,val);
// }
// static SQRESULT sq_getbasic_voidptr( const SQTBasicTypeDef *bt, HSQUIRRELVM v, SQInteger idx, SQUserPointer p)
// {
//     SQUserPointer val;
//     SQUserPointer basicvalue;
// 	SQObjectType ot = sq_gettype(v,idx);
// 	if( ot == OT_USERPOINTER) {
// 		sq_getuserpointer(v,idx,&val);
// 	}
// 	else if( ot == OT_USERDATA) {
// 		sq_getuserdata(v,idx,&val,NULL);
// 	}
// 	else if( ot == OT_NULL) {
// 		val = NULL;
// 	}
//     else if( (ot == OT_INSTANCE) && (SQ_SUCCEEDED(sq_getinstanceup(v,idx,(SQUserPointer*)&basicvalue,SQT_BASICVALUE_TYPE_TAG)))) {
//         const SQTBasicTypeDef *type;
//         if(SQ_FAILED(sqt_basicvalue_get(v,idx, &type, val)))
//             return SQ_ERROR;
//     }
// 	else return sq_throwerror(v,_SC("expecting userpointer, userdata, basicvalue or null"));
//     *(void**)p = (void*)val;
//     return SQ_OK;
// }
// const SQTBasicTypeDef SQ_Basic_voidptr = {
//     sq_pushbasic_voidptr,
//     sq_getbasic_voidptr,
//     sizeof(void*),
//     offsetof( struct { uint8_t b; void *me; }, me),
//     _SC("voidptr"),
// };

/* ------------------------------------
    Basic type
------------------------------------ */

SQRESULT sqt_getbasictype(HSQUIRRELVM v, SQInteger idx, const SQTBasicTypeDef **pbasictype)
{
    if(SQ_FAILED(sq_getinstanceup(v,idx,(SQUserPointer*)pbasictype,SQT_BASICTYPE_TYPE_TAG)))
        return sq_throwerror(v,_SC("expecting basetype instance"));
    return SQ_OK;
}

static SQInteger _basic_constructor(HSQUIRRELVM v)
{
    SQUserPointer bt;
    sq_getuserpointer(v,2,&bt);
	if(SQ_FAILED(sq_setinstanceup(v,1,bt))) {
		return sq_throwerror(v, _SC("cannot create basictype instance"));
	}
	return 0;
}

static SQInteger _basic__typeof(HSQUIRRELVM v)
{
    sq_pushstring(v,sqt_basictype_decl.name,-1);
    return 1;
}

static SQInteger _basic_size(HSQUIRRELVM v)
{
    const SQTBasicTypeDef *self;
    if(SQ_FAILED(sqt_getbasictype(v,1,&self))) return SQ_ERROR;
    sq_pushinteger(v,sqt_basicgetsize(self));
    return 1;
}

static SQInteger _basic_align(HSQUIRRELVM v)
{
    const SQTBasicTypeDef *self;
    if(SQ_FAILED(sqt_getbasictype(v,1,&self))) return SQ_ERROR;
    sq_pushinteger(v,sqt_basicgetalign(self));
    return 1;
}

static SQInteger _basic_name(HSQUIRRELVM v)
{
    const SQTBasicTypeDef *self;
    if(SQ_FAILED(sqt_getbasictype(v,1,&self))) return SQ_ERROR;
    sq_pushstring(v,self->name,-1);
    return 1;
}

static SQInteger _basic_pointer(HSQUIRRELVM v)
{
    const SQTBasicTypeDef *self;
    if(SQ_FAILED(sqt_getbasictype(v,1,&self))) return SQ_ERROR;
    sq_pushregistrytable(v);                    // self, size, registry
    sq_pushstring(v,sqt_basicpointer_decl.reg_name,-1);    // self, size, registry, "basicpointer"
    if(SQ_FAILED(sq_get(v,-2)))                 // self, size, registry, basicpointer
        return SQ_ERROR;
    sq_pushnull(v);                             // self, size, registry, basicpointer, dummy_this
    sq_push(v,1);  // repush oftype             // self, size, registry, basicpointer, dummy_this, oftype
    if(SQ_FAILED(sq_call(v,2,SQTrue,SQFalse)))  // self, size, registry, basicpointer, ret_val
        return SQ_ERROR;
    return 1;
}

static SQInteger _basic_array(HSQUIRRELVM v)
{
    const SQTBasicTypeDef *self;
    if(SQ_FAILED(sqt_getbasictype(v,1,&self))) return SQ_ERROR;
    sq_pushregistrytable(v);                    // self, size, registry
    sq_pushstring(v,sqt_basicarray_decl.reg_name,-1);    // self, size, registry, "basicarray"
    if(SQ_FAILED(sq_get(v,-2)))                 // self, size, registry, basicarray
        return SQ_ERROR;
    sq_pushnull(v);                             // self, size, registry, basicarray, dummy_this
    sq_push(v,1);  // repush oftype             // self, size, registry, basicarray, dummy_this, oftype
    sq_push(v,2);  // repush size               // self, size, registry, basicarray, dummy_this, oftype, length
    if(SQ_FAILED(sq_call(v,3,SQTrue,SQFalse)))  // self, size, registry, basicarray, ret_val
        return SQ_ERROR;
    return 1;
}

static SQInteger _basic_member(HSQUIRRELVM v)
{
    const SQTBasicTypeDef *self;
    if(SQ_FAILED(sqt_getbasictype(v,1,&self))) return SQ_ERROR;
    sq_pushregistrytable(v);                    // self, size, registry
    sq_pushstring(v,sqt_basicmember_decl.reg_name,-1);    // self, size, registry, "basicmember"
    if(SQ_FAILED(sq_get(v,-2)))                 // self, size, registry, basicmember
        return SQ_ERROR;
    sq_pushnull(v);                             // self, size, registry, basicmember, dummy_this
    sq_push(v,2);  // repush name               // self, size, registry, basicmember, dummy_this, name
    sq_push(v,1);  // repush type               // self, size, registry, basicmember, dummy_this, name, type
    if(SQ_FAILED(sq_call(v,3,SQTrue,SQFalse)))  // self, size, registry, basicmember, ret_val
        return SQ_ERROR;
    return 1;
}

static SQInteger _basic_value(HSQUIRRELVM v)
{
    SQInteger top;
    const SQTBasicTypeDef *self;
    if(SQ_FAILED(sqt_getbasictype(v,1,&self))) return SQ_ERROR;
    top = sq_gettop(v);
    if( top > 3) {
        sq_settop(v,3);
        top = 3;
    }
    sq_pushregistrytable(v);                    // self, opts, registry
    sq_pushstring(v,sqt_basicvalue_decl.reg_name,-1);    // self, opts, registry, "basicvalue"
    if(SQ_FAILED(sq_get(v,-2)))                 // self, opts, registry, basicvalue
        return SQ_ERROR;
    sq_pushnull(v);                             // self, opts, registry, basicvalue, dummy_this
    sq_push(v,1); // repush this                // self, opts, registry, basicvalue, dummy_this, type
    if(top > 1) {
        sq_push(v,2); // repush data            // self, opts, registry, basicvalue, dummy_this, type, data
        if(top > 2) {
            sq_push(v,3); // repush offset      // self, opts, registry, basicvalue, dummy_this, type, data, offset
        }
    }
    if(SQ_FAILED(sq_call(v,1+top,SQTrue,SQFalse)))  // self, size, registry, basicvalue, ret_val
        return SQ_ERROR;
    return 1;
}

#define _DECL_BASICTYPE_FUNC(name,nparams,typecheck) {_SC(#name),_basic_##name,nparams,typecheck}
static const SQRegFunction _type_methods[] = {
    _DECL_BASICTYPE_FUNC(constructor,2,_SC("xp")),
    _DECL_BASICTYPE_FUNC(_typeof,1,_SC("x")),
    _DECL_BASICTYPE_FUNC(size,1,_SC("x")),
    _DECL_BASICTYPE_FUNC(align,1,_SC("x")),
    _DECL_BASICTYPE_FUNC(name,1,_SC("x")),
    _DECL_BASICTYPE_FUNC(pointer,1,_SC("x")),
    _DECL_BASICTYPE_FUNC(array,2,_SC("xi")),
    _DECL_BASICTYPE_FUNC(member,2,_SC("xs")),
    _DECL_BASICTYPE_FUNC(value,-1,_SC("xp|ui")),
    {NULL,(SQFUNCTION)0,0,NULL}
};

const SQTClassDecl sqt_basictype_decl = {
	NULL,                   // base_class
    _SC("sqt_basictype"),	// reg_name
    _SC("basictype"),		// name
	NULL,                   // members
	_type_methods,          // methods
	NULL,                   // globals
};

/* ------------------------------------
    Pointer type
------------------------------------ */

typedef struct tagSQTBasicPointerTypeDef {
    SQTBasicTypeDef b;
    const SQTBasicTypeDef *oftype;
} SQTBasicPointerTypeDef;

SQRESULT sqt_getbasicpointer(HSQUIRRELVM v, SQInteger idx, SQTBasicPointerTypeDef **pbasicpointer)
{
    if(SQ_FAILED(sq_getinstanceup(v,idx,(SQUserPointer*)pbasicpointer,SQT_BASICPOINTER_TYPE_TAG)))
        return sq_throwerror(v,_SC("expecting basepointer instance"));
    return SQ_OK;
}

static HSQMEMBERHANDLE _pointer_type_oftype;

static void sq_pushbasic_pointer( const SQTBasicTypeDef *bt, HSQUIRRELVM v,const SQUserPointer p)
{
    SQUserPointer val = (SQUserPointer)*(const void**)p;
	sq_pushuserpointer(v,val);
}
static SQRESULT sq_getbasic_pointer( const SQTBasicTypeDef *bt, HSQUIRRELVM v, SQInteger idx, SQUserPointer p)
{
    SQUserPointer val;
    SQUserPointer basicvalue;
	SQObjectType ot = sq_gettype(v,idx);
	if( ot == OT_USERPOINTER) {
		sq_getuserpointer(v,idx,&val);
	}
	else if( ot == OT_USERDATA) {
		sq_getuserdata(v,idx,&val,NULL);
	}
	else if( ot == OT_NULL) {
		val = NULL;
	}
    else if( (ot == OT_INSTANCE) && (SQ_SUCCEEDED(sq_getinstanceup(v,idx,(SQUserPointer*)&basicvalue,SQT_BASICVALUE_TYPE_TAG)))) {
        const SQTBasicTypeDef *type;
        if(SQ_FAILED(sqt_basicvalue_get(v,idx, &type, val)))
            return SQ_ERROR;
    }
	else return sq_throwerror(v,_SC("expecting userpointer, userdata, basicvalue or null"));
    *(void**)p = (void*)val;
    return SQ_OK;
}
const SQTBasicTypeDef SQ_Basic_pointer = {
    sq_pushbasic_pointer,
    sq_getbasic_pointer,
    sizeof(void*),
    offsetof( struct { uint8_t b; void *me; }, me),
    _SC("pointer"),
};

static SQInteger __basetype_pointer_releasehook(SQUserPointer p, SQInteger SQ_UNUSED_ARG(size))
{
    sq_free( p, sizeof(SQTBasicPointerTypeDef));
    return 1;
}

static SQInteger _pointer_constructor(HSQUIRRELVM v)
{
    const SQTBasicTypeDef *oftype;
    SQTBasicPointerTypeDef *self;
    if(SQ_FAILED(sqt_getbasictype(v,2,&oftype))) return SQ_ERROR;
    self = sq_malloc( sizeof(SQTBasicPointerTypeDef));
    self->b = SQ_Basic_pointer;
    self->oftype = oftype;
	if(SQ_FAILED(sq_setinstanceup(v,1,self))) {
		return sq_throwerror(v, _SC("cannot create basic pointer instance"));
	}
	sq_setreleasehook(v,1,__basetype_pointer_releasehook);
	sq_push(v,2);
	sq_setbyhandle(v,1,&_pointer_type_oftype);
	return 0;
}

static SQInteger _pointer__typeof(HSQUIRRELVM v)
{
    sq_pushstring(v,sqt_basicpointer_decl.name,-1);
    return 1;
}

static SQInteger _pointer_type(HSQUIRRELVM v)
{
    SQTBasicPointerTypeDef *self;
    if(SQ_FAILED(sqt_getbasicpointer(v,1,&self))) return SQ_ERROR;
    sq_getbyhandle(v,1,&_pointer_type_oftype);
    return 1;
}

static SQInteger _pointer_refmember(HSQUIRRELVM v)
{
    // this, userdata/userpointer, offset, index
    const SQTBasicPointerTypeDef *self = NULL;
    const SQTBasicTypeDef *oftype;
    SQUserPointer ptr;
    SQInteger index;
    SQInteger offset;
    SQInteger aroff;
    if(SQ_FAILED(sq_getinstanceup(v,1,(SQUserPointer*)&self,SQT_BASICPOINTER_TYPE_TAG)))
        return sq_throwerror(v,_SC("invalid type tag"));
    sq_getbyhandle(v,1,&_pointer_type_oftype);                  // oftype
    if(SQ_FAILED(sqt_getbasictype(v,-1,&oftype)))
        return SQ_ERROR;
    if(SQ_FAILED(sqt_basicget(&SQ_Basic_pointer,v,2,&ptr))) return SQ_ERROR;
    sq_getinteger(v,3,&offset);
    ptr = (uint8_t*)ptr + offset;
    if(SQ_FAILED(sq_getinteger(v,4,&index)))
        return sq_throwerror(v,_SC("expecting integer index"));
    aroff = sqt_basicgetsize(oftype) * index;
    
    sq_pushregistrytable(v);                    // oftype, registry
    sq_pushstring(v,sqt_basicvalue_decl.reg_name,-1);    // oftype, registry, "basicvalue"
    if(SQ_FAILED(sq_get(v,-2)))                 // oftype, registry, basicvalue
        return SQ_ERROR;
    sq_pushnull(v);                             // oftype, registry, basicvalue, dummy_this
    sq_push(v,-4);  // repush oftype            // oftype, registry, basicvalue, dummy_this, oftype
    sqt_basicpush(&SQ_Basic_pointer,v,ptr);     // oftype, registry, basicvalue, dummy_this, oftype, ptr
    sq_pushinteger(v,aroff);                    // oftype, registry, basicvalue, dummy_this, oftype, ptr, offset
    if(SQ_FAILED(sq_call(v,4,SQTrue,SQFalse)))  // oftype, registry, basicvalue, ret_val
        return SQ_ERROR;
    return 1;
}

static const SQTMemberDecl _pointer_members[] = {
	{_SC("$oftype"), &_pointer_type_oftype },
	{NULL,NULL}
};

#define _DECL_POINTERTYPE_FUNC(name,nparams,typecheck) {_SC(#name),_pointer_##name,nparams,typecheck}
static const SQRegFunction _pointer_methods[] = {
    _DECL_POINTERTYPE_FUNC(constructor,2,_SC("xx")),
    _DECL_POINTERTYPE_FUNC(_typeof,1,_SC("x")),
    _DECL_POINTERTYPE_FUNC(type,1,_SC("x")),
    _DECL_POINTERTYPE_FUNC(refmember,4,_SC("xp|ui.")),
    {NULL,(SQFUNCTION)0,0,NULL}
};

const SQTClassDecl sqt_basicpointer_decl = {
	&sqt_basictype_decl,		// base_class
    _SC("sqt_basicpointer"),	// reg_name
    _SC("basicpointer"),		// name
	_pointer_members,         // members
	_pointer_methods,         // methods
	NULL,                   // globals
};

/* ------------------------------------
    Array type
------------------------------------ */

typedef struct tagSQTBasicArrayTypeDef {
    SQTBasicTypeDef b;
    const SQTBasicTypeDef *oftype;
    SQInteger length;
} SQTBasicArrayTypeDef;

SQRESULT sqt_getbasicarray(HSQUIRRELVM v, SQInteger idx, SQTBasicArrayTypeDef **pbasicarray)
{
    if(SQ_FAILED(sq_getinstanceup(v,idx,(SQUserPointer*)pbasicarray,SQT_BASICARRAY_TYPE_TAG)))
        return sq_throwerror(v,_SC("expecting basearray instance"));
    return SQ_OK;
}

static HSQMEMBERHANDLE _array_type_oftype;

static void sq_pushbasic_array(const SQTBasicTypeDef *bt, HSQUIRRELVM v, SQUserPointer p)
{
    const SQTBasicArrayTypeDef *self = (const SQTBasicArrayTypeDef*)bt;
    SQInteger elsize = sqt_basicgetsize( self->oftype);
    SQInteger idx = 0;
    sq_newarray(v,self->length);            // array
    while( idx < self->length) {
        sq_pushinteger(v,idx);              // array, index
        sqt_basicpush( self->oftype, v, p); // array, index, value
        sq_set(v,-3);                       // array
        p = (uint8_t*)p + elsize;
        idx++;
    }
}

static SQRESULT sq_getbasic_array(const SQTBasicTypeDef *bt, HSQUIRRELVM v, SQInteger idx, SQUserPointer p)
{
    const SQTBasicArrayTypeDef *self = (const SQTBasicArrayTypeDef*)bt;
    SQInteger length;
    SQInteger elsize;
    idx = (idx > 0) ? idx : sq_gettop(v) + idx + 1;
    if(sq_gettype(v,idx) != OT_ARRAY) return sq_throwerror(v,_SC("expecting array"));
    length = sq_getsize(v,idx);
    if(length != self->length) return sq_throwerror(v,_SC("array size don't match"));
    elsize = sqt_basicgetsize( self->oftype);
    sq_pushnull(v);
    while(SQ_SUCCEEDED(sq_next(v,idx))) {
        if(SQ_FAILED(sqt_basicget( self->oftype, v, -1, p))) return SQ_ERROR;
        p = (uint8_t*)p + elsize;
        sq_pop(v,2);
    }
    sq_poptop(v);
    return SQ_OK;
}

const SQTBasicTypeDef SQ_Basic_array_init = {
    sq_pushbasic_array,
    sq_getbasic_array,
    0, // size
    1, // align
    _SC("array"),
};

static SQInteger __basetype_array_releasehook(SQUserPointer p, SQInteger SQ_UNUSED_ARG(size))
{
    sq_free( p, sizeof(SQTBasicArrayTypeDef));
    return 1;
}

static SQInteger _array_constructor(HSQUIRRELVM v)
{
    // this, oftype, length
    const SQTBasicTypeDef *oftype;
    SQTBasicArrayTypeDef *self;
    SQInteger length;
    if(SQ_FAILED(sqt_getbasictype(v,2,&oftype))) return SQ_ERROR;
    sq_getinteger(v,3,&length);
    if( length <= 0)
        return sq_throwerror(v,_SC("array length must be non-zero and pozitive"));
    self = sq_malloc( sizeof(SQTBasicArrayTypeDef));
    self->b = SQ_Basic_array_init;
    self->b.size = length * sqt_basicgetsize(oftype);
    self->b.align = sqt_basicgetalign(oftype);
    self->oftype = oftype;
    self->length = length;
//    self = sq_malloc( sizeof(SQTBasicTypeDef));
//    *self = SQ_Basic_array_init;
//    self->size = length * sqt_basicgetsize(oftype);
//    self->align = sqt_basicgetalign(oftype);
	if(SQ_FAILED(sq_setinstanceup(v,1,self))) {
		return sq_throwerror(v, _SC("cannot create basic array instance"));
	}
	sq_setreleasehook(v,1,__basetype_array_releasehook);
	sq_push(v,2);
	sq_setbyhandle(v,1,&_array_type_oftype);
	return 0;
}

static SQInteger _array__typeof(HSQUIRRELVM v)
{
    sq_pushstring(v,sqt_basicarray_decl.name,-1);
    return 1;
}

static SQInteger _array_type(HSQUIRRELVM v)
{
    SQTBasicArrayTypeDef *self;
    if(SQ_FAILED(sqt_getbasicarray(v,1,&self))) return SQ_ERROR;
    sq_getbyhandle(v,1,&_array_type_oftype);
    return 1;
}

static SQInteger _array_len(HSQUIRRELVM v)
{
    SQTBasicArrayTypeDef *self;
    if(SQ_FAILED(sqt_getbasicarray(v,1,&self))) return SQ_ERROR;
    sq_pushinteger(v,self->length);
    return 1;
}

static SQInteger _array_refmember(HSQUIRRELVM v)
{
    const SQTBasicTypeDef *self = NULL;
    const SQTBasicTypeDef *oftype;
    SQInteger index;
    SQInteger offset;
    SQInteger aroff;
    if(SQ_FAILED(sq_getinstanceup(v,1,(SQUserPointer*)&self,SQT_BASICARRAY_TYPE_TAG)))
        return sq_throwerror(v,_SC("invalid type tag"));
    if(SQ_FAILED(sq_getinteger(v,4,&index)))
        return sq_throwerror(v,_SC("expecting integer index"));
    sq_getbyhandle(v,1,&_array_type_oftype);    // oftype
    if(SQ_FAILED(sqt_getbasictype(v,-1,&oftype)))
        return SQ_ERROR;
    sq_getinteger(v,3,&offset);
    if( (index < 0) || (sqt_basicgetsize(self) < (aroff=(sqt_basicgetsize(oftype) * index))) )
        return sq_throwerror(v,_SC("index out of bounds"));
    offset += aroff;

    sq_pushregistrytable(v);                    // oftype, registry
    sq_pushstring(v,sqt_basicvalue_decl.reg_name,-1);    // oftype, registry, "basicvalue"
    if(SQ_FAILED(sq_get(v,-2)))                 // oftype, registry, basicvalue
        return SQ_ERROR;
    sq_pushnull(v);                             // oftype, registry, basicvalue, dummy_this
    sq_push(v,-4);  // repush oftype            // oftype, registry, basicvalue, dummy_this, oftype
    sq_push(v,2);   // repush ptr               // oftype, registry, basicvalue, dummy_this, oftype, ptr
    sq_pushinteger(v,offset);                   // oftype, registry, basicvalue, dummy_this, oftype, ptr, offset
    if(SQ_FAILED(sq_call(v,4,SQTrue,SQFalse)))  // oftype, registry, basicvalue, ret_val
        return SQ_ERROR;
    return 1;
}

static const SQTMemberDecl _array_members[] = {
	{_SC("$oftype"), &_array_type_oftype },
	{NULL,NULL}
};

#define _DECL_ARRAYTYPE_FUNC(name,nparams,typecheck) {_SC(#name),_array_##name,nparams,typecheck}
static const SQRegFunction _array_methods[] = {
    _DECL_ARRAYTYPE_FUNC(constructor,3,_SC("xxi")),
    _DECL_ARRAYTYPE_FUNC(_typeof,1,_SC("x")),
    _DECL_ARRAYTYPE_FUNC(type,1,_SC("x")),
    _DECL_ARRAYTYPE_FUNC(len,1,_SC("x")),
    _DECL_ARRAYTYPE_FUNC(refmember,4,_SC("xp|ui.")),
    {NULL,(SQFUNCTION)0,0,NULL}
};

const SQTClassDecl sqt_basicarray_decl = {
	&sqt_basictype_decl,		// base_class
    _SC("sqt_basicarray"),	// reg_name
    _SC("basicarray"),		// name
	_array_members,         // members
	_array_methods,         // methods
	NULL,                   // globals
};

/* ------------------------------------
    Basic member
------------------------------------ */

typedef struct tagSQTBasicMemberDef {
    struct tagSQTBasicMemberDef *next;
    const SQTBasicTypeDef *type;
    SQInteger offset;
    const SQChar *name;
} SQTBasicMemberDef;

SQRESULT sqt_getbasicmember(HSQUIRRELVM v, SQInteger idx, SQTBasicMemberDef **pbasicmember)
{
    if(SQ_FAILED(sq_getinstanceup(v,idx,(SQUserPointer*)pbasicmember,SQT_BASICMEMBER_TYPE_TAG)))
        return sq_throwerror(v,_SC("expecting basemember instance"));
    return SQ_OK;
}

static HSQMEMBERHANDLE _member_name_handle;
static HSQMEMBERHANDLE _member_type_handle;

static SQInteger __basetype_member_releasehook(SQUserPointer p, SQInteger SQ_UNUSED_ARG(size))
{
    sq_free( p, sizeof(SQTBasicMemberDef));
    return 1;
}

static SQInteger _member_constructor(HSQUIRRELVM v)
{
    const SQTBasicTypeDef *bt;
    SQTBasicMemberDef *self;
    if(SQ_FAILED(sqt_getbasictype(v,3,&bt))) return SQ_ERROR;
    self = sq_malloc( sizeof(SQTBasicMemberDef));
    self->next = NULL;
    self->type = bt;
    self->offset = -1;
    sq_getstring(v,2,&self->name);
	if(SQ_FAILED(sq_setinstanceup(v,1,self))) {
		return sq_throwerror(v, _SC("cannot create basic member instance"));
	}
	sq_setreleasehook(v,1,__basetype_member_releasehook);
    sq_setbyhandle(v,1,&_member_type_handle);
    sq_setbyhandle(v,1,&_member_name_handle);
	return 0;
}

static SQInteger _member__typeof(HSQUIRRELVM v)
{
    sq_pushstring(v,sqt_basicmember_decl.name,-1);
    return 1;
}

static SQInteger _member_type(HSQUIRRELVM v)
{
    SQTBasicMemberDef *self;
    if(SQ_FAILED(sqt_getbasicmember(v,1,&self))) return SQ_ERROR;
    sq_getbyhandle(v,1,&_member_type_handle);
    return 1;
}

static SQInteger _member_name(HSQUIRRELVM v)
{
    SQTBasicMemberDef *self;
    if(SQ_FAILED(sqt_getbasicmember(v,1,&self))) return SQ_ERROR;
    sq_getbyhandle(v,1,&_member_name_handle);
    return 1;
}

static SQInteger _member_offset(HSQUIRRELVM v)
{
    SQTBasicMemberDef *self;
    if(SQ_FAILED(sqt_getbasicmember(v,1,&self))) return SQ_ERROR;
    sq_pushinteger(v,self->offset);
    return 1;
}

static const SQTMemberDecl _member_members[] = {
	{_SC("$name"), &_member_name_handle },
	{_SC("$type"), &_member_type_handle },
	{NULL,NULL}
};

#define _DECL_BASICMEMBER_FUNC(name,nparams,typecheck) {_SC(#name),_member_##name,nparams,typecheck}
static const SQRegFunction _member_methods[] = {
    _DECL_BASICMEMBER_FUNC(constructor,3,_SC("xsx")),
    _DECL_BASICMEMBER_FUNC(_typeof,1,_SC("x")),
    _DECL_BASICMEMBER_FUNC(type,1,_SC("x")),
    _DECL_BASICMEMBER_FUNC(name,1,_SC("x")),
    _DECL_BASICMEMBER_FUNC(offset,1,_SC("x")),
    {NULL,(SQFUNCTION)0,0,NULL}
};

const SQTClassDecl sqt_basicmember_decl = {
	NULL,				// base_class
    _SC("sqt_basicmember"),	// reg_name
    _SC("basicmember"),		// name
	_member_members,        // members
	_member_methods,        // methods
	NULL,               // globals
};

/* ------------------------------------
    Basic struct
------------------------------------ */

typedef struct tagSQTBasicStructTypeDef {
    SQTBasicTypeDef b;
    SQTBasicMemberDef *members;
    const SQChar *name;
} SQTBasicStructTypeDef;

SQRESULT sqt_getbasicstruct(HSQUIRRELVM v, SQInteger idx, SQTBasicStructTypeDef **pbasicstruct)
{
    if(SQ_FAILED(sq_getinstanceup(v,idx,(SQUserPointer*)pbasicstruct,SQT_BASICSTRUCT_TYPE_TAG)))
        return sq_throwerror(v,_SC("expecting basestruct instance"));
    return SQ_OK;
}

static void sq_pushbasic_struct(const SQTBasicTypeDef *bt, HSQUIRRELVM v, SQUserPointer p)
{
    const SQTBasicStructTypeDef *self = (const SQTBasicStructTypeDef*)bt;
    SQInteger len = 0;
    SQTBasicMemberDef *memb = self->members;
    while( memb) {
        len++;
        memb = memb->next;
    }
    sq_newtableex(v,len);                                             // table
    memb = self->members;
    while( memb) {
        sq_pushstring(v,memb->name,-1);                             // table, index
        sqt_basicpush( memb->type, v, (uint8_t*)p + memb->offset);  // table, index, value
        sq_newslot(v,-3,SQFalse);                                   // table
        memb = memb->next;
    }
}

static SQRESULT sq_getbasic_struct(const SQTBasicTypeDef *bt, HSQUIRRELVM v, SQInteger idx, SQUserPointer p)
{
    const SQTBasicStructTypeDef *self = (const SQTBasicStructTypeDef*)bt;
    SQTBasicMemberDef *memb = self->members;
    idx = (idx > 0) ? idx : sq_gettop(v) + idx + 1;
    if(sq_gettype(v,idx) != OT_TABLE) return sq_throwerror(v,_SC("expecting table"));
    while( memb) {
        sq_pushstring(v,memb->name,-1);     // name
        if(SQ_SUCCEEDED(sq_get(v,idx))) {   // value
            if(SQ_FAILED(sqt_basicget( memb->type, v, -1, (uint8_t*)p + memb->offset))) return SQ_ERROR;
            sq_poptop(v);
        }
        else {
            // nothing
        }
        memb = memb->next;
    }
    return SQ_OK;
}

const SQTBasicTypeDef SQ_Basic_struct_init = {
    sq_pushbasic_struct,
    sq_getbasic_struct,
    0, // size
    1, // align
    _SC("struct"),
};

static HSQMEMBERHANDLE _struct_members_handle;
static HSQMEMBERHANDLE _struct_name_handle;

static SQInteger __basetype_struct_releasehook(SQUserPointer p, SQInteger SQ_UNUSED_ARG(size))
{
    sq_free( p, sizeof(SQTBasicStructTypeDef));
    return 1;
}

static SQInteger _struct_setmembers(HSQUIRRELVM v)
{
    SQTBasicStructTypeDef *self;
    SQTBasicMemberDef **plist;
    SQInteger offset;
    if(SQ_FAILED(sqt_getbasicstruct(v,1,&self))) return SQ_ERROR;
    if( self->members) return sq_throwerror(v,_SC("struct already have members"));
    
    plist = &self->members;
    offset = 0;
                                                // inst, array
    sq_pushnull(v);                             // inst, array, null-iter
    while(SQ_SUCCEEDED(sq_next(v,2))) {         // inst, array, iter, key, val
        SQTBasicMemberDef *memb;
        if(SQ_FAILED(sqt_getbasicmember(v, -1, &memb))) return SQ_ERROR;
        if( memb->offset != -1) return sq_throwerror(v,_SC("basicmember can't be reused"));
        offset = sqt_basicalign( sqt_basicgetalign( memb->type), offset);
        memb->offset = offset;
        offset += sqt_basicgetsize( memb->type);
        *plist = memb;
        plist = &memb->next;
        sq_pop(v,2);                            // inst, array, iter
    }
    sq_poptop(v);                               // inst, array
    
    if( self->members) {
        self->b.align = sqt_basicgetalign( self->members->type);
        self->b.size = sqt_basicalign( self->b.align, offset);
        sq_setbyhandle(v,1,&_struct_members_handle);   // inst
    }
    return 0;
}

static SQInteger _struct_constructor(HSQUIRRELVM v)
{
    SQTBasicStructTypeDef *self;
    SQInteger top = sq_gettop(v);
    self = sq_malloc( sizeof(SQTBasicStructTypeDef));
    self->b = SQ_Basic_struct_init;
    self->members = NULL;
    sq_getstring(v,2,&self->name);
	if(SQ_FAILED(sq_setinstanceup(v,1,self))) {
		return sq_throwerror(v, _SC("cannot create basic struct instance"));
	}
	sq_setreleasehook(v,1,__basetype_struct_releasehook);

    if( top > 2) {
        sq_settop(v,3);
                                                    // inst, name, array
        sq_push(v,2); // repush name                // inst, name, array, name
        sq_setbyhandle(v,1,&_struct_name_handle);   // inst, name, array
        sq_remove(v,2);                             // inst, array
        
        return _struct_setmembers(v);
    }
    else {
                                                    // inst, name
        sq_setbyhandle(v,1,&_struct_name_handle);   // inst
    }
	return 0;
}

static SQInteger _struct_refmember(HSQUIRRELVM v)
{
    SQTBasicStructTypeDef *self = NULL;
    const SQChar *index;
    SQTBasicMemberDef *memb = NULL;
    SQInteger offset;
    if(SQ_FAILED(sqt_getbasicstruct(v,1,&self))) return SQ_ERROR;
    sq_getinteger(v,3,&offset);
    if(SQ_FAILED(sq_getstring(v,4,&index)))
        return sq_throwerror(v,_SC("expecting string index"));

    sq_getbyhandle(v,1,&_struct_members_handle);    // members
    sq_pushnull(v);                                 // members, null-iter
    while(SQ_SUCCEEDED(sq_next(v,-2))) {             // members, iter, key, memb
        if(SQ_FAILED(sqt_getbasicmember(v, -1, &memb))) return SQ_ERROR;
        if( memb->name == index) {
            sq_remove(v,-2);                        // members, iter, memb
            sq_remove(v,-2);                        // members, memb
            sq_remove(v,-2);                        // memb
            break;
        }
        memb = NULL;
        sq_pop(v,2);                                // members, iter
    }
    if( memb) {
        offset += memb->offset;
        sq_getbyhandle(v,-1,&_member_type_handle);  // memb, mtype
        sq_pushregistrytable(v);                    // memb, mtype, registry
        sq_pushstring(v,sqt_basicvalue_decl.reg_name,-1);    // memb, mtype, registry, "basicvalue"
        if(SQ_FAILED(sq_get(v,-2)))                 // memb, mtype, registry, basicvalue
            return SQ_ERROR;
        sq_pushnull(v);                             // memb, mtype, registry, basicvalue, dummy_this
        sq_push(v,-4);  // repush mtype             // memb, mtype, registry, basicvalue, dummy_this, oftype
        sq_push(v,2);   // repush ptr               // memb, mtype, registry, basicvalue, dummy_this, oftype, ptr
        sq_pushinteger(v,offset);                   // memb, mtype, registry, basicvalue, dummy_this, oftype, ptr, offset
        if(SQ_FAILED(sq_call(v,4,SQTrue,SQFalse)))  // memb, mtype, registry, basicvalue, ret_val
            return SQ_ERROR;
        return 1;
    }
    else {
        sq_poptop(v);                               // members
        return sq_throwerror(v,_SC("unknown member"));
    }
}

static SQInteger _struct__typeof(HSQUIRRELVM v)
{
    sq_pushstring(v,sqt_basicstruct_decl.name,-1);
    return 1;
}

static SQInteger _struct_len(HSQUIRRELVM v)
{
    SQTBasicStructTypeDef *self = NULL;
    SQInteger len = 0;
    SQTBasicMemberDef *memb = NULL;
    if(SQ_FAILED(sqt_getbasicstruct(v,1,&self))) return SQ_ERROR;

    memb = self->members;
    while( memb) {
        len++;
        memb = memb->next;
    }    
    sq_pushinteger(v,len);
    return 1;
}

static SQInteger _struct_name(HSQUIRRELVM v)
{
    SQTBasicStructTypeDef *self = NULL;
    if(SQ_FAILED(sqt_getbasicstruct(v,1,&self))) return SQ_ERROR;
    sq_getbyhandle(v,1,&_struct_name_handle);
    return 1;
}

static const SQTMemberDecl _struct_members[] = {
	{_SC("$members"), &_struct_members_handle },
	{_SC("$name"), &_struct_name_handle },
	{NULL,NULL}
};

#define _DECL_BASICSTRUCT_FUNC(name,nparams,typecheck) {_SC(#name),_struct_##name,nparams,typecheck}
static const SQRegFunction _struct_methods[] = {
    _DECL_BASICSTRUCT_FUNC(constructor,-2,_SC("xsa")),
    _DECL_BASICSTRUCT_FUNC(setmembers,2,_SC("xa")),
    _DECL_BASICSTRUCT_FUNC(refmember,4,_SC("xp|ui.")),
    _DECL_BASICSTRUCT_FUNC(_typeof,1,_SC("x")),
    _DECL_BASICSTRUCT_FUNC(len,1,_SC("x")),
    _DECL_BASICSTRUCT_FUNC(name,1,_SC("x")),
    {NULL,(SQFUNCTION)0,0,NULL}
};

const SQTClassDecl sqt_basicstruct_decl = {
	&sqt_basictype_decl,    // base_class
    _SC("sqt_basicstruct"),	// reg_name
    _SC("basicstruct"),		// name
	_struct_members,        // members
	_struct_methods,        // methods
	NULL,               // globals
};

// /* ------------------------------------
//     Function type
// ------------------------------------ */
//
// typedef struct tagSQTBasicFunctionTypeDef {
//     SQTBasicTypeDef b;
//     const SQTBasicTypeDef *rettype;
//     SQTBasicMemberDef *args;
// } SQTBasicFunctionTypeDef;
//
// SQRESULT sqt_getbasicfunction(HSQUIRRELVM v, SQInteger idx, SQTBasicFunctionTypeDef **pbasicfunct)
// {
//     if(SQ_FAILED(sq_getinstanceup(v,idx,(SQUserPointer*)pbasicfunct,SQT_BASICFUNCTION_TYPE_TAG)))
//         return sq_throwerror(v,_SC("expecting basearray instance"));
//     return SQ_OK;
// }
//
// static void sq_pushbasic_function(const SQTBasicTypeDef *bt, HSQUIRRELVM v, SQUserPointer p)
// {
//     sq_throwerror(v,_SC("can't push C function"));
// }
//
// static SQRESULT sq_getbasic_function(const SQTBasicTypeDef *bt, HSQUIRRELVM v, SQInteger idx, SQUserPointer p)
// {
//     return sq_throwerror(v,_SC("can't get C function"));
// }
//
// const SQTBasicTypeDef SQ_Basic_struct_init = {
//     sq_pushbasic_function,
//     sq_getbasic_function,
//     0, // size
//     1, // align
//     _SC("function"),
// };
//
// static HSQMEMBERHANDLE _struct_rettype_handle;
// static HSQMEMBERHANDLE _struct_args_handle;
//
// static SQInteger __basetype_function_releasehook(SQUserPointer p, SQInteger SQ_UNUSED_ARG(size))
// {
//     sq_free( p, sizeof(SQTBasicFunctionTypeDef));
//     return 1;
// }
//
// static SQInteger _function_setargs(HSQUIRRELVM v)
// {
//     SQTBasicFunctionTypeDef *self;
//     SQTBasicMemberDef **plist;
//     SQInteger offset;
//     if(SQ_FAILED(sqt_getbasicfunction(v,1,&self))) return SQ_ERROR;
//     if( self->args) return sq_throwerror(v,_SC("function already have arguments"));
//     
//     plist = &self->args;
//     offset = 0;
//                                                 // inst, array
//     sq_pushnull(v);                             // inst, array, null-iter
//     while(SQ_SUCCEEDED(sq_next(v,2))) {         // inst, array, iter, key, val
//         SQTBasicMemberDef *memb;
//         if(SQ_FAILED(sqt_getbasicmember(v, -1, &memb))) return SQ_ERROR;
//         if( memb->offset != -1) return sq_throwerror(v,_SC("basicmember can't be reused"));
//         offset = sqt_basicalign( sqt_basicgetalign( memb->type), offset);
//         memb->offset = offset;
//         offset += sqt_basicgetsize( memb->type);
//         *plist = memb;
//         plist = &memb->next;
//         sq_pop(v,2);                            // inst, array, iter
//     }
//     sq_poptop(v);                               // inst, array
//     
//     if( self->members) {
//         self->b.align = sqt_basicgetalign( self->members->type);
//         self->b.size = sqt_basicalign( self->b.align, offset);
//         sq_setbyhandle(v,1,&_struct_members_handle);   // inst
//     }
//     return 0;
// }

/* ------------------------------------
    Basic value
------------------------------------ */

static HSQMEMBERHANDLE _value_type_handle;
static HSQMEMBERHANDLE _value_ptr_handle;
static HSQMEMBERHANDLE _value_offset_handle;

SQInteger sqt_basicvalue_get( HSQUIRRELVM v, SQInteger idx, const SQTBasicTypeDef **ptype, SQUserPointer *pptr)
{
    SQUserPointer ptr;
    SQInteger offset;
    SQObjectType ot;
    sq_getbyhandle(v,idx,&_value_type_handle);
    if(SQ_FAILED(sqt_getbasictype(v,-1,ptype)))
        return SQ_ERROR;
    sq_poptop(v);
    sq_getbyhandle(v,idx,&_value_ptr_handle);
    ot = sq_gettype(v,-1);
    if( ot == OT_USERDATA) {
        if(SQ_FAILED(sq_getuserdata(v,-1,&ptr,NULL)))
            return SQ_ERROR;
    }
    else if( ot == OT_USERPOINTER) {
        if(SQ_FAILED(sq_getuserpointer(v,-1,&ptr)))
            return SQ_ERROR;
    }
    else return sq_throwerror(v,_SC("bad basic value"));
    sq_poptop(v);
    sq_getbyhandle(v,idx,&_value_offset_handle);
    if(SQ_FAILED(sq_getinteger(v,-1,&offset)))
        return SQ_ERROR;
    sq_poptop(v);
    *pptr = (SQUserPointer)( (uint8_t*)ptr + offset);
    return SQ_OK;
}

static SQInteger _value_constructor(HSQUIRRELVM v)
{
    const SQTBasicTypeDef *bt;
    SQInteger top = sq_gettop(v);
    if(SQ_FAILED(sqt_getbasictype(v,2,&bt))) return SQ_ERROR;
    if( top < 3) {
        sq_newuserdata(v,bt->size);
    }
    if( top < 4) {
        sq_pushinteger(v,0);
    }
    sq_setbyhandle(v,1,&_value_offset_handle);
    sq_setbyhandle(v,1,&_value_ptr_handle);
    sq_setbyhandle(v,1,&_value_type_handle);
	return 0;
}

static SQInteger _value__typeof(HSQUIRRELVM v)
{
    sq_pushstring(v,sqt_basicvalue_decl.name,-1);
    return 1;
}

static SQInteger _value_get(HSQUIRRELVM v)
{
    // value
    const SQTBasicTypeDef *type;
    SQUserPointer ptr;
    if(SQ_FAILED(sqt_basicvalue_get(v,1,&type,&ptr)))
        return SQ_ERROR;
    sqt_basicpush( type, v, ptr);
    return 1;
}

static SQInteger _value_getref(HSQUIRRELVM v)
{
    // value, index
    sq_getbyhandle(v,1,&_value_type_handle);    // type
    sq_pushstring(v,"refmember",-1);            // type, "refmember"
    if(SQ_FAILED(sq_get(v,-2)))                 // type, refmember
        return SQ_ERROR;
    sq_push(v,-2); // repush type               // type, refmember, type
    sq_getbyhandle(v,1,&_value_ptr_handle);     // type, refmember, type, ptr
    sq_getbyhandle(v,1,&_value_offset_handle);  // type, refmember, type, ptr, offset
    sq_push(v,2);                               // type, refmember, type, ptr, offset, index
    if(SQ_FAILED(sq_call(v,4,SQTrue,SQFalse)))          // type, refmember, ret_value
        return SQ_ERROR;
    return 1;
}

static SQInteger _value__get(HSQUIRRELVM v)
{
    // value, index
    if(SQ_FAILED(_value_getref(v)))
        return SQ_ERROR;

    // dereference base types
    sq_getbyhandle(v,-1,&_value_type_handle);   // type, refmember, ret_value, ret_type
    const SQTBasicTypeDef *reftype;
    if(SQ_FAILED(sqt_getbasictype(v,-1,&reftype)))
        return SQ_ERROR;
    sq_poptop(v);                               // type, refmember, ret_value
    if( 0) {                // !!! missing basictype identification !!!
        HSQOBJECT hval;
        sq_resetobject(&hval);
        sq_getstackobj(v,-2,&hval);
        sq_addref(v,&hval);
        sq_settop(v,0);
        sq_pushobject(v,hval);
        sq_release(v,&hval);
        return _value_get(v);
    }
    return 1;
}

static SQInteger _value_set(HSQUIRRELVM v)
{
    // value, new_value
    SQUserPointer dummy;
    if(SQ_SUCCEEDED(sq_getinstanceup(v,2,(SQUserPointer*)&dummy,SQT_BASICVALUE_TYPE_TAG))) {
        const SQTBasicTypeDef *dst_type, *src_type;
        SQUserPointer dst_ptr, src_ptr;
        if(SQ_FAILED(sqt_basicvalue_get(v,1,&dst_type,&dst_ptr)))
            return SQ_ERROR;
        if(SQ_FAILED(sqt_basicvalue_get(v,2,&src_type,&src_ptr)))
            return SQ_ERROR;

        if( sqt_basicgetsize(dst_type) == sqt_basicgetsize(src_type)) {
            memcpy( dst_ptr, src_ptr, sqt_basicgetsize(dst_type));
        }
        else
            return sq_throwerror(v,_SC("incompatible basic types"));
    }
    else {
        const SQTBasicTypeDef *type;
        SQUserPointer ptr;
        if(SQ_FAILED(sqt_basicvalue_get(v,1,&type,&ptr)))
            return SQ_ERROR;
        if(SQ_FAILED(sqt_basicget( type, v, 2, ptr)))
            return SQ_ERROR;
    }
    return 0;
}

static SQInteger _value__set(HSQUIRRELVM v)
{
    // value, index, new_value
    HSQOBJECT hnewval;
    sq_resetobject(&hnewval);
    sq_getstackobj(v,3,&hnewval);
    sq_addref(v,&hnewval);
    sq_poptop(v);                       // value, index
    if(SQ_FAILED(_value_getref(v))) {   // ... ret_value
        sq_release(v,&hnewval);
        return SQ_ERROR;
    }
    HSQOBJECT hretval;
    sq_resetobject(&hretval);
    sq_getstackobj(v,-1,&hretval);
    sq_addref(v,&hretval);
    sq_settop(v,0);                       //
    sq_pushobject(v,hretval);          // ret_val
    sq_release(v,&hretval);
    sq_pushobject(v,hnewval);          // ret_val, new_val
    sq_release(v,&hnewval);
    return _value_set(v);
}

static SQInteger _value_clear(HSQUIRRELVM v)
{
    const SQTBasicTypeDef *type;
    SQUserPointer ptr;
    if(SQ_FAILED(sqt_basicvalue_get(v,1,&type,&ptr)))
        return SQ_ERROR;
    memset( ptr, 0, sqt_basicgetsize(type));
    return 1;
}

static SQInteger _value__cloned(HSQUIRRELVM v)
{
    const SQTBasicTypeDef *type;
    SQUserPointer srcptr, dstptr;
    SQInteger size;
    if(SQ_FAILED(sqt_basicvalue_get(v,1,&type,&srcptr)))
        return SQ_ERROR;
    size = sqt_basicgetsize(type);
    dstptr = sq_newuserdata(v,size);
    memcpy(dstptr,srcptr,size);
    sq_setbyhandle(v,1,&_value_ptr_handle);
    return 0;
}

static SQInteger _value_address(HSQUIRRELVM v)
{
    const SQTBasicTypeDef *type;
    SQUserPointer ptr;
    if(SQ_FAILED(sqt_basicvalue_get(v,1,&type,&ptr)))
        return SQ_ERROR;
    sq_pushuserpointer(v,ptr);
    return 1;
}

static const SQTMemberDecl _value_members[] = {
	{_SC("$type"), &_value_type_handle },
	{_SC("$ptr"), &_value_ptr_handle },
	{_SC("$offset"), &_value_offset_handle },
	{NULL,NULL}
};

#define _DECL_BASICVALUE_FUNC(name,nparams,typecheck) {_SC(#name),_value_##name,nparams,typecheck}
static const SQRegFunction _value_methods[] = {
    _DECL_BASICVALUE_FUNC(constructor,-2,_SC("xxp|ui")),
    _DECL_BASICVALUE_FUNC(_typeof,1,_SC("x")),
	_DECL_BASICVALUE_FUNC(set,2,_SC("x.")),
	_DECL_BASICVALUE_FUNC(_set,3,_SC("x..")),
    _DECL_BASICVALUE_FUNC(get,1,_SC("x")),
    _DECL_BASICVALUE_FUNC(getref,2,_SC("x.")),
    _DECL_BASICVALUE_FUNC(_get,2,_SC("x.")),
	_DECL_BASICVALUE_FUNC(clear,1,_SC("x")),
	_DECL_BASICVALUE_FUNC(_cloned,2,_SC("xx")),
	_DECL_BASICVALUE_FUNC(address,1,_SC("x")),
    {NULL,(SQFUNCTION)0,0,NULL}
};

const SQTClassDecl sqt_basicvalue_decl = {
	NULL,				// base_class
    _SC("sqt_basicvalue"),	// reg_name
    _SC("basicvalue"),		// name
	_value_members,        // members
	_value_methods,        // methods
	NULL,               // globals
};

/* ------------------------------------
------------------------------------ */

static void _register_basetype( HSQUIRRELVM v, const SQTBasicTypeDef *bt, const SQChar *name)
{
                                                // table, basetype_class
    sq_pushnull(v); // fake this                // table, basetype_class, null
    sq_pushuserpointer(v,(SQUserPointer)bt);    // table, basetype_class, null, type_ptr
    sq_call(v,2,SQTrue,SQFalse);                // table, basetype_class, type_inst
    sq_pushstring(v,name,-1);                   // table, basetype_class, type_inst, type_name
    sq_push(v,-2); // repush type_inst          // table, basetype_class, type_inst, type_name, type_inst
    sq_newslot(v,-5,SQFalse);                   // table, basetype_class, type_inst
    sq_poptop(v);                               // table, basetype_class
}

#define REG_TYPE( _name)                 _register_basetype(v, &SQ_Basic_##_name, SQ_Basic_##_name.name)
#define REG_TYPE_ALIAS( _name, _regname) _register_basetype(v, &SQ_Basic_##_name, _SC(#_regname))

SQUIRREL_API SQRESULT sqstd_register_basictypes(HSQUIRRELVM v)
{
    sq_pushstring(v,_SC("ctypes"),-1);
    sq_newtable(v);

	if(SQ_FAILED(sqt_declareclass(v,&sqt_basictype_decl))) {
		return SQ_ERROR;
	}

    REG_TYPE( uint8_t);
    REG_TYPE( int8_t);
    REG_TYPE( uint16_t);
    REG_TYPE( int16_t);
    REG_TYPE( uint32_t);
    REG_TYPE( int32_t);
#ifdef _SQ64
    REG_TYPE( uint64_t);
    REG_TYPE( int64_t);
    REG_TYPE_ALIAS( int64_t, SQInteger);
#else // _SQ64
    REG_TYPE_ALIAS( int32_t, SQInteger);
#endif // _SQ64
	REG_TYPE( void);
//	REG_TYPE( voidptr);
	
    REG_TYPE( float);
    REG_TYPE( double);
#ifdef SQUSEDOUBLE
    REG_TYPE_ALIAS( double, SQFloat);
#else // SQUSEDOUBLE
    REG_TYPE_ALIAS( float, SQFloat);
#endif // SQUSEDOUBLE

 	sq_poptop(v);

	if(SQ_FAILED(sqt_declareclass(v,&sqt_basicpointer_decl))) {
		return SQ_ERROR;
	}
 	sq_poptop(v);
    
	if(SQ_FAILED(sqt_declareclass(v,&sqt_basicarray_decl))) {
		return SQ_ERROR;
	}
 	sq_poptop(v);
    
	if(SQ_FAILED(sqt_declareclass(v,&sqt_basicmember_decl))) {
		return SQ_ERROR;
	}
 	sq_poptop(v);

	if(SQ_FAILED(sqt_declareclass(v,&sqt_basicstruct_decl))) {
		return SQ_ERROR;
	}
 	sq_poptop(v);

	if(SQ_FAILED(sqt_declareclass(v,&sqt_basicvalue_decl))) {
		return SQ_ERROR;
	}
 	sq_poptop(v);

    sq_newslot(v,-3,SQFalse);

	return SQ_OK;
}
