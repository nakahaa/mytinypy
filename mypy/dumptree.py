import tokenize
from tokenize import Token
import tokenize
import parse
import encode

syntaxMap = {}

def merge(a, b):
    if isinstance(a, dict):
        for k in b:
            a[k] = b[k]
    else:
        for k in b:
            setattr(a, k, b[k])

def number(v):
    if type(v) is str and v[0:2] == '0x':
        v = int(v[2:], 16)
    return float(v)


def istype(v, t):
    if t == 'string':
        return isinstance(v, str)
    elif t == 'list':
        return (isinstance(v, list) or isinstance(v, tuple))
    elif t == 'dict':
        return isinstance(v, dict)
    elif t == 'number':
        return (isinstance(v, float) or isinstance(v, int))
    raise '?'


def fpack(v):
    import struct
    return struct.pack(FTYPE, v)


def system(cmd):
    import os
    return os.system(cmd)


def load(fname):
    f = open(fname, 'rb')
    r = f.read()
    f.close()
    return r


def save(fname, v):
    f = open(fname, 'wb')
    f.write(v)
    f.close()


EOF, ADD, SUB, MUL, DIV, POW, BITAND, BITOR, CMP, GET, SET, NUMBER, STRING, GGET, GSET, MOVE, DEF, PASS, JUMP, CALL, RETURN, IF, DEBUG, EQ, LE, LT, DICT, LIST, NONE, LEN, POS, PARAMS, IGET, FILE, NAME, NE, HAS, RAISE, SETJMP, MOD, LSH, RSH, ITER, DEL, REGS, BITXOR, IFN, NOT, BITNOT = 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48


class DState:
    def __init__(self, code, fname):
        self.code, self.fname = code, fname
        self.lines = self.code.split('\n')

        self.stack, self.out, self._scopei, self.tstack, self._tagi, self.data = [], [
            ('tag', 'EOF')], 0, [], 0, {}
        self.error = False

    def begin(self, gbl=False):
        if len(self.stack):
            self.stack.append((self.vars, self.r2n, self.n2r, self._tmpi, self.mreg, self.snum,
                              self._globals, self.lineno, self.globals, self.rglobals, self.cregs, self.tmpc))
        else:
            self.stack.append(None)
        self.vars, self.r2n, self.n2r, self._tmpi, self.mreg, self.snum, self._globals, self.lineno, self.globals, self.rglobals, self.cregs, self.tmpc = [
        ], {}, {}, 0, 0, str(self._scopei), gbl, -1, [], [], ['regs'], 0
        self._scopei += 1
        insert(self.cregs)

    def end(self):
        self.cregs.append(self.mreg)
        code(EOF)

        if self.tmpc != 0:
            print("Warning:\nencode.py contains a register leak\n")

        if len(self.stack) > 1:
            self.vars, self.r2n, self.n2r, self._tmpi, self.mreg, self.snum, self._globals, self.lineno, self.globals, self.rglobals, self.cregs, self.tmpc = self.stack.pop()
        else:
            self.stack.pop()


def insert(v): D.out.append(v)


def write(v):
    if istype(v, 'list'):
        insert(v)
        return
    for n in range(0, len(v), 4):
        insert(('data', v[n:n+4]))


def setpos(v):
    line, x = v
    if line == D.lineno:
        return
    text = D.lines[line-1]
    D.lineno = line
    val = text + "\0"*(4-len(text) % 4)
    write(val)


def code(i, a=0, b=0, c=0):
    if not istype(i, 'number'):
        raise
    if not istype(a, 'number'):
        raise
    if not istype(b, 'number'):
        raise
    if not istype(c, 'number'):
        raise
    write(('code', i, a, b, c))

def _visit_string(v, tabs):
    r = get_tmp(r)
    val = v + "\0"*(4-len(v) % 4)
    write(val)
    return r


def visit_string(t, tabs):
    return _visit_string(t.val, tabs + 2)


