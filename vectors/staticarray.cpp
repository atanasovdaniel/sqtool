
#include <new>
#include <string.h>
#include <stdint.h>
#include <squirrel.h>
#include <sqt_basictypes.h>
#include <sqtool.h>

extern "C" {

    extern SQUIRREL_API_VAR const SQTClassDecl sqt_staticarray_decl;
    #define SQT_STATICARRAY_TYPE_TAG ((SQUserPointer)(SQHash)&sqt_staticarray_decl)

    SQUIRREL_API SQRESULT sqstd_register_staticarray(HSQUIRRELVM v);
    SQUIRREL_API SQRESULT sqt_push_staticarray( HSQUIRRELVM v, SQUserPointer sa);
}

#define SQTSA_OWNS_DATA 0x800
#define SQTSA_OWNS_DIMS 0x400
#define SQTSA_OWNS_MASK (SQTSA_OWNS_DATA | SQTSA_OWNS_DIMS)

static HSQMEMBERHANDLE sa__up_handle;

struct SQTStatcArray
{
    SQUserPointer _data;
    const SQInteger *_dims;
    SQInteger _flags;
    SQInteger _elem_size;
    const SQTBasicTypeDef *_type;
    
    SQTStatcArray( const SQTBasicTypeDef *type, SQUserPointer data, const SQInteger *dims, SQInteger flags)
    {
        _data = data;
        _dims = dims;
        _flags = flags;
        _type = type;
        _elem_size = 0;
        _elem_size = get_elem_size();
        if( flags & SQTSA_OWNS_DIMS) {
            SQInteger len = (1 + get_dims_count()) * sizeof(SQInteger);
            SQUserPointer tmp = sq_malloc( len);
            memcpy( tmp, dims, len);
            _dims = (SQInteger*)tmp;
        }
        if( flags & SQTSA_OWNS_DATA) {
            SQInteger len = get_data_size();
            _data = sq_malloc( len);
            if( data != NULL) {
                memcpy( _data, data, len);
            }
        }
    }
    
    ~SQTStatcArray()
    {
        if( _data && (_flags & SQTSA_OWNS_DATA)) {
            sq_free( _data, get_data_size());
        }
        if( _dims && (_flags & SQTSA_OWNS_DIMS)) {
            sq_free( (SQUserPointer)(SQHash)_dims, (1 + get_dims_count()) * sizeof(SQInteger));
        }
        _flags = 0;
    }
    
    void _Release() {
        this->~SQTStatcArray();
        sq_free( this, sizeof(SQTStatcArray));
    }
    
    static SQTStatcArray* Create( const SQTBasicTypeDef *type, SQUserPointer data, const SQInteger *dims, SQInteger flags)
    {
        return new (sq_malloc(sizeof(SQTStatcArray)))SQTStatcArray(type,data,dims,flags);
    }
    
    SQTStatcArray* CreateSame( SQUserPointer data, const SQInteger *dims, SQInteger flags) const
    {
        return Create(_type,data,dims,flags);
    }
    
    SQBool IsValid() const { return _type && _data && _dims && _dims[0]; }
    
    SQInteger GetValueSize() const { return _type->size; }
    
    SQRESULT ValueGet( HSQUIRRELVM v, const SQUserPointer data) const
    {
        _type->push(v, data);
        return SQ_OK;
    }
    
    // pops value from stack
    SQRESULT ValueSet( HSQUIRRELVM v, SQUserPointer data) const
    {
        if( SQ_FAILED(_type->get(v, -1, data))) return SQ_ERROR;
        sq_poptop(v);
        return SQ_OK;
    }

    SQInteger get_dims_count() const
    {
        const SQInteger *d = _dims;
        SQInteger count = 0;
        while( *d != 0) {
            count++;
            d++;
        }
        return count;
    }
    
    SQInteger get_data_size() const
    {
        if( _dims[0]) {
            const SQInteger *d = _dims;
            SQInteger size = GetValueSize();
            while( *d != 0) {
                size *= *d;
                d++;
            }
            return size;
        }
        return 0;
    }
    
