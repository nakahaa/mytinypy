
void tp_grey(TP,ObjType v) {
    if (v.type < TP_STRING || (!v.gci.data) || *v.gci.data) { return; }
    *v.gci.data = 1;
    if (v.type == TP_STRING || v.type == DATATYPE) {
        app_list(tp,tp->black,v);
        return;
    }
    app_list(tp,tp->grey,v);
}

void tp_follow(TP,ObjType v) {
    int type = v.type;
    if (type == LISTTYPE) {
        int n;
        for (n=0; n<v.list.val->len; n++) {
            tp_grey(tp,v.list.val->items[n]);
        }
    }
    if (type == DICTTYPE) {
        int i;
        for (i=0; i<v.dict.val->len; i++) {
            int n = _tp_dict_next(tp,v.dict.val);
            tp_grey(tp,v.dict.val->items[n].key);
            tp_grey(tp,v.dict.val->items[n].val);
        }
        tp_grey(tp,v.dict.val->meta); 
    }
    if (type == FUNCTYPE) {
        tp_grey(tp,v.fnc.info->self);
        tp_grey(tp,v.fnc.info->globals);
        tp_grey(tp,v.fnc.info->code);
    }
}

void tp_reset(TP) {
    int n;
    _list *tmp;
    for (n=0; n<tp->black->len; n++) {
        *tp->black->items[n].gci.data = 0;
    }
    tmp = tp->white;
    tp->white = tp->black;
    tp->black = tmp;
}

void tp_gc_init(TP) {
    tp->white = new_list(tp);
    tp->grey = new_list(tp);
    tp->black = new_list(tp);
    tp->steps = 0;
}

void tp_gc_deinit(TP) {
    free_list(tp, tp->white);
    free_list(tp, tp->grey);
    free_list(tp, tp->black);
}

void tp_delete(TP,ObjType v) {
    int type = v.type;
    if (type == LISTTYPE) {
        free_list(tp, v.list.val);
        return;
    } else if (type == DICTTYPE) {
        _tp_dict_free(tp, v.dict.val);
        return;
    } else if (type == TP_STRING) {
        tp_free(tp, v.string.info);
        return;
    } else if (type == DATATYPE) {
        if (v.data.info->free) {
            v.data.info->free(tp,v);
        }
        tp_free(tp, v.data.info);
        return;
    } else if (type == FUNCTYPE) {
        tp_free(tp, v.fnc.info);
        return;
    }
    tp_raise(,tp_string("(tp_delete) TypeError: ?"));
}

void tp_collect(TP) {
    int n;
    for (n=0; n<tp->white->len; n++) {
        ObjType r = tp->white->items[n];
        if (*r.gci.data) { continue; }
        tp_delete(tp,r);
    }
    tp->white->len = 0;
    tp_reset(tp);
}

void _tp_gcinc(TP) {
    ObjType v;
    if (!tp->grey->len) {
        return;
    }
    v = pop_list(tp,tp->grey,tp->grey->len-1,"_tp_gcinc");
    tp_follow(tp,v);
    app_list(tp,tp->black,v);
}

void tp_full(TP) {
    while (tp->grey->len) {
        _tp_gcinc(tp);
    }
    tp_collect(tp);
    tp_follow(tp,tp->root);
}

void tp_gcinc(TP) {
    tp->steps += 1;
    if (tp->steps < TP_GCMAX || tp->grey->len > 0) {
        _tp_gcinc(tp); _tp_gcinc(tp); 
    }
    if (tp->steps < TP_GCMAX || tp->grey->len > 0) { return; }
    tp->steps = 0;
    tp_full(tp);
    return;
}

ObjType tp_track(TP,ObjType v) {
    tp_gcinc(tp);
    tp_grey(tp,v);
    return v;
}

