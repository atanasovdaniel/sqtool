
#include <squirrel.h>
#include <sqstdpackage.h>

extern const SQSTDPackageList sq_builtin_packages[];

#ifdef SQ_BUILTIN_PACKAGE_LIST
#define SQ_PACKAGE_DEF(_vname,_loader)    extern SQInteger _loader(HSQUIRRELVM v);
#include SQ_BUILTIN_PACKAGE_LIST
#undef SQ_PACKAGE_DEF
#endif // SQ_BUILTIN_PACKAGE_LIST

const SQSTDPackageList sq_builtin_packages[] = {
#ifdef SQ_BUILTIN_PACKAGE_LIST
#define SQ_PACKAGE_DEF(_vname,_loader)    {_SC(#_vname),_loader},
#include SQ_BUILTIN_PACKAGE_LIST
#undef SQ_PACKAGE_DEF
#endif // SQ_BUILTIN_PACKAGE_LIST
    {0,0}
};