    SQInteger get_elem_size() const
    {
        if( _elem_size) return _elem_size;
        if( _dims[0] && _dims[1]) {
            const SQInteger *d = _dims + 1;
            SQInteger size = GetValueSize();
            while( *d != 0) {
                size *= *d;
                d++;
            }
            return size;
        }
        return 0;
    }
};

/*
struct SQTStatcArray
{
    SQUserPointer _data;
    const SQInteger *_dims;
    SQInteger _flags;
    SQInteger _elem_size;

    // create same instance type as present
    virtual SQTStatcArray* CreateSame( SQUserPointer data, const SQInteger *dims, SQInteger flags) const = 0;
    // push value to stack
    virtual SQRESULT ValueGet( HSQUIRRELVM v, const SQUserPointer data) const = 0;
    // pops value from stack
    virtual SQRESULT ValueSet( HSQUIRRELVM v, SQUserPointer data) const = 0;
    // returns value size
    virtual SQInteger GetValueSize() const = 0;
    // free the instance
    virtual void _Release() = 0;
    
    SQTStatcArray( SQUserPointer data, const SQInteger *dims, SQInteger flags)
    {
        _data = data;
        _dims = dims;
        _flags = flags;
        _elem_size = get_elem_size();
        if( flags & SQTSA_OWNS_DIMS) {
            SQInteger len = (1 + get_dims_count()) * sizeof(SQInteger);
            SQUserPointer tmp = sq_malloc( len);
            memcpy( tmp, dims, len);
            _dims = (SQInteger*)tmp;
        }
        if( flags & SQTSA_OWNS_DATA) {
            SQInteger len = get_data_size();
            _data = sq_malloc( len);
            if( data != NULL) {
                memcpy( _data, data, len);
            }
        }
    }
    
    ~SQTStatcArray()
    {
        if( _data && (_flags & SQTSA_OWNS_DATA)) {
            sq_free( _data, get_data_size());
        }
        if( _dims && (_flags & SQTSA_OWNS_DIMS)) {
            sq_free( (SQUserPointer)(SQHash)_dims, (1 + get_dims_count()) * sizeof(SQInteger));
        }
        _flags = 0;
    }

    SQBool IsValid() const { return _data && _dims && _dims[0] && _dims[1]; }
    
    // 5 elements of size 8
    // 5, 8, 0
    
    SQRESULT Get( HSQUIRRELVM v, SQInteger index) const
    {
        if( _dims[0] && _dims[1]) {
            if( (index < 0) || (index >= _dims[0])) return sq_throwerror(v,_SC("index out of range"));
            SQUserPointer data = ((uint8_t*)_data) + index * _elem_size;
            if( _dims[2]) {
                SQTStatcArray *sa = CreateSame( data, _dims + 1, _flags & ~SQTSA_OWNS_MASK);
                return sqt_push_staticarray(v,sa);
            }
            else if( GetValueSize() == _dims[1]) {
                return ValueGet(v, data);
            }
        }
        return sq_throwerror(v,_SC("bad staticarray object"));
    }
    
    SQRESULT Set( HSQUIRRELVM v, SQInteger index) const
    {
        return SQ_OK;
    }
    
    SQInteger get_dims_count() const
    {
        const SQInteger *d = _dims;
        SQInteger count = 0;
        while( *d != 0) {
            count++;
            d++;
        }
        return count;
    }
    
    SQInteger get_data_size() const
    {
        if( _dims[0]) {
            const SQInteger *d = _dims;
            SQInteger size = 1;
            while( *d != 0) {
                size *= *d;
                d++;
            }
            return size;
        }
        return 0;
    }
    
    SQInteger get_elem_size() const
    {
        if( _dims[0] && _dims[1]) {
            const SQInteger *d = _dims + 1;
            SQInteger size = 1;
            while( *d != 0) {
                size *= *d;
                d++;
            }
            return size;
        }
        return 0;
    }
};
*/

