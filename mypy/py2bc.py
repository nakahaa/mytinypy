import sys

import tokenize
import parse
import encode
import dumptree

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

def _dumptree(src):
    s = load(src)
    # dumptree.genTree(s, src)
    tokens = tokenize.tokenize(s)
    t = parse.parse(s, tokens)
    print(t)

def _compile(s, fname):
    tokens = tokenize.tokenize(s)
    t = parse.parse(s, tokens)
    r, tmpCode = encode.encode(fname, s, t)
    return r, tmpCode

def genCode(src):
    s = load(src)
    _, genCodes = _compile(s, src)
    for code in genCodes:
        print( code )

if __name__ == '__main__':
    if sys.argv[2] == "gencode":
        genCode(sys.argv[1])

    if sys.argv[2] == "dumpCode":
        main(sys.argv[1], "tmpCode")

    if sys.argv[2] == "dumptree":
        _dumptree(sys.argv[1])