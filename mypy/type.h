#ifndef TP_H
#define TP_H

#include <setjmp.h>
#include <sys/stat.h>
#ifndef __USE_ISOC99
#define __USE_ISOC99
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>

#ifdef __GNUC__
#define tp_inline __inline__
#endif

#ifdef _MSC_VER
#ifdef NDEBUG
#define tp_inline __inline
#else
/* don't inline in debug builds (for easier debugging) */
#define tp_inline
#endif
#endif

#ifndef tp_inline
#error "Unsuported compiler"
#endif

enum {
    NONETYPE,TP_NUMBER,TP_STRING,DICTTYPE,
    LISTTYPE,FUNCTYPE,DATATYPE,
};

typedef double tp_num;

typedef struct NumberType {
    int type;
    tp_num val;
} NumberType;
typedef struct StringType {
    int type;
    struct _string *info;
    char const *val;
    int len;
} StringType;
typedef struct ListType {
    int type;
    struct _list *val;
} ListType;
typedef struct DictType {
    int type;
    struct _dict *val;
    int dtype;
} DictType;
typedef struct FuncType {
    int type;
    struct _func *info;
    int ftype;
    void *cfnc;
} FuncType;
typedef struct DataType {
    int type;
    struct _tp_data *info;
    void *val;
    int magic;
} DataType;

typedef union ObjType {
    int type;
    NumberType number;
    struct { int type; int *data; } gci;
    StringType string;
    DictType dict;
    ListType list;
    FuncType fnc;
    DataType data;
} ObjType;

typedef struct _string {
    int gci;
    int len;
    char s[1];
} _string;
typedef struct _list {
    int gci;
    ObjType *items;
    int len;
    int alloc;
} _list;
typedef struct ItemType {
    int used;
    int hash;
    ObjType key;
    ObjType val;
} ItemType;
typedef struct _dict {
    int gci;
    ItemType *items;
    int len;
    int alloc;
    int cur;
    int mask;
    int used;
    ObjType meta;
} _dict;
typedef struct _func {
    int gci;
    ObjType self;
    ObjType globals;
    ObjType code;
} _func;


typedef union CodeType {
    unsigned char i;
    struct { unsigned char i,a,b,c; } regs;
    struct { char val[4]; } string;
    struct { float val; } number;
} CodeType;

typedef struct FrameType {
    ObjType code;
    CodeType *cur;
    CodeType *jmp;
    ObjType *regs;
    ObjType *ret_dest;
    ObjType fname;
    ObjType name;
    ObjType line;
    ObjType globals;
    int lineno;
    int cregs;
} FrameType;

#define TP_GCMAX 4096
#define TP_FRAMES 256
#define TP_REGS_EXTRA 2
/* #define TP_REGS_PER_FRAME 256*/
#define TP_REGS 16384

typedef struct VmType {
    ObjType builtins;
    ObjType modules;
    FrameType frames[TP_FRAMES];
    ObjType _params;
    ObjType params;
    ObjType _regs;
    ObjType *regs;
    ObjType root;
    jmp_buf buf;
#ifdef CPYTHON_MOD
    jmp_buf nextexpr;
#endif
    int jmp;
    ObjType ex;
    char chars[256][2];
    int cur;
    _list *white;
    _list *grey;
    _list *black;
    int steps;
    clock_t clocks;
    double time_elapsed;
    double time_limit;
    unsigned long mem_limit;
    unsigned long mem_used;
    int mem_exceeded;
} VmType;

#define TP VmType *tp
typedef struct _tp_data {
    int gci;
    void (*free)(TP,ObjType);
} _tp_data;

#define tp_True tp_number(1)
#define tp_False tp_number(0)

extern ObjType tp_None;

#ifdef TP_SANDBOX
void *tp_malloc(TP, unsigned long);
void *tp_realloc(TP, void *, unsigned long);
void tp_free(TP, void *);
#else
#define tp_malloc(TP,x) calloc((x),1)
#define tp_realloc(TP,x,y) realloc(x,y)
#define tp_free(TP,x) free(x)
#endif

void tp_sandbox(TP, double, unsigned long);
void tp_time_update(TP);
void tp_mem_update(TP);

void tp_run(TP,int cur);
void tp_set(TP,ObjType,ObjType,ObjType);
ObjType tp_get(TP,ObjType,ObjType);
ObjType tp_has(TP,ObjType self, ObjType k);
ObjType tp_len(TP,ObjType);
void tp_del(TP,ObjType,ObjType);
ObjType tp_str(TP,ObjType);
int tp_bool(TP,ObjType);
int compare(TP,ObjType,ObjType);
void _tp_raise(TP,ObjType);
ObjType _printf(TP,char const *fmt,...);
ObjType tp_track(TP,ObjType);
void tp_grey(TP,ObjType);
ObjType tp_call(TP, ObjType fnc, ObjType params);
ObjType tp_add(TP,ObjType a, ObjType b) ;


