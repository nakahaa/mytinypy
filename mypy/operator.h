ObjType tp_str(TP, ObjType self)
{
    int type = self.type;
    if (type == TP_STRING)
    {
        return self;
    }
    if (type == TP_NUMBER)
    {
        tp_num v = self.number.val;
        if ((fabs(v) - fabs((long)v)) < 0.000001)
        {
            return _printf(tp, "%ld", (long)v);
        }
        return _printf(tp, "%f", v);
    }
    else if (type == DICTTYPE)
    {
        return _printf(tp, "<dict 0x%x>", self.dict.val);
    }
    else if (type == LISTTYPE)
    {
        return _printf(tp, "<list 0x%x>", self.list.val);
    }
    else if (type == NONETYPE)
    {
        return mkstring("None");
    }
    else if (type == DATATYPE)
    {
        return _printf(tp, "<data 0x%x>", self.data.val);
    }
    else if (type == FUNCTYPE)
    {
        return _printf(tp, "<fnc 0x%x>", self.fnc.info);
    }
    return mkstring("<?>");
}

int mk_bool(TP, ObjType v)
{
    switch (v.type)
    {
    case TP_NUMBER:
        return v.number.val != 0;
    case NONETYPE:
        return 0;
    case TP_STRING:
        return v.string.len != 0;
    case LISTTYPE:
        return v.list.val->len != 0;
    case DICTTYPE:
        return v.dict.val->len != 0;
    }
    return 1;
}

ObjType has(TP, ObjType self, ObjType k)
{
    int type = self.type;
    if (type == DICTTYPE)
    {
        if (_tp_dict_find(tp, self.dict.val, k) != -1)
        {
            return tp_True;
        }
        return tp_False;
    }
    else if (type == TP_STRING && k.type == TP_STRING)
    {
        return number(_str_ind_(self, k) != -1);
    }
    else if (type == LISTTYPE)
    {
        return number(find_list(tp, self.list.val, k) != -1);
    }
    tp_raise(NONE, mkstring("(has) TypeError: iterable argument required"));
}

void del(TP, ObjType self, ObjType k)
{
    int type = self.type;
    if (type == DICTTYPE)
    {
        _tp_dict_del(tp, self.dict.val, k, "del");
        return;
    }
    tp_raise(, mkstring("(del) TypeError: object does not support item deletion"));
}

ObjType tp_iter(TP, ObjType self, ObjType k)
{
    int type = self.type;
    if (type == LISTTYPE || type == TP_STRING)
    {
        return get(tp, self, k);
    }
    if (type == DICTTYPE && k.type == TP_NUMBER)
    {
        return self.dict.val->items[_tp_dict_next(tp, self.dict.val)].key;
    }
    tp_raise(NONE, mkstring("(tp_iter) TypeError: iteration over non-sequence"));
}

