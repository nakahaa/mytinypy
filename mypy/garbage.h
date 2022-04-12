
void greyFunc(TP, ObjType v)
{
    if (v.type < TP_STRING || (!v.gci.data) || *v.gci.data)
    {
        return;
    }
    *v.gci.data = 1;
    if (v.type == TP_STRING || v.type == DATATYPE)
    {
        app_list(tp, tp->black, v);
        return;
    }
    app_list(tp, tp->grey, v);
}

void followFunc(TP, ObjType v)
{
    int type = v.type;
    if (type == LISTTYPE)
    {
        int n;
        for (n = 0; n < v.list.val->len; n++)
        {
            greyFunc(tp, v.list.val->items[n]);
        }
    }
    if (type == DICTTYPE)
    {
        int i;
        for (i = 0; i < v.dict.val->len; i++)
        {
            int n = _tp_dict_next(tp, v.dict.val);
            greyFunc(tp, v.dict.val->items[n].key);
            greyFunc(tp, v.dict.val->items[n].val);
        }
        greyFunc(tp, v.dict.val->meta);
    }
    if (type == FUNCTYPE)
    {
        greyFunc(tp, v.fnc.info->self);
        greyFunc(tp, v.fnc.info->globals);
        greyFunc(tp, v.fnc.info->code);
    }
}

void resetFunc(TP)
{
    int n;
    _list *tmp;
    for (n = 0; n < tp->black->len; n++)
    {
        *tp->black->items[n].gci.data = 0;
    }
    tmp = tp->white;
    tp->white = tp->black;
    tp->black = tmp;
}

void gc_vm_init(TP)
{
    tp->white = new_list(tp);
    tp->grey = new_list(tp);
    tp->black = new_list(tp);
    tp->steps = 0;
}

void gc_deinit(TP)
{
    free_list(tp, tp->white);
    free_list(tp, tp->grey);
    free_list(tp, tp->black);
}

void deleteFunc(TP, ObjType v)
{
    int type = v.type;
    if (type == LISTTYPE)
    {
        free_list(tp, v.list.val);
        return;
    }
    else if (type == DICTTYPE)
    {
        _tp_dict_free(tp, v.dict.val);
        return;
    }
    else if (type == TP_STRING)
    {
        tp_free(tp, v.string.info);
        return;
    }
    else if (type == DATATYPE)
    {
        if (v.data.info->free)
        {
            v.data.info->free(tp, v);
        }
        tp_free(tp, v.data.info);
        return;
    }
    else if (type == FUNCTYPE)
    {
        tp_free(tp, v.fnc.info);
        return;
    }
    tp_raise(, mkstring("(deleteFunc) TypeError: ?"));
}

void collectFunc(TP)
{
    int n;
    for (n = 0; n < tp->white->len; n++)
    {
        ObjType r = tp->white->items[n];
        if (*r.gci.data)
        {
            continue;
        }
        deleteFunc(tp, r);
    }
    tp->white->len = 0;
    resetFunc(tp);
}

void _gcinc(TP)
{
    ObjType v;
    if (!tp->grey->len)
    {
        return;
    }
    v = pop_list(tp, tp->grey, tp->grey->len - 1, "_gcinc");
    followFunc(tp, v);
    app_list(tp, tp->black, v);
}

void fullFunc(TP)
{
    while (tp->grey->len)
    {
        _gcinc(tp);
    }
    collectFunc(tp);
    followFunc(tp, tp->root);
}

void gcincFunc(TP)
{
    tp->steps += 1;
    if (tp->steps < TP_GCMAX || tp->grey->len > 0)
    {
        _gcinc(tp);
        _gcinc(tp);
    }
    if (tp->steps < TP_GCMAX || tp->grey->len > 0)
    {
        return;
    }
    tp->steps = 0;
    fullFunc(tp);
    return;
}

ObjType track(TP, ObjType v)
{
    gcincFunc(tp);
    greyFunc(tp, v);
    return v;
}