def _visit_number(v, tabs):
    r = get_tmp(r)
    code(NUMBER, r, 0, 0)
    write(fpack(number(v)))
    return r


def visit_number(t, tabs):
    return _visit_number(t.val, r, tabs)


def get_tag():
    k = str(D._tagi)
    D._tagi += 1
    return k


def stack_tag():
    k = get_tag()
    D.tstack.append(k)
    return k


def pop_tag():
    D.tstack.pop()


def tag(*t):
    t = D.snum+':'+':'.join([str(v) for v in t])
    insert(('tag', t))


def jump(*t):
    t = D.snum+':'+':'.join([str(v) for v in t])
    insert(('jump', t))


def setjmp(*t):
    t = D.snum+':'+':'.join([str(v) for v in t])
    insert(('setjmp', t))


def fnc(*t):
    t = D.snum+':'+':'.join([str(v) for v in t])
    r = get_reg(t)
    insert(('fnc', r, t))
    return r


def get_tmp(tabs):
    if r != None:
        return r
    return get_tmps(1)[0]


def get_tmps(t):
    rs = alloc(t)
    regs = range(rs, rs+t)
    for r in regs:
        set_reg(r, "$"+str(D._tmpi))
        D._tmpi += 1
    D.tmpc += t  # REG
    return regs


def alloc(t):
    s = ''.join(["01"[r in D.r2n] for r in range(0, min(256, D.mreg+t))])
    return s.index('0'*t)


def is_tmp(r):
    if r is None:
        return False
    return (D.r2n[r][0] == '$')


def un_tmp(r):
    n = D.r2n[r]
    free_reg(r)
    set_reg(r, '*'+n)


def free_tmp(r):
    if is_tmp(r):
        free_reg(r)
    return r


def free_tmps(r):
    for k in r:
        free_tmp(k)


def get_reg(n):
    if n not in D.n2r:
        set_reg(alloc(1), n)
    return D.n2r[n]


def set_reg(r, n):
    D.n2r[n] = r
    D.r2n[r] = n
    D.mreg = max(D.mreg, r+1)


def free_reg(r):
    if is_tmp(r):
        D.tmpc -= 1
    n = D.r2n[r]
    del D.r2n[r]
    del D.n2r[n]


def imanage(orig, fnc):
    items = orig.items
    orig.val = orig.val[:-1]
    t = Token(orig.pos, 'symbol', '=', [items[0], orig])
    return fnc(t)


def unary(i, tb, tabs):
    r = get_tmp(r)
    b = visit(tb)
    code(i, r, b)
    if r != b:
        free_tmp(b)
    return r


def infix(i, tb, tc, tabs):
    r = get_tmp(r)
    b, c = visit(tb, r), visit(tc)
    code(i, r, b, c)
    if r != b:
        free_tmp(b)
    free_tmp(c)
    return r


def logic_infix(op, tb, tc, tabs):
    t = get_tag()
    r = visit(tb, _r)
    if _r != r:
        free_tmp(_r)  # REG
    if op == 'and':
        code(IF, r)
    elif op == 'or':
        code(IFN, r)
    jump(t, 'end')
    _r = r
    r = visit(tc, _r,tabs+2)
    if _r != r:
        free_tmp(_r)  # REG
    tag(t, 'end')
    return r


def _visit_none(tabs):
    r = get_tmp(r)
    code(NONE, r)
    return r


