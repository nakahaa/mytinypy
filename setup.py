import os
import sys

VARS = {'$CPYTHON':''}
TOPDIR = os.path.abspath(os.path.dirname(__file__))
TEST = False
CLEAN = False
BOOT = False
DEBUG = False
VALGRIND = False
SANDBOX = False
CORE = ['tokenize','parse','encode','py2bc']
MODULES = []

def main():
    chksize()
    if len(sys.argv) < 2:
        print HELP
        return
    
    global TEST,CLEAN,BOOT,DEBUG,VALGRIND,SANDBOX
    TEST = 'test' in sys.argv
    CLEAN = 'clean' in sys.argv
    BOOT = 'boot' in sys.argv
    DEBUG = 'debug' in sys.argv
    VALGRIND = 'valgrind' in sys.argv
    SANDBOX = 'sandbox' in sys.argv
    CLEAN = CLEAN or BOOT
    TEST = TEST or BOOT
        
    get_libs()
    build_mymain()

    for mod in ['tokenize','parse','encode','py2bc']: os.system('cd tinypy/; python2 py2bc.py {} {} -nopos '.format( mod + ".py", mod + ".tpc" ))

    cmd = sys.argv[1]
    if cmd == "mypy":
        
        build_gcc()
    else:
        print 'invalid command'

HELP = """
python setup.py mypy
"""

def vars_linux():
    VARS['$RM'] = 'rm -f'
    VARS['$VM'] = './vm'
    VARS['$TINYPY'] = './tinypy'
    VARS['$SYS'] = '-linux'
    VARS['$FLAGS'] = ''
    
    VARS['$WFLAGS'] = '-std=c89 -Wall -Wc++-compat'
    #-Wwrite-strings - i think this is included in -Wc++-compat
    
    if 'pygame' in MODULES:
        VARS['$FLAGS'] += ' `sdl-config --cflags --libs` '

    if SANDBOX:
        VARS['$SYS'] += " -sandbox "
        VARS['$FLAGS'] += " -DTP_SANDBOX "

def do_cmd(cmd):
    for k,v in VARS.items():
        cmd = cmd.replace(k,v)
    if '$' in cmd:
        print 'vars_error',cmd
        sys.exit(-1)
    if VALGRIND and (cmd.startswith("./") or cmd.startswith("../")):
        cmd = "valgrind " + cmd
    
    print cmd
    r = os.system(cmd)
    if r:
        print 'exit_status',r
        sys.exit(r)
        
def do_chdir(dest):
    print 'cd',dest
    os.chdir(dest)

def build_bc(opt=False):
    out = []
    for mod in CORE:
        out.append("""unsigned char tp_%s[] = {"""%mod)
        fname = mod+".tpc"
        data = open(fname,'rb').read()
        cols = 16
        for n in xrange(0,len(data),cols):
            out.append(",".join([str(ord(v)) for v in data[n:n+cols]])+',')
        out.append("""};""")
    out.append("")
    f = open('bc.c','wb')
    f.write('\n'.join(out))
    f.close()
    
def open_tinypy(fname,*args):
    return open(os.path.join(TOPDIR,'tinypy',fname),*args)
                
def py2bc(cmd,mod):
    src = '%s.py'%mod
    dest = '%s.tpc'%mod
    if CLEAN or not os.path.exists(dest) or os.stat(src).st_mtime > os.stat(dest).st_mtime:
        cmd = cmd.replace('$SRC',src)
        cmd = cmd.replace('$DEST',dest)
        do_cmd(cmd)
    else:
        print '#',dest,'is up to date'

def build_gcc():
    do_chdir(os.path.join(TOPDIR,'tinypy'))
    build_bc(True)
    if BOOT:
        do_cmd("gcc $WFLAGS -O2 tpmain.c $FLAGS -lm -o tinypy")
        do_cmd('$TINYPY tests.py $SYS')
        print("# OK - we'll try -O3 for extra speed ...")
        do_cmd("gcc $WFLAGS -O3 tpmain.c $FLAGS -lm -o tinypy")
        do_cmd('$TINYPY tests.py $SYS')
    if DEBUG:
        do_cmd("gcc $WFLAGS -g mymain.c $FLAGS -lm -o ../tinypy/tinypy")
    else:
        do_cmd("gcc $WFLAGS -O3 mymain.c $FLAGS -lm -o ../tinypy/tinypy")
    
    do_chdir('..')
    print("# OK")
    
def get_libs():
    modules = os.listdir('modules')
    for m in modules[:]:
        if m not in sys.argv: modules.remove(m)
    global MODULES
    MODULES = modules

def build_mymain():
    src = os.path.join(TOPDIR,'tinypy','tpmain.c')
    out = open(src,'r').read()
    dest = os.path.join(TOPDIR,'tinypy','mymain.c')
        
    vs = []
    for m in MODULES:
        vs.append('#include "../modules/%s/init.c"'%m)
    out = out.replace('/* INCLUDE */','\n'.join(vs))
    
    vs = []
    for m in MODULES:
        vs.append('%s_init(tp);'%m)
    out = out.replace('/* INIT */','\n'.join(vs))
    
    f = open(dest,'w')
    f.write(out)
    f.close()
    return True
    
def test_mods(cmd):
    for m in MODULES:
        tests = os.path.join('..','modules',m,'tests.py')
        if not os.path.exists(tests): continue
        cmd = cmd.replace('$TESTS',tests)
        do_cmd(cmd)


def shrink(fname):
    f = open(fname,'r'); lines = f.readlines(); f.close()
    out = []
    fixes = [
    'vm','gc','params','STR',
    'int','float','return','free','delete','init',
    'abs','round','system','pow','div','raise','hash','index','printf','main']
    passing = False
    for line in lines:
        #quit if we've already converted
        if '\t' in line: return ''.join(lines)
        
        #change "    " into "\t" and remove blank lines
        if len(line.strip()) == 0: continue
        line = line.rstrip()
        l1,l2 = len(line),len(line.lstrip())
        line = "\t"*((l1-l2)/4)+line.lstrip()
        
        #remove comments
        if '.c' in fname or '.h' in fname:
            #start block comment
            if line.strip()[:2] == '/*':
                passing = True;
            #end block comment
            if line.strip()[-2:] == '*/':
               passing = False;
               continue
            #skip lines inside block comments
            if passing:
                continue
        if '.py' in fname:
            if line.strip()[:1] == '#': continue
        
        #remove the "namespace penalty" from tinypy ...
        for name in fixes:
            line = line.replace('TP_'+name,'t'+name)
            line = line.replace('tp_'+name,'t'+name)
        line = line.replace('TP_','')
        line = line.replace('tp_','')
        
        out.append(line)
    return '\n'.join(out)+'\n'
    
def chksize():
    t1,t2 = 0,0
    for fname in [
        'tokenize.py','parse.py','encode.py','py2bc.py',
        'tp.h','list.c','dict.c','misc.c','string.c','builtins.c',
        'gc.c','ops.c','vm.c','tp.c','tpmain.c',
        ]:
        fname = os.path.join(TOPDIR,'tinypy',fname)
        f = open(fname,'r'); t1 += len(f.read()); f.close()
        txt = shrink(fname)
        t2 += len(txt)
    print "#",t1,t2,t2-65536
    return t2
    
if __name__ == '__main__':
    main()
