/*
Copyright (c) 2016-2017 Daniel Atanasov

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
#ifndef __SQTOOL_H__
#define __SQTOOL_H__

#define SQTOOL_VERSION			"sqtool 0.09 devel"
#define SQTOOL_VERSION_NUMBER	9

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tagSQTClassDecl {
	const struct tagSQTClassDecl *base_class;
    const SQChar *reg_name;
    const SQChar *name;
	const SQRegFunction	*methods;
	const SQRegFunction	*globals;
} SQTClassDecl;

SQUIRREL_API SQInteger sqt_declareclass( HSQUIRRELVM v, const SQTClassDecl *decl);
SQUIRREL_API SQInteger sqt_declarefunctions( HSQUIRRELVM v, const SQRegFunction *fcts);

#ifdef _WIN32
#include <wchar.h>
SQUIRREL_API void sqt_pushstring_wc( HSQUIRRELVM v, const wchar_t *src, SQInteger len);
SQUIRREL_API SQInteger sqt_getstring_wc( HSQUIRRELVM v, SQInteger idx, wchar_t **dst);
SQUIRREL_API SQInteger sqt_strtowc( SQInteger n, SQChar **strs, SQUnsignedInteger *ptotal_len);
SQUIRREL_API SQInteger sqt_wctostr( SQInteger n, SQChar **strs, SQUnsignedInteger *ptotal_len);
#endif // _WIN32

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // __SQTOOL_H__
