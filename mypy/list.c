void realloc_list(TP, _list *self,int len) {
    if (!len) { len=1; }
    self->items = (ObjType*)tp_realloc(tp, self->items,len*sizeof(ObjType));
    self->alloc = len;
}

void set_list(TP,_list *self,int k, ObjType v, const char *error) {
    if (k >= self->len) {
        tp_raise(,tp_string("(set_list) KeyError"));
    }
    self->items[k] = v;
    tp_grey(tp,v);
}
void free_list(TP, _list *self) {
    tp_free(tp, self->items);
    tp_free(tp, self);
}

ObjType get_list(TP,_list *self,int k,const char *error) {
    if (k >= self->len) {
        tp_raise(tp_None,tp_string("(set_list) KeyError"));
    }
    return self->items[k];
}
void insertx_list(TP,_list *self, int n, ObjType v) {
    if (self->len >= self->alloc) {
        realloc_list(tp, self,self->alloc*2);
    }
    if (n < self->len) { memmove(&self->items[n+1],&self->items[n],sizeof(ObjType)*(self->len-n)); }
    self->items[n] = v;
    self->len += 1;
}
void app_list(TP,_list *self, ObjType v) {
    insertx_list(tp,self,self->len,v);
}
void insert_list(TP,_list *self, int n, ObjType v) {
    insertx_list(tp,self,n,v);
    tp_grey(tp,v);
}
void append_list(TP,_list *self, ObjType v) {
    insert_list(tp,self,self->len,v);
}
ObjType pop_list(TP,_list *self, int n, const char *error) {
    ObjType r = get_list(tp,self,n,error);
    if (n != self->len-1) { memmove(&self->items[n],&self->items[n+1],sizeof(ObjType)*(self->len-(n+1))); }
    self->len -= 1;
    return r;
}

int find_list(TP,_list *self, ObjType v) {
    int n;
    for (n=0; n<self->len; n++) {
        if (compare(tp,v,self->items[n]) == 0) {
            return n;
        }
    }
    return -1;
}

ObjType index_list(TP) {
    ObjType self = TP_OBJ();
    ObjType v = TP_OBJ();
    int i = find_list(tp,self.list.val,v);
    if (i < 0) {
        tp_raise(tp_None,tp_string("(index_list) ValueError: list.index(x): x not in list"));
    }
    return tp_number(i);
}

_list *new_list(TP) {
    return (_list*)tp_malloc(tp, sizeof(_list));
}

ObjType cp_list(TP, ObjType rr) {
    ObjType val = {LISTTYPE};
    _list *o = rr.list.val;
    _list *r = new_list(tp);
    *r = *o; r->gci = 0;
    r->items = (ObjType*)tp_malloc(tp, sizeof(ObjType)*o->len);
    memcpy(r->items,o->items,sizeof(ObjType)*o->len);
    val.list.val = r;
    return tp_track(tp,val);
}

ObjType append(TP) {
    ObjType self = TP_OBJ();
    ObjType v = TP_OBJ();
    append_list(tp,self.list.val,v);
    return tp_None;
}

ObjType pop(TP) {
    ObjType self = TP_OBJ();
    return pop_list(tp,self.list.val,self.list.val->len-1,"pop");
}

ObjType insert(TP) {
    ObjType self = TP_OBJ();
    int n = TP_NUM();
    ObjType v = TP_OBJ();
    insert_list(tp,self.list.val,n,v);
    return tp_None;
}

ObjType extend(TP) {
    ObjType self = TP_OBJ();
    ObjType v = TP_OBJ();
    int i;
    for (i=0; i<v.list.val->len; i++) {
        append_list(tp,self.list.val,v.list.val->items[i]);
    }
    return tp_None;
}

ObjType list_nt(TP) {
    ObjType r = {LISTTYPE};
    r.list.val = new_list(tp);
    return r;
}

ObjType to_list(TP) {
    ObjType r = {LISTTYPE};
    r.list.val = new_list(tp);
    return tp_track(tp,r);
}

ObjType to_list_n(TP,int n,ObjType *argv) {
    int i;
    ObjType r = to_list(tp); realloc_list(tp, r.list.val,n);
    for (i=0; i<n; i++) {
        append_list(tp,r.list.val,argv[i]);
    }
    return r;
}

int sort_compare(ObjType *a,ObjType *b) {
    return compare(0,*a,*b);
}

ObjType sort(TP) {
    ObjType self = TP_OBJ();
    qsort(self.list.val->items, self.list.val->len, sizeof(ObjType), (int(*)(const void*,const void*))sort_compare);
    return tp_None;
}

