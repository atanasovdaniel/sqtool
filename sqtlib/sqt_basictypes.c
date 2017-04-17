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
extern SQUIRREL_API_VAR const SQTClassDecl sqt_basicarray_decl;
#define SQT_BASICARRAY_TYPE_TAG ((SQUserPointer)(SQHash)&sqt_basicarray_decl)

typedef void (*sqt_basicpush_fct)(const SQTBasicTypeDef *bt, HSQUIRRELVM v,const SQUserPointer p);
typedef SQRESULT (*sqt_basicget_fct)(const SQTBasicTypeDef *bt, HSQUIRRELVM v, SQInteger idx, SQUserPointer p);
//typedef SQRESULT (*sqt_basicrefmember_fct)(const SQTBasicTypeDef *bt, HSQUIRRELVM v, SQInteger idx);

struct tagSQTBasicTypeDef
{
    sqt_basicpush_fct push;
    sqt_basicget_fct get;
//    const sqt_basicrefmember_fct refmember;
    SQInteger size;
    SQInteger align;
    const SQChar *name;
};

SQInteger sqt_basicsize( const SQTBasicTypeDef *bt)
{
    return bt->size;
}
SQInteger sqt_basicalign( const SQTBasicTypeDef *bt)
{
    return bt->align;
}
void sqt_basicpush( const SQTBasicTypeDef *bt, HSQUIRRELVM v, const SQUserPointer p)
{
    bt->push( bt, v, p);
}
SQRESULT sqt_basicget( const SQTBasicTypeDef *bt, HSQUIRRELVM v, SQInteger idx, SQUserPointer p)
{
    return bt->get( bt, v, idx, p);
}
/*
SQRESULT sqt_basicrefmember( const SQTBasicTypeDef *bt, HSQUIRRELVM v)
{
    return bt->refmember ? bt->refmember( bt, v) : sq_throwerror(v, _SC("basictype don't have members"));
}
*/
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

/*
static SQInteger _basic_refmember(HSQUIRRELVM v)
{
    sq_pushstring(v,sqt_basictype_decl.name,-1);
    return 1;
}
*/

#define _DECL_BASICTYPE_FUNC(name,nparams,typecheck) {_SC(#name),_basic_##name,nparams,typecheck}
static const SQRegFunction _cv_methods[] = {
    _DECL_BASICTYPE_FUNC(constructor,2,_SC("xp")),
    _DECL_BASICTYPE_FUNC(_typeof,1,_SC("x")),
//    _DECL_BASICTYPE_FUNC(refmember,1,_SC("xp|ui.")), // type, ptr, offset, index
    {NULL,(SQFUNCTION)0,0,NULL}
};

const SQTClassDecl sqt_basictype_decl = {
	NULL,				// base_class
    _SC("sqt_basictype"),	// reg_name
    _SC("basictype"),		// name
	NULL,               // members
	_cv_methods,        // methods
	NULL,               // globals
};

/* ------------------------------------
    Array type
------------------------------------ */

/*
typedef struct tagSQTBasicArrayTypeDef {
    SQTBasicTypeDef b;
    const SQTBasicTypeDef *oftype;
    SQInteger length;
} SQTBasicArrayTypeDef;

#define SETUP_ARRAY(v) \
    SQTBasicArrayTypeDef *self = NULL; \
    if(SQ_FAILED(sq_getinstanceup(v,1,(SQUserPointer*)&self,(SQUserPointer)SQT_BASICARRAY_TYPE_TAG))) \
        return sq_throwerror(v,_SC("invalid type tag"));
*/

static HSQMEMBERHANDLE _array_type_oftype;

static void sq_pushbasic_array(const SQTBasicTypeDef *bt, HSQUIRRELVM v,const SQUserPointer p)
{
}

static SQRESULT sq_getbasic_array(const SQTBasicTypeDef *bt, HSQUIRRELVM v, SQInteger idx, SQUserPointer p)
{
    return SQ_ERROR;
}

/*
static SQRESULT sq_refmember_array(const SQTBasicTypeDef *bt, HSQUIRRELVM v, SQInteger idx)
{
    SQTBasicArrayTypeDef *at = (SQTBasicArrayTypeDef*)bt;
    // value, index
    SQInteger index;
    if(SQ_FAILED(sq_getinteger(v,-1,&index)))
        return sq_throwerror(v,_SC("bad index type"));
    if( (index < 0) || (index >= at->length))
        return sq_throwerror(v,_SC("index out of range"));
    SQInteger offset;
    sq_getbyhandle(v,-2,&_value_offset_handle);
    sq_getinteger(v,-1,&offset);
    sq_poptop(v);
    offset += index * sqt_basesize(at->oftype);
    
}
*/

