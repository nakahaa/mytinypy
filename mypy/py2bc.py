import sys
# if not "tinypy" in sys.version:
#     from boot import *

import tokenize,parse,encode

def _boot_init():
    global FTYPE 
    f = open('tp.h','r').read()
    FTYPE = 'f'
    if 'double tp_num' in f: FTYPE = 'd'
    import sys
    global ARGV
    ARGV = sys.argv
_boot_init()

def merge(a,b):
    if isinstance(a,dict):
        for k in b: a[k] = b[k]
    else:
        for k in b: setattr(a,k,b[k])

def number(v):
    if type(v) is str and v[0:2] == '0x':
        v = int(v[2:],16)
    return float(v)

def istype(v,t):
    if t == 'string': return isinstance(v,str)
    elif t == 'list': return (isinstance(v,list) or isinstance(v,tuple))
    elif t == 'dict': return isinstance(v,dict)
    elif t == 'number': return (isinstance(v,float) or isinstance(v,int))
    raise '?'

def fpack(v):
    import struct
    return struct.pack(FTYPE,v)

def system(cmd):
    import os
    return os.system(cmd)

def load(fname):
    f = open(fname,'rb')
    r = f.read()
    f.close()
    return r

def save(fname,v):
    f = open(fname,'wb')
    f.write(v)
    f.close()
    
def _compile(s,fname):
    tokens = tokenize.tokenize(s)
    t = parse.parse(s,tokens)
    r = encode.encode(fname,s,t)
    return r

def _import(name):
    if name in MODULES:
        return MODULES[name]
    py = name+".py"
    tpc = name+".tpc"
    if exists(py):
        if not exists(tpc) or mtime(py) > mtime(tpc):
            s = load(py)
            code = _compile(s,py)
            save(tpc,code)
    if not exists(tpc): raise
    code = load(tpc)
    g = {'__name__':name,'__code__':code}
    g['__dict__'] = g
    MODULES[name] = g
    exec(code,g)
    return g
    
    
def _init():
    BUILTINS['compile'] = _compile
    BUILTINS['import'] = _import

def import_fname(fname,name):
    g = {}
    g['__name__'] = name
    MODULES[name] = g
    s = load(fname)
    code = _compile(s,fname)
    g['__code__'] = code
    exec(code,g)
    return g

def tinypy():
    return import_fname(ARGV[0],'__main__')

def main(src,dest):
    s = load(src)
    r = _compile(s,src)
    save(dest,r)

if __name__ == '__main__':
    main(ARGV[1],ARGV[2])
    main(ARGV[1],ARGV[2])
    main(ARGV[1],ARGV[2])
    main(ARGV[1],ARGV[2])
    main(ARGV[1],ARGV[2])
    main(ARGV[1],ARGV[2])
    main(ARGV[1],ARGV[2])
    main(ARGV[1],ARGV[2])