static SQInteger _staticarray_releasehook(SQUserPointer p, SQInteger SQ_UNUSED_ARG(size))
{
    SQTStatcArray *sa = (SQTStatcArray*)p;
    sa->_Release();
    return 1;
}
/*
#define _STARRAY_DEF_BEGIN( _st_name) \
struct _st_name : public SQTStatcArray \
{ \
    _st_name( SQUserPointer data, const SQInteger *dims, SQInteger flags) \
        : SQTStatcArray(data,dims,flags) {} \
    void _Release() { \
        this->~_st_name(); \
        sq_free( this, sizeof(_st_name)); \
    } \
    static SQTStatcArray* Create( SQUserPointer data, const SQInteger *dims, SQInteger flags) { \
        return new (sq_malloc(sizeof(_st_name)))_st_name(data,dims,flags); \
    } \
    SQTStatcArray* CreateSame( SQUserPointer data, const SQInteger *dims, SQInteger flags) const { \
        return Create(data,dims,flags); \
    } \
    SQInteger GetValueSize() const { return ValueSize; } \

#define _STARRAY_DEF_END()  };

_STARRAY_DEF_BEGIN( SQTStatcArray_int32)
    // size of type
    static const SQInteger ValueSize = sizeof(int32_t);
    
    // push value to stack
    SQRESULT ValueGet( HSQUIRRELVM v, const SQUserPointer data) const
    {
        SQFloat sv = (SQInteger)*(int32_t*)data;
        sq_pushinteger(v,sv);
        return SQ_OK;
    }
    
    // pops value from stack
    SQRESULT ValueSet( HSQUIRRELVM v, SQUserPointer data) const
    {
        SQInteger sv;
        if( SQ_FAILED(sq_getinteger(v, -1, &sv))) return SQ_ERROR;
        *(int32_t*)data = (int32_t)sv;
        sq_poptop(v);
        return SQ_OK;
    }
    
_STARRAY_DEF_END()

_STARRAY_DEF_BEGIN( SQTStatcArray_double)
    // size of type
    static const SQInteger ValueSize = sizeof(double);
    
    // push value to stack
    SQRESULT ValueGet( HSQUIRRELVM v, const SQUserPointer data) const
    {
        SQFloat sv = (SQFloat)*(double*)data;
        sq_pushfloat(v,sv);
        return SQ_OK;
    }
    
    // pops value from stack
    SQRESULT ValueSet( HSQUIRRELVM v, SQUserPointer data) const
    {
        SQFloat sv;
        if( SQ_FAILED(sq_getfloat(v, -1, &sv))) return SQ_ERROR;
        *(double*)data = (double)sv;
        sq_poptop(v);
        return SQ_OK;
    }
    
_STARRAY_DEF_END()
*/
/*
struct SQTStatcArray_double : public SQTStatcArray
{
    SQTStatcArray_double( SQUserPointer data, const SQInteger *dims, SQInteger flags)
        : SQTStatcArray(data,dims,flags) {}

    void _Release()
    {
        this->~SQTStatcArray_double();
        sq_free( this, sizeof(SQTStatcArray_double));
    }
    
    static SQTStatcArray* Create( SQUserPointer data, const SQInteger *dims, SQInteger flags)
    {
        return new (sq_malloc(sizeof(SQTStatcArray_double)))SQTStatcArray_double(data,dims,flags);
    }
    
    static const SQInteger ValueSize = sizeof(double);

    SQTStatcArray* CreateSame( SQUserPointer data, const SQInteger *dims, SQInteger flags) const
    {
        return Create(data,dims,flags);
    }
    
    SQInteger GetValueSize() const { return ValueSize; }

    // push value to stack
    SQRESULT ValueGet( HSQUIRRELVM v, const SQUserPointer data) const
    {
        SQFloat sv = (SQFloat)*(double*)data;
        sq_pushfloat(v,sv);
        return SQ_OK;
    }
    
    // pops value from stack
    SQRESULT ValueSet( HSQUIRRELVM v, SQUserPointer data) const
    {
        SQFloat sv;
        if( SQ_FAILED(sq_getfloat(v, -1, &sv))) return SQ_ERROR;
        *(double*)data = (double)sv;
        sq_poptop(v);
        return SQ_OK;
    }
};
*/

