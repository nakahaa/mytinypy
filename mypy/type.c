#ifndef TP_COMPILER
#define TP_COMPILER 1
#endif

#include "type.h"
#include "listdict.h"
#include "string.h"
#include "builtins.h"
#include "garbage.h"
#include "operator.h"
#include "myvm.h"

void compile_code(TP);

ObjType NONE = {NONETYPE};

#if TP_COMPILER
#include "bc.c"
void compile_code(TP)
{
    tp_import(tp, 0, "tokenize", tp_tokenize, sizeof(tp_tokenize));
    tp_import(tp, 0, "parse", tp_parse, sizeof(tp_parse));
    tp_import(tp, 0, "encode", tp_encode, sizeof(tp_encode));
    tp_import(tp, 0, "py2bc", tp_py2bc, sizeof(tp_py2bc));
    tp_ez_call(tp, "py2bc", "_init", NONE);
}
#else
void compile_code(TP)
{
}
#endif
