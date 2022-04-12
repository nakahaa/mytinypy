VmType *_vm_init(void)
{
    int i;
    VmType *tp = (VmType *)calloc(sizeof(VmType), 1);
    tp->time_limit = TP_NO_LIMIT;
    tp->clocks = clock();
    tp->time_elapsed = 0.0;
    tp->mem_limit = TP_NO_LIMIT;
    tp->mem_exceeded = 0;
    tp->mem_used = sizeof(VmType);
    tp->cur = 0;
    tp->jmp = 0;
    tp->ex = NONE;
    tp->root = list_nt(tp);
    for (i = 0; i < 256; i++)
    {
        tp->chars[i][0] = i;
    }
    tp_gc_init(tp);
    tp->_regs = to_list(tp);
    for (i = 0; i < TP_REGS; i++)
    {
        tp_set(tp, tp->_regs, NONE, NONE);
    }
    tp->builtins = tp_dict(tp);
    tp->modules = tp_dict(tp);
    tp->_params = to_list(tp);
    for (i = 0; i < TP_FRAMES; i++)
    {
        tp_set(tp, tp->_params, NONE, to_list(tp));
    }
    tp_set(tp, tp->root, NONE, tp->builtins);
    tp_set(tp, tp->root, NONE, tp->modules);
    tp_set(tp, tp->root, NONE, tp->_regs);
    tp_set(tp, tp->root, NONE, tp->_params);
    tp_set(tp, tp->builtins, tp_string("MODULES"), tp->modules);
    tp_set(tp, tp->modules, tp_string("BUILTINS"), tp->builtins);
    tp_set(tp, tp->builtins, tp_string("BUILTINS"), tp->builtins);
    ObjType sys = tp_dict(tp);
    tp_set(tp, sys, tp_string("version"), tp_string("tinypy 1.2+SVN"));
    tp_set(tp, tp->modules, tp_string("sys"), sys);
    tp->regs = tp->_regs.list.val->items;
    tp_full(tp);
    return tp;
}

void tp_deinit(TP)
{
    while (tp->root.list.val->len)
    {
        pop_list(tp, tp->root.list.val, 0, "tp_deinit");
    }
    tp_full(tp);
    tp_full(tp);
    tp_delete(tp, tp->root);
    tp_gc_deinit(tp);
    tp->mem_used -= sizeof(VmType);
    free(tp);
}

void tp_frame(TP, ObjType globals, ObjType code, ObjType *ret_dest)
{
    FrameType f;
    f.globals = globals;
    f.code = code;
    f.cur = (CodeType *)f.code.string.val;
    f.jmp = 0;
    f.regs = (tp->cur <= 0 ? tp->regs : tp->frames[tp->cur].regs + tp->frames[tp->cur].cregs);

    f.regs[0] = f.globals;
    f.regs[1] = f.code;
    f.regs += TP_REGS_EXTRA;

    f.ret_dest = ret_dest;
    f.lineno = 0;
    f.line = tp_string("");
    f.name = tp_string("?");
    f.fname = tp_string("?");
    f.cregs = 0;
    if (f.regs + (256 + TP_REGS_EXTRA) >= tp->regs + TP_REGS || tp->cur >= TP_FRAMES - 1)
    {
        tp_raise(, tp_string("(tp_frame) RuntimeError: stack overflow"));
    }
    tp->cur += 1;
    tp->frames[tp->cur] = f;
}

void _tp_raise(TP, ObjType e)
{
    if (!tp || !tp->jmp)
    {
#ifndef CPYTHON_MOD
        printf("\nException:\n");
        tp_echo(tp, e);
        printf("\n");
        exit(-1);
#else
        tp->ex = e;
        longjmp(tp->nextexpr, 1);
#endif
    }
    if (e.type != NONETYPE)
    {
        tp->ex = e;
    }
    tp_grey(tp, e);
    longjmp(tp->buf, 1);
}

void tp_print_stack(TP)
{
    int i;
    printf("\n");
    for (i = 0; i <= tp->cur; i++)
    {
        if (!tp->frames[i].lineno)
        {
            continue;
        }
        printf("File \"");
        tp_echo(tp, tp->frames[i].fname);
        printf("\", ");
        printf("line %d, in ", tp->frames[i].lineno);
        tp_echo(tp, tp->frames[i].name);
        printf("\n ");
        tp_echo(tp, tp->frames[i].line);
        printf("\n");
    }
    printf("\nException:\n");
    tp_echo(tp, tp->ex);
    printf("\n");
}

