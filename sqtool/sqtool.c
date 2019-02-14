/*
Copyright (c) 2019 Daniel Atanasov

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
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include <squirrel.h>
#include <sqstdblob.h>
#include <sqstdsystem.h>
#include <sqstdio.h>
#include <sqstdmath.h>
#include <sqstdstring.h>
#include <sqstdaux.h>

/*
To be done:
    - wmain
    - interactive mode
    - suspend_handler
    - at_exit callbacks
*/

#define ERR_NOTSTARTED      255
#define ERR_THROW           254

#define ERR_OPTION          ERR_NOTSTARTED
#define ERR_FAILED_TO_LOAD  ERR_NOTSTARTED

#ifdef SQUNICODE
#define scfprintf fwprintf
#define scvprintf vfwprintf
#else
#define scfprintf fprintf
#define scvprintf vfprintf
#endif

static void printfunc(HSQUIRRELVM SQ_UNUSED_ARG(v),const SQChar *s,...)
{
    va_list vl;
    va_start(vl, s);
    scvprintf(stdout, s, vl);
    va_end(vl);
}

static void errorfunc(HSQUIRRELVM SQ_UNUSED_ARG(v),const SQChar *s,...)
{
    va_list vl;
    va_start(vl, s);
    scvprintf(stderr, s, vl);
    va_end(vl);
}

static SQRESULT init_vm(HSQUIRRELVM v)
{
    sq_setprintfunc(v,printfunc,errorfunc);

    sq_pushroottable(v);

    sqstd_register_bloblib(v);
    sqstd_register_iolib(v);
    sqstd_register_systemlib(v);
    sqstd_register_mathlib(v);
    sqstd_register_stringlib(v);

    //aux library
    //sets error handlers
    sqstd_seterrorhandlers(v);

    sq_poptop(v);

    return SQ_OK;
}

static void push_arguments(HSQUIRRELVM v, int argc, const SQChar **argv)
{
    while( argc)
    {
        sq_pushstring(v,*argv,-1);
        argc--;
        argv++;
    }
}

static int get_result(HSQUIRRELVM v)
{
    SQInteger st = sq_getvmstate(v);
//    if( (st == SQ_VMSTATE_SUSPENDED) && suspend_handler) {
//        if(SQ_SUCCEEDED(suspend_handler(v)))
//            st = sq_getvmstate(v);
//        else
//            return ERR_THROW;
//    }
    while( st == SQ_VMSTATE_SUSPENDED) {
        sq_throwerror(v,_SC("Unhandled suspend"));
        sq_poptop(v); // suspend arg
        if(SQ_SUCCEEDED(sq_wakeupvm(v,SQFalse,SQTrue,SQTrue,SQTrue))) {
            st = sq_getvmstate(v);
        }
        else
            return ERR_THROW;
    }

    int r = 0;
    if( sq_gettype(v,-1) == OT_INTEGER) {
        SQInteger res;
        sq_getinteger(v,-1,&res);
        r = res;
    }
    sq_poptop(v); // remove result

    return r;
}

#ifndef NO_COMPILER

static int run_script(HSQUIRRELVM v, int argc, const SQChar **argv)
{
    // allow errors from loadfile to be thrown
    static const SQChar load[] = _SC("return ::loadfile(vargv[0],true);");
    const SQChar *name = argv[0];
    int r;
    argv++;
    argc--;
    if(SQ_SUCCEEDED(sq_compilebuffer(v,load,sizeof(load)/sizeof(*load)-1,_SC("<sqtool.run_script>"),SQTrue))) {
        sq_pushroottable(v);                                        // loadfile, root
        sq_pushstring(v,name,-1);                                   // loadfile, root, filename
        if(SQ_SUCCEEDED(sq_call(v,2,SQTrue,SQTrue))) {              // loadfile, [root, filenamem, true], function
            sq_remove(v,-2);                                        // [loadfile], function
            sq_pushroottable(v);                                    // function, root
            push_arguments(v,argc,argv);                            // function, root, arguments
            if(SQ_SUCCEEDED(sq_call(v,1+argc,SQTrue,SQTrue))) {     // function, [root, arguments], result
                sq_remove(v,-2);                                    // [function], result
                r = get_result(v);
            }
            else {
                sq_poptop(v);                                       // [function]
                r = ERR_THROW;
            }
        }
        else {
            sq_poptop(v);                                           // [loadfile]
            r = ERR_FAILED_TO_LOAD;
        }
    }
    else {
        fprintf(stderr,"Failed to compile run_script\n");
        r = ERR_FAILED_TO_LOAD;
    }
    return r;
}

#else // NO_COMPILER

