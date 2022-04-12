int tp_lua_hash(void const *v, int l)
{
    int i, step = (l >> 5) + 1;
    int h = l + (l >= 4 ? *(int *)v : 0);
    for (i = l; i >= step; i -= step)
    {
        h = h ^ ((h << 5) + (h >> 2) + ((unsigned char *)v)[i - 1]);
    }
    return h;
}
void _tp_dict_free(TP, _dict *self)
{
    tp_free(tp, self->items);
    tp_free(tp, self);
}

int tp_hash(TP, ObjType v)
{
    switch (v.type)
    {
    case NONETYPE:
        return 0;
    case TP_NUMBER:
        return tp_lua_hash(&v.number.val, sizeof(tp_num));
    case TP_STRING:
        return tp_lua_hash(v.string.val, v.string.len);
    case DICTTYPE:
        return tp_lua_hash(&v.dict.val, sizeof(void *));
    case LISTTYPE:
    {
        int r = v.list.val->len;
        int n;
        for (n = 0; n < v.list.val->len; n++)
        {
            ObjType vv = v.list.val->items[n];
            r += vv.type != LISTTYPE ? tp_hash(tp, v.list.val->items[n]) : tp_lua_hash(&vv.list.val, sizeof(void *));
        }
        return r;
    }
    case FUNCTYPE:
        return tp_lua_hash(&v.fnc.info, sizeof(void *));
    case DATATYPE:
        return tp_lua_hash(&v.data.val, sizeof(void *));
    }
    tp_raise(0, tp_string("(tp_hash) TypeError: value unhashable"));
}

void _tp_dict_hash_set(TP, _dict *self, int hash, ObjType k, ObjType v)
{
    ItemType item;
    int i, idx = hash & self->mask;
    for (i = idx; i < idx + self->alloc; i++)
    {
        int n = i & self->mask;
        if (self->items[n].used > 0)
        {
            continue;
        }
        if (self->items[n].used == 0)
        {
            self->used += 1;
        }
        item.used = 1;
        item.hash = hash;
        item.key = k;
        item.val = v;
        self->items[n] = item;
        self->len += 1;
        return;
    }
    tp_raise(, tp_string("(_tp_dict_hash_set) RuntimeError: ?"));
}

void _tp_dict_tp_realloc(TP, _dict *self, int len)
{
    ItemType *items = self->items;
    int i, alloc = self->alloc;
    len = _tp_max(8, len);

    self->items = (ItemType *)tp_malloc(tp, len * sizeof(ItemType));
    self->alloc = len;
    self->mask = len - 1;
    self->len = 0;
    self->used = 0;

    for (i = 0; i < alloc; i++)
    {
        if (items[i].used != 1)
        {
            continue;
        }
        _tp_dict_hash_set(tp, self, items[i].hash, items[i].key, items[i].val);
    }
    tp_free(tp, items);
}

int _tp_dict_hash_find(TP, _dict *self, int hash, ObjType k)
{
    int i, idx = hash & self->mask;
    for (i = idx; i < idx + self->alloc; i++)
    {
        int n = i & self->mask;
        if (self->items[n].used == 0)
        {
            break;
        }
        if (self->items[n].used < 0)
        {
            continue;
        }
        if (self->items[n].hash != hash)
        {
            continue;
        }
        if (compare(tp, self->items[n].key, k) != 0)
        {
            continue;
        }
        return n;
    }
    return -1;
}
int _tp_dict_find(TP, _dict *self, ObjType k)
{
    return _tp_dict_hash_find(tp, self, tp_hash(tp, k), k);
}

void _tp_dict_setx(TP, _dict *self, ObjType k, ObjType v)
{
    int hash = tp_hash(tp, k);
    int n = _tp_dict_hash_find(tp, self, hash, k);
    if (n == -1)
    {
        if (self->len >= (self->alloc / 2))
        {
            _tp_dict_tp_realloc(tp, self, self->alloc * 2);
        }
        else if (self->used >= (self->alloc * 3 / 4))
        {
            _tp_dict_tp_realloc(tp, self, self->alloc);
        }
        _tp_dict_hash_set(tp, self, hash, k, v);
    }
    else
    {
        self->items[n].val = v;
    }
}

