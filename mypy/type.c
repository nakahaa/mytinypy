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
#include "tmp.h"

void compile_code(TP)
{
    importCall(tp, 0, "tokenize", tokenize, sizeof(tokenize));
    importCall(tp, 0, "parse", parse, sizeof(parse));
    importCall(tp, 0, "encode", encode, sizeof(encode));
    importCall(tp, 0, "py2bc", py2bc, sizeof(py2bc));
    ezCall(tp, "py2bc", "_init", NONE);
}
#else
void compile_code(TP)
{
}
#endif
