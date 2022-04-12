tp_obj to_string(TP, int n) {
    tp_obj r = tp_string_n(0,n);
    r.string.info = (_tp_string*)tp_malloc(tp, sizeof(_tp_string)+n);
    r.string.info->len = n;
    r.string.val = r.string.info->s;
    return r;
}

tp_obj str_cp(TP, const char *s, int n) {
    tp_obj r = to_string(tp,n);
    memcpy(r.string.info->s,s,n);
    return tp_track(tp,r);
}

tp_obj strsub(TP, tp_obj s, int a, int b) {
    int l = s.string.len;
    a = _tp_max(0,(a<0?l+a:a)); b = _tp_min(l,(b<0?l+b:b));
    tp_obj r = s;
    r.string.val += a;
    r.string.len = b-a;
    return r;
}

tp_obj _printf(TP, char const *fmt,...) {
    int l;
    tp_obj r;
    char *s;
    va_list arg;
    va_start(arg, fmt);
    l = vsnprintf(NULL, 0, fmt,arg);
    r = to_string(tp,l);
    s = r.string.info->s;
    va_end(arg);
    va_start(arg, fmt);
    vsprintf(s,fmt,arg);
    va_end(arg);
    return tp_track(tp,r);
}

int _str_ind_(tp_obj s, tp_obj k) {
    int i=0;
    while ((s.string.len - i) >= k.string.len) {
        if (memcmp(s.string.val+i,k.string.val,k.string.len) == 0) {
            return i;
        }
        i += 1;
    }
    return -1;
}

tp_obj str_join(TP) {
    tp_obj delim = TP_OBJ();
    tp_obj val = TP_OBJ();
    int l=0,i;
    tp_obj r;
    char *s;
    for (i=0; i<val.list.val->len; i++) {
        if (i!=0) { l += delim.string.len; }
        l += tp_str(tp,val.list.val->items[i]).string.len;
    }
    r = to_string(tp,l);
    s = r.string.info->s;
    l = 0;
    for (i=0; i<val.list.val->len; i++) {
        tp_obj e;
        if (i!=0) {
            memcpy(s+l,delim.string.val,delim.string.len); l += delim.string.len;
        }
        e = tp_str(tp,val.list.val->items[i]);
        memcpy(s+l,e.string.val,e.string.len); l += e.string.len;
    }
    return tp_track(tp,r);
}

tp_obj strsplit(TP) {
    tp_obj v = TP_OBJ();
    tp_obj d = TP_OBJ();
    tp_obj r = tp_list(tp);

    int i;
    while ((i=_str_ind_(v,d))!=-1) {
        _tp_list_append(tp,r.list.val,strsub(tp,v,0,i));
        v.string.val += i + d.string.len; v.string.len -= i + d.string.len;
    }
    _tp_list_append(tp,r.list.val,strsub(tp,v,0,v.string.len));
    return r;
}


tp_obj _find(TP) {
    tp_obj s = TP_OBJ();
    tp_obj v = TP_OBJ();
    return tp_number(_str_ind_(s,v));
}

tp_obj _str_index(TP) {
    tp_obj s = TP_OBJ();
    tp_obj v = TP_OBJ();
    int n = _str_ind_(s,v);
    if (n >= 0) { return tp_number(n); }
    tp_raise(tp_None,tp_string("(_str_index) ValueError: substring not found"));
}

tp_obj tp_str2(TP) {
    tp_obj v = TP_OBJ();
    return tp_str(tp,v);
}

tp_obj tp_chr(TP) {
    int v = TP_NUM();
    return tp_string_n(tp->chars[(unsigned char)v],1);
}
tp_obj tp_ord(TP) {
    tp_obj s = TP_STR();
    if (s.string.len != 1) {
        tp_raise(tp_None,tp_string("(tp_ord) TypeError: ord() expected a character"));
    }
    return tp_number((unsigned char)s.string.val[0]);
}

tp_obj tp_strip(TP) {
    tp_obj o = TP_TYPE(TP_STRING);
    char const *v = o.string.val; int l = o.string.len;
    int i; int a = l, b = 0;
    tp_obj r;
    char *s;
    for (i=0; i<l; i++) {
        if (v[i] != ' ' && v[i] != '\n' && v[i] != '\t' && v[i] != '\r') {
            a = _tp_min(a,i); b = _tp_max(b,i+1);
        }
    }
    if ((b-a) < 0) { return tp_string(""); }
    r = to_string(tp,b-a);
    s = r.string.info->s;
    memcpy(s,v+a,b-a);
    return tp_track(tp,r);
}

tp_obj tp_replace(TP) {
    tp_obj s = TP_OBJ();
    tp_obj k = TP_OBJ();
    tp_obj v = TP_OBJ();
    tp_obj p = s;
    int i,n = 0;
    int c;
    int l;
    tp_obj rr;
    char *r;
    char *d;
    tp_obj z;
    while ((i = _str_ind_(p,k)) != -1) {
        n += 1;
        p.string.val += i + k.string.len; p.string.len -= i + k.string.len;
    }
    l = s.string.len + n * (v.string.len-k.string.len);
    rr = to_string(tp,l);
    r = rr.string.info->s;
    d = r;
    z = p = s;
    while ((i = _str_ind_(p,k)) != -1) {
        p.string.val += i; p.string.len -= i;
        memcpy(d,z.string.val,c=(p.string.val-z.string.val)); d += c;
        p.string.val += k.string.len; p.string.len -= k.string.len;
        memcpy(d,v.string.val,v.string.len); d += v.string.len;
        z = p;
    }
    memcpy(d,z.string.val,(s.string.val + s.string.len) - z.string.val);

    return tp_track(tp,rr);
}