SQRESULT sqt_staticarray_sameas( HSQUIRRELVM v, SQUserPointer data, const SQInteger *dims, SQInteger flags)
{
    SQInteger top = sq_gettop(v);                       // sarray
    sq_pushregistrytable(v);                            // sarray, registry
    sq_pushstring(v,sqt_staticarray_decl.reg_name,-1);  // sarray, registry, name
    if(SQ_SUCCEEDED(sq_get(v,-2))) {                    // sarray, registry, class
        sq_remove(v,-2); //removes the registry         // sarray, class
        sq_pushroottable(v); // push the this           // sarray, class, root
        sq_push(v,-3); // repush array                  // sarray, class, root, sarray
        sq_pushuserpointer(v,data); //data              // sarray, class, root, sarray, data
        sq_pushuserpointer(v,(SQUserPointer)dims); //dims              // sarray, class, root, sarray, data, dims
        sq_pushinteger(v,flags); //flags                // sarray, class, root, sarray, data, dims, flags
        if(SQ_SUCCEEDED( sq_call(v,5,SQTrue,SQFalse) )) {
                                                        // sarray, class, new_sarray
            sq_remove(v,-2);                            // sarray, new_sarray
            sq_remove(v,-2);                            // new_sarray
            return SQ_OK;
        }
    }
    sq_settop(v,top);
    return SQ_ERROR;
}

static SQRESULT _sa_toarray( HSQUIRRELVM v, SQTStatcArray *sa, SQUserPointer *pdata, const SQInteger *dims)
{
    sq_newarray(v, dims[0]);
    SQInteger len = dims[0];
    SQInteger idx = 0;
    if( dims[2]) {
        while( idx < len) {
            sq_pushinteger(v,idx);
            if( SQ_FAILED(_sa_toarray( v, sa, pdata, dims+1))) return SQ_ERROR;
            sq_set(v, -3);
            idx++;
        }
    }
    else {
        SQUserPointer data = *pdata;
        while( idx < len) {
            sq_pushinteger(v,idx);
            if( SQ_FAILED(sa->ValueGet(v, data))) return SQ_ERROR;
            sq_set(v, -3);
            data = (SQUserPointer)((uint8_t*)data + dims[1]);
            idx++;
        }
        *pdata = data;
    }
    return SQ_OK;
} 

SQRESULT sqt_staticarray_toarray( HSQUIRRELVM v, SQTStatcArray *sa)
{
    if( !sa->IsValid()) return sq_throwerror(v,_SC("invalid staticarray object"));
    SQUserPointer data = sa->_data;
    return _sa_toarray( v, sa, &data, sa->_dims);
}


#define SETUP_STATICARRAY(v) \
    SQTStatcArray *self = NULL; \
    if(SQ_FAILED(sq_getinstanceup(v,1,(SQUserPointer*)&self,SQT_STATICARRAY_TYPE_TAG))) \
        return sq_throwerror(v,_SC("invalid type tag")); \
    if(!self || !self->IsValid())  \
        return sq_throwerror(v,_SC("invalid staticarray object"));

static SQInteger _staticarray__typeof(HSQUIRRELVM v)
{
    sq_pushstring(v,sqt_staticarray_decl.name,-1);
    return 1;
}