def visit_symbol(t, tabs):
    print(" "* tabs, "symbol")
    sets = ['=']
    isets = ['+=', '-=', '*=', '/=', '|=', '&=', '^=']
    cmps = ['<', '>', '<=', '>=', '==', '!=']
    metas = {
        '+': ADD, '*': MUL, '/': DIV, '**': POW,
        '-': SUB,
        '%': MOD, '>>': RSH, '<<': LSH,
        '&': BITAND, '|': BITOR, '^': BITXOR,
    }
    if t.val == 'None':
        return _visit_none(r, tabs+2)
    if t.val == 'True':
        return _visit_number('1', r, tabs+2)
    if t.val == 'False':
        return _visit_number('0', r, tabs+2)
    items = t.items

    if t.val in ['and', 'or']:
        return logic_infix(t.val, items[0], items[1], r)
    if t.val in isets:
        return imanage(t, visit_symbol)
    if t.val == 'is':
        return infix(EQ, items[0], items[1], r)
    if t.val == 'isnot':
        return infix(CMP, items[0], items[1], r)
    if t.val == 'not':
        return unary(NOT, items[0], r)
    if t.val == 'in':
        return infix(HAS, items[1], items[0], r)
    if t.val == 'notin':
        r = infix(HAS, items[1], items[0], r)
        zero = _visit_number('0', tabs+2)
        code(EQ, r, r, free_tmp(zero))
        return r
    if t.val in sets:
        return visit_set_ctx(items[0], items[1], tabs+2)
    elif t.val in cmps:
        b, c = items[0], items[1]
        v = t.val
        if v[0] in ('>', '>='):
            b, c, v = c, b, '<'+v[1:]
        cd = EQ
        if v == '<':
            cd = LT
        if v == '<=':
            cd = LE
        if v == '!=':
            cd = NE
        return infix(cd, b, c, r)
    else:
        return infix(metas[t.val], items[0], items[1], r)


def visit_set_ctx(k, v, tabs):
    print(" "* tabs, "set ctx")
    if k.type == 'name':
        if (D._globals and k.val not in D.vars) or (k.val in D.globals):
            c = visit_string(k, tabs+2)
            b = visit(v)
            code(GSET, c, b)
            free_tmp(c)
            free_tmp(b)
            return
        a = visit_local(k, tabs+2)
        b = visit(v)
        code(MOVE, a, b)
        free_tmp(b)
        return a
    elif k.type in ('tuple', 'list'):
        if v.type in ('tuple', 'list'):
            n, tmps = 0, []
            for kk in k.items:
                vv = v.items[n]
                tmp = get_tmp()
                tmps.append(tmp)
                r = visit(vv)
                code(MOVE, tmp, r)
                free_tmp(r)  # REG
                n += 1
            n = 0
            for kk in k.items:
                vv = v.items[n]
                tmp = tmps[n]
                free_tmp(visit_set_ctx(kk, Token(vv.pos, 'reg', tmp), tabs+2))
                n += 1
            return

        r = visit(v)
        un_tmp(r)
        n, tmp = 0, Token(v.pos, 'reg', r)
        for tt in k.items:
            free_tmp(visit_set_ctx(tt, Token(tmp.pos, 'get', None, [
                     tmp, Token(tmp.pos, 'number', str(n))], tabs+2)))
            n += 1
        free_reg(r)
        return
    r = visit(k.items[0], tabs+2)
    rr = visit(v)
    tmp = visit(k.items[1],tabs+2)
    code(SET, r, tmp, rr)
    free_tmp(r)  
    free_tmp(tmp)  
    return rr


def manage_seq(i, a, items, sav=0):
    l = max(sav, len(items))
    n, tmps = 0, get_tmps(l)
    for tt in items:
        r = tmps[n]
        b = visit(tt, r)
        if r != b:
            code(MOVE, r, b)
            free_tmp(b)
        n += 1
    if not len(tmps):
        code(i, a, 0, 0)
        return 0
    code(i, a, tmps[0], len(items))
    free_tmps(tmps[sav:])
    return tmps[0]


def p_filter(items):
    a, b, c, d = [], [], None, None
    for t in items:
        if t.type == 'symbol' and t.val == '=':
            b.append(t)
        elif t.type == 'args':
            c = t
        elif t.type == 'nargs':
            d = t
        else:
            a.append(t)
    return a, b, c, d


def visit_import(t, tabs):
    print(" "* tabs, "import")
    for mod in t.items:
        mod.type = 'string'
        v = visit_call(Token(t.pos, 'call', None, [
            Token(t.pos, 'name', 'import'),
            mod]), tabs + 2)
        mod.type = 'name'
        visit_set_ctx(mod, Token(t.pos, 'reg', v), tabs + 2)