void _tp_dict_set(TP, _dict *self, ObjType k, ObjType v)
{
    _tp_dict_setx(tp, self, k, v);
    tp_grey(tp, k);
    tp_grey(tp, v);
}

ObjType _tp_dict_get(TP, _dict *self, ObjType k, const char *error)
{
    int n = _tp_dict_find(tp, self, k);
    if (n < 0)
    {
        tp_raise(NONE, tp_add(tp, tp_string("(_tp_dict_get) KeyError: "), tp_str(tp, k)));
    }
    return self->items[n].val;
}

void _tp_dict_del(TP, _dict *self, ObjType k, const char *error)
{
    int n = _tp_dict_find(tp, self, k);
    if (n < 0)
    {
        tp_raise(, tp_add(tp, tp_string("(_tp_dict_del) KeyError: "), tp_str(tp, k)));
    }
    self->items[n].used = -1;
    self->len -= 1;
}

_dict *_tp_dict_new(TP)
{
    _dict *self = (_dict *)tp_malloc(tp, sizeof(_dict));
    return self;
}
ObjType _tp_dict_copy(TP, ObjType rr)
{
    ObjType obj = {DICTTYPE};
    _dict *o = rr.dict.val;
    _dict *r = _tp_dict_new(tp);
    *r = *o;
    r->gci = 0;
    r->items = (ItemType *)tp_malloc(tp, sizeof(ItemType) * o->alloc);
    memcpy(r->items, o->items, sizeof(ItemType) * o->alloc);
    obj.dict.val = r;
    obj.dict.dtype = 1;
    return tp_track(tp, obj);
}

int _tp_dict_next(TP, _dict *self)
{
    if (!self->len)
    {
        tp_raise(0, tp_string("(_tp_dict_next) RuntimeError"));
    }
    while (1)
    {
        self->cur = ((self->cur + 1) & self->mask);
        if (self->items[self->cur].used > 0)
        {
            return self->cur;
        }
    }
}

ObjType tp_merge(TP)
{
    ObjType self = TP_OBJ();
    ObjType v = TP_OBJ();
    int i;
    for (i = 0; i < v.dict.val->len; i++)
    {
        int n = _tp_dict_next(tp, v.dict.val);
        _tp_dict_set(tp, self.dict.val,
                     v.dict.val->items[n].key, v.dict.val->items[n].val);
    }
    return NONE;
}

ObjType tp_dict(TP)
{
    ObjType r = {DICTTYPE};
    r.dict.val = _tp_dict_new(tp);
    r.dict.dtype = 1;
    return tp ? tp_track(tp, r) : r;
}

ObjType tp_dict_n(TP, int n, ObjType *argv)
{
    ObjType r = tp_dict(tp);
    int i;
    for (i = 0; i < n; i++)
    {
        tp_set(tp, r, argv[i * 2], argv[i * 2 + 1]);
    }
    return r;
}

void realloc_list(TP, _list *self, int len)
{
    if (!len)
    {
        len = 1;
    }
    self->items = (ObjType *)tp_realloc(tp, self->items, len * sizeof(ObjType));
    self->alloc = len;
}

void set_list(TP, _list *self, int k, ObjType v, const char *error)
{
    if (k >= self->len)
    {
        tp_raise(, tp_string("(set_list) KeyError"));
    }
    self->items[k] = v;
    tp_grey(tp, v);
}
void free_list(TP, _list *self)
{
    tp_free(tp, self->items);
    tp_free(tp, self);
}