void tp_handle(TP)
{
    int i;
    for (i = tp->cur; i >= 0; i--)
    {
        if (tp->frames[i].jmp)
        {
            break;
        }
    }
    if (i >= 0)
    {
        tp->cur = i;
        tp->frames[i].cur = tp->frames[i].jmp;
        tp->frames[i].jmp = 0;
        return;
    }
#ifndef CPYTHON_MOD
    tp_print_stack(tp);
    exit(-1);
#else
    longjmp(tp->nextexpr, 1);
#endif
}

ObjType tp_call(TP, ObjType self, ObjType params)
{
    tp->params = params;

    if (self.type == DICTTYPE)
    {
        if (self.dict.dtype == 1)
        {
            ObjType meta;
            if (lookupFunc(tp, self, tp_string("__new__"), &meta))
            {
                insert_list(tp, params.list.val, 0, self);
                return tp_call(tp, meta, params);
            }
        }
        else if (self.dict.dtype == 2)
        {
            TP_META_BEGIN(self, "__call__");
            return tp_call(tp, meta, params);
            TP_META_END;
        }
    }
    if (self.type == FUNCTYPE && !(self.fnc.ftype & 1))
    {
        ObjType r = _tp_tcall(tp, self);
        tp_grey(tp, r);
        return r;
    }
    if (self.type == FUNCTYPE)
    {
        ObjType dest = NONE;
        tp_frame(tp, self.fnc.info->globals, self.fnc.info->code, &dest);
        if ((self.fnc.ftype & 2))
        {
            tp->frames[tp->cur].regs[0] = params;
            insert_list(tp, params.list.val, 0, self.fnc.info->self);
        }
        else
        {
            tp->frames[tp->cur].regs[0] = params;
        }
        tp_run(tp, tp->cur);
        return dest;
    }
    tp_params_v(tp, 1, self);
    printFunc(tp);
    tp_raise(NONE, tp_string("(tp_call) TypeError: object is not callable"));
}

void tp_return(TP, ObjType v)
{
    ObjType *dest = tp->frames[tp->cur].ret_dest;
    if (dest)
    {
        *dest = v;
        tp_grey(tp, v);
    }
    memset(tp->frames[tp->cur].regs - TP_REGS_EXTRA, 0, (TP_REGS_EXTRA + tp->frames[tp->cur].cregs) * sizeof(ObjType));
    tp->cur -= 1;
}

enum
{
    TP_IEOF,
    TP_IADD,
    TP_ISUB,
    TP_IMUL,
    TP_IDIV,
    TP_IPOW,
    TP_IBITAND,
    TP_IBITOR,
    TP_ICMP,
    TP_IGET,
    TP_ISET,
    TP_INUMBER,
    TP_ISTRING,
    TP_IGGET,
    TP_IGSET,
    TP_IMOVE,
    TP_IDEF,
    TP_IPASS,
    TP_IJUMP,
    TP_ICALL,
    TP_IRETURN,
    TP_IIF,
    TP_IDEBUG,
    TP_IEQ,
    TP_ILE,
    TP_ILT,
    TP_IDICT,
    TP_ILIST,
    TP_INONE,
    TP_ILEN,
    TP_ILINE,
    TP_IPARAMS,
    TP_IIGET,
    TP_IFILE,
    TP_INAME,
    TP_INE,
    TP_IHAS,
    TP_IRAISE,
    TP_ISETJMP,
    TP_IMOD,
    TP_ILSH,
    TP_IRSH,
    TP_IITER,
    TP_IDEL,
    TP_IREGS,
    TP_IBITXOR,
    TP_IIFN,
    TP_INOT,
    TP_IBITNOT,
    TP_ITOTAL
};