def visit_from(t, tabs):
    print(" "* tabs, "from")
    mod = t.items[0]
    mod.type = 'string'
    v = visit(Token(t.pos, 'call', None, [
        Token(t.pos, 'name', 'import'),
        mod]), tabs + 2)
    item = t.items[1]
    if item.val == '*':
        free_tmp(visit(Token(t.pos, 'call', None, [
            Token(t.pos, 'name', 'merge'),
            Token(t.pos, 'name', '__dict__'),
            Token(t.pos, 'reg', v)], tabs + 2)))  # REG
    else:
        item.type = 'string'
        free_tmp(visit_set_ctx(
            Token(t.pos, 'get', None, [
                  Token(t.pos, 'name', '__dict__'), item]),
            Token(t.pos, 'get', None, [Token(t.pos, 'reg', v), item]),
            tabs + 2
        ))  # REG


def visit_globals(t, tabs):
    print(" "* tabs, "global")
    for t in t.items:
        if t.val not in D.globals:
            D.globals.append(t.val)


def visit_del(tt, tabs):
    print(" "* tabs, "del")
    for t in tt.items:
        r = visit(t.items[0])
        r2 = visit(t.items[1])
        code(DEL, r, r2)
        free_tmp(r)
        free_tmp(r2)  # REG


def visit_call(t, tabs):
    print(" "* tabs, "call")
    r = get_tmp(r)
    items = t.items
    fnc = visit(items[0])
    a, b, c, d = p_filter(t.items[1:])
    e = None
    if len(b) != 0 or d != None:
        e = visit(Token(t.pos, 'dict', None, []))
        un_tmp(e)
        for p in b:
            p.items[0].type = 'string'
            t1, t2 = visit(p.items[0]), visit(p.items[1])
            code(SET, e, t1, t2)
            free_tmp(t1)
            free_tmp(t2)  # REG
        if d:
            free_tmp(visit(Token(t.pos, 'call', None, [
                     Token(t.pos, 'name', 'merge'), Token(t.pos, 'reg', e), d.items[0]])))  # REG
    manage_seq(PARAMS, r, a)
    if c != None:
        t1, t2 = _visit_string('*', tabs + 2), visit(c.items[0])
        code(SET, r, t1, t2)
        free_tmp(t1)
        free_tmp(t2)  # REG
    if e != None:
        t1 = _visit_none( tabs + 2)
        code(SET, r, t1, e)
        free_tmp(t1)  # REG
    code(CALL, r, fnc, r)
    free_tmp(fnc)  # REG
    return r


def visit_name(t, tabs):
    if t.val in D.vars:
        return visit_local(t, r,  tabs +2)
    if t.val not in D.rglobals:
        D.rglobals.append(t.val)
    r = get_tmp(r)
    c = visit_string(t,  tabs +2)
    code(GGET, r, c)
    free_tmp(c)
    return r


def visit_local(t, tabs):
    print(" "* tabs, "local")
    if t.val in D.rglobals:
        D.error = True
    if t.val not in D.vars:
        D.vars.append(t.val)
    return get_reg(t.val)