ObjType get(TP, ObjType self, ObjType k)
{
    int type = self.type;
    ObjType r;
    if (type == DICTTYPE)
    {
        TP_META_BEGIN(self, "__get__");
        return callfunc(tp, meta, tp_params_v(tp, 1, k));
        TP_META_END;
        if (self.dict.dtype && lookupFunc(tp, self, k, &r))
        {
            return r;
        }
        return _tp_dict_get(tp, self.dict.val, k, "get");
    }
    else if (type == LISTTYPE)
    {
        if (k.type == TP_NUMBER)
        {
            int l = len_func(tp, self).number.val;
            int n = k.number.val;
            n = (n < 0 ? l + n : n);
            return get_list(tp, self.list.val, n, "get");
        }
        else if (k.type == TP_STRING)
        {
            if (compare(tp, mkstring("append"), k) == 0)
            {
                return tp_method(tp, self, append);
            }
            else if (compare(tp, mkstring("pop"), k) == 0)
            {
                return tp_method(tp, self, pop);
            }
            else if (compare(tp, mkstring("index"), k) == 0)
            {
                return tp_method(tp, self, index_list);
            }
            else if (compare(tp, mkstring("sort"), k) == 0)
            {
                return tp_method(tp, self, sort);
            }
            else if (compare(tp, mkstring("extend"), k) == 0)
            {
                return tp_method(tp, self, extend);
            }
            else if (compare(tp, mkstring("*"), k) == 0)
            {
                tp_params_v(tp, 1, self);
                r = copyFunc(tp);
                self.list.val->len = 0;
                return r;
            }
        }
        else if (k.type == NONETYPE)
        {
            return pop_list(tp, self.list.val, 0, "get");
        }
    }
    else if (type == TP_STRING)
    {
        if (k.type == TP_NUMBER)
        {
            int l = self.string.len;
            int n = k.number.val;
            n = (n < 0 ? l + n : n);
            if (n >= 0 && n < l)
            {
                return tp_string_n(tp->chars[(unsigned char)self.string.val[n]], 1);
            }
        }
        else if (k.type == TP_STRING)
        {
            if (compare(tp, mkstring("join"), k) == 0)
            {
                return tp_method(tp, self, str_join);
            }
            else if (compare(tp, mkstring("split"), k) == 0)
            {
                return tp_method(tp, self, strsplit);
            }
            else if (compare(tp, mkstring("index"), k) == 0)
            {
                return tp_method(tp, self, _str_index);
            }
            else if (compare(tp, mkstring("strip"), k) == 0)
            {
                return tp_method(tp, self, tp_strip);
            }
            else if (compare(tp, mkstring("replace"), k) == 0)
            {
                return tp_method(tp, self, tp_replace);
            }
        }
    }

    if (k.type == LISTTYPE)
    {
        int a, b, l;
        ObjType tmp;
        l = len_func(tp, self).number.val;
        tmp = get(tp, k, number(0));
        if (tmp.type == TP_NUMBER)
        {
            a = tmp.number.val;
        }
        else if (tmp.type == NONETYPE)
        {
            a = 0;
        }
        else
        {
            tp_raise(NONE, mkstring("(get) TypeError: indices must be numbers"));
        }
        tmp = get(tp, k, number(1));
        if (tmp.type == TP_NUMBER)
        {
            b = tmp.number.val;
        }
        else if (tmp.type == NONETYPE)
        {
            b = l;
        }
        else
        {
            tp_raise(NONE, mkstring("(get) TypeError: indices must be numbers"));
        }
        a = _tp_max(0, (a < 0 ? l + a : a));
        b = _tp_min(l, (b < 0 ? l + b : b));
        if (type == LISTTYPE)
        {
            return list_n(tp, b - a, &self.list.val->items[a]);
        }
        else if (type == TP_STRING)
        {
            return strsub(tp, self, a, b);
        }
    }

    tp_raise(NONE, mkstring("(get) TypeError: ?"));
}

int iget(TP, ObjType *r, ObjType self, ObjType k)
{
    if (self.type == DICTTYPE)
    {
        int n = _tp_dict_find(tp, self.dict.val, k);
        if (n == -1)
        {
            return 0;
        }
        *r = self.dict.val->items[n].val;
        tp_grey(tp, *r);
        return 1;
    }
    if (self.type == LISTTYPE && !self.list.val->len)
    {
        return 0;
    }
    *r = get(tp, self, k);
    tp_grey(tp, *r);
    return 1;
}

void set(TP, ObjType self, ObjType k, ObjType v)
{
    int type = self.type;

    if (type == DICTTYPE)
    {
        TP_META_BEGIN(self, "__set__");
        callfunc(tp, meta, tp_params_v(tp, 2, k, v));
        return;
        TP_META_END;
        _tp_dict_set(tp, self.dict.val, k, v);
        return;
    }
    else if (type == LISTTYPE)
    {
        if (k.type == TP_NUMBER)
        {
            set_list(tp, self.list.val, k.number.val, v, "set");
            return;
        }
        else if (k.type == NONETYPE)
        {
            append_list(tp, self.list.val, v);
            return;
        }
        else if (k.type == TP_STRING)
        {
            if (compare(tp, mkstring("*"), k) == 0)
            {
                tp_params_v(tp, 2, self, v);
                extend(tp);
                return;
            }
        }
    }
    tp_raise(, mkstring("(set) TypeError: object does not support item assignment"));
}

ObjType add(TP, ObjType a, ObjType b)
{
    if (a.type == TP_NUMBER && a.type == b.type)
    {
        return number(a.number.val + b.number.val);
    }
    else if (a.type == TP_STRING && a.type == b.type)
    {
        int al = a.string.len, bl = b.string.len;
        ObjType r = to_string(tp, al + bl);
        char *s = r.string.info->s;
        memcpy(s, a.string.val, al);
        memcpy(s + al, b.string.val, bl);
        return tp_track(tp, r);
    }
    else if (a.type == LISTTYPE && a.type == b.type)
    {
        ObjType r;
        tp_params_v(tp, 1, a);
        r = copyFunc(tp);
        tp_params_v(tp, 2, r, b);
        extend(tp);
        return r;
    }
    tp_raise(NONE, mkstring("(add) TypeError: ?"));
}