#define VA ((int)e.regs.a)
#define VB ((int)e.regs.b)
#define VC ((int)e.regs.c)
#define RA regs[e.regs.a]
#define RB regs[e.regs.b]
#define RC regs[e.regs.c]
#define UVBC (unsigned short)(((VB << 8) + VC))
#define SVBC (short)(((VB << 8) + VC))
#define GA tp_grey(tp, RA)
#define SR(v)     \
    f->cur = cur; \
    return (v);

int tp_step(TP)
{
    FrameType *f = &tp->frames[tp->cur];
    ObjType *regs = f->regs;
    CodeType *cur = f->cur;
    while (1)
    {
#ifdef TP_SANDBOX
        tp_bounds(tp, cur, 1);
#endif
        CodeType e = *cur;

        switch (e.i)
        {
        case TP_IEOF:
            tp_return(tp, NONE);
            SR(0);
            break;
        case TP_IADD:
            RA = tp_add(tp, RB, RC);
            break;
        case TP_ISUB:
            RA = tp_sub(tp, RB, RC);
            break;
        case TP_IMUL:
            RA = tp_mul(tp, RB, RC);
            break;
        case TP_IDIV:
            RA = tp_div(tp, RB, RC);
            break;
        case TP_IPOW:
            RA = tp_pow(tp, RB, RC);
            break;
        case TP_IBITAND:
            RA = tp_bitwise_and(tp, RB, RC);
            break;
        case TP_IBITOR:
            RA = tp_bitwise_or(tp, RB, RC);
            break;
        case TP_IBITXOR:
            RA = tp_bitwise_xor(tp, RB, RC);
            break;
        case TP_IMOD:
            RA = tp_mod(tp, RB, RC);
            break;
        case TP_ILSH:
            RA = tp_lsh(tp, RB, RC);
            break;
        case TP_IRSH:
            RA = tp_rsh(tp, RB, RC);
            break;
        case TP_ICMP:
            RA = tp_number(compare(tp, RB, RC));
            break;
        case TP_INE:
            RA = tp_number(compare(tp, RB, RC) != 0);
            break;
        case TP_IEQ:
            RA = tp_number(compare(tp, RB, RC) == 0);
            break;
        case TP_ILE:
            RA = tp_number(compare(tp, RB, RC) <= 0);
            break;
        case TP_ILT:
            RA = tp_number(compare(tp, RB, RC) < 0);
            break;
        case TP_IBITNOT:
            RA = tp_bitwise_not(tp, RB);
            break;
        case TP_INOT:
            RA = tp_number(!tp_bool(tp, RB));
            break;
        case TP_IPASS:
            break;
        case TP_IIF:
            if (tp_bool(tp, RA))
            {
                cur += 1;
            }
            break;
        case TP_IIFN:
            if (!tp_bool(tp, RA))
            {
                cur += 1;
            }
            break;
        case TP_IGET:
            RA = tp_get(tp, RB, RC);
            GA;
            break;
        case TP_IITER:
            if (RC.number.val < tp_len(tp, RB).number.val)
            {
                RA = tp_iter(tp, RB, RC);
                GA;
                RC.number.val += 1;
#ifdef TP_SANDBOX
                tp_bounds(tp, cur, 1);
#endif
                cur += 1;
            }
            break;
        case TP_IHAS:
            RA = tp_has(tp, RB, RC);
            break;
        case TP_IIGET:
            tp_iget(tp, &RA, RB, RC);
            break;
        case TP_ISET:
            tp_set(tp, RA, RB, RC);
            break;
        case TP_IDEL:
            tp_del(tp, RA, RB);
            break;
        case TP_IMOVE:
            RA = RB;
            break;
        case TP_INUMBER:
#ifdef TP_SANDBOX
            tp_bounds(tp, cur, sizeof(tp_num) / 4);
#endif
            RA = tp_number(*(tp_num *)(*++cur).string.val);
            cur += sizeof(tp_num) / 4;
            continue;
        case TP_ISTRING:
        {
#ifdef TP_SANDBOX
            tp_bounds(tp, cur, (UVBC / 4) + 1);
#endif
            /* RA = tp_string_n((*(cur+1)).string.val,UVBC); */
            int a = (*(cur + 1)).string.val - f->code.string.val;
            RA = strsub(tp, f->code, a, a + UVBC),
            cur += (UVBC / 4) + 1;
        }
        break;
        case TP_IDICT:
            RA = tp_dict_n(tp, VC / 2, &RB);
            break;
        case TP_ILIST:
            RA = to_list_n(tp, VC, &RB);
            break;
        case TP_IPARAMS:
            RA = tp_params_n(tp, VC, &RB);
            break;
        case TP_ILEN:
            RA = tp_len(tp, RB);
            break;
        case TP_IJUMP:
            cur += SVBC;
            continue;
            break;
        case TP_ISETJMP:
            f->jmp = SVBC ? cur + SVBC : 0;
            break;
        case TP_ICALL:
#ifdef TP_SANDBOX
            tp_bounds(tp, cur, 1);
#endif
            f->cur = cur + 1;
            RA = tp_call(tp, RB, RC);
            GA;
            return 0;
            break;
        case TP_IGGET:
            if (!tp_iget(tp, &RA, f->globals, RB))
            {
                RA = tp_get(tp, tp->builtins, RB);
                GA;
            }
            break;
        case TP_IGSET:
            tp_set(tp, f->globals, RA, RB);
            break;
        case TP_IDEF:
        {
#ifdef TP_SANDBOX
            tp_bounds(tp, cur, SVBC);
#endif
            int a = (*(cur + 1)).string.val - f->code.string.val;
            RA = tp_def(tp,
                        /*tp_string_n((*(cur+1)).string.val,(SVBC-1)*4),*/
                        strsub(tp, f->code, a, a + (SVBC - 1) * 4),
                        f->globals);
            cur += SVBC;
            continue;
        }
        break;

        case TP_IRETURN:
            tp_return(tp, RA);
            SR(0);
            break;
        case TP_IRAISE:
            _tp_raise(tp, RA);
            SR(0);
            break;
        case TP_IDEBUG:
            tp_params_v(tp, 3, tp_string("DEBUG:"), tp_number(VA), RA);
            printFunc(tp);
            break;
        case TP_INONE:
            RA = NONE;
            break;
        case TP_ILINE:
#ifdef TP_SANDBOX
            tp_bounds(tp, cur, VA);
#endif
            ;
            int a = (*(cur + 1)).string.val - f->code.string.val;
            f->line = strsub(tp, f->code, a, a + VA * 4 - 1);
            cur += VA;
            f->lineno = UVBC;
            break;
        case TP_IFILE:
            f->fname = RA;
            break;
        case TP_INAME:
            f->name = RA;
            break;
        case TP_IREGS:
            f->cregs = VA;
            break;
        default:
            tp_raise(0, tp_string("(tp_step) RuntimeError: invalid instruction"));
            break;
        }
#ifdef TP_SANDBOX
        tp_time_update(tp);
        tp_mem_update(tp);
        tp_bounds(tp, cur, 1);
#endif
        cur += 1;
    }
    SR(0);
}

