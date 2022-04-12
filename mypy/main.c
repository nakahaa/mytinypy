#include "tp.c"

int main(int argc, char *argv[]) {
    VmType *tp = tp_init(argc,argv);
    
    tp_ez_call(tp,"py2bc","tinypy", tp_None);
    tp_deinit(tp);
    return(0);
}
