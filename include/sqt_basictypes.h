#ifndef _SQT_BASICTYPES_H_
#define _SQT_BASICTYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tagSQTBasicTypeDef SQTBasicTypeDef;

SQUIRREL_API SQInteger sqt_basicgetsize( const SQTBasicTypeDef *bt);
SQUIRREL_API SQInteger sqt_basicgetalign( const SQTBasicTypeDef *bt);
SQUIRREL_API void sqt_basicpush( const SQTBasicTypeDef *bt, HSQUIRRELVM v, const SQUserPointer p);
SQUIRREL_API SQRESULT sqt_basicget( const SQTBasicTypeDef *bt, HSQUIRRELVM v, SQInteger idx, SQUserPointer p);
//SQUIRREL_API SQRESULT sqt_basicrefmember( const SQTBasicTypeDef *bt, HSQUIRRELVM v);

SQUIRREL_API SQInteger sqt_basicvalue_get( HSQUIRRELVM v, SQInteger idx, const SQTBasicTypeDef **ptype, SQUserPointer *pptr);

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