static SQInteger _staticarray_constructor(HSQUIRRELVM v)
{
    SQTStatcArray *sa;
    SQBool have_up = SQFalse;
    SQInteger top = sq_gettop(v);
    if( sq_gettype(v,2) == OT_INSTANCE) {
        // staticarray, data, dims, flags
        if( top != 5) return sq_throwerror(v,_SC("wrong number of parameters"));
        SQTStatcArray *up;
        if(SQ_FAILED(sq_getinstanceup(v,2,(SQUserPointer*)&up,SQT_STATICARRAY_TYPE_TAG)) || (up == NULL))
            return sq_throwerror(v,_SC("bad argument type"));
        if( (sq_gettype(v,3) != OT_USERPOINTER) || (sq_gettype(v,4) != OT_USERPOINTER) || (sq_gettype(v,5) != OT_INTEGER))
                return sq_throwerror(v,_SC("bad argument type"));
            
        SQUserPointer data;
        SQUserPointer dims;
        SQInteger flags;
        sq_getuserpointer(v,3,&data);
        sq_getuserpointer(v,4,&dims);
        sq_getinteger(v,5,&flags);
        sa = up->CreateSame( data, (SQInteger*)dims, flags);
        have_up = SQTrue;
    }
    else {
        // typeid, dim1, dim2, ... dimN
        if( sq_gettype(v,2) != OT_INTEGER) {
            return sq_throwerror(v, _SC("bad argument type"));
        }
        SQInteger ndims = top - 2;
        SQInteger *dims = (SQInteger*)sq_getscratchpad(v, (ndims+1)*sizeof(SQInteger));
        SQInteger n;
    
        dims[ndims] = 0;
        for( n=0; n < ndims; n++) {
            SQInteger p=n+3;
            if( sq_gettype(v,p) == OT_INTEGER) {
                sq_getinteger(v,p,&dims[n]);
                if( dims[n] <= 0) {
                    return sq_throwerror(v, _SC("bad dimension size"));
                }
            }
            else {
                return sq_throwerror(v, _SC("bad argument type"));
            }
        }
    
        SQInteger type_id;
        sq_getinteger(v,2,&type_id);
        const SQTBasicTypeDef *bt = sqt_basictypebyid( type_id);
        if( bt == NULL) return sq_throwerror(v, _SC("unknown type id"));
        sa = SQTStatcArray::Create( bt, NULL, dims, SQTSA_OWNS_DATA | SQTSA_OWNS_DIMS);
    }
    
    if( !sa->IsValid()) {
        sa->_Release();
        return sq_throwerror(v, _SC("cannot create staticarray"));
    }
    
    if(SQ_FAILED(sq_setinstanceup(v,1,sa))) {
		sa->_Release();
        return sq_throwerror(v, _SC("cannot create staticarray instance"));
    }
    
    if( have_up) {
		sq_push(v,2);
		sq_setbyhandle(v,1,&sa__up_handle);
    }
    
    sq_setreleasehook(v,1,_staticarray_releasehook);
    return 0;
}

static SQInteger _staticarray__get(HSQUIRRELVM v)
{
    SETUP_STATICARRAY(v);
    SQInteger index;
    sq_getinteger(v,2,&index);
    if( (index < 0) || (index >= self->_dims[0])) return sq_throwerror(v,_SC("index out of range"));
    SQUserPointer data = ((uint8_t*)self->_data) + index * self->_elem_size;
    if( self->_dims[2]) {
        sq_push(v,1);
        if( SQ_FAILED(sqt_staticarray_sameas(v, data, self->_dims + 1, self->_flags & ~SQTSA_OWNS_MASK)))
            return SQ_ERROR;
    }
    else if( self->GetValueSize() == self->_dims[1]) {
        self->ValueGet(v, data);
    }
    else
        return sq_throwerror(v,_SC("invalid staticarray object"));
        
    return 1;
}

