# mypy

mypy interpreter

## Build

1. cd mypy/
2. python build.py mypy
3. 输出可执行文件为: mypy/mypy  

## clean （可选）
1. python build.py clean

## 使用目标代码
1. cd mypy/mypy
2. python py2bc.py ../examples/add.py gencode

## 输出词法分析流
1. cd mypy/mypy
2. python py2bc.py ../examples/add.py gentokens


## 输出语法树
1. cd mypy/mypy
2. python py2bc.py ../examples/add.py dumptree


## mypy 使用 
    1. cd mypy/mypy
    2. ./mypy ../examples/add.py

#### examples 说明
    1. examples/for.py : for循环
    2. examples/while.py: while循环
    3. examples/class.py: python自定义类型
    4. examples/add.py: 浮点型加法运算
    5. examples/func.py:  测试方法调用
    6. examples/subclass.py: 测试 class 继承


### py2bc 流程
    1. 词法分析
        tokenize.py -> do_tokenize()  -> do_symbol / do_name / do_number ... -> 最终返回一串符号表
    2. 语法分析
        parse.py 是语法分析程序， 接受 do_tokenize 返回的 tokens.

    3. 目标代码
        1. encode.py -> encode() -> do()（总入口） -> 通过 rmap/ fmap 查找到 语法单元对应处理逻辑
        2. rmap -> 基本类型
        3. fmap-> 对应方法之类的东西 
        4. item -> 子 items -> 自定向下进行扫描

### 运行流程
     tp_init 虚拟机初始化 -> py2bc (词法，语法，目标代码 ) -> ezcall -> callfunc -> runFunc -> _run -> -> stepFunc （定义了各种目标代码如何执行） 
     -> 销毁虚拟机