ObjType tp_mul(TP, ObjType a, ObjType b)
{
    if (a.type == TP_NUMBER && a.type == b.type)
    {
        return number(a.number.val * b.number.val);
    }
    else if ((a.type == TP_STRING && b.type == TP_NUMBER) ||
             (a.type == TP_NUMBER && b.type == TP_STRING))
    {
        if (a.type == TP_NUMBER)
        {
            ObjType c = a;
            a = b;
            b = c;
        }
        int al = a.string.len;
        int n = b.number.val;
        if (n <= 0)
        {
            ObjType r = to_string(tp, 0);
            return tp_track(tp, r);
        }
        ObjType r = to_string(tp, al * n);
        char *s = r.string.info->s;
        int i;
        for (i = 0; i < n; i++)
        {
            memcpy(s + al * i, a.string.val, al);
        }
        return tp_track(tp, r);
    }
    tp_raise(NONE, mkstring("(tp_mul) TypeError: ?"));
}

ObjType len_func(TP, ObjType self)
{
    int type = self.type;
    if (type == TP_STRING)
    {
        return number(self.string.len);
    }
    else if (type == DICTTYPE)
    {
        return number(self.dict.val->len);
    }
    else if (type == LISTTYPE)
    {
        return number(self.list.val->len);
    }

    tp_raise(NONE, mkstring("(len_func) TypeError: len() of unsized object"));
}

int compare(TP, ObjType a, ObjType b)
{
    if (a.type != b.type)
    {
        return a.type - b.type;
    }
    switch (a.type)
    {
    case NONETYPE:
        return 0;
    case TP_NUMBER:
        return _tp_sign(a.number.val - b.number.val);
    case TP_STRING:
    {
        int l = _tp_min(a.string.len, b.string.len);
        int v = memcmp(a.string.val, b.string.val, l);
        if (v == 0)
        {
            v = a.string.len - b.string.len;
        }
        return v;
    }
    case LISTTYPE:
    {
        int n, v;
        for (n = 0; n < _tp_min(a.list.val->len, b.list.val->len); n++)
        {
            ObjType aa = a.list.val->items[n];
            ObjType bb = b.list.val->items[n];
            if (aa.type == LISTTYPE && bb.type == LISTTYPE)
            {
                v = aa.list.val - bb.list.val;
            }
            else
            {
                v = compare(tp, aa, bb);
            }
            if (v)
            {
                return v;
            }
        }
        return a.list.val->len - b.list.val->len;
    }
    case DICTTYPE:
        return a.dict.val - b.dict.val;
    case FUNCTYPE:
        return a.fnc.info - b.fnc.info;
    case DATATYPE:
        return (char *)a.data.val - (char *)b.data.val;
    }
    tp_raise(0, mkstring("(compare) TypeError: ?"));
}

#define TP_OP(name, expr)                                                                \
    ObjType name(TP, ObjType _a, ObjType _b)                                             \
    {                                                                                    \
        if (_a.type == TP_NUMBER && _a.type == _b.type)                                  \
        {                                                                                \
            tp_num a = _a.number.val;                                                    \
            tp_num b = _b.number.val;                                                    \
            return number(expr);                                                      \
        }                                                                                \
        tp_raise(NONE, mkstring("(" #name ") TypeError: unsupported operand type(s)")); \
    }

TP_OP(tp_bitwise_and, ((long)a) & ((long)b));
TP_OP(tp_bitwise_or, ((long)a) | ((long)b));
TP_OP(tp_bitwise_xor, ((long)a) ^ ((long)b));
TP_OP(tp_mod, ((long)a) % ((long)b));
TP_OP(tp_lsh, ((long)a) << ((long)b));
TP_OP(tp_rsh, ((long)a) >> ((long)b));
TP_OP(tp_sub, a - b);
TP_OP(tp_div, a / b);
TP_OP(tp_pow, pow(a, b));

ObjType bitwise_not(TP, ObjType a)
{
    if (a.type == TP_NUMBER)
    {
        return number(~(long)a.number.val);
    }
    tp_raise(NONE, mkstring("(bitwise_not) TypeError: unsupported operand type"));
}
