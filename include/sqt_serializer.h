#ifndef _SQT_SERIALIZER_H_
#define _SQT_SERIALIZER_H_

SQUIRREL_API SQRESULT sqt_serbin_load_cb(HSQUIRRELVM v, const SQChar *opts, SQREADFUNC readfct, SQUserPointer user);
SQUIRREL_API SQRESULT sqt_serbin_load(HSQUIRRELVM v, const SQChar *opts, SQFILE file);
SQUIRREL_API SQRESULT sqt_serbin_save_cb( HSQUIRRELVM v, const SQChar *opts, SQWRITEFUNC writefct, SQUserPointer user);
SQUIRREL_API SQRESULT sqt_serbin_save( HSQUIRRELVM v, const SQChar *opts, SQFILE file);
SQUIRREL_API SQRESULT sqstd_register_serbin(HSQUIRRELVM v);

SQUIRREL_API SQRESULT sqt_serjson_load_cb(HSQUIRRELVM v, const SQChar *opts, SQREADFUNC readfct, SQUserPointer user);
SQUIRREL_API SQRESULT sqt_serjson_load(HSQUIRRELVM v, const SQChar *opts, SQFILE file);
SQUIRREL_API SQRESULT sqt_serjson_save_cb( HSQUIRRELVM v, const SQChar *opts, SQWRITEFUNC writefct, SQUserPointer user);
SQUIRREL_API SQRESULT sqt_serjson_save( HSQUIRRELVM v, const SQChar *opts, SQFILE file);
SQUIRREL_API SQRESULT sqstd_register_serjson(HSQUIRRELVM v);

#endif // _SQT_SERIALIZER_H_
