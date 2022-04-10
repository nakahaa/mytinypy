import os
import sys


TOPDIR = os.path.abspath(os.path.dirname(__file__))

HELP = """
python build.py mypy
python build.py clean
"""

def main():
    if len(sys.argv) < 2:
        print(HELP)
        return

    for mod in ['tokenize','parse','encode','py2bc']: os.system('cd mypy/; python2 py2bc.py {} {} -nopos '.format( mod + ".py", mod + ".tpc" ))

    cmd = sys.argv[1]
    if cmd == "mypy":
        os.system('gcc -std=c99 -Wall -Wc++-compat  -O3 mypy/mymain.c -lm -o mypy/mypy')
    elif cmd == "clean":
        os.system("rm -f mypy/*.pyc; rm -f mypy/*.tpc; rm -rf mypy/mypy")
    else:
        print('invalid command')
    
if __name__ == '__main__':
    main()
