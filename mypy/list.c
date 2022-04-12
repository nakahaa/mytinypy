void _tp_list_realloc(TP, _list *self,int len) {
    if (!len) { len=1; }
    self->items = (ObjType*)tp_realloc(tp, self->items,len*sizeof(ObjType));
    self->alloc = len;
}

void _tp_list_set(TP,_list *self,int k, ObjType v, const char *error) {
    if (k >= self->len) {
        tp_raise(,tp_string("(_tp_list_set) KeyError"));
    }
    self->items[k] = v;
    tp_grey(tp,v);
}
void _tp_list_free(TP, _list *self) {
    tp_free(tp, self->items);
    tp_free(tp, self);
}

ObjType _tp_list_get(TP,_list *self,int k,const char *error) {
    if (k >= self->len) {
        tp_raise(tp_None,tp_string("(_tp_list_set) KeyError"));
    }
    return self->items[k];
}
void _tp_list_insertx(TP,_list *self, int n, ObjType v) {
    if (self->len >= self->alloc) {
        _tp_list_realloc(tp, self,self->alloc*2);
    }
    if (n < self->len) { memmove(&self->items[n+1],&self->items[n],sizeof(ObjType)*(self->len-n)); }
    self->items[n] = v;
    self->len += 1;
}
void _tp_list_appendx(TP,_list *self, ObjType v) {
    _tp_list_insertx(tp,self,self->len,v);
}
void _tp_list_insert(TP,_list *self, int n, ObjType v) {
    _tp_list_insertx(tp,self,n,v);
    tp_grey(tp,v);
}
void _tp_list_append(TP,_list *self, ObjType v) {
    _tp_list_insert(tp,self,self->len,v);
}
ObjType _tp_list_pop(TP,_list *self, int n, const char *error) {
    ObjType r = _tp_list_get(tp,self,n,error);
    if (n != self->len-1) { memmove(&self->items[n],&self->items[n+1],sizeof(ObjType)*(self->len-(n+1))); }
    self->len -= 1;
    return r;
}

int _tp_list_find(TP,_list *self, ObjType v) {
    int n;
    for (n=0; n<self->len; n++) {
        if (tp_cmp(tp,v,self->items[n]) == 0) {
            return n;
        }
    }
    return -1;
}

ObjType tp_index(TP) {
    ObjType self = TP_OBJ();
    ObjType v = TP_OBJ();
    int i = _tp_list_find(tp,self.list.val,v);
    if (i < 0) {
        tp_raise(tp_None,tp_string("(tp_index) ValueError: list.index(x): x not in list"));
    }
    return tp_number(i);
}

_list *_tp_list_new(TP) {
    return (_list*)tp_malloc(tp, sizeof(_list));
}

ObjType _tp_list_copy(TP, ObjType rr) {
    ObjType val = {TP_LIST};
    _list *o = rr.list.val;
    _list *r = _tp_list_new(tp);
    *r = *o; r->gci = 0;
    r->items = (ObjType*)tp_malloc(tp, sizeof(ObjType)*o->len);
    memcpy(r->items,o->items,sizeof(ObjType)*o->len);
    val.list.val = r;
    return tp_track(tp,val);
}

ObjType tp_append(TP) {
    ObjType self = TP_OBJ();
    ObjType v = TP_OBJ();
    _tp_list_append(tp,self.list.val,v);
    return tp_None;
}

ObjType tp_pop(TP) {
    ObjType self = TP_OBJ();
    return _tp_list_pop(tp,self.list.val,self.list.val->len-1,"pop");
}

ObjType tp_insert(TP) {
    ObjType self = TP_OBJ();
    int n = TP_NUM();
    ObjType v = TP_OBJ();
    _tp_list_insert(tp,self.list.val,n,v);
    return tp_None;
}

ObjType tp_extend(TP) {
    ObjType self = TP_OBJ();
    ObjType v = TP_OBJ();
    int i;
    for (i=0; i<v.list.val->len; i++) {
        _tp_list_append(tp,self.list.val,v.list.val->items[i]);
    }
    return tp_None;
}

ObjType tp_list_nt(TP) {
    ObjType r = {TP_LIST};
    r.list.val = _tp_list_new(tp);
    return r;
}

ObjType tp_list(TP) {
    ObjType r = {TP_LIST};
    r.list.val = _tp_list_new(tp);
    return tp_track(tp,r);
}

ObjType tp_list_n(TP,int n,ObjType *argv) {
    int i;
    ObjType r = tp_list(tp); _tp_list_realloc(tp, r.list.val,n);
    for (i=0; i<n; i++) {
        _tp_list_append(tp,r.list.val,argv[i]);
    }
    return r;
}

int _tp_sort_cmp(ObjType *a,ObjType *b) {
    return tp_cmp(0,*a,*b);
}

ObjType tp_sort(TP) {
    ObjType self = TP_OBJ();
    qsort(self.list.val->items, self.list.val->len, sizeof(ObjType), (int(*)(const void*,const void*))_tp_sort_cmp);
    return tp_None;
}