def visit_def(tok, tabs, kls=None):
    items = tok.items

    t = get_tag()
    rf = fnc(t, 'end')

    D.begin()
    setpos(tok.pos)
    r = visit_local(Token(tok.pos, 'name', '__params'), tabs + 2)
    visit_info(tabs + 2, items[0].val)
    a, b, c, d = p_filter(items[1].items)
    for p in a:
        v = visit_local(p, tabs + 2)
        tmp = _visit_none(tabs + 2)
        code(GET, v, r, tmp)
        free_tmp(tmp)  # REG
    for p in b:
        v = visit_local(p.items[0], tabs + 2)
        visit(p.items[1], v)
        tmp = _visit_none(tabs + 2)
        code(IGET, v, r, tmp)
        free_tmp(tmp)  # REG
    if c != None:
        v = visit_local(c.items[0], tabs + 2)
        tmp = _visit_string('*', tabs + 2)
        code(GET, v, r, tmp)
        free_tmp(tmp)  # REG
    if d != None:
        e = visit_local(d.items[0], tabs + 2)
        code(DICT, e, 0, 0)
        tmp = _visit_none(tabs + 2)
        code(IGET, e, r, tmp)
        free_tmp(tmp)  # REG
    free_tmp(visit(items[2]))  # REG
    D.end()

    tag(t, 'end')

    if kls == None:
        if D._globals:
            visit_globals(Token(tok.pos, 0, 0, [items[0]]), tabs + 2)
        r = visit_set_ctx(items[0], Token(tok.pos, 'reg', rf), tabs + 2)
    else:
        rn = visit_string(items[0], tabs +2)
        code(SET, kls, rn, rf)
        free_tmp(rn)

    free_tmp(rf)


def visit_class(t):
    tok = t
    items = t.items
    parent = None
    if items[0].type == 'name':
        name = items[0].val
        parent = Token(tok.pos, 'name', 'object')
    else:
        name = items[0].items[0].val
        parent = items[0].items[1]

    kls = visit(Token(t.pos, 'dict', 0, []))
    un_tmp(kls)
    ts = _visit_string(name)
    code(GSET, ts, kls)
    free_tmp(ts)  # REG

    free_tmp(visit(Token(tok.pos, 'call', None, [
        Token(tok.pos, 'name', 'setmeta'),
        Token(tok.pos, 'reg', kls),
        parent])))

    for member in items[1].items:
        if member.type == 'def':
            visit_def(member, kls)
        elif member.type == 'symbol' and member.val == '=':
            visit_classvar(member, kls)
        else:
            continue

    free_reg(kls)  # REG


def visit_classvar(t, r):
    var = visit_string(t.items[0])
    val = visit(t.items[1])
    code(SET, r, var, val)
    free_reg(var)
    free_reg(val)


def visit_while(t):
    items = t.items
    t = stack_tag()
    tag(t, 'begin')
    tag(t, 'continue')
    r = visit(items[0])
    code(IF, r)
    free_tmp(r)  # REG
    jump(t, 'end')
    free_tmp(visit(items[1]))  # REG
    jump(t, 'begin')
    tag(t, 'break')
    tag(t, 'end')
    pop_tag()


def visit_for(tok, tabs):
    print(" "* tabs, "for")
    items = tok.items

    reg = visit_local(items[0], tabs + 2)
    itr = visit(items[1])
    i = _visit_number('0')

    t = stack_tag()
    tag(t, 'loop')
    tag(t, 'continue')
    code(ITER, reg, itr, i)
    jump(t, 'end')
    free_tmp(visit(items[2]))  # REG
    jump(t, 'loop')
    tag(t, 'break')
    tag(t, 'end')
    pop_tag()

    free_tmp(itr)  # REG
    free_tmp(i)


def visit_comp(t, tabs):
    print(" "* tabs, "comp")
    name = 'comp:'+get_tag()
    r = visit_local(Token(t.pos, 'name', name), tabs + 2)
    code(LIST, r, 0, 0)
    key = Token(t.pos, 'get', None, [
        Token(t.pos, 'reg', r),
        Token(t.pos, 'symbol', 'None')])
    ap = Token(t.pos, 'symbol', '=', [key, t.items[0]])
    visit(Token(t.pos, 'for', None, [t.items[1], t.items[2], ap]), tabs)
    return r


def visit_if(t, tabs):
    print(" "* tabs, "if")
    items = t.items
    t = get_tag()
    n = 0
    for tt in items:
        tag(t, n)
        if tt.type == 'elif':
            a = visit(tt.items[0])
            code(IF, a)
            free_tmp(a)
            jump(t, n+1)
            free_tmp(visit(tt.items[1], tabs))  # REG
        elif tt.type == 'else':
            free_tmp(visit(tt.items[0], tabs))  # REG
        else:
            raise
        jump(t, 'end')
        n += 1
    tag(t, n)
    tag(t, 'end')