void _tp_run(TP, int cur)
{
    tp->jmp += 1;
    if (setjmp(tp->buf))
    {
        tp_handle(tp);
    }
    while (tp->cur >= cur && tp_step(tp) != -1)
        ;
    tp->jmp -= 1;
}

void tp_run(TP, int cur)
{
    jmp_buf tmp;
    memcpy(tmp, tp->buf, sizeof(jmp_buf));
    _tp_run(tp, cur);
    memcpy(tp->buf, tmp, sizeof(jmp_buf));
}

ObjType ezCall(TP, const char *mod, const char *fnc, ObjType params)
{
    ObjType tmp;
    tmp = tp_get(tp, tp->modules, tp_string(mod));
    tmp = tp_get(tp, tmp, tp_string(fnc));
    return tp_call(tp, tmp, params);
}

ObjType _tp_import(TP, ObjType fname, ObjType name, ObjType code)
{
    ObjType g;

    if (!((fname.type != NONETYPE && _str_ind_(fname, tp_string(".tpc")) != -1) || code.type != NONETYPE))
    {
        return ezCall(tp, "py2bc", "import_fname", tp_params_v(tp, 2, fname, name));
    }

    if (code.type == NONETYPE)
    {
        tp_params_v(tp, 1, fname);
        code = loadFunc(tp);
    }

    g = tp_dict(tp);
    tp_set(tp, g, tp_string("__name__"), name);
    tp_set(tp, g, tp_string("__code__"), code);
    tp_set(tp, g, tp_string("__dict__"), g);
    tp_frame(tp, g, code, 0);
    tp_set(tp, tp->modules, name, g);

    if (!tp->jmp)
    {
        tp_run(tp, tp->cur);
    }

    return g;
}