static int run_script(HSQUIRRELVM v, int argc, const SQChar **argv)
{
    const SQChar *name = argv[0];
    int r;
    argv++;
    argc--;
    if(SQ_SUCCEEDED(sqstd_loadfile(v,name,SQTrue))) {
        sq_pushroottable(v);
        push_arguments(v,argc,argv);
        if(SQ_SUCCEEDED(sq_call(v,1+argc,SQTrue,SQTrue))) {
            sq_remove(v,-2); //removes the closure
            r = get_result(v);
        }
        else {
            sq_pop(v,1); //removes the closure
            r = ERR_THROW;
        }
    }
    else {
        const SQChar *e;
        sq_getlasterror(v);
        if(SQ_SUCCEEDED(sq_getstring(v,-1,&e))) {
            printf("loadfile error: %s\n",e);
        }
        else {
            fprintf(stderr,"loadfile error\n");
        }
        sq_poptop(v);
        r = ERR_FAILED_TO_LOAD;
    }
    return r;
}

#endif // NO_COMPILER

static int compile_script(HSQUIRRELVM v, const SQChar *name, const SQChar *out_name)
{
    int r = 0;
    scfprintf(stderr,_SC("Compiling %s to %s\n"), name, out_name);
    if(SQ_SUCCEEDED(sqstd_loadfile(v,name,SQTrue))) {
        if(SQ_FAILED(sqstd_writeclosuretofile(v,out_name))) {
            r = ERR_THROW;
        }
        sq_poptop(v);
    }
    else {
        r = ERR_FAILED_TO_LOAD;
    }
    return r;
}

static void print_version( void)
{
    scfprintf(stdout,_SC("%s %s (%d bits)\n"),SQUIRREL_VERSION,SQUIRREL_COPYRIGHT,((int)(sizeof(SQInteger)*8)));
}

static void print_usage( void)
{
    print_version();
    printf( "usage: sqtool [options] [script [arguments]]\n");
    printf( " options:\n");
    printf( "  -c            Compile script to bytecode and exit\n");
    printf( "  -ofile        Specifies output file for the -c option (default out.cnut)\n");
    printf( "  -d            Enable debug info\n");
    printf( "  -Dname=value  Define in root table entry with name and value\n");
    printf( "  -v            Print version info and exit\n");
    printf( "  -h            Print this help and exit\n");
}

int main( int argc, const SQChar **argv)
{
    int r = 0;
    int o_compile_only = 0;
    const SQChar *o_output_file = _SC("out.cnut");

    HSQUIRRELVM v = sq_open(1024);
    init_vm( v);

    argc--;
    argv++;
    while( argc && (argv[0][0] == _SC('-')) && !r) {
        int opt = argv[0][1] < 127 ? argv[0][1] : 0;
        const SQChar *arg = 0;
        if( strchr( "oD", opt)) {
            if( argv[0][2] == _SC('\0')) {
                if( argc > 1) {
                    arg = argv[1];
                    argc--;
                    argv++;
                }
                else {
                    r = ERR_OPTION;
                    fprintf( stderr, "Missing argument for option %c\n", opt);
                }
            }
            else {
                arg = argv[0] + 2;
            }
        }
        argc--;
        argv++;
        switch( opt) {
            case 'c':
                o_compile_only = 1;
                break;

            case 'o':
                o_output_file = arg;
                break;

            case 'd':
                sq_enabledebuginfo(v,1);
                break;

            case 'v':
                print_version();
                r = -1;
                break;

            case 'D': {
                const SQChar *val = arg;
                while( (*val != _SC('\0')) && (*val != _SC('='))) val++;
                sq_pushroottable(v);
                sq_pushstring(v,arg,val-arg);
                if( *val == _SC('=')) val++;
                sq_pushstring(v,val,-1);
                sq_newslot(v,-3,SQFalse);
                sq_poptop(v);
            } break;

            case 'h':
                print_usage();
                r = -1;
                break;

            default:
                r = ERR_OPTION;
                fprintf( stderr, "Unknown option %c\n", opt);
                break;
        }
    }
    if( (r == 0) && o_compile_only) {
        if( argc == 0) {
            fprintf( stderr, "Expecting script for -c option\n");
            r = ERR_OPTION;
        }
        else if( argc > 1) {
            fprintf( stderr, "Script arguments are ignored with -c option\n");
        }
        else {
            r = compile_script(v, argv[0], o_output_file);
        }
    }
    else if( r == 0) {
        if( argc) {
            // run script
            r = run_script(v, argc, argv);
        }
        else {
            // interactive
            fprintf(stderr,"No script given\n");
            r = ERR_OPTION;
        }
    }
    else if( r == -1) {
        r = 0;
    }

    sq_close(v);

    return r;
}
