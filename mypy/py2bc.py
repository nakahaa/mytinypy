import sys

import tokenize
import parse
import encode

global FTYPE
f = open('type.h', 'r').read()
FTYPE = 'f'
if 'double tp_num' in f:
    FTYPE = 'd'

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

def _import(name):
    if name in MODULES:
        return MODULES[name]
    py = name+".py"
    tpc = name+".tpc"
    if exists(py):
        if not exists(tpc) or mtime(py) > mtime(tpc):
            s = load(py)
            code, _ = _compile(s, py)
            save(tpc, code)
    if not exists(tpc):
        raise
    code = load(tpc)
    g = {'__name__': name, '__code__': code}
    g['__dict__'] = g
    MODULES[name] = g
    exec(code, g)
    return g


def _init():
    BUILTINS['compile'] = _compile
    BUILTINS['import'] = _import


def import_fname(fname, name):
    g = {}
    g['__name__'] = name
    MODULES[name] = g
    s = load(fname)
    code, _ = _compile(s, fname)
    g['__code__'] = code
    exec(code, g)
    return g


def tinypy():
    return import_fname(ARGV[0], '__main__')


def main(src, dest):
    s = load(src)
    r, _ = _compile(s, src)
    save(dest, r)

def _dumptree(s, fname):
    
    tokens = tokenize.tokenize(s)
    t = parse.parse(s, tokens)
    print("syntax Tree: ")
    visitTree(t, 0)

def visitTree(t, tabs):
    print("{} type={} val={}".format( tabs * " ",t.type, t.val))
    if t.items is None:
        return

    for item in t.items:
        if item is not None:
            visitTree(item, tabs + 2) 



def dumptrees(src):
    print("visit_")
    s = load(src)
    _dumptree(s, src)

def _compile(s, fname):
    tokens = tokenize.tokenize(s)
    t = parse.parse(s, tokens)
    r, tmpCode = encode.encode(fname, s, t)
    return r, tmpCode

def genTokens(src):
    print("----------------------gen tokens--------------------------")
    # 打开 src 文件
    s = load(src)
    # 实际上调用 tokenize.py 内部的方法
    tokens = tokenize.tokenize(s)
    for token in tokens:
        print(token.pos, token.type, token.val )

def genCode(src):
    print("visit_")
    s = load(src)
    _, genCodes = _compile(s, src)
    for code in genCodes:
        print( code )

if __name__ == '__main__':
    # 输出 目标代码
    if sys.argv[2] == "gencode":
        genCode(sys.argv[1])

    # 输出 语法分析树
    if sys.argv[2] == "dumptree":
        dumptrees(sys.argv[1])

    # 输出 词法分析流
    if sys.argv[2] == "gentokens":
        genTokens(sys.argv[1])