#define tp_raise(r,v) { \
    _tp_raise(tp,v); \
    return r; \
}


tp_inline static ObjType tp_string(char const *v) {
    ObjType val;
    StringType s = {TP_STRING, 0, v, 0};
    s.len = strlen(v);
    val.string = s;
    return val;
}

#define TP_CSTR_LEN 256

tp_inline static void tp_cstr(TP,ObjType v, char *s, int l) {
    if (v.type != TP_STRING) { 
        tp_raise(,tp_string("(tp_cstr) TypeError: value not a string"));
    }
    if (v.string.len >= l) {
        tp_raise(,tp_string("(tp_cstr) TypeError: value too long"));
    }
    memset(s,0,l);
    memcpy(s,v.string.val,v.string.len);
}


#define TP_OBJ() (tp_get(tp,tp->params,tp_None))
tp_inline static ObjType tp_type(TP,int t,ObjType v) {
    if (v.type != t) { tp_raise(tp_None,tp_string("(tp_type) TypeError: unexpected type")); }
    return v;
}



#define TP_NO_LIMIT 0
#define TP_TYPE(t) tp_type(tp,t,TP_OBJ())
#define TP_NUM() (TP_TYPE(TP_NUMBER).number.val)
#define TP_STR() (TP_TYPE(TP_STRING))
#define TP_DEFAULT(d) (tp->params.list.val->len?tp_get(tp,tp->params,tp_None):(d))

#define TP_LOOP(e) \
    int __l = tp->params.list.val->len; \
    int __i; for (__i=0; __i<__l; __i++) { \
    (e) = get_list(tp,tp->params.list.val,__i,"TP_LOOP");
#define TP_END \
    }

tp_inline static int _tp_min(int a, int b) { return (a<b?a:b); }
tp_inline static int _tp_max(int a, int b) { return (a>b?a:b); }
tp_inline static int _tp_sign(tp_num v) { return (v<0?-1:(v>0?1:0)); }

tp_inline static ObjType tp_number(tp_num v) {
    ObjType val = {TP_NUMBER};
    val.number.val = v;
    return val;
}

tp_inline static void tp_echo(TP,ObjType e) {
    e = tp_str(tp,e);
    fwrite(e.string.val,1,e.string.len,stdout);
}

tp_inline static ObjType tp_string_n(char const *v,int n) {
    ObjType val;
    StringType s = {TP_STRING, 0,v,n};
    val.string = s;
    return val;
}


ObjType _tp_dcall(TP,ObjType fnc(TP)) {
    return fnc(tp);
}

ObjType _tp_tcall(TP,ObjType fnc) {
    if (fnc.fnc.ftype&2) {
        insert_list(tp,tp->params.list.val,0,fnc.fnc.info->self);
    }
    return _tp_dcall(tp,(ObjType (*)(VmType *))fnc.fnc.cfnc);
}

ObjType tp_fnc_new(TP,int t, void *v, ObjType c,ObjType s, ObjType g) {
    ObjType r = {FUNCTYPE};
    _func *info = (_func*)tp_malloc(tp, sizeof(_func));
    info->code = c;
    info->self = s;
    info->globals = g;
    r.fnc.ftype = t;
    r.fnc.info = info;
    r.fnc.cfnc = v;
    return tp_track(tp,r);
}

ObjType tp_def(TP,ObjType code, ObjType g) {
    ObjType r = tp_fnc_new(tp,1,0,code,tp_None,g);
    return r;
}

ObjType tp_fnc(TP,ObjType v(TP)) {
    return tp_fnc_new(tp,0,v,tp_None,tp_None,tp_None);
}

ObjType tp_method(TP,ObjType self,ObjType v(TP)) {
    return tp_fnc_new(tp,2,v,tp_None,self,tp_None);
}

ObjType tp_data(TP,int magic,void *v) {
    ObjType r = {DATATYPE};
    r.data.info = (_tp_data*)tp_malloc(tp, sizeof(_tp_data));
    r.data.val = v;
    r.data.magic = magic;
    return tp_track(tp,r);
}


ObjType tp_params(TP) {
    ObjType r;
    tp->params = tp->_params.list.val->items[tp->cur];
    r = tp->_params.list.val->items[tp->cur];
    r.list.val->len = 0;
    return r;
}


ObjType tp_params_n(TP,int n, ObjType argv[]) {
    ObjType r = tp_params(tp);
    int i; for (i=0; i<n; i++) { append_list(tp,r.list.val,argv[i]); }
    return r;
}


ObjType tp_params_v(TP,int n,...) {
    int i;
    ObjType r = tp_params(tp);
    va_list a; va_start(a,n);
    for (i=0; i<n; i++) {
        append_list(tp,r.list.val,va_arg(a,ObjType));
    }
    va_end(a);
    return r;
}


#endif
