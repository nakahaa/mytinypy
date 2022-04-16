#include "type.c"

int main(int argc, char *argv[])
{
    // 虚拟机的初始化
    VmType *tp = tp_init(argc, argv);

    // 先调用 py2bc.py 执行语法分析，目标代码生成，然后 ezcall 拿到目标代码，挨个执行，等待结束
    ezCall(tp, "py2bc", "tinypy", NONE);
    // 虚拟机的销毁，比如清理内存之类的事情
    deinit(tp);
    return (0);
}