static SQInteger _staticarray__set(HSQUIRRELVM v)
{
    SETUP_STATICARRAY(v);
    SQInteger index;
    sq_getinteger(v,2,&index);
    SQUserPointer data = ((uint8_t*)self->_data) + index * self->_elem_size;
    if( (index < 0) || (index >= self->_dims[0])) return sq_throwerror(v,_SC("index out of range"));
    if( self->_dims[1]) {
        SQObjectType ot = sq_gettype(v,3);
        if( ot == OT_ARRAY) {
        }
        else if( ot == OT_INSTANCE) {
            SQTStatcArray *other = NULL;
            if(SQ_SUCCEEDED(sq_getinstanceup(v,3,(SQUserPointer*)&other,SQT_STATICARRAY_TYPE_TAG))) {
                if(!other || !other->IsValid())
                    return sq_throwerror(v,_SC("invalid staticarray object"));
                if( self->_elem_size != other->get_data_size())
                    return sq_throwerror(v,_SC("staticarray sizes don't match"));
                memcpy( data, other->_data, self->_elem_size);
                return 0;
            }
        }
        return sq_throwerror(v,_SC("bad argument type"));
    }
    else if( self->GetValueSize() == self->_dims[1]) {
        if( SQ_FAILED(self->ValueSet(v, data)))
            return SQ_ERROR;
    }
    else
        return sq_throwerror(v,_SC("invalid staticarray object"));
    
    return 0;
}

static SQInteger _staticarray__nexti(HSQUIRRELVM v)
{
    SETUP_STATICARRAY(v);
    if(sq_gettype(v,2) == OT_NULL) {
        sq_pushinteger(v, 0);
        return 1;
    }
    SQInteger idx;
    if(SQ_SUCCEEDED(sq_getinteger(v, 2, &idx))) {
        if(idx+1 < self->_dims[0]) {
            sq_pushinteger(v, idx+1);
            return 1;
        }
        sq_pushnull(v);
        return 1;
    }
    return sq_throwerror(v,_SC("internal error (_nexti) wrong argument type"));
}

static SQInteger _staticarray_len(HSQUIRRELVM v)
{
    SETUP_STATICARRAY(v);
    sq_pushinteger(v, self->_dims[0]);
    return 1;
}

static SQInteger _staticarray_toarray(HSQUIRRELVM v)
{
    SETUP_STATICARRAY(v);
    sqt_staticarray_toarray( v, self);
    return 1;
}


static SQInteger _staticarray_clear(HSQUIRRELVM v)
{
    SETUP_STATICARRAY(v);
    memset( self->_data, 0, self->get_data_size());
    return 1;
}

//bindings
#define _DECL_STATICARRAY_FUNC(name,nparams,typecheck) {_SC(#name),_staticarray_##name,nparams,typecheck}
static const SQRegFunction _staticarray_methods[] = {
    _DECL_STATICARRAY_FUNC(constructor,-3,_SC("x")),
    _DECL_STATICARRAY_FUNC(_get,2,_SC("xi")),
    _DECL_STATICARRAY_FUNC(_nexti,2,_SC("x.")),
    _DECL_STATICARRAY_FUNC(_set,3,_SC("xi.")),
    _DECL_STATICARRAY_FUNC(clear,1,_SC("x")),
    _DECL_STATICARRAY_FUNC(len,1,_SC("x")),
    _DECL_STATICARRAY_FUNC(toarray,1,_SC("x")),
    _DECL_STATICARRAY_FUNC(_typeof,1,_SC("x")),
    {NULL,(SQFUNCTION)0,0,NULL}
};

static const SQTMemberDecl _staticarray_members[] = {
	{_SC("_up"), &sa__up_handle },
	{NULL,NULL}
};


const SQTClassDecl sqt_staticarray_decl = {
	NULL,                   // base_class
    _SC("sqt_staticarray"),	// reg_name
    _SC("staticarray"),		// name
	_staticarray_members,   // members
	_staticarray_methods,	// methods
	NULL,                   // globals
};

SQRESULT sqstd_register_staticarray(HSQUIRRELVM v)
{
	if(SQ_FAILED(sqt_declareclass(v,&sqt_staticarray_decl)))
	{
		return SQ_ERROR;
	}
	sq_poptop(v);
    return SQ_OK;
}