def visit_try(t, tabs):
    print(" "* tabs, "try")
    items = t.items
    t = get_tag()
    setjmp(t, 'except')
    free_tmp(visit(items[0]))  # REG
    code(SETJMP, 0)
    jump(t, 'end')
    tag(t, 'except')
    free_tmp(visit(items[1].items[1]))  # REG
    tag(t, 'end')


def visit_return(t, tabs):
    print(" "* tabs, "return")
    if t.items:
        r = visit(t.items[0], tabs +2)
    else:
        r = _visit_none(tabs + 2)
    code(RETURN, r)
    free_tmp(r)
    return


def visit_raise(t, tabs):
    print(" "* tabs, "raise")
    if t.items:
        r = visit(t.items[0], tabs + 2 )
    else:
        r = _visit_none(tabs + 2)
    code(RAISE, r)
    free_tmp(r)
    return


def visit_statements(t, tabs):
    print(" "* tabs, "statements")
    for tt in t.items:
        free_tmp(visit(tt))


def visit_list(t, tabs):
    print(" "* tabs, "list")
    r = get_tmp(r)
    manage_seq(LIST, r, t.items)
    return r


def visit_dict(t, tabs):
    print(" "* tabs, "dict")
    r = get_tmp(r)
    manage_seq(DICT, r, t.items)
    return r


def visit_get(t, tabs):
    items = t.items
    print(" "* tabs, "get")
    return infix(GET, items[0], items[1], r)


def visit_break(t, tags): 
    print(" "* tags, "break")
    jump(D.tstack[-1], 'break')

def visit_continue(t, tags): 
    print(" "* tags, "continue")
    jump(D.tstack[-1], 'continue')

def visit_pass(t, tags): 
    print(" "* tags, "pass")
    code(PASS)


def visit_info(tabs, name='?'):
    print(" " * tabs, "info")
    code(FILE, free_tmp(_visit_string(D.fname, tabs +2 )))
    code(NAME, free_tmp(_visit_string(name, tabs + 2)))


def visit_module(t, tabs):
    print(" "* tabs, "module")
    visit_info(tabs + 2)
    free_tmp(visit(t.items[0], tag + 2))  # REG


def visit_reg(t, tabs): 
    print(" " * int(tabs), "reg", t.val)
    return t.val


fmap = {
    'module': visit_module, 'statements': visit_statements, 'def': visit_def,
    'return': visit_return, 'while': visit_while, 'if': visit_if,
    'break': visit_break, 'pass': visit_pass, 'continue': visit_continue, 'for': visit_for,
    'class': visit_class, 'raise': visit_raise, 'try': visit_try, 'import': visit_import,
    'globals': visit_globals, 'del': visit_del, 'from': visit_from,
}
rmap = {
    'list': visit_list, 'tuple': visit_list, 'dict': visit_dict, 'slice': visit_list,
    'comp': visit_comp, 'name': visit_name, 'symbol': visit_symbol, 'number': visit_number,
    'string': visit_string, 'get': visit_get, 'call': visit_call, 'reg': visit_reg,
}


def visit(t, tabs):
    if t.pos:
        setpos(t.pos)
    try:
        if t.type in rmap:
            return rmap[t.type](t, tabs)
        return fmap[t.type](t, tabs)
    except:
        if D.error:
            raise
        D.error = True
        tokenize.u_error('encode', D.code, t.pos)


def genTree(fname, s):
    tokens = tokenize.tokenize(s)
    t = parse.parse(s, tokens)
    t = Token((1, 1), 'module', 'module', [t])
    global D
    s = tokenize.clean(s)
    D = DState(s, fname)
    D.begin(True)
    visit(fname, tabs = 2)
    D.end()
