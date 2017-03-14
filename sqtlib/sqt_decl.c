/* see copyright notice in sqtool.h */
#include <squirrel.h>
#include <sqtool.h>

SQInteger sqt_declarefunctions( HSQUIRRELVM v, const SQRegFunction *fcts)
{
														// table
	if( fcts != 0) {
		const SQRegFunction	*f = fcts;
		while( f->name != 0) {
			SQBool bstatic = SQFalse;
			const SQChar *typemask = f->typemask;
			if( typemask && (typemask[0] == _SC('-'))) {	// if first char of typemask is '-', make static function
				typemask++;
				bstatic = SQTrue;
			}
			sq_pushstring(v,f->name,-1);				// table, method_name
			sq_newclosure(v,f->f,0);					// table, method_name, method_fct
			sq_setparamscheck(v,f->nparamscheck,typemask);
			sq_newslot(v,-3,bstatic);					// table
			f++;
		}
	}
    return SQ_OK;
}

SQInteger sqt_declareclass( HSQUIRRELVM v, const SQTClassDecl *decl)
{															// root
    sq_pushregistrytable(v);								// root, registry
    sq_pushstring(v,decl->reg_name,-1);						// root, registry, reg_name
    if(SQ_FAILED(sq_get(v,-2))) {
															// root, registry
        sq_pop(v,1);										// root
		if( decl->base_class != 0) {
			sqt_declareclass( v, decl->base_class);			// root, base_class
			sq_newclass(v,SQTrue);							// root, new_class
		}
		else {
			sq_newclass(v,SQFalse);							// root, new_class
		}
        sq_settypetag(v,-1,(SQUserPointer)decl);
		sqt_declarefunctions(v,decl->methods);
/*
		if( decl->methods != 0) {
			const SQRegFunction	*f = decl->methods;
			while( f->name != 0) {
				SQBool bstatic = SQFalse;
				const SQChar *typemask = f->typemask;
				if( typemask && (typemask[0] == _SC('-'))) {	// if first char of typemask is '-', make static function
					typemask++;
					bstatic = SQTrue;
				}
				sq_pushstring(v,f->name,-1);				// root, new_class, method_name
				sq_newclosure(v,f->f,0);					// root, new_class, method_name, method_fct
				sq_setparamscheck(v,f->nparamscheck,typemask);
				sq_newslot(v,-3,bstatic);					// root, new_class
				f++;
			}
		}
*/
		sq_pushregistrytable(v);							// root, new_class, registry
		sq_pushstring(v,decl->reg_name,-1);					// root, new_class, registry, reg_name
		sq_push(v,-3);										// root, new_class, registry, reg_name, new_class
        sq_newslot(v,-3,SQFalse);							// root, new_class, registry
        sq_poptop(v);                                       // root, new_class
		if( decl->name != 0) {
			sq_pushstring(v,decl->name,-1);					// root, new_class, class_name
			sq_push(v,-2);									// root, new_class, class_name, new_class
			sq_newslot(v,-4,SQFalse);						// root, new_class
		}
		if( decl->globals) {
			sq_push(v,-2);									// root, new_class, root
			sqt_declarefunctions(v,decl->globals);
			sq_poptop(v);									// root, new_class
		}
    }
    else {
															// root, registry, reg_class
		sq_remove(v,-2);									// root, reg_class
    }
    return SQ_OK;
}