ObjType get_list(TP, _list *self, int k, const char *error)
{
    if (k >= self->len)
    {
        tp_raise(NONE, tp_string("(set_list) KeyError"));
    }
    return self->items[k];
}
void insertx_list(TP, _list *self, int n, ObjType v)
{
    if (self->len >= self->alloc)
    {
        realloc_list(tp, self, self->alloc * 2);
    }
    if (n < self->len)
    {
        memmove(&self->items[n + 1], &self->items[n], sizeof(ObjType) * (self->len - n));
    }
    self->items[n] = v;
    self->len += 1;
}
void app_list(TP, _list *self, ObjType v)
{
    insertx_list(tp, self, self->len, v);
}
void insert_list(TP, _list *self, int n, ObjType v)
{
    insertx_list(tp, self, n, v);
    tp_grey(tp, v);
}
void append_list(TP, _list *self, ObjType v)
{
    insert_list(tp, self, self->len, v);
}
ObjType pop_list(TP, _list *self, int n, const char *error)
{
    ObjType r = get_list(tp, self, n, error);
    if (n != self->len - 1)
    {
        memmove(&self->items[n], &self->items[n + 1], sizeof(ObjType) * (self->len - (n + 1)));
    }
    self->len -= 1;
    return r;
}

int find_list(TP, _list *self, ObjType v)
{
    int n;
    for (n = 0; n < self->len; n++)
    {
        if (compare(tp, v, self->items[n]) == 0)
        {
            return n;
        }
    }
    return -1;
}

ObjType index_list(TP)
{
    ObjType self = TP_OBJ();
    ObjType v = TP_OBJ();
    int i = find_list(tp, self.list.val, v);
    if (i < 0)
    {
        tp_raise(NONE, tp_string("(index_list) ValueError: list.index(x): x not in list"));
    }
    return tp_number(i);
}

_list *new_list(TP)
{
    return (_list *)tp_malloc(tp, sizeof(_list));
}

ObjType cp_list(TP, ObjType rr)
{
    ObjType val = {LISTTYPE};
    _list *o = rr.list.val;
    _list *r = new_list(tp);
    *r = *o;
    r->gci = 0;
    r->items = (ObjType *)tp_malloc(tp, sizeof(ObjType) * o->len);
    memcpy(r->items, o->items, sizeof(ObjType) * o->len);
    val.list.val = r;
    return tp_track(tp, val);
}

ObjType append(TP)
{
    ObjType self = TP_OBJ();
    ObjType v = TP_OBJ();
    append_list(tp, self.list.val, v);
    return NONE;
}

ObjType pop(TP)
{
    ObjType self = TP_OBJ();
    return pop_list(tp, self.list.val, self.list.val->len - 1, "pop");
}

ObjType insert(TP)
{
    ObjType self = TP_OBJ();
    int n = TP_NUM();
    ObjType v = TP_OBJ();
    insert_list(tp, self.list.val, n, v);
    return NONE;
}

ObjType extend(TP)
{
    ObjType self = TP_OBJ();
    ObjType v = TP_OBJ();
    int i;
    for (i = 0; i < v.list.val->len; i++)
    {
        append_list(tp, self.list.val, v.list.val->items[i]);
    }
    return NONE;
}

ObjType list_nt(TP)
{
    ObjType r = {LISTTYPE};
    r.list.val = new_list(tp);
    return r;
}

ObjType to_list(TP)
{
    ObjType r = {LISTTYPE};
    r.list.val = new_list(tp);
    return tp_track(tp, r);
}

ObjType to_list_n(TP, int n, ObjType *argv)
{
    int i;
    ObjType r = to_list(tp);
    realloc_list(tp, r.list.val, n);
    for (i = 0; i < n; i++)
    {
        append_list(tp, r.list.val, argv[i]);
    }
    return r;
}

int sort_compare(ObjType *a, ObjType *b)
{
    return compare(0, *a, *b);
}

ObjType sort(TP)
{
    ObjType self = TP_OBJ();
    qsort(self.list.val->items, self.list.val->len, sizeof(ObjType), (int (*)(const void *, const void *))sort_compare);
    return NONE;
}
