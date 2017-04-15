#include <stdint.h>
#include <stddef.h>
#include <squirrel.h>
#include <sqt_basictypes.h>

//#define SQTSA_IS_FLOAT  0x200
//#define SQTSA_IS_SIGNED 0x100
//#define SQTSA_TYPE_MASK (SQTSA_IS_FLOAT | SQTSA_IS_SIGNED)

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
static void sq_pushbasic_##_type( HSQUIRRELVM v,const SQUserPointer p) \
{ \
    _sqtype val = (_sqtype)*(const _type*)p; \
    _push(v,val); \
} \
static SQRESULT sq_getbasic_##_type( HSQUIRRELVM v, SQInteger idx, SQUserPointer p) \
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

