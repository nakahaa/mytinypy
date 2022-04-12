ObjType to_string(TP, int n)
{
    ObjType r = tp_string_n(0, n);
    r.string.info = (_string *)tp_malloc(tp, sizeof(_string) + n);
    r.string.info->len = n;
    r.string.val = r.string.info->s;
    return r;
}

ObjType str_cp(TP, const char *s, int n)
{
    ObjType r = to_string(tp, n);
    memcpy(r.string.info->s, s, n);
    return tp_track(tp, r);
}

ObjType strsub(TP, ObjType s, int a, int b)
{
    int l = s.string.len;
    a = _tp_max(0, (a < 0 ? l + a : a));
    b = _tp_min(l, (b < 0 ? l + b : b));
    ObjType r = s;
    r.string.val += a;
    r.string.len = b - a;
    return r;
}

ObjType _printf(TP, char const *fmt, ...)
{
    int l;
    ObjType r;
    char *s;
    va_list arg;
    va_start(arg, fmt);
    l = vsnprintf(NULL, 0, fmt, arg);
    r = to_string(tp, l);
    s = r.string.info->s;
    va_end(arg);
    va_start(arg, fmt);
    vsprintf(s, fmt, arg);
    va_end(arg);
    return tp_track(tp, r);
}

int _str_ind_(ObjType s, ObjType k)
{
    int i = 0;
    while ((s.string.len - i) >= k.string.len)
    {
        if (memcmp(s.string.val + i, k.string.val, k.string.len) == 0)
        {
            return i;
        }
        i += 1;
    }
    return -1;
}

ObjType str_join(TP)
{
    ObjType delim = TP_OBJ();
    ObjType val = TP_OBJ();
    int l = 0, i;
    ObjType r;
    char *s;
    for (i = 0; i < val.list.val->len; i++)
    {
        if (i != 0)
        {
            l += delim.string.len;
        }
        l += tp_str(tp, val.list.val->items[i]).string.len;
    }
    r = to_string(tp, l);
    s = r.string.info->s;
    l = 0;
    for (i = 0; i < val.list.val->len; i++)
    {
        ObjType e;
        if (i != 0)
        {
            memcpy(s + l, delim.string.val, delim.string.len);
            l += delim.string.len;
        }
        e = tp_str(tp, val.list.val->items[i]);
        memcpy(s + l, e.string.val, e.string.len);
        l += e.string.len;
    }
    return tp_track(tp, r);
}

ObjType strsplit(TP)
{
    ObjType v = TP_OBJ();
    ObjType d = TP_OBJ();
    ObjType r = to_list(tp);

    int i;
    while ((i = _str_ind_(v, d)) != -1)
    {
        append_list(tp, r.list.val, strsub(tp, v, 0, i));
        v.string.val += i + d.string.len;
        v.string.len -= i + d.string.len;
    }
    append_list(tp, r.list.val, strsub(tp, v, 0, v.string.len));
    return r;
}

ObjType _find(TP)
{
    ObjType s = TP_OBJ();
    ObjType v = TP_OBJ();
    return number(_str_ind_(s, v));
}

ObjType _str_index(TP)
{
    ObjType s = TP_OBJ();
    ObjType v = TP_OBJ();
    int n = _str_ind_(s, v);
    if (n >= 0)
    {
        return number(n);
    }
    tp_raise(NONE, mkstring("(_str_index) ValueError: substring not found"));
}

ObjType tp_str2(TP)
{
    ObjType v = TP_OBJ();
    return tp_str(tp, v);
}

ObjType tp_chr(TP)
{
    int v = TP_NUM();
    return tp_string_n(tp->chars[(unsigned char)v], 1);
}
ObjType tp_ord(TP)
{
    ObjType s = TP_STR();
    if (s.string.len != 1)
    {
        tp_raise(NONE, mkstring("(tp_ord) TypeError: ord() expected a character"));
    }
    return number((unsigned char)s.string.val[0]);
}

ObjType tp_strip(TP)
{
    ObjType o = TP_TYPE(TP_STRING);
    char const *v = o.string.val;
    int l = o.string.len;
    int i;
    int a = l, b = 0;
    ObjType r;
    char *s;
    for (i = 0; i < l; i++)
    {
        if (v[i] != ' ' && v[i] != '\n' && v[i] != '\t' && v[i] != '\r')
        {
            a = _tp_min(a, i);
            b = _tp_max(b, i + 1);
        }
    }
    if ((b - a) < 0)
    {
        return mkstring("");
    }
    r = to_string(tp, b - a);
    s = r.string.info->s;
    memcpy(s, v + a, b - a);
    return tp_track(tp, r);
}

ObjType tp_replace(TP)
{
    ObjType s = TP_OBJ();
    ObjType k = TP_OBJ();
    ObjType v = TP_OBJ();
    ObjType p = s;
    int i, n = 0;
    int c;
    int l;
    ObjType rr;
    char *r;
    char *d;
    ObjType z;
    while ((i = _str_ind_(p, k)) != -1)
    {
        n += 1;
        p.string.val += i + k.string.len;
        p.string.len -= i + k.string.len;
    }
    l = s.string.len + n * (v.string.len - k.string.len);
    rr = to_string(tp, l);
    r = rr.string.info->s;
    d = r;
    z = p = s;
    while ((i = _str_ind_(p, k)) != -1)
    {
        p.string.val += i;
        p.string.len -= i;
        memcpy(d, z.string.val, c = (p.string.val - z.string.val));
        d += c;
        p.string.val += k.string.len;
        p.string.len -= k.string.len;
        memcpy(d, v.string.val, v.string.len);
        d += v.string.len;
        z = p;
    }
    memcpy(d, z.string.val, (s.string.val + s.string.len) - z.string.val);

    return tp_track(tp, rr);
}
