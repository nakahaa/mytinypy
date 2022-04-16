import os
import sys


TOPDIR = os.path.abspath(os.path.dirname(__file__))

HELP = """
python build.py mypy
python build.py clean
"""

def main():
    # 读出命令行参数，如果小于 2， 就输出帮助信息
    if len(sys.argv) < 2:
        print(HELP)
        return
    
    # 执行一段 shell 代码， python2 py2bc.py  tokenize.py tokenize.tpc
    for mod in ['tokenize','parse','encode','py2bc']: os.system('cd mypy/; python2 py2bc.py {} {} -nopos '.format( mod + ".py", mod + ".tpc" ))

    cmd = sys.argv[1]
    if cmd == "mypy":
        # 调用 gcc 编译 Mypy, gcc -std=c99 -Wall -Wc++-compat  -O3 mypy/main.c -lm -o mypy/mypy
        os.system('gcc -std=c99 -Wall -Wc++-compat  -O3 mypy/main.c -lm -o mypy/mypy')
    elif cmd == "clean":
        # rm -f: 移除文件，* 通配符，代表匹配所有文件，后缀为 tpc， 同时移除 mypy/mypy
        os.system("rm -f mypy/*.pyc; rm -f mypy/*.tpc; rm -rf mypy/mypy")
    else:
        print('invalid command')
    
if __name__ == '__main__':
    main()
