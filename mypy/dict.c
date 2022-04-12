int tp_lua_hash(void const *v,int l) {
    int i,step = (l>>5)+1;
    int h = l + (l >= 4?*(int*)v:0);
    for (i=l; i>=step; i-=step) {
        h = h^((h<<5)+(h>>2)+((unsigned char *)v)[i-1]);
    }
    return h;
}
void _tp_dict_free(TP, _dict *self) {
    tp_free(tp, self->items);
    tp_free(tp, self);
}

int tp_hash(TP,ObjType v) {
    switch (v.type) {
        case TP_NONE: return 0;
        case TP_NUMBER: return tp_lua_hash(&v.number.val,sizeof(tp_num));
        case TP_STRING: return tp_lua_hash(v.string.val,v.string.len);
        case TP_DICT: return tp_lua_hash(&v.dict.val,sizeof(void*));
        case TP_LIST: {
            int r = v.list.val->len; int n; for(n=0; n<v.list.val->len; n++) {
            ObjType vv = v.list.val->items[n]; r += vv.type != TP_LIST?tp_hash(tp,v.list.val->items[n]):tp_lua_hash(&vv.list.val,sizeof(void*)); } return r;
        }
        case TP_FNC: return tp_lua_hash(&v.fnc.info,sizeof(void*));
        case TP_DATA: return tp_lua_hash(&v.data.val,sizeof(void*));
    }
    tp_raise(0,tp_string("(tp_hash) TypeError: value unhashable"));
}

void _tp_dict_hash_set(TP,_dict *self, int hash, ObjType k, ObjType v) {
    ItemType item;
    int i,idx = hash&self->mask;
    for (i=idx; i<idx+self->alloc; i++) {
        int n = i&self->mask;
        if (self->items[n].used > 0) { continue; }
        if (self->items[n].used == 0) { self->used += 1; }
        item.used = 1;
        item.hash = hash;
        item.key = k;
        item.val = v;
        self->items[n] = item;
        self->len += 1;
        return;
    }
    tp_raise(,tp_string("(_tp_dict_hash_set) RuntimeError: ?"));
}

void _tp_dict_tp_realloc(TP,_dict *self,int len) {
    ItemType *items = self->items;
    int i,alloc = self->alloc;
    len = _tp_max(8,len);

    self->items = (ItemType*)tp_malloc(tp, len*sizeof(ItemType));
    self->alloc = len; self->mask = len-1;
    self->len = 0; self->used = 0;

    for (i=0; i<alloc; i++) {
        if (items[i].used != 1) { continue; }
        _tp_dict_hash_set(tp,self,items[i].hash,items[i].key,items[i].val);
    }
    tp_free(tp, items);
}

int _tp_dict_hash_find(TP,_dict *self, int hash, ObjType k) {
    int i,idx = hash&self->mask;
    for (i=idx; i<idx+self->alloc; i++) {
        int n = i&self->mask;
        if (self->items[n].used == 0) { break; }
        if (self->items[n].used < 0) { continue; }
        if (self->items[n].hash != hash) { continue; }
        if (compare(tp,self->items[n].key,k) != 0) { continue; }
        return n;
    }
    return -1;
}
int _tp_dict_find(TP,_dict *self,ObjType k) {
    return _tp_dict_hash_find(tp,self,tp_hash(tp,k),k);
}

void _tp_dict_setx(TP,_dict *self,ObjType k, ObjType v) {
    int hash = tp_hash(tp,k); int n = _tp_dict_hash_find(tp,self,hash,k);
    if (n == -1) {
        if (self->len >= (self->alloc/2)) {
            _tp_dict_tp_realloc(tp,self,self->alloc*2);
        } else if (self->used >= (self->alloc*3/4)) {
            _tp_dict_tp_realloc(tp,self,self->alloc);
        }
        _tp_dict_hash_set(tp,self,hash,k,v);
    } else {
        self->items[n].val = v;
    }
}

void _tp_dict_set(TP,_dict *self,ObjType k, ObjType v) {
    _tp_dict_setx(tp,self,k,v);
    tp_grey(tp,k); tp_grey(tp,v);
}

ObjType _tp_dict_get(TP,_dict *self,ObjType k, const char *error) {
    int n = _tp_dict_find(tp,self,k);
    if (n < 0) {
        tp_raise(tp_None,tp_add(tp,tp_string("(_tp_dict_get) KeyError: "),tp_str(tp,k)));
    }
    return self->items[n].val;
}

void _tp_dict_del(TP,_dict *self,ObjType k, const char *error) {
    int n = _tp_dict_find(tp,self,k);
    if (n < 0) {
        tp_raise(,tp_add(tp,tp_string("(_tp_dict_del) KeyError: "),tp_str(tp,k)));
    }
    self->items[n].used = -1;
    self->len -= 1;
}

_dict *_tp_dict_new(TP) {
    _dict *self = (_dict*)tp_malloc(tp, sizeof(_dict));
    return self;
}
ObjType _tp_dict_copy(TP,ObjType rr) {
    ObjType obj = {TP_DICT};
    _dict *o = rr.dict.val;
    _dict *r = _tp_dict_new(tp);
    *r = *o; r->gci = 0;
    r->items = (ItemType*)tp_malloc(tp, sizeof(ItemType)*o->alloc);
    memcpy(r->items,o->items,sizeof(ItemType)*o->alloc);
    obj.dict.val = r;
    obj.dict.dtype = 1;
    return tp_track(tp,obj);
}

int _tp_dict_next(TP,_dict *self) {
    if (!self->len) {
        tp_raise(0,tp_string("(_tp_dict_next) RuntimeError"));
    }
    while (1) {
        self->cur = ((self->cur + 1) & self->mask);
        if (self->items[self->cur].used > 0) {
            return self->cur;
        }
    }
}

ObjType tp_merge(TP) {
    ObjType self = TP_OBJ();
    ObjType v = TP_OBJ();
    int i; for (i=0; i<v.dict.val->len; i++) {
        int n = _tp_dict_next(tp,v.dict.val);
        _tp_dict_set(tp,self.dict.val,
            v.dict.val->items[n].key,v.dict.val->items[n].val);
    }
    return tp_None;
}

ObjType tp_dict(TP) {
    ObjType r = {TP_DICT};
    r.dict.val = _tp_dict_new(tp);
    r.dict.dtype = 1;
    return tp ? tp_track(tp,r) : r;
}

ObjType tp_dict_n(TP,int n, ObjType* argv) {
    ObjType r = tp_dict(tp);
    int i; for (i=0; i<n; i++) { tp_set(tp,r,argv[i*2],argv[i*2+1]); }
    return r;
}


