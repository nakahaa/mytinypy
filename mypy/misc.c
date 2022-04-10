
tp_obj _tp_dcall(TP,tp_obj fnc(TP)) {
    return fnc(tp);
}

tp_obj _tp_tcall(TP,tp_obj fnc) {
    if (fnc.fnc.ftype&2) {
        _tp_list_insert(tp,tp->params.list.val,0,fnc.fnc.info->self);
    }
    return _tp_dcall(tp,(tp_obj (*)(tp_vm *))fnc.fnc.cfnc);
}

tp_obj tp_fnc_new(TP,int t, void *v, tp_obj c,tp_obj s, tp_obj g) {
    tp_obj r = {TP_FNC};
    _tp_fnc *info = (_tp_fnc*)tp_malloc(tp, sizeof(_tp_fnc));
    info->code = c;
    info->self = s;
    info->globals = g;
    r.fnc.ftype = t;
    r.fnc.info = info;
    r.fnc.cfnc = v;
    return tp_track(tp,r);
}

tp_obj tp_def(TP,tp_obj code, tp_obj g) {
    tp_obj r = tp_fnc_new(tp,1,0,code,tp_None,g);
    return r;
}

tp_obj tp_fnc(TP,tp_obj v(TP)) {
    return tp_fnc_new(tp,0,v,tp_None,tp_None,tp_None);
}

tp_obj tp_method(TP,tp_obj self,tp_obj v(TP)) {
    return tp_fnc_new(tp,2,v,tp_None,self,tp_None);
}

tp_obj tp_data(TP,int magic,void *v) {
    tp_obj r = {TP_DATA};
    r.data.info = (_tp_data*)tp_malloc(tp, sizeof(_tp_data));
    r.data.val = v;
    r.data.magic = magic;
    return tp_track(tp,r);
}


tp_obj tp_params(TP) {
    tp_obj r;
    tp->params = tp->_params.list.val->items[tp->cur];
    r = tp->_params.list.val->items[tp->cur];
    r.list.val->len = 0;
    return r;
}


tp_obj tp_params_n(TP,int n, tp_obj argv[]) {
    tp_obj r = tp_params(tp);
    int i; for (i=0; i<n; i++) { _tp_list_append(tp,r.list.val,argv[i]); }
    return r;
}


tp_obj tp_params_v(TP,int n,...) {
    int i;
    tp_obj r = tp_params(tp);
    va_list a; va_start(a,n);
    for (i=0; i<n; i++) {
        _tp_list_append(tp,r.list.val,va_arg(a,tp_obj));
    }
    va_end(a);
    return r;
}