const SQTBasicTypeDef SQ_Basic_array_init = {
    sq_pushbasic_array,
    sq_getbasic_array,
//    sq_refmember_array,
    0,
    0,
    _SC("array"),
};

static SQInteger __basetype_array_releasehook(SQUserPointer p, SQInteger SQ_UNUSED_ARG(size))
{
    sq_free( p, sizeof(SQTBasicTypeDef));
    return 1;
}

static SQInteger _array_constructor(HSQUIRRELVM v)
{
    // this, oftype, length
    SQTBasicTypeDef *oftype;
    SQTBasicTypeDef *self;
    SQInteger length;
    if(SQ_FAILED(sq_getinstanceup(v,2,(SQUserPointer*)&oftype,SQT_BASICTYPE_TYPE_TAG)))
        return sq_throwerror(v,_SC("expecting basetype instance"));
    sq_getinteger(v,3,&length);
    if( length <= 0)
        return sq_throwerror(v,_SC("array length must be non-zero and pozitive"));
    self = sq_malloc( sizeof(SQTBasicTypeDef));
    *self = SQ_Basic_array_init;
    self->size = length * sqt_basicsize(oftype);
    self->align = sqt_basicalign(oftype);
	if(SQ_FAILED(sq_setinstanceup(v,1,self))) {
		return sq_throwerror(v, _SC("cannot create basic array instance"));
	}
	sq_setreleasehook(v,1,__basetype_array_releasehook);
	sq_push(v,2);
	sq_setbyhandle(v,1,&_array_type_oftype);
	return 0;
}

/*
static SQInteger _array_constructor_1(HSQUIRRELVM v)
{
    SQTBasicTypeDef *oftype;
    SQInteger length;
    SQInteger size;
    if(SQ_FAILED(sq_getinstanceup(v,2,(SQUserPointer*)&oftype,SQT_BASICTYPE_TYPE_TAG)))
        return sq_throwerror(v,_SC("expecting basetype instance"));
    sq_getinteger(v,3,&length);
    if( length <= 0)
        return sq_throwerror(v,_SC("array length must be non-zero and pozitive"));
    //sq_getuserpointer(v,1,&at)
    SQTBasicArrayTypeDef *at = sq_malloc( sizeof(SQTBasicArrayTypeDef));
    at.b = SQ_Basic_array_init;
    at.b.size = length * sqt_basesize(oftype);
    at.b.align = sqt_basealign(oftype);
    at.oftype = oftype;
    at.length = length;
	if(SQ_FAILED(sq_setinstanceup(v,1,at))) {
		return sq_throwerror(v, _SC("cannot create basic array instance"));
	}
	sq_setreleasehook(v,1,__basetype_array_releasehook);
	// reference oftype in array type
	sq_push(v,2);
	sq_setbyhandle(v,1,&_array_type_oftype);
	return 0;
}
*/