ObjType importCall(TP, const char *fname, const char *name, void *codes, int len)
{
    ObjType f = fname ? tp_string(fname) : NONE;
    ObjType bc = codes ? tp_string_n((const char *)codes, len) : NONE;
    return _tp_import(tp, f, tp_string(name), bc);
}

ObjType tp_exec_(TP)
{
    ObjType code = TP_OBJ();
    ObjType globals = TP_OBJ();
    ObjType r = NONE;
    tp_frame(tp, globals, code, &r);
    tp_run(tp, tp->cur);
    return r;
}

ObjType tp_import_(TP)
{
    ObjType mod = TP_OBJ();
    ObjType r;

    if (tp_has(tp, tp->modules, mod).number.val)
    {
        return tp_get(tp, tp->modules, mod);
    }

    r = _tp_import(tp, tp_add(tp, mod, tp_string(".tpc")), mod, NONE);
    return r;
}

void tp_builtins(TP)
{
    ObjType o;
    struct
    {
        const char *s;
        void *f;
    } b[] = {
        {"print", printFunc},
        {"range", rangeFunc},
        {"min", minFunc},
        {"max", maxFunc},
        {"bind", bindFunc},
        {"copy", copyFunc},
        {"import", tp_import_},
        {"len", lenFunc},
        {"assert", assertFunc},
        {"str", tp_str2},
        {"float", tp_float},
        {"system", sysFunc},
        {"istype", isTypeFunc},
        {"chr", tp_chr},
        {"save", saveFunc},
        {"load", loadFunc},
        {"fpack", fpackFunc},
        {"abs", absFunc},
        {"int", intFunc},
        {"exec", tp_exec_},
        {"exists", existFunc},
        {"mtime", mtimeFunc},
        {"number", tp_float},
        {"round", tp_round},
        {"ord", tp_ord},
        {"merge", tp_merge},
        {"getraw", getrawFunc},
        {"setmeta", setmetaFunc},
        {"getmeta", getmetaFunc},
        {"bool", boolFunc},
#ifdef TP_SANDBOX
        {"sandbox", tp_sandbox_},
#endif
        {0, 0},
    };
    int i;
    for (i = 0; b[i].s; i++)
    {
        tp_set(tp, tp->builtins, tp_string(b[i].s), tp_fnc(tp, (ObjType(*)(VmType *))b[i].f));
    }

    o = objectFunc(tp);
    tp_set(tp, o, tp_string("__call__"), tp_fnc(tp, objectCallFunc));
    tp_set(tp, o, tp_string("__new__"), tp_fnc(tp, newObjFunc));
    tp_set(tp, tp->builtins, tp_string("object"), o);
}

void tp_args(TP, int argc, char *argv[])
{
    ObjType self = to_list(tp);
    int i;
    for (i = 1; i < argc; i++)
    {
        append_list(tp, self.list.val, tp_string(argv[i]));
    }
    tp_set(tp, tp->builtins, tp_string("ARGV"), self);
}

ObjType tp_main(TP, char *fname, void *code, int len)
{
    return importCall(tp, fname, "__main__", code, len);
}

ObjType tp_compile(TP, ObjType text, ObjType fname)
{
    return ezCall(tp, "BUILTINS", "compile", tp_params_v(tp, 2, text, fname));
}

ObjType tp_exec(TP, ObjType code, ObjType globals)
{
    ObjType r = NONE;
    tp_frame(tp, globals, code, &r);
    tp_run(tp, tp->cur);
    return r;
}

ObjType tp_eval(TP, const char *text, ObjType globals)
{
    ObjType code = tp_compile(tp, tp_string(text), tp_string("<eval>"));
    return tp_exec(tp, code, globals);
}

VmType *tp_init(int argc, char *argv[])
{
    VmType *tp = _vm_init();
    tp_builtins(tp);
    tp_args(tp, argc, argv);
    compile_code(tp);
    return tp;
}