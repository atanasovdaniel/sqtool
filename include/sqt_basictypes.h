#ifndef _SQT_BASICTYPES_H_
#define _SQT_BASICTYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

#define SQT_BASICTYPES_MAXSIZE  16

typedef void (*sqt_basicpush_fct)(HSQUIRRELVM v,const SQUserPointer p);
typedef SQRESULT (*sqt_basicget_fct)(HSQUIRRELVM v, SQInteger idx, SQUserPointer p);

typedef struct tagSQTBasicTypeDef {
    const sqt_basicpush_fct push;
    const sqt_basicget_fct get;
    SQInteger size;
    SQInteger align;
    const SQChar *name;
} SQTBasicTypeDef;

extern SQUIRREL_API_VAR const SQTBasicTypeDef SQ_Basic_uint8_t;
extern SQUIRREL_API_VAR const SQTBasicTypeDef SQ_Basic_int8_t;
extern SQUIRREL_API_VAR const SQTBasicTypeDef SQ_Basic_uint16_t;
extern SQUIRREL_API_VAR const SQTBasicTypeDef SQ_Basic_int16_t;
extern SQUIRREL_API_VAR const SQTBasicTypeDef SQ_Basic_uint32_t;
extern SQUIRREL_API_VAR const SQTBasicTypeDef SQ_Basic_int32_t;
#ifdef _SQ64
extern SQUIRREL_API_VAR const SQTBasicTypeDef SQ_Basic_uint64_t;
extern SQUIRREL_API_VAR const SQTBasicTypeDef SQ_Basic_int64_t;
#define SQ_Basic_SQInteger  SQ_Basic_int64_t
#else // _SQ64
#define SQ_Basic_SQInteger  SQ_Basic_int32_t
#endif // _SQ64

extern SQUIRREL_API_VAR const SQTBasicTypeDef SQ_Basic_float;
extern SQUIRREL_API_VAR const SQTBasicTypeDef SQ_Basic_double;
#ifdef SQUSEDOUBLE
#define SQ_Basic_SQFloat  SQ_Basic_double
#else // SQUSEDOUBLE
#define SQ_Basic_SQFloat  SQ_Basic_float
#endif // SQUSEDOUBLE

SQUIRREL_API const SQTBasicTypeDef *sqt_basictypebyid( SQInteger id);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // _SQT_BASICTYPES_H_