static SQInteger _array__typeof(HSQUIRRELVM v)
{
    sq_pushstring(v,sqt_basictype_decl.name,-1);
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
    if( (index < 0) || (sqt_basicsize(self) < (aroff=(sqt_basicsize(oftype) * index))) )
        return sq_throwerror(v,_SC("index out of bounds"));
    offset += index * aroff;
    
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
/*
static SQInteger _array_refmember_1(HSQUIRRELVM v)
{
    // type, ptr, offset, index
    const SQTBasicTypeDef *oftype;
    SQInteger index;
    SQInteger offset;
    SETUP_ARRAY(v);
    if(SQ_FAILED(sq_getinteger(v,4,&index)))
        return sq_throwerror(v,_SC("expecting integer index"));
    if( (index < 0) || (index >= self->length))
        return sq_throwerror(v,_SC("index out of bounds"));
    sq_getbyhandle(v,1,&_array_type_oftype);    // oftype
    if(SQ_FAILED(sqt_getbasictype(v,-1,&oftype)))
        return SQ_ERROR;
    sq_getinteger(v,3,&offset);
    offset += index * sqt_basesize(oftype);

    sq_pushregistrytable(v);                    // oftype, registry
    sq_pushstring(v,sqt_basicvalue_decl,-1);    // oftype, registry, "basicvalue"
    if(SQ_FAILED(sq_get(v,-2)))                 // oftype, registry, basicvalue
        return SQ_ERROR;
    sq_pushnull(v);                             // oftype, registry, basicvalue, dummy_this
    sq_push(v,-4);  // repush oftype            // oftype, registry, basicvalue, dummy_this, oftype
    sq_push(v,2);   // repush ptr               // oftype, registry, basicvalue, dummy_this, oftype, ptr
    sq_pushinteger(v,offset);                   // oftype, registry, basicvalue, dummy_this, oftype, ptr, offset
    if(SQ_FAILED(sq_call(v,4,SQTrue)))          // oftype, registry, basicvalue, ret_val
        return SQ_ERROR;
    return 1;
}
*/
static const SQTMemberDecl _array_members[] = {
	{_SC("$oftype"), &_array_type_oftype },
	{NULL,NULL}
};

#define _DECL_ARRAYTYPE_FUNC(name,nparams,typecheck) {_SC(#name),_array_##name,nparams,typecheck}
static const SQRegFunction _array_methods[] = {
    _DECL_ARRAYTYPE_FUNC(constructor,3,_SC("xxi")),
    _DECL_ARRAYTYPE_FUNC(_typeof,1,_SC("x")),
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
    Basic value
------------------------------------ */

static HSQMEMBERHANDLE _value_type_handle;
static HSQMEMBERHANDLE _value_ptr_handle;
static HSQMEMBERHANDLE _value_offset_handle;

SQInteger sqt_basicvalue_get( HSQUIRRELVM v, SQInteger idx, const SQTBasicTypeDef **ptype, SQUserPointer *pptr)
{
    SQUserPointer ptr;
    SQInteger offset;
    sq_getbyhandle(v,idx,&_value_type_handle);
    if(SQ_FAILED(sqt_getbasictype(v,-1,ptype)))
        return SQ_ERROR;
    sq_poptop(v);
    sq_getbyhandle(v,idx,&_value_ptr_handle);
    if( sq_gettype(v,-1) == OT_USERDATA) {
        if(SQ_FAILED(sq_getuserdata(v,-1,&ptr,NULL)))
            return SQ_ERROR;
    }
    else {
        if(SQ_FAILED(sq_getuserpointer(v,-1,&ptr)))
            return SQ_ERROR;
    }
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
    SQTBasicTypeDef *bt;
    SQInteger top = sq_gettop(v);
    if(SQ_FAILED(sq_getinstanceup(v,2,(SQUserPointer*)&bt,SQT_BASICTYPE_TYPE_TAG)))
        return sq_throwerror(v,_SC("expecting basetype instance"));
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
    if(SQ_FAILED(_value_getref(v)))
        return SQ_ERROR;

    // dereference base types    
    sq_getbyhandle(v,-1,&_value_type_handle);   // type, refmember, ret_value, ret_type
    const SQTBasicTypeDef *reftype;
    if(SQ_FAILED(sqt_getbasictype(v,-1,&reftype)))
        return SQ_ERROR;
    if( 0) {                // !!! missing basictype identification !!!
        HSQOBJECT hval;
        sq_resetobject(&hval);
        sq_getstackobj(v,-2,&hval);
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
        
        if( sqt_basicsize(dst_type) == sqt_basicsize(src_type)) {
            memcpy( dst_ptr, src_ptr, sqt_basicsize(dst_type));
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
    sq_poptop(v);                       // value, index
    if(SQ_FAILED(_value_getref(v))) {   // ... ret_value
        sq_release(v,&hnewval);
        return SQ_ERROR;
    }
    HSQOBJECT hretval;
    sq_resetobject(&hretval);
    sq_getstackobj(v,-1,&hretval);
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
    memset( ptr, 0, sqt_basicsize(type));
    return 1;
}
/*
static SQInteger _value__set(HSQUIRRELVM v)
{
    // value, index
    SQTBasicTypeDef *bt;
    sq_getbyhandle(v,1,&_value_type_handle);
    if(SQ_FAILED(sq_getinstanceup(v,-1,(SQUserPointer*)&bt,SQT_BASICTYPE_TYPE_TAG)))
        return sq_throwerror(v,_SC("bad type"));
    sq_poptop(v);
    HSQOBJECT hval;
    sq_resetobject(&hval);
    sq_getstackobj(v,-1,&hval);
    sq_poptop(v);
    bt->ref_member( bt);
    SQTBasicTypeDef *rt;
    if(SQ_FAILED(sq_getinstanceup(v,-1,(SQUserPointer*)&rt,SQT_BASICTYPE_TYPE_TAG)))
        return sq_throwerror(v,_SC("bad type"));
    if( rt->flags & BT_BASIC_TYPE) {
        
    }
    return 1;
}
*/

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

    REG_TYPE( float);
    REG_TYPE( double);
#ifdef SQUSEDOUBLE
    REG_TYPE_ALIAS( double, SQFloat);
#else // SQUSEDOUBLE
    REG_TYPE_ALIAS( float, SQFloat);
#endif // SQUSEDOUBLE
    
 	sq_poptop(v);
    
	if(SQ_FAILED(sqt_declareclass(v,&sqt_basicarray_decl))) {
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
