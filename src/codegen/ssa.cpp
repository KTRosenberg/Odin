struct ssaModule;
struct ssaProcedure;
struct ssaBlock;
struct ssaValue;


struct ssaModule {
	CheckerInfo * info;
	BaseTypeSizes sizes;
	gbArena       arena;
	gbAllocator   allocator;

	u32 stmt_state_flags;

	String layout;

	Map<ssaValue *> values;  // Key: Entity *
	Map<ssaValue *> members; // Key: String
	i32 global_string_index;
};


struct ssaBlock {
	i32 id;
	AstNode *node;
	Scope *scope;
	isize scope_index;
	String label;
	ssaProcedure *parent;
	b32 added;

	gbArray(ssaValue *) instrs;
	gbArray(ssaValue *) values;
};

struct ssaTargetList {
	ssaTargetList *prev;
	ssaBlock *     break_;
	ssaBlock *     continue_;
	ssaBlock *     fallthrough_;
};

enum ssaDeferKind {
	ssaDefer_Default,
	ssaDefer_Return,
	ssaDefer_Branch,
};
struct ssaDefer {
	AstNode *stmt;
	isize scope_index;
	ssaBlock *block;
};

struct ssaProcedure {
	ssaProcedure *parent;
	gbArray(ssaProcedure *) children;

	ssaModule *   module;
	String        name;
	Type *        type;
	AstNode *     type_expr;
	AstNode *     body;
	u64           tags;

	isize               scope_index;
	gbArray(ssaDefer)   defer_stmts;
	gbArray(ssaBlock *) blocks;
	ssaBlock *          curr_block;
	ssaTargetList *     target_list;
};

#define SSA_STARTUP_RUNTIME_PROC_NAME  "__$startup_runtime"
#define SSA_TYPE_INFO_DATA_NAME        "__$type_info_data"
#define SSA_TYPE_INFO_DATA_MEMBER_NAME "__$type_info_data_member"


#define SSA_INSTR_KINDS \
	SSA_INSTR_KIND(Invalid), \
	SSA_INSTR_KIND(Comment), \
	SSA_INSTR_KIND(Local), \
	SSA_INSTR_KIND(Store), \
	SSA_INSTR_KIND(Load), \
	SSA_INSTR_KIND(GetElementPtr), \
	SSA_INSTR_KIND(ExtractValue), \
	SSA_INSTR_KIND(Conv), \
	SSA_INSTR_KIND(Br), \
	SSA_INSTR_KIND(Ret), \
	SSA_INSTR_KIND(Select), \
	SSA_INSTR_KIND(Unreachable), \
	SSA_INSTR_KIND(BinaryOp), \
	SSA_INSTR_KIND(Call), \
	SSA_INSTR_KIND(NoOp), \
	SSA_INSTR_KIND(ExtractElement), \
	SSA_INSTR_KIND(InsertElement), \
	SSA_INSTR_KIND(ShuffleVector), \
	SSA_INSTR_KIND(StartupRuntime), \
	SSA_INSTR_KIND(Count),

enum ssaInstrKind {
#define SSA_INSTR_KIND(x) GB_JOIN2(ssaInstr_, x)
	SSA_INSTR_KINDS
#undef SSA_INSTR_KIND
};

String const ssa_instr_strings[] = {
#define SSA_INSTR_KIND(x) {cast(u8 *)#x, gb_size_of(#x)-1}
	SSA_INSTR_KINDS
#undef SSA_INSTR_KIND
};

#define SSA_CONV_KINDS \
	SSA_CONV_KIND(Invalid), \
	SSA_CONV_KIND(trunc), \
	SSA_CONV_KIND(zext), \
	SSA_CONV_KIND(fptrunc), \
	SSA_CONV_KIND(fpext), \
	SSA_CONV_KIND(fptoui), \
	SSA_CONV_KIND(fptosi), \
	SSA_CONV_KIND(uitofp), \
	SSA_CONV_KIND(sitofp), \
	SSA_CONV_KIND(ptrtoint), \
	SSA_CONV_KIND(inttoptr), \
	SSA_CONV_KIND(bitcast), \
	SSA_CONV_KIND(Count)

enum ssaConvKind {
#define SSA_CONV_KIND(x) GB_JOIN2(ssaConv_, x)
	SSA_CONV_KINDS
#undef SSA_CONV_KIND
};

String const ssa_conv_strings[] = {
#define SSA_CONV_KIND(x) {cast(u8 *)#x, gb_size_of(#x)-1}
	SSA_CONV_KINDS
#undef SSA_CONV_KIND
};

struct ssaInstr {
	ssaInstrKind kind;

	ssaBlock *parent;
	Type *type;

	union {
		struct {
			String text;
		} Comment;

		struct {
			Entity *entity;
			Type *  type;
			b32     zero_initialized;
		} Local;
		struct {
			ssaValue *address;
			ssaValue *value;
		} Store;
		struct {
			Type *type;
			ssaValue *address;
		} Load;
		struct {
			ssaValue *address;
			Type *    result_type;
			Type *    elem_type;
			ssaValue *indices[2];
			isize     index_count;
			b32       inbounds;
		} GetElementPtr;
		struct {
			ssaValue *address;
			Type *    result_type;
			Type *    elem_type;
			i32       index;
		} ExtractValue;
		struct {
			ssaConvKind kind;
			ssaValue *value;
			Type *from, *to;
		} Conv;
		struct {
			ssaValue *cond;
			ssaBlock *true_block;
			ssaBlock *false_block;
		} Br;
		struct { ssaValue *value; } Ret;
		struct {} Unreachable;
		struct {
			ssaValue *cond;
			ssaValue *true_value;
			ssaValue *false_value;
		} Select;
		struct {
			Type *type;
			Token op;
			ssaValue *left, *right;
		} BinaryOp;
		struct {
			Type *type; // return type
			ssaValue *value;
			ssaValue **args;
			isize arg_count;
		} Call;
		struct {
			ssaValue *vector;
			ssaValue *index;
		} ExtractElement;
		struct {
			ssaValue *vector;
			ssaValue *elem;
			ssaValue *index;
		} InsertElement;
		struct {
			ssaValue *vector;
			i32 *indices;
			isize index_count;
			Type *type;
		} ShuffleVector;

		struct {} StartupRuntime;
	};
};


enum ssaValueKind {
	ssaValue_Invalid,

	ssaValue_Constant,
	ssaValue_TypeName,
	ssaValue_Global,
	ssaValue_Param,

	ssaValue_Proc,
	ssaValue_Block,
	ssaValue_Instr,

	ssaValue_Count,
};

struct ssaValue {
	ssaValueKind kind;
	i32 id;

	union {
		struct {
			Type *     type;
			ExactValue value;
		} Constant;
		struct {
			String name;
			Type *  type;
		} TypeName;
		struct {
			b32 is_constant;
			b32 is_private;
			b32 is_thread_local;
			Entity *  entity;
			Type *    type;
			ssaValue *value;
		} Global;
		struct {
			ssaProcedure *parent;
			Entity *entity;
			Type *  type;
		} Param;
		ssaProcedure Proc;
		ssaBlock     Block;
		ssaInstr     Instr;
	};
};

gb_global ssaValue *v_zero    = NULL;
gb_global ssaValue *v_one     = NULL;
gb_global ssaValue *v_zero32  = NULL;
gb_global ssaValue *v_one32   = NULL;
gb_global ssaValue *v_two32   = NULL;
gb_global ssaValue *v_false   = NULL;
gb_global ssaValue *v_true    = NULL;

struct ssaAddr {
	ssaValue *addr;
	AstNode *expr; // NOTE(bill): Just for testing - probably remove later

	// HACK(bill): Fix how lvalues for vectors work
	b32 is_vector;
	ssaValue *index;
};

ssaAddr ssa_make_addr(ssaValue *addr, AstNode *expr) {
	ssaAddr v = {addr, expr, false, NULL};
	return v;
}

ssaAddr ssa_make_addr_vector(ssaValue *addr, ssaValue *index, AstNode *expr) {
	ssaAddr v = {addr, expr, true, index};
	return v;
}


ssaValue *ssa_make_value_global(gbAllocator a, Entity *e, ssaValue *value);


void ssa_module_add_value(ssaModule *m, Entity *e, ssaValue *v) {
	map_set(&m->values, hash_pointer(e), v);
}

void ssa_init_module(ssaModule *m, Checker *c) {
	// TODO(bill): Determine a decent size for the arena
	isize token_count = c->parser->total_token_count;
	isize arena_size = 4 * token_count * gb_size_of(ssaValue);
	gb_arena_init_from_allocator(&m->arena, gb_heap_allocator(), arena_size);
	m->allocator = gb_arena_allocator(&m->arena);
	m->info = &c->info;
	m->sizes = c->sizes;

	map_init(&m->values,  gb_heap_allocator());
	map_init(&m->members, gb_heap_allocator());

	// Default states
	m->stmt_state_flags = 0;
	m->stmt_state_flags |= StmtStateFlag_abc;

	{
		// Add type info data
		{
			String name = make_string(SSA_TYPE_INFO_DATA_NAME);
			Token token = {Token_Identifier};
			token.string = name;


			isize count = gb_array_count(c->info.type_info_map.entries);
			Entity *e = make_entity_variable(m->allocator, NULL, token, make_type_array(m->allocator, t_type_info, count));
			ssaValue *g = ssa_make_value_global(m->allocator, e, NULL);
			g->Global.is_private  = true;
			ssa_module_add_value(m, e, g);
			map_set(&m->members, hash_string(name), g);
		}

		// Type info member buffer
		{
			// NOTE(bill): Removes need for heap allocation by making it global memory
			isize count = 0;

			gb_for_array(entry_index, m->info->type_info_map.entries) {
				auto *entry = &m->info->type_info_map.entries[entry_index];
				Type *t = cast(Type *)cast(uintptr)entry->key.key;

				switch (t->kind) {
				case Type_Record:
					switch (t->Record.kind) {
					case TypeRecord_Struct:
					case TypeRecord_RawUnion:
						count += t->Record.field_count;
					}
					break;
				case Type_Tuple:
					count += t->Tuple.variable_count;
					break;
				}
			}

			String name = make_string(SSA_TYPE_INFO_DATA_MEMBER_NAME);
			Token token = {Token_Identifier};
			token.string = name;

			Entity *e = make_entity_variable(m->allocator, NULL, token,
			                                 make_type_array(m->allocator, t_type_info_member, count));
			ssaValue *g = ssa_make_value_global(m->allocator, e, NULL);
			ssa_module_add_value(m, e, g);
			map_set(&m->members, hash_string(name), g);
		}
	}
}

void ssa_destroy_module(ssaModule *m) {
	map_destroy(&m->values);
	map_destroy(&m->members);
	gb_arena_free(&m->arena);
}


Type *ssa_type(ssaValue *value);
Type *ssa_type(ssaInstr *instr) {
	switch (instr->kind) {
	case ssaInstr_Local:
		return instr->Local.type;
	case ssaInstr_Store:
		return ssa_type(instr->Store.value);
	case ssaInstr_Load:
		return instr->Load.type;
	case ssaInstr_GetElementPtr:
		return instr->GetElementPtr.result_type;
	case ssaInstr_ExtractValue:
		return instr->ExtractValue.result_type;
	case ssaInstr_BinaryOp:
		return instr->BinaryOp.type;
	case ssaInstr_Conv:
		return instr->Conv.to;
	case ssaInstr_Select:
		return ssa_type(instr->Select.true_value);
	case ssaInstr_Call: {
		Type *pt = get_base_type(instr->Call.type);
		if (pt != NULL) {
			if (pt->kind == Type_Tuple && pt->Tuple.variable_count == 1)
				return pt->Tuple.variables[0]->type;
			return pt;
		}
		return NULL;
	} break;
	case ssaInstr_ExtractElement: {
		Type *vt = ssa_type(instr->ExtractElement.vector);
		Type *bt = base_vector_type(get_base_type(vt));
		GB_ASSERT(!is_type_vector(bt));
		return bt;
	} break;
	case ssaInstr_InsertElement:
		return ssa_type(instr->InsertElement.vector);
	case ssaInstr_ShuffleVector:
		return instr->ShuffleVector.type;
	}
	return NULL;
}

Type *ssa_type(ssaValue *value) {
	switch (value->kind) {
	case ssaValue_Constant:
		return value->Constant.type;
	case ssaValue_TypeName:
		return value->TypeName.type;
	case ssaValue_Global:
		return value->Global.type;
	case ssaValue_Param:
		return value->Param.type;
	case ssaValue_Proc:
		return value->Proc.type;
	case ssaValue_Instr:
		return ssa_type(&value->Instr);
	}
	return NULL;
}




ssaValue *ssa_build_expr(ssaProcedure *proc, AstNode *expr);
ssaValue *ssa_build_single_expr(ssaProcedure *proc, AstNode *expr, TypeAndValue *tv);
ssaAddr   ssa_build_addr(ssaProcedure *proc, AstNode *expr);
ssaValue *ssa_emit_conv(ssaProcedure *proc, ssaValue *value, Type *a_type, b32 is_argument = false);
ssaValue *ssa_emit_transmute(ssaProcedure *proc, ssaValue *value, Type *a_type);
void      ssa_build_proc(ssaValue *value, ssaProcedure *parent);




ssaValue *ssa_alloc_value(gbAllocator a, ssaValueKind kind) {
	ssaValue *v = gb_alloc_item(a, ssaValue);
	v->kind = kind;
	return v;
}

ssaValue *ssa_alloc_instr(ssaProcedure *proc, ssaInstrKind kind) {
	ssaValue *v = ssa_alloc_value(proc->module->allocator, ssaValue_Instr);
	v->Instr.kind = kind;
	if (proc->curr_block) {
		gb_array_append(proc->curr_block->values, v);
	}
	return v;
}

ssaValue *ssa_make_value_type_name(gbAllocator a, String name, Type *type) {
	ssaValue *v = ssa_alloc_value(a, ssaValue_TypeName);
	v->TypeName.name = name;
	v->TypeName.type = type;
	return v;
}

ssaValue *ssa_make_value_global(gbAllocator a, Entity *e, ssaValue *value) {
	ssaValue *v = ssa_alloc_value(a, ssaValue_Global);
	v->Global.entity = e;
	v->Global.type = make_type_pointer(a, e->type);
	v->Global.value = value;
	return v;
}
ssaValue *ssa_make_value_param(gbAllocator a, ssaProcedure *parent, Entity *e) {
	ssaValue *v = ssa_alloc_value(a, ssaValue_Param);
	v->Param.parent = parent;
	v->Param.entity = e;
	v->Param.type   = e->type;
	return v;
}


ssaValue *ssa_make_instr_local(ssaProcedure *p, Entity *e, b32 zero_initialized) {
	ssaValue *v = ssa_alloc_instr(p, ssaInstr_Local);
	ssaInstr *i = &v->Instr;
	i->Local.entity = e;
	i->Local.type = make_type_pointer(p->module->allocator, e->type);
	i->Local.zero_initialized = zero_initialized;
	ssa_module_add_value(p->module, e, v);
	return v;
}


ssaValue *ssa_make_instr_store(ssaProcedure *p, ssaValue *address, ssaValue *value) {
	ssaValue *v = ssa_alloc_instr(p, ssaInstr_Store);
	ssaInstr *i = &v->Instr;
	i->Store.address = address;
	i->Store.value = value;
	return v;
}

ssaValue *ssa_make_instr_load(ssaProcedure *p, ssaValue *address) {
	ssaValue *v = ssa_alloc_instr(p, ssaInstr_Load);
	ssaInstr *i = &v->Instr;
	i->Load.address = address;
	i->Load.type = type_deref(ssa_type(address));
	return v;
}

ssaValue *ssa_make_instr_get_element_ptr(ssaProcedure *p, ssaValue *address,
                                         ssaValue *index0, ssaValue *index1, isize index_count,
                                         b32 inbounds) {
	ssaValue *v = ssa_alloc_instr(p, ssaInstr_GetElementPtr);
	ssaInstr *i = &v->Instr;
	i->GetElementPtr.address = address;
	i->GetElementPtr.indices[0]   = index0;
	i->GetElementPtr.indices[1]   = index1;
	i->GetElementPtr.index_count  = index_count;
	i->GetElementPtr.elem_type    = ssa_type(address);
	i->GetElementPtr.inbounds     = inbounds;
	GB_ASSERT_MSG(is_type_pointer(ssa_type(address)),
	              "%s", type_to_string(ssa_type(address)));
	return v;
}

ssaValue *ssa_make_instr_extract_value(ssaProcedure *p, ssaValue *address, i32 index, Type *result_type) {
	ssaValue *v = ssa_alloc_instr(p, ssaInstr_ExtractValue);
	ssaInstr *i = &v->Instr;
	i->ExtractValue.address = address;
	i->ExtractValue.index = index;
	i->ExtractValue.result_type = result_type;
	Type *et = ssa_type(address);
	i->ExtractValue.elem_type = et;
	// GB_ASSERT(et->kind == Type_Struct || et->kind == Type_Array || et->kind == Type_Tuple);
	return v;
}


ssaValue *ssa_make_instr_binary_op(ssaProcedure *p, Token op, ssaValue *left, ssaValue *right, Type *type) {
	ssaValue *v = ssa_alloc_instr(p, ssaInstr_BinaryOp);
	ssaInstr *i = &v->Instr;
	i->BinaryOp.op = op;
	i->BinaryOp.left = left;
	i->BinaryOp.right = right;
	i->BinaryOp.type = type;
	return v;
}

ssaValue *ssa_make_instr_br(ssaProcedure *p, ssaValue *cond, ssaBlock *true_block, ssaBlock *false_block) {
	ssaValue *v = ssa_alloc_instr(p, ssaInstr_Br);
	ssaInstr *i = &v->Instr;
	i->Br.cond = cond;
	i->Br.true_block = true_block;
	i->Br.false_block = false_block;
	return v;
}

ssaValue *ssa_make_instr_unreachable(ssaProcedure *p) {
	ssaValue *v = ssa_alloc_instr(p, ssaInstr_Unreachable);
	return v;
}

ssaValue *ssa_make_instr_ret(ssaProcedure *p, ssaValue *value) {
	ssaValue *v = ssa_alloc_instr(p, ssaInstr_Ret);
	v->Instr.Ret.value = value;
	return v;
}

ssaValue *ssa_make_instr_select(ssaProcedure *p, ssaValue *cond, ssaValue *t, ssaValue *f) {
	ssaValue *v = ssa_alloc_instr(p, ssaInstr_Select);
	v->Instr.Select.cond = cond;
	v->Instr.Select.true_value = t;
	v->Instr.Select.false_value = f;
	return v;
}

ssaValue *ssa_make_instr_call(ssaProcedure *p, ssaValue *value, ssaValue **args, isize arg_count, Type *result_type) {
	ssaValue *v = ssa_alloc_instr(p, ssaInstr_Call);
	v->Instr.Call.value = value;
	v->Instr.Call.args = args;
	v->Instr.Call.arg_count = arg_count;
	v->Instr.Call.type = result_type;
	return v;
}

ssaValue *ssa_make_instr_conv(ssaProcedure *p, ssaConvKind kind, ssaValue *value, Type *from, Type *to) {
	ssaValue *v = ssa_alloc_instr(p, ssaInstr_Conv);
	v->Instr.Conv.kind = kind;
	v->Instr.Conv.value = value;
	v->Instr.Conv.from = from;
	v->Instr.Conv.to = to;
	return v;
}

ssaValue *ssa_make_instr_extract_element(ssaProcedure *p, ssaValue *vector, ssaValue *index) {
	ssaValue *v = ssa_alloc_instr(p, ssaInstr_ExtractElement);
	v->Instr.ExtractElement.vector = vector;
	v->Instr.ExtractElement.index = index;
	return v;
}

ssaValue *ssa_make_instr_insert_element(ssaProcedure *p, ssaValue *vector, ssaValue *elem, ssaValue *index) {
	ssaValue *v = ssa_alloc_instr(p, ssaInstr_InsertElement);
	v->Instr.InsertElement.vector = vector;
	v->Instr.InsertElement.elem   = elem;
	v->Instr.InsertElement.index  = index;
	return v;
}

ssaValue *ssa_make_instr_shuffle_vector(ssaProcedure *p, ssaValue *vector, i32 *indices, isize index_count) {
	ssaValue *v = ssa_alloc_instr(p, ssaInstr_ShuffleVector);
	v->Instr.ShuffleVector.vector      = vector;
	v->Instr.ShuffleVector.indices     = indices;
	v->Instr.ShuffleVector.index_count = index_count;

	Type *vt = get_base_type(ssa_type(vector));
	v->Instr.ShuffleVector.type = make_type_vector(p->module->allocator, vt->Vector.elem, index_count);

	return v;
}

ssaValue *ssa_make_instr_no_op(ssaProcedure *p) {
	ssaValue *v = ssa_alloc_instr(p, ssaInstr_NoOp);
	return v;
}


ssaValue *ssa_make_instr_comment(ssaProcedure *p, String text) {
	ssaValue *v = ssa_alloc_instr(p, ssaInstr_Comment);
	v->Instr.Comment.text = text;
	return v;
}



ssaValue *ssa_make_value_constant(gbAllocator a, Type *type, ExactValue value) {
	ssaValue *v = ssa_alloc_value(a, ssaValue_Constant);
	v->Constant.type  = type;
	v->Constant.value = value;
	return v;
}

ssaValue *ssa_make_value_procedure(gbAllocator a, ssaModule *m, Type *type, AstNode *type_expr, AstNode *body, String name) {
	ssaValue *v = ssa_alloc_value(a, ssaValue_Proc);
	v->Proc.module = m;
	v->Proc.type   = type;
	v->Proc.type_expr = type_expr;
	v->Proc.body   = body;
	v->Proc.name   = name;
	return v;
}

ssaValue *ssa_make_value_block(ssaProcedure *proc, AstNode *node, Scope *scope, String label) {
	ssaValue *v = ssa_alloc_value(proc->module->allocator, ssaValue_Block);
	v->Block.label  = label;
	v->Block.node   = node;
	v->Block.scope  = scope;
	v->Block.parent = proc;

	gb_array_init(v->Block.instrs, gb_heap_allocator());
	gb_array_init(v->Block.values, gb_heap_allocator());

	return v;
}

b32 ssa_is_blank_ident(AstNode *node) {
	if (node->kind == AstNode_Ident) {
		ast_node(i, Ident, node);
		return is_blank_ident(i->string);
	}
	return false;
}


ssaInstr *ssa_get_last_instr(ssaBlock *block) {
	if (block != NULL) {
		isize len = 0;
		if (block->instrs != NULL) {
			len = gb_array_count(block->instrs);
		}
		if (len > 0) {
			ssaValue *v = block->instrs[len-1];
			GB_ASSERT(v->kind == ssaValue_Instr);
			return &v->Instr;
		}
	}
	return NULL;

}

b32 ssa_is_instr_terminating(ssaInstr *i) {
	if (i != NULL) {
		switch (i->kind) {
		case ssaInstr_Ret:
		case ssaInstr_Unreachable:
			return true;
		}
	}

	return false;
}

ssaValue *ssa_emit(ssaProcedure *proc, ssaValue *instr) {
	GB_ASSERT(instr->kind == ssaValue_Instr);
	ssaBlock *b = proc->curr_block;
	instr->Instr.parent = b;
	if (b != NULL) {
		ssaInstr *i = ssa_get_last_instr(b);
		if (!ssa_is_instr_terminating(i)) {
			gb_array_append(b->instrs, instr);
		}
	}
	return instr;
}
ssaValue *ssa_emit_store(ssaProcedure *p, ssaValue *address, ssaValue *value) {
	return ssa_emit(p, ssa_make_instr_store(p, address, value));
}
ssaValue *ssa_emit_load(ssaProcedure *p, ssaValue *address) {
	return ssa_emit(p, ssa_make_instr_load(p, address));
}
ssaValue *ssa_emit_select(ssaProcedure *p, ssaValue *cond, ssaValue *t, ssaValue *f) {
	return ssa_emit(p, ssa_make_instr_select(p, cond, t, f));
}

ssaValue *ssa_emit_comment(ssaProcedure *p, String text) {
	return ssa_emit(p, ssa_make_instr_comment(p, text));
}


ssaValue *ssa_add_local(ssaProcedure *proc, Entity *e, b32 zero_initialized = true) {
	return ssa_emit(proc, ssa_make_instr_local(proc, e, zero_initialized));
}

ssaValue *ssa_add_local_for_identifier(ssaProcedure *proc, AstNode *name, b32 zero_initialized) {
	Entity **found = map_get(&proc->module->info->definitions, hash_pointer(name));
	if (found) {
		Entity *e = *found;
		ssa_emit_comment(proc, e->token.string);
		return ssa_add_local(proc, e, zero_initialized);
	}
	return NULL;
}

ssaValue *ssa_add_local_generated(ssaProcedure *proc, Type *type) {
	GB_ASSERT(type != NULL);

	Scope *scope = NULL;
	if (proc->curr_block) {
		scope = proc->curr_block->scope;
	}
	Entity *entity = make_entity_variable(proc->module->allocator,
	                                      scope,
	                                      empty_token,
	                                      type);
	return ssa_emit(proc, ssa_make_instr_local(proc, entity, true));
}

ssaValue *ssa_add_param(ssaProcedure *proc, Entity *e) {
	ssaValue *v = ssa_make_value_param(proc->module->allocator, proc, e);
	ssaValue *l = ssa_add_local(proc, e);
	ssa_emit_store(proc, l, v);
	return v;
}


ssaValue *ssa_emit_call(ssaProcedure *p, ssaValue *value, ssaValue **args, isize arg_count) {
	Type *pt = get_base_type(ssa_type(value));
	GB_ASSERT(pt->kind == Type_Proc);
	Type *results = pt->Proc.results;
	return ssa_emit(p, ssa_make_instr_call(p, value, args, arg_count, results));
}

ssaValue *ssa_emit_global_call(ssaProcedure *proc, char *name_, ssaValue **args, isize arg_count) {
	String name = make_string(name_);
	ssaValue **found = map_get(&proc->module->members, hash_string(name));
	GB_ASSERT_MSG(found != NULL, "%.*s", LIT(name));
	ssaValue *gp = *found;
	return ssa_emit_call(proc, gp, args, arg_count);
}


Type *ssa_addr_type(ssaAddr lval) {
	if (lval.addr != NULL) {
		return type_deref(ssa_type(lval.addr));
	}
	return NULL;
}

ssaBlock *ssa__make_block(ssaProcedure *proc, AstNode *node, String label) {
	Scope *scope = NULL;
	if (node != NULL) {
		Scope **found = map_get(&proc->module->info->scopes, hash_pointer(node));
		if (found) {
			scope = *found;
		} else {
			GB_PANIC("Block scope not found for %.*s", LIT(ast_node_strings[node->kind]));
		}
	}

	ssaValue *block = ssa_make_value_block(proc, node, scope, label);
	return &block->Block;
}

ssaBlock *ssa_add_block(ssaProcedure *proc, AstNode *node, String label) {
	ssaBlock *block = ssa__make_block(proc, node, label);
	gb_array_append(proc->blocks, block);
	return block;
}

void ssa_build_stmt(ssaProcedure *proc, AstNode *s);
void ssa_emit_no_op(ssaProcedure *proc);
void ssa_emit_jump(ssaProcedure *proc, ssaBlock *block);

void ssa_build_defer_stmt(ssaProcedure *proc, ssaDefer d) {
	ssaBlock *b = ssa__make_block(proc, NULL, make_string("defer"));
	// HACK(bill): The prev block may defer injection before it's terminator
	ssaInstr *last_instr = ssa_get_last_instr(proc->curr_block);
	if (last_instr == NULL || !ssa_is_instr_terminating(last_instr)) {
		ssa_emit_jump(proc, b);
	}
	gb_array_append(proc->blocks, b);
	proc->curr_block = b;
	ssa_emit_comment(proc, make_string("defer"));
	ssa_build_stmt(proc, d.stmt);
}

void ssa_emit_defer_stmts(ssaProcedure *proc, ssaDeferKind kind, ssaBlock *block) {
	isize count = gb_array_count(proc->defer_stmts);
	isize i = count;
	while (i --> 0) {
		ssaDefer d = proc->defer_stmts[i];
		if (kind == ssaDefer_Return) {
			ssa_build_defer_stmt(proc, d);
		} else if (kind == ssaDefer_Default) {
			if (proc->scope_index == d.scope_index &&
			    d.scope_index > 1) {
				ssa_build_defer_stmt(proc, d);
				gb_array_pop(proc->defer_stmts);
				continue;
			} else {
				break;
			}
		} else if (kind == ssaDefer_Branch) {
			GB_ASSERT(block != NULL);
			isize lower_limit = block->scope_index+1;
			if (lower_limit < d.scope_index) {
				ssa_build_defer_stmt(proc, d);
			}
		}
	}
}




void ssa_emit_unreachable(ssaProcedure *proc) {
	ssa_emit(proc, ssa_make_instr_unreachable(proc));
}

void ssa_emit_ret(ssaProcedure *proc, ssaValue *v) {
	ssa_emit_defer_stmts(proc, ssaDefer_Return, NULL);
	ssa_emit(proc, ssa_make_instr_ret(proc, v));
}

void ssa_emit_jump(ssaProcedure *proc, ssaBlock *block) {
	ssa_emit(proc, ssa_make_instr_br(proc, NULL, block, NULL));
	proc->curr_block = NULL;
}

void ssa_emit_if(ssaProcedure *proc, ssaValue *cond, ssaBlock *true_block, ssaBlock *false_block) {
	ssaValue *br = ssa_make_instr_br(proc, cond, true_block, false_block);
	ssa_emit(proc, br);
	proc->curr_block = NULL;
}

void ssa_emit_no_op(ssaProcedure *proc) {
	ssa_emit(proc, ssa_make_instr_no_op(proc));
}



ssaValue *ssa_lvalue_store(ssaProcedure *proc, ssaAddr lval, ssaValue *value) {
	if (lval.addr != NULL) {
		if (lval.is_vector) {
			// HACK(bill): Fix how lvalues for vectors work
			ssaValue *v = ssa_emit_load(proc, lval.addr);
			Type *elem_type = get_base_type(ssa_type(v))->Vector.elem;
			ssaValue *elem = ssa_emit_conv(proc, value, elem_type);
			ssaValue *out = ssa_emit(proc, ssa_make_instr_insert_element(proc, v, elem, lval.index));
			return ssa_emit_store(proc, lval.addr, out);
		} else {
			// gb_printf_err("%s <- %s\n", type_to_string(ssa_addr_type(lval)), type_to_string(ssa_type(value)));
			// gb_printf_err("%.*s - %s\n", LIT(ast_node_strings[lval.expr->kind]), expr_to_string(lval.expr));
			value = ssa_emit_conv(proc, value, ssa_addr_type(lval));
			return ssa_emit_store(proc, lval.addr, value);
		}
	}
	return NULL;
}
ssaValue *ssa_lvalue_load(ssaProcedure *proc, ssaAddr lval) {
	if (lval.addr != NULL) {
		if (lval.is_vector) {
			// HACK(bill): Fix how lvalues for vectors work
			ssaValue *v = ssa_emit_load(proc, lval.addr);
			return ssa_emit(proc, ssa_make_instr_extract_element(proc, v, lval.index));
		}
		return ssa_emit_load(proc, lval.addr);
	}
	GB_PANIC("Illegal lvalue load");
	return NULL;
}




void ssa_begin_procedure_body(ssaProcedure *proc) {
	gb_array_init(proc->blocks, gb_heap_allocator());
	gb_array_init(proc->defer_stmts, gb_heap_allocator());
	proc->curr_block = ssa_add_block(proc, proc->type_expr, make_string("entry"));

	if (proc->type->Proc.params != NULL) {
		auto *params = &proc->type->Proc.params->Tuple;
		for (isize i = 0; i < params->variable_count; i++) {
			Entity *e = params->variables[i];
			ssa_add_param(proc, e);
		}
	}
}

void ssa_end_procedure_body(ssaProcedure *proc) {
	if (proc->type->Proc.result_count == 0) {
		ssa_emit_ret(proc, NULL);
	}


// Number blocks and registers
	i32 reg_id = 0;
	gb_for_array(i, proc->blocks) {
		ssaBlock *b = proc->blocks[i];
		b->id = i;
		gb_for_array(j, b->instrs) {
			ssaValue *value = b->instrs[j];
			GB_ASSERT(value->kind == ssaValue_Instr);
			ssaInstr *instr = &value->Instr;
			// NOTE(bill): Ignore non-returning instructions
			switch (instr->kind) {
			case ssaInstr_Comment:
			case ssaInstr_Store:
			case ssaInstr_Br:
			case ssaInstr_Ret:
			case ssaInstr_Unreachable:
			case ssaInstr_StartupRuntime:
				continue;
			case ssaInstr_Call:
				if (instr->Call.type == NULL) {
					continue;
				}
				break;
			}
			value->id = reg_id;
			reg_id++;
		}
	}
}

void ssa_push_target_list(ssaProcedure *proc, ssaBlock *break_, ssaBlock *continue_, ssaBlock *fallthrough_) {
	ssaTargetList *tl = gb_alloc_item(proc->module->allocator, ssaTargetList);
	tl->prev          = proc->target_list;
	tl->break_        = break_;
	tl->continue_     = continue_;
	tl->fallthrough_  = fallthrough_;
	proc->target_list = tl;
}

void ssa_pop_target_list(ssaProcedure *proc) {
	proc->target_list = proc->target_list->prev;
}



ssaValue *ssa_emit_arith(ssaProcedure *proc, Token op, ssaValue *left, ssaValue *right, Type *type) {
	switch (op.kind) {
	case Token_AndNot: {
		// NOTE(bill): x &~ y == x & (~y) == x & (y ~ -1)
		// NOTE(bill): "not" `x` == `x` "xor" `-1`
		ssaValue *neg = ssa_make_value_constant(proc->module->allocator, type, make_exact_value_integer(-1));
		op.kind = Token_Xor;
		right = ssa_emit_arith(proc, op, right, neg, type);
		GB_ASSERT(right->Instr.kind == ssaInstr_BinaryOp);
		right->Instr.BinaryOp.type = type;
		op.kind = Token_And;
	} /* fallthrough */
	case Token_Add:
	case Token_Sub:
	case Token_Mul:
	case Token_Quo:
	case Token_Mod:
	case Token_And:
	case Token_Or:
	case Token_Xor:
		left  = ssa_emit_conv(proc, left, type);
		right = ssa_emit_conv(proc, right, type);
		break;
	}

	return ssa_emit(proc, ssa_make_instr_binary_op(proc, op, left, right, type));
}

ssaValue *ssa_emit_comp(ssaProcedure *proc, Token op, ssaValue *left, ssaValue *right) {
	Type *a = get_base_type(ssa_type(left));
	Type *b = get_base_type(ssa_type(right));

	if (are_types_identical(a, b)) {
		// NOTE(bill): No need for a conversion
	} else if (left->kind == ssaValue_Constant) {
		left = ssa_emit_conv(proc, left, ssa_type(right));
	} else if (right->kind == ssaValue_Constant) {
		right = ssa_emit_conv(proc, right, ssa_type(left));
	}

	Type *result = t_bool;
	if (is_type_vector(a)) {
		result = make_type_vector(proc->module->allocator, t_bool, a->Vector.count);
	}
	return ssa_emit(proc, ssa_make_instr_binary_op(proc, op, left, right, result));
}

ssaValue *ssa_emit_ptr_offset(ssaProcedure *proc, ssaValue *ptr, ssaValue *offset) {
	ssaValue *gep = NULL;
	offset = ssa_emit_conv(proc, offset, t_int);
	gep = ssa_make_instr_get_element_ptr(proc, ptr, offset, NULL, 1, false);
	gep->Instr.GetElementPtr.result_type = ssa_type(ptr);
	return ssa_emit(proc, gep);
}

ssaValue *ssa_emit_zero_gep(ssaProcedure *proc, ssaValue *s) {
	ssaValue *gep = NULL;
	// NOTE(bill): For some weird legacy reason in LLVM, structure elements must be accessed as an i32
	gep = ssa_make_instr_get_element_ptr(proc, s, NULL, NULL, 0, true);
	gep->Instr.GetElementPtr.result_type = ssa_type(s);
	return ssa_emit(proc, gep);
}

ssaValue *ssa_emit_struct_gep(ssaProcedure *proc, ssaValue *s, ssaValue *index, Type *result_type) {
	ssaValue *gep = NULL;
	// NOTE(bill): For some weird legacy reason in LLVM, structure elements must be accessed as an i32
	index = ssa_emit_conv(proc, index, t_i32);
	gep = ssa_make_instr_get_element_ptr(proc, s, v_zero, index, 2, true);
	gep->Instr.GetElementPtr.result_type = result_type;

	return ssa_emit(proc, gep);
}

ssaValue *ssa_emit_struct_gep(ssaProcedure *proc, ssaValue *s, i32 index, Type *result_type) {
	ssaValue *i = ssa_make_value_constant(proc->module->allocator, t_i32, make_exact_value_integer(index));
	return ssa_emit_struct_gep(proc, s, i, result_type);
}


ssaValue *ssa_emit_struct_ev(ssaProcedure *proc, ssaValue *s, i32 index, Type *result_type) {
	// NOTE(bill): For some weird legacy reason in LLVM, structure elements must be accessed as an i32
	return ssa_emit(proc, ssa_make_instr_extract_value(proc, s, index, result_type));
}


ssaValue *ssa_emit_deep_field_gep(ssaProcedure *proc, Type *type, ssaValue *e, Selection sel) {
	GB_ASSERT(gb_array_count(sel.index) > 0);

	gb_for_array(i, sel.index) {
		isize index = sel.index[i];
		if (is_type_pointer(type)) {
			type = type_deref(type);
			e = ssa_emit_load(proc, e);
			e = ssa_emit_ptr_offset(proc, e, v_zero);
		}
		type = get_base_type(type);


		if (is_type_raw_union(type)) {
			type = type->Record.fields[index]->type;
			e = ssa_emit_conv(proc, e, make_type_pointer(proc->module->allocator, type));
		} else if (type->kind == Type_Record) {
			type = type->Record.fields[index]->type;
			e = ssa_emit_struct_gep(proc, e, index, make_type_pointer(proc->module->allocator, type));
		} else if (type->kind == Type_Basic) {
			switch (type->Basic.kind) {
			case Basic_any: {
				if (index == 0) {
					type = t_type_info_ptr;
				} else if (index == 1) {
					type = t_rawptr;
				}
				e = ssa_emit_struct_gep(proc, e, index, make_type_pointer(proc->module->allocator, type));
			} break;

			default:
				GB_PANIC("un-gep-able type");
				break;
			}
		} else {
			GB_PANIC("un-gep-able type");
		}
	}

	return e;
}


ssaValue *ssa_emit_deep_field_ev(ssaProcedure *proc, Type *type, ssaValue *e, Selection sel) {
	GB_ASSERT(gb_array_count(sel.index) > 0);

	gb_for_array(i, sel.index) {
		isize index = sel.index[i];
		if (is_type_pointer(type)) {
			type = type_deref(type);
			e = ssa_emit_load(proc, e);
			e = ssa_emit_ptr_offset(proc, e, v_zero);
		}
		type = get_base_type(type);


		if (is_type_raw_union(type)) {
			type = type->Record.fields[index]->type;
			e = ssa_emit_conv(proc, e, make_type_pointer(proc->module->allocator, type));
		} else if (type->kind == Type_Record) {
			type = type->Record.fields[index]->type;
			e = ssa_emit_struct_ev(proc, e, index, type);
		} else if (type->kind == Type_Basic) {
			switch (type->Basic.kind) {
			case Basic_any: {
				if (index == 0) {
					type = t_type_info_ptr;
				} else if (index == 1) {
					type = t_rawptr;
				}
				e = ssa_emit_struct_ev(proc, e, index, type);
			} break;

			default:
				GB_PANIC("un-ev-able type");
				break;
			}
		} else {
			GB_PANIC("un-ev-able type");
		}
	}

	return e;
}



isize ssa_type_info_index(CheckerInfo *info, Type *type) {
	isize entry_index = -1;
	HashKey key = hash_pointer(type);
	auto *found_entry_index = map_get(&info->type_info_map, key);
	if (found_entry_index) {
		entry_index = *found_entry_index;
	}
	if (entry_index < 0) {
		// NOTE(bill): Do manual search
		// TODO(bill): This is O(n) and can be very slow
		gb_for_array(i, info->type_info_map.entries){
			auto *e = &info->type_info_map.entries[i];
			Type *prev_type = cast(Type *)cast(uintptr)e->key.key;
			if (are_types_identical(prev_type, type)) {
				entry_index = e->value;
				// NOTE(bill): Add it to the search map
				map_set(&info->type_info_map, key, entry_index);
				break;
			}
		}
	}
	GB_ASSERT(entry_index >= 0);
	return entry_index;
}

ssaValue *ssa_type_info(ssaProcedure *proc, Type *type) {
	ssaValue **found = map_get(&proc->module->members, hash_string(make_string(SSA_TYPE_INFO_DATA_NAME)));
	GB_ASSERT(found != NULL);
	ssaValue *type_info_data = *found;

	CheckerInfo *info = proc->module->info;
	isize entry_index = ssa_type_info_index(info, type);
	return ssa_emit_struct_gep(proc, type_info_data, entry_index, t_type_info_ptr);
}







ssaValue *ssa_array_elem(ssaProcedure *proc, ssaValue *array) {
	Type *t = type_deref(ssa_type(array));
	GB_ASSERT(t->kind == Type_Array);
	Type *result_type = make_type_pointer(proc->module->allocator, t->Array.elem);
	return ssa_emit_struct_gep(proc, array, v_zero32, result_type);
}
ssaValue *ssa_array_len(ssaProcedure *proc, ssaValue *array) {
	Type *t = ssa_type(array);
	GB_ASSERT(t->kind == Type_Array);
	return ssa_make_value_constant(proc->module->allocator, t_int, make_exact_value_integer(t->Array.count));
}
ssaValue *ssa_array_cap(ssaProcedure *proc, ssaValue *array) {
	return ssa_array_len(proc, array);
}

ssaValue *ssa_slice_elem(ssaProcedure *proc, ssaValue *slice) {
	Type *t = ssa_type(slice);
	GB_ASSERT(t->kind == Type_Slice);

	Type *result_type = make_type_pointer(proc->module->allocator, t->Slice.elem);
	return ssa_emit_struct_ev(proc, slice, 0, result_type);
}
ssaValue *ssa_slice_len(ssaProcedure *proc, ssaValue *slice) {
	Type *t = ssa_type(slice);
	GB_ASSERT(t->kind == Type_Slice);
	return ssa_emit_struct_ev(proc, slice, 1, t_int);
}
ssaValue *ssa_slice_cap(ssaProcedure *proc, ssaValue *slice) {
	Type *t = ssa_type(slice);
	GB_ASSERT(t->kind == Type_Slice);
	return ssa_emit_struct_ev(proc, slice, 2, t_int);
}

ssaValue *ssa_string_elem(ssaProcedure *proc, ssaValue *string) {
	Type *t = ssa_type(string);
	GB_ASSERT(t->kind == Type_Basic && t->Basic.kind == Basic_string);
	Type *t_u8_ptr = make_type_pointer(proc->module->allocator, t_u8);
	return ssa_emit_struct_ev(proc, string, 0, t_u8_ptr);
}
ssaValue *ssa_string_len(ssaProcedure *proc, ssaValue *string) {
	Type *t = ssa_type(string);
	GB_ASSERT(t->kind == Type_Basic && t->Basic.kind == Basic_string);
	return ssa_emit_struct_ev(proc, string, 1, t_int);
}



ssaValue *ssa_add_local_slice(ssaProcedure *proc, Type *slice_type, ssaValue *base, ssaValue *low, ssaValue *high, ssaValue *max) {
	// TODO(bill): array bounds checking for slice creation
	// TODO(bill): check that low < high <= max
	gbAllocator a = proc->module->allocator;
	Type *base_type = get_base_type(ssa_type(base));

	if (low == NULL) {
		low = v_zero;
	}
	if (high == NULL) {
		switch (base_type->kind) {
		case Type_Array:   high = ssa_array_len(proc, base); break;
		case Type_Slice:   high = ssa_slice_len(proc, base); break;
		case Type_Pointer: high = v_one;                     break;
		}
	}
	if (max == NULL) {
		switch (base_type->kind) {
		case Type_Array:   max = ssa_array_cap(proc, base); break;
		case Type_Slice:   max = ssa_slice_cap(proc, base); break;
		case Type_Pointer: max = high;                      break;
		}
	}
	GB_ASSERT(max != NULL);

	Token op_sub = {Token_Sub};
	ssaValue *len = ssa_emit_arith(proc, op_sub, high, low, t_int);
	ssaValue *cap = ssa_emit_arith(proc, op_sub, max,  low, t_int);

	ssaValue *elem = NULL;
	switch (base_type->kind) {
	case Type_Array:   elem = ssa_array_elem(proc, base); break;
	case Type_Slice:   elem = ssa_slice_elem(proc, base); break;
	case Type_Pointer: elem = ssa_emit_load(proc, base);  break;
	}

	elem = ssa_emit_ptr_offset(proc, elem, low);

	ssaValue *slice = ssa_add_local_generated(proc, slice_type);

	ssaValue *gep = NULL;
	gep = ssa_emit_struct_gep(proc, slice, v_zero32, ssa_type(elem));
	ssa_emit_store(proc, gep, elem);

	gep = ssa_emit_struct_gep(proc, slice, v_one32, t_int);
	ssa_emit_store(proc, gep, len);

	gep = ssa_emit_struct_gep(proc, slice, v_two32, t_int);
	ssa_emit_store(proc, gep, cap);

	return slice;
}

ssaValue *ssa_emit_substring(ssaProcedure *proc, ssaValue *base, ssaValue *low, ssaValue *high) {
	Type *bt = get_base_type(ssa_type(base));
	GB_ASSERT(bt == t_string);
	if (low == NULL) {
		low = v_zero;
	}
	if (high == NULL) {
		high = ssa_string_len(proc, base);
	}

	Token op_sub = {Token_Sub};
	ssaValue *elem, *len;
	len = ssa_emit_arith(proc, op_sub, high, low, t_int);

	elem = ssa_string_elem(proc, base);
	elem = ssa_emit_ptr_offset(proc, elem, low);

	ssaValue *str, *gep;
	str = ssa_add_local_generated(proc, t_string);
	gep = ssa_emit_struct_gep(proc, str, v_zero32, ssa_type(elem));
	ssa_emit_store(proc, gep, elem);

	gep = ssa_emit_struct_gep(proc, str, v_one32, t_int);
	ssa_emit_store(proc, gep, len);

	return str;
}


ssaValue *ssa_add_global_string_array(ssaModule *m, ExactValue value) {
	GB_ASSERT(value.kind == ExactValue_String);
	gbAllocator a = gb_heap_allocator();

	isize max_len = 4+8+1;
	u8 *str = cast(u8 *)gb_alloc_array(a, u8, max_len);
	isize len = gb_snprintf(cast(char *)str, max_len, "__str$%x", m->global_string_index);
	m->global_string_index++;

	String name = make_string(str, len-1);
	Token token = {Token_String};
	token.string = name;
	Type *type = make_type_array(a, t_u8, value.value_string.len);
	Entity *entity = make_entity_constant(a, NULL, token, type, value);
	ssaValue *g = ssa_make_value_global(a, entity, ssa_make_value_constant(a, type, value));
	g->Global.is_private  = true;
	// g->Global.is_constant = true;

	ssa_module_add_value(m, entity, g);
	map_set(&m->members, hash_string(name), g);

	return g;
}

ssaValue *ssa_emit_string(ssaProcedure *proc, ssaValue *elem, ssaValue *len) {
	Type *t_u8_ptr = ssa_type(elem);
	GB_ASSERT(t_u8_ptr->kind == Type_Pointer);

	GB_ASSERT(is_type_u8(t_u8_ptr->Pointer.elem));

	ssaValue *str = ssa_add_local_generated(proc, t_string);
	ssaValue *str_elem = ssa_emit_struct_gep(proc, str, v_zero32, t_u8_ptr);
	ssaValue *str_len = ssa_emit_struct_gep(proc, str, v_one32, t_int);
	ssa_emit_store(proc, str_elem, elem);
	ssa_emit_store(proc, str_len, len);
	return ssa_emit_load(proc, str);
}


ssaValue *ssa_emit_global_string(ssaProcedure *proc, ExactValue value) {
	GB_ASSERT(value.kind == ExactValue_String);
	ssaValue *global_array = ssa_add_global_string_array(proc->module, value);
	ssaValue *elem = ssa_array_elem(proc, global_array);
	ssaValue *len = ssa_array_len(proc, ssa_emit_load(proc, global_array));
	return ssa_emit_string(proc, elem, len);
}


String lookup_polymorphic_field(CheckerInfo *info, Type *dst, Type *src) {
	Type *prev_src = src;
	// Type *prev_dst = dst;
	src = get_base_type(type_deref(src));
	// dst = get_base_type(type_deref(dst));
	b32 src_is_ptr = src != prev_src;
	// b32 dst_is_ptr = dst != prev_dst;

	GB_ASSERT(is_type_struct(src));
	for (isize i = 0; i < src->Record.field_count; i++) {
		Entity *f = src->Record.fields[i];
		if (f->kind == Entity_Variable && f->Variable.anonymous) {
			if (are_types_identical(dst, f->type)) {
				return f->token.string;
			}
			if (src_is_ptr && is_type_pointer(dst)) {
				if (are_types_identical(type_deref(dst), f->type)) {
					return f->token.string;
				}
			}
			String name = lookup_polymorphic_field(info, dst, f->type);
			if (name.len > 0) {
				return name;
			}
		}
	}
	return make_string("");
}


ssaValue *ssa_emit_conv(ssaProcedure *proc, ssaValue *value, Type *t, b32 is_argument) {
	Type *src_type = ssa_type(value);
	if (are_types_identical(t, src_type)) {
		return value;
	}


	Type *src = get_enum_base_type(get_base_type(src_type));
	Type *dst = get_enum_base_type(get_base_type(t));
	if (are_types_identical(src, dst)) {
		return value;
	}

	if (value->kind == ssaValue_Constant) {
		if (is_type_any(dst)) {
			Type *dt = default_type(src);
			ssaValue *default_value = ssa_add_local_generated(proc, dt);
			ssa_emit_store(proc, default_value, value);
			return ssa_emit_conv(proc, ssa_emit_load(proc, default_value), t_any, is_argument);
		} else if (dst->kind == Type_Basic) {
			ExactValue ev = value->Constant.value;
			if (is_type_float(dst)) {
				ev = exact_value_to_float(ev);
			} else if (is_type_string(dst)) {
				// Handled elsewhere
			} else if (is_type_integer(dst)) {
				ev = exact_value_to_integer(ev);
			} else if (is_type_pointer(dst)) {
				// IMPORTANT NOTE(bill): LLVM doesn't support pointer constants expect `null`
				ssaValue *i = ssa_make_value_constant(proc->module->allocator, t_uint, ev);
				return ssa_emit(proc, ssa_make_instr_conv(proc, ssaConv_inttoptr, i, t_uint, dst));
			}
			return ssa_make_value_constant(proc->module->allocator, t, ev);
		}
	}

	// integer -> integer
	if (is_type_integer(src) && is_type_integer(dst)) {
		GB_ASSERT(src->kind == Type_Basic &&
		          dst->kind == Type_Basic);
		i64 sz = type_size_of(proc->module->sizes, proc->module->allocator, src);
		i64 dz = type_size_of(proc->module->sizes, proc->module->allocator, dst);
		if (sz == dz) {
			// NOTE(bill): In LLVM, all integers are signed and rely upon 2's compliment
			return value;
		}

		ssaConvKind kind = ssaConv_trunc;
		if (dz >= sz) {
			kind = ssaConv_zext;
		}
		return ssa_emit(proc, ssa_make_instr_conv(proc, kind, value, src, dst));
	}

	// boolean -> integer
	if (is_type_boolean(src) && is_type_integer(dst)) {
		return ssa_emit(proc, ssa_make_instr_conv(proc, ssaConv_zext, value, src, dst));
	}

	// integer -> boolean
	if (is_type_integer(src) && is_type_boolean(dst)) {
		Token op = {Token_NotEq};
		return ssa_emit_comp(proc, op, value, v_zero);
	}


	// float -> float
	if (is_type_float(src) && is_type_float(dst)) {
		i64 sz = basic_type_sizes[src->Basic.kind];
		i64 dz = basic_type_sizes[dst->Basic.kind];
		ssaConvKind kind = ssaConv_fptrunc;
		if (dz >= sz) {
			kind = ssaConv_fpext;
		}
		return ssa_emit(proc, ssa_make_instr_conv(proc, kind, value, src, dst));
	}

	// float <-> integer
	if (is_type_float(src) && is_type_integer(dst)) {
		ssaConvKind kind = ssaConv_fptosi;
		if (is_type_unsigned(dst)) {
			kind = ssaConv_fptoui;
		}
		return ssa_emit(proc, ssa_make_instr_conv(proc, kind, value, src, dst));
	}
	if (is_type_integer(src) && is_type_float(dst)) {
		ssaConvKind kind = ssaConv_sitofp;
		if (is_type_unsigned(src)) {
			kind = ssaConv_uitofp;
		}
		return ssa_emit(proc, ssa_make_instr_conv(proc, kind, value, src, dst));
	}

	// Pointer <-> int
	if (is_type_pointer(src) && is_type_int_or_uint(dst)) {
		return ssa_emit(proc, ssa_make_instr_conv(proc, ssaConv_ptrtoint, value, src, dst));
	}
	if (is_type_int_or_uint(src) && is_type_pointer(dst)) {
		return ssa_emit(proc, ssa_make_instr_conv(proc, ssaConv_inttoptr, value, src, dst));
	}

	if (is_type_union(dst)) {
		for (isize i = 0; i < dst->Record.field_count; i++) {
			Entity *f = dst->Record.fields[i];
			if (are_types_identical(f->type, src_type)) {
				ssa_emit_comment(proc, make_string("union - child to parent"));
				gbAllocator allocator = proc->module->allocator;
				ssaValue *parent = ssa_add_local_generated(proc, t);
				ssaValue *tag = ssa_make_value_constant(allocator, t_int, make_exact_value_integer(i));
				ssa_emit_store(proc, ssa_emit_struct_gep(proc, parent, v_one32, t_int), tag);

				ssaValue *data = ssa_emit_conv(proc, parent, t_rawptr);

				Type *tag_type = src_type;
				Type *tag_type_ptr = make_type_pointer(allocator, tag_type);
				ssaValue *underlying = ssa_emit(proc, ssa_make_instr_conv(proc, ssaConv_bitcast, data, t_rawptr, tag_type_ptr));
				ssa_emit_store(proc, underlying, value);

				return ssa_emit_load(proc, parent);
			}
		}
	}

	// NOTE(bill): This has to be done beofre `Pointer <-> Pointer` as it's
	// subtype polymorphism casting
	if (is_argument) {
		Type *sb = get_base_type(type_deref(src));
		b32 src_is_ptr = src != sb;
		if (is_type_struct(sb)) {
			String field_name = lookup_polymorphic_field(proc->module->info, t, src);
			// gb_printf("field_name: %.*s\n", LIT(field_name));
			if (field_name.len > 0) {
				// NOTE(bill): It can be casted
				Selection sel = lookup_field(sb, field_name, false);
				if (sel.entity != NULL) {
					ssa_emit_comment(proc, make_string("cast - polymorphism"));
					if (src_is_ptr) {
						value = ssa_emit_load(proc, value);
					}
					return ssa_emit_deep_field_ev(proc, sb, value, sel);
				}
			}
		}
	}



	// Pointer <-> Pointer
	if (is_type_pointer(src) && is_type_pointer(dst)) {
		return ssa_emit(proc, ssa_make_instr_conv(proc, ssaConv_bitcast, value, src, dst));
	}



	// proc <-> proc
	if (is_type_proc(src) && is_type_proc(dst)) {
		return ssa_emit(proc, ssa_make_instr_conv(proc, ssaConv_bitcast, value, src, dst));
	}

	// pointer -> proc
	if (is_type_pointer(src) && is_type_proc(dst)) {
		return ssa_emit(proc, ssa_make_instr_conv(proc, ssaConv_bitcast, value, src, dst));
	}
	// proc -> pointer
	if (is_type_proc(src) && is_type_pointer(dst)) {
		return ssa_emit(proc, ssa_make_instr_conv(proc, ssaConv_bitcast, value, src, dst));
	}



	// []byte/[]u8 <-> string
	if (is_type_u8_slice(src) && is_type_string(dst)) {
		ssaValue *elem = ssa_slice_elem(proc, value);
		ssaValue *len  = ssa_slice_len(proc, value);
		return ssa_emit_string(proc, elem, len);
	}
	if (is_type_string(src) && is_type_u8_slice(dst)) {
		ssaValue *elem = ssa_string_elem(proc, value);
		ssaValue *elem_ptr = ssa_add_local_generated(proc, ssa_type(elem));
		ssa_emit_store(proc, elem_ptr, elem);

		ssaValue *len  = ssa_string_len(proc, value);
		ssaValue *slice = ssa_add_local_slice(proc, dst, elem_ptr, v_zero, len, len);
		return ssa_emit_load(proc, slice);
	}

	if (is_type_vector(dst)) {
		Type *dst_elem = dst->Vector.elem;
		value = ssa_emit_conv(proc, value, dst_elem);
		ssaValue *v = ssa_add_local_generated(proc, t);
		v = ssa_emit_load(proc, v);
		v = ssa_emit(proc, ssa_make_instr_insert_element(proc, v, value, v_zero32));
		// NOTE(bill): Broadcast lowest value to all values
		isize index_count = dst->Vector.count;
		i32 *indices = gb_alloc_array(proc->module->allocator, i32, index_count);
		for (isize i = 0; i < index_count; i++) {
			indices[i] = 0;
		}

		v = ssa_emit(proc, ssa_make_instr_shuffle_vector(proc, v, indices, index_count));
		return v;
	}

	if (is_type_any(dst)) {
		ssaValue *result = ssa_add_local_generated(proc, t_any);

		ssaValue *data = NULL;
		if (value->kind == ssaValue_Instr &&
		    value->Instr.kind == ssaInstr_Load) {
			// NOTE(bill): Addressable value
			data = value->Instr.Load.address;
		} else {
			// NOTE(bill): Non-addressable value
			data = ssa_add_local_generated(proc, src_type);
			ssa_emit_store(proc, data, value);
		}
		data = ssa_emit_conv(proc, data, t_rawptr);


		ssaValue *ti = ssa_type_info(proc, src_type);

		ssaValue *gep0 = ssa_emit_struct_gep(proc, result, v_zero32, make_type_pointer(proc->module->allocator, t_type_info_ptr));
		ssaValue *gep1 = ssa_emit_struct_gep(proc, result, v_one32,  make_type_pointer(proc->module->allocator, t_rawptr));
		ssa_emit_store(proc, gep0, ti);
		ssa_emit_store(proc, gep1, data);

		return ssa_emit_load(proc, result);
	}


	gb_printf_err("Not Identical %s != %s\n", type_to_string(src_type), type_to_string(t));
	gb_printf_err("Not Identical %s != %s\n", type_to_string(src), type_to_string(dst));


	GB_PANIC("Invalid type conversion: `%s` to `%s`", type_to_string(src_type), type_to_string(t));

	return NULL;
}


ssaValue *ssa_emit_transmute(ssaProcedure *proc, ssaValue *value, Type *t) {
	Type *src_type = ssa_type(value);
	if (are_types_identical(t, src_type)) {
		return value;
	}

	Type *src = get_base_type(src_type);
	Type *dst = get_base_type(t);
	if (are_types_identical(t, src_type))
		return value;

	i64 sz = type_size_of(proc->module->sizes, proc->module->allocator, src);
	i64 dz = type_size_of(proc->module->sizes, proc->module->allocator, dst);

	if (sz == dz) {
		return ssa_emit(proc, ssa_make_instr_conv(proc, ssaConv_bitcast, value, src, dst));
	}


	GB_PANIC("Invalid transmute conversion: `%s` to `%s`", type_to_string(src_type), type_to_string(t));

	return NULL;
}

ssaValue *ssa_emit_down_cast(ssaProcedure *proc, ssaValue *value, Type *t) {
	GB_ASSERT(is_type_pointer(ssa_type(value)));
	gbAllocator allocator = proc->module->allocator;

	String field_name = check_down_cast_name(t, type_deref(ssa_type(value)));
	GB_ASSERT(field_name.len > 0);
	Selection sel = lookup_field(t, field_name, false);
	Type *t_u8_ptr = make_type_pointer(allocator, t_u8);
	ssaValue *bytes = ssa_emit_conv(proc, value, t_u8_ptr);

	// IMPORTANT TODO(bill): THIS ONLY DOES ONE LAYER DEEP!!! FUCKING HELL THIS IS NOT WHAT I SIGNED UP FOR!

	i64 offset_ = type_offset_of_from_selection(proc->module->sizes, allocator, type_deref(t), sel);
	ssaValue *offset = ssa_make_value_constant(allocator, t_int, make_exact_value_integer(-offset_));
	ssaValue *head = ssa_emit_ptr_offset(proc, bytes, offset);
	return ssa_emit_conv(proc, head, t);
}

void ssa_build_cond(ssaProcedure *proc, AstNode *cond, ssaBlock *true_block, ssaBlock *false_block);

ssaValue *ssa_emit_logical_binary_expr(ssaProcedure *proc, AstNode *expr) {
	ast_node(be, BinaryExpr, expr);
	ssaBlock *true_   = ssa_add_block(proc, NULL, make_string("logical.cmp.true"));
	ssaBlock *false_  = ssa_add_block(proc, NULL, make_string("logical.cmp.false"));
	ssaBlock *done  = ssa__make_block(proc, NULL, make_string("logical.cmp.done"));

	ssaValue *result = ssa_add_local_generated(proc, t_bool);
	ssa_build_cond(proc, expr, true_, false_);

	proc->curr_block = true_;
	ssa_emit_store(proc, result, v_true);
	ssa_emit_jump(proc, done);

	proc->curr_block = false_;
	ssa_emit_store(proc, result, v_false);
	ssa_emit_jump(proc, done);

	gb_array_append(proc->blocks, done);
	proc->curr_block = done;

	return ssa_emit_load(proc, result);
}


void ssa_array_bounds_check(ssaProcedure *proc, Token token, ssaValue *index, ssaValue *len) {
	if ((proc->module->stmt_state_flags & StmtStateFlag_no_abc) != 0) {
		return;
	}

	index = ssa_emit_conv(proc, index, t_int);
	len   = ssa_emit_conv(proc, len, t_int);

	ssa_emit_comment(proc, make_string("ArrayBoundsCheck"));

	Token lt = {Token_Lt};
	Token ge = {Token_GtEq};
	Token cmp_or = {Token_Or}; // NOTE(bill): Just `or` it as both sides have been evaluated anyway
	ssaValue *lower = ssa_emit_comp(proc, lt, index, v_zero);
	ssaValue *upper = ssa_emit_comp(proc, ge, index, len);
	ssaValue *cond  = ssa_emit_arith(proc, cmp_or, lower, upper, t_bool);

	ssaBlock *then = ssa_add_block(proc, NULL, make_string("abc.then"));
	ssaBlock *done = ssa__make_block(proc, NULL, make_string("abc.done")); // NOTE(bill): Append later

	ssa_emit_if(proc, cond, then, done);
	proc->curr_block = then;

	gbAllocator a = proc->module->allocator;

	ssaValue *file   = ssa_emit_global_string(proc, make_exact_value_string(token.pos.file));
	ssaValue *line   = ssa_make_value_constant(a, t_int, make_exact_value_integer(token.pos.line));
	ssaValue *column = ssa_make_value_constant(a, t_int, make_exact_value_integer(token.pos.column));

	ssaValue **args = gb_alloc_array(a, ssaValue *, 5);
	args[0] = file;
	args[1] = line;
	args[2] = column;
	args[3] = index;
	args[4] = len;

	ssa_emit_global_call(proc, "__abc_error", args, 5);

	ssa_emit_jump(proc, done);
	gb_array_append(proc->blocks, done);
	proc->curr_block = done;
}


ssaValue *ssa_build_single_expr(ssaProcedure *proc, AstNode *expr, TypeAndValue *tv) {
	switch (expr->kind) {
	case_ast_node(bl, BasicLit, expr);
		GB_PANIC("Non-constant basic literal");
	case_end;

	case_ast_node(i, Ident, expr);
		Entity *e = *map_get(&proc->module->info->uses, hash_pointer(expr));
		if (e->kind == Entity_Builtin) {
			Token token = ast_node_token(expr);
			GB_PANIC("TODO(bill): ssa_build_single_expr Entity_Builtin `%.*s`\n"
			         "\t at %.*s(%td:%td)", LIT(builtin_procs[e->Builtin.id].name),
			         LIT(token.pos.file), token.pos.line, token.pos.column);
			return NULL;
		}

		auto *found = map_get(&proc->module->values, hash_pointer(e));
		if (found) {
			ssaValue *v = *found;
			if (v->kind == ssaValue_Proc)
				return v;
			return ssa_emit_load(proc, v);
		}
		return NULL;
	case_end;

	case_ast_node(pe, ParenExpr, expr);
		return ssa_build_single_expr(proc, unparen_expr(expr), tv);
	case_end;

	case_ast_node(de, DerefExpr, expr);
		return ssa_lvalue_load(proc, ssa_build_addr(proc, expr));
	case_end;

	case_ast_node(se, SelectorExpr, expr);
		return ssa_lvalue_load(proc, ssa_build_addr(proc, expr));
	case_end;

	case_ast_node(ue, UnaryExpr, expr);
		switch (ue->op.kind) {
		case Token_Pointer: {
			return ssa_emit_zero_gep(proc, ssa_build_addr(proc, ue->expr).addr);
		}
		case Token_Add:
			return ssa_build_expr(proc, ue->expr);
		case Token_Sub: {
			// NOTE(bill): -`x` == 0 - `x`
			ssaValue *left = v_zero;
			ssaValue *right = ssa_build_expr(proc, ue->expr);
			return ssa_emit_arith(proc, ue->op, left, right, tv->type);
		} break;
		case Token_Not: // Boolean not
		case Token_Xor: { // Bitwise not
			// NOTE(bill): "not" `x` == `x` "xor" `-1`
			ExactValue neg_one = make_exact_value_integer(-1);
			ssaValue *left = ssa_build_expr(proc, ue->expr);
			ssaValue *right = ssa_make_value_constant(proc->module->allocator, tv->type, neg_one);
			return ssa_emit_arith(proc, ue->op, left, right, tv->type);
		} break;
		}
	case_end;

	case_ast_node(be, BinaryExpr, expr);
		switch (be->op.kind) {
		case Token_Add:
		case Token_Sub:
		case Token_Mul:
		case Token_Quo:
		case Token_Mod:
		case Token_And:
		case Token_Or:
		case Token_Xor:
		case Token_AndNot:
		case Token_Shl:
		case Token_Shr:
			return ssa_emit_arith(proc, be->op,
			                      ssa_build_expr(proc, be->left),
			                      ssa_build_expr(proc, be->right),
			                      tv->type);


		case Token_CmpEq:
		case Token_NotEq:
		case Token_Lt:
		case Token_LtEq:
		case Token_Gt:
		case Token_GtEq: {
			ssaValue *left  = ssa_build_expr(proc, be->left);
			ssaValue *right = ssa_build_expr(proc, be->right);

			ssaValue *cmp = ssa_emit_comp(proc, be->op, left, right);
			return ssa_emit_conv(proc, cmp, default_type(tv->type));
		} break;

		case Token_CmpAnd:
		case Token_CmpOr:
			return ssa_emit_logical_binary_expr(proc, expr);

		case Token_as:
			ssa_emit_comment(proc, make_string("cast - as"));
			return ssa_emit_conv(proc, ssa_build_expr(proc, be->left), tv->type);

		case Token_transmute:
			ssa_emit_comment(proc, make_string("cast - transmute"));
			return ssa_emit_transmute(proc, ssa_build_expr(proc, be->left), tv->type);

		case Token_down_cast:
			ssa_emit_comment(proc, make_string("cast - down_cast"));
			return ssa_emit_down_cast(proc, ssa_build_expr(proc, be->left), tv->type);

		default:
			GB_PANIC("Invalid binary expression");
			break;
		}
	case_end;

	case_ast_node(pl, ProcLit, expr);
		if (proc->children == NULL) {
			gb_array_init(proc->children, gb_heap_allocator());
		}
		// NOTE(bill): Generate a new name
		// parent$count
		isize name_len = proc->name.len + 1 + 8 + 1;
		u8 *name_text = gb_alloc_array(proc->module->allocator, u8, name_len);
		name_len = gb_snprintf(cast(char *)name_text, name_len, "%.*s$%d", LIT(proc->name), cast(i32)gb_array_count(proc->children));
		String name = make_string(name_text, name_len-1);

		Type *type = type_of_expr(proc->module->info, expr);
		ssaValue *value = ssa_make_value_procedure(proc->module->allocator,
		                                           proc->module, type, pl->type, pl->body, name);

		value->Proc.tags = pl->tags;

		gb_array_append(proc->children, &value->Proc);
		ssa_build_proc(value, proc);

		return value;
	case_end;


	case_ast_node(cl, CompoundLit, expr);
		ssa_emit_comment(proc, make_string("CompoundLit"));
		Type *type = type_of_expr(proc->module->info, expr);
		Type *base_type = get_base_type(type);
		ssaValue *v = ssa_add_local_generated(proc, type);

		Type *et = NULL;
		switch (base_type->kind) {
		case Type_Vector: et = base_type->Vector.elem; break;
		case Type_Array:  et = base_type->Array.elem;  break;
		case Type_Slice:  et = base_type->Slice.elem;  break;
		}

		switch (base_type->kind) {
		default: GB_PANIC("Unknown CompoundLit type: %s", type_to_string(type)); break;

		case Type_Vector: {
			isize index = 0;
			ssaValue *result = ssa_emit_load(proc, v);
			for (; index < gb_array_count(cl->elems); index++) {
				AstNode *elem = cl->elems[index];
				ssaValue *field_elem = ssa_build_expr(proc, elem);
				Type *t = ssa_type(field_elem);
				GB_ASSERT(t->kind != Type_Tuple);
				ssaValue *ev = ssa_emit_conv(proc, field_elem, et);
				ssaValue *i = ssa_make_value_constant(proc->module->allocator, t_int, make_exact_value_integer(index));
				result = ssa_emit(proc, ssa_make_instr_insert_element(proc, result, ev, i));
			}
			if (index == 1 && base_type->Vector.count > 1) {
				isize index_count = base_type->Vector.count;
				i32 *indices = gb_alloc_array(proc->module->allocator, i32, index_count);
				for (isize i = 0; i < index_count; i++) {
					indices[i] = 0;
				}
				ssaValue *sv = ssa_emit(proc, ssa_make_instr_shuffle_vector(proc, result, indices, index_count));
				ssa_emit_store(proc, v, sv);
				return ssa_emit_load(proc, v);
			}

			return result;
		} break;

		case Type_Record: {
			GB_ASSERT(is_type_struct(base_type));
			auto *st = &base_type->Record;
			if (cl->elems != NULL && gb_array_count(cl->elems) > 0) {
				gb_for_array(field_index, cl->elems) {
					isize index = field_index;
					AstNode *elem = cl->elems[index];
					ssaValue *field_expr = NULL;
					Entity *field = NULL;

					if (elem->kind == AstNode_FieldValue) {
						ast_node(kv, FieldValue, elem);
						Selection sel = lookup_field(base_type, kv->field->Ident.string, false);
						index = sel.index[0];
						field_expr = ssa_build_expr(proc, kv->value);
					} else {
						Selection sel = lookup_field(base_type, st->fields_in_src_order[field_index]->token.string, false);
						index = sel.index[0];
						field_expr = ssa_build_expr(proc, elem);
					}

					GB_ASSERT(ssa_type(field_expr)->kind != Type_Tuple);

					field = st->fields[index];

					Type *ft = field->type;
					ssaValue *fv = ssa_emit_conv(proc, field_expr, ft);
					ssaValue *gep = ssa_emit_struct_gep(proc, v, index, ft);
					ssa_emit_store(proc, gep, fv);
				}
			}
		} break;
		case Type_Array: {
			gb_for_array(i, cl->elems) {
				AstNode *elem = cl->elems[i];
				ssaValue *field_expr = ssa_build_expr(proc, elem);
				Type *t = ssa_type(field_expr);
				GB_ASSERT(t->kind != Type_Tuple);
				ssaValue *ev = ssa_emit_conv(proc, field_expr, et);
				ssaValue *gep = ssa_emit_struct_gep(proc, v, i, et);
				ssa_emit_store(proc, gep, ev);
			}
		} break;
		case Type_Slice: {
			i64 count = gb_array_count(cl->elems);
			Type *elem_type = base_type->Slice.elem;
			Type *elem_ptr_type = make_type_pointer(proc->module->allocator, elem_type);
			ssaValue *array = ssa_add_local_generated(proc, make_type_array(proc->module->allocator, elem_type, count));

			gb_for_array(i, cl->elems) {
				AstNode *elem = cl->elems[i];
				ssaValue *field_expr = ssa_build_expr(proc, elem);
				Type *t = ssa_type(field_expr);
				GB_ASSERT(t->kind != Type_Tuple);
				ssaValue *ev = ssa_emit_conv(proc, field_expr, elem_type);
				ssaValue *gep = ssa_emit_struct_gep(proc, array, i, elem_ptr_type);
				ssa_emit_store(proc, gep, ev);
			}

			ssaValue *elem = ssa_array_elem(proc, array);
			ssaValue *len = ssa_array_len(proc, ssa_emit_load(proc, array));
			ssaValue *gep0 = ssa_emit_struct_gep(proc, v, v_zero32, ssa_type(elem));
			ssaValue *gep1 = ssa_emit_struct_gep(proc, v, v_one32, t_int);
			ssaValue *gep2 = ssa_emit_struct_gep(proc, v, v_two32, t_int);

			ssa_emit_store(proc, gep0, elem);
			ssa_emit_store(proc, gep1, len);
			ssa_emit_store(proc, gep2, len);
		} break;
		}

		return ssa_emit_load(proc, v);
	case_end;

	case_ast_node(ce, CallExpr, expr);
		AstNode *p = unparen_expr(ce->proc);
		if (p->kind == AstNode_Ident) {
			Entity **found = map_get(&proc->module->info->uses, hash_pointer(p));
			if (found && (*found)->kind == Entity_Builtin) {
				Entity *e = *found;
				switch (e->Builtin.id) {
				case BuiltinProc_type_info: {
					ssaValue *x = ssa_build_expr(proc, ce->args[0]);
					Type *t = default_type(type_of_expr(proc->module->info, ce->args[0]));
					return ssa_type_info(proc, t);
				} break;

				case BuiltinProc_new: {
					ssa_emit_comment(proc, make_string("new"));
					// new :: proc(Type) -> ^Type
					gbAllocator allocator = proc->module->allocator;

					Type *type = type_of_expr(proc->module->info, ce->args[0]);
					Type *ptr_type = make_type_pointer(allocator, type);

					i64 s = type_size_of(proc->module->sizes, allocator, type);
					i64 a = type_align_of(proc->module->sizes, allocator, type);

					ssaValue **args = gb_alloc_array(allocator, ssaValue *, 2);
					args[0] = ssa_make_value_constant(allocator, t_int, make_exact_value_integer(s));
					args[1] = ssa_make_value_constant(allocator, t_int, make_exact_value_integer(a));
					ssaValue *call = ssa_emit_global_call(proc, "alloc_align", args, 2);
					ssaValue *v = ssa_emit_conv(proc, call, ptr_type);
					return v;
				} break;

				case BuiltinProc_new_slice: {
					ssa_emit_comment(proc, make_string("new_slice"));
					// new_slice :: proc(Type, len: int[, cap: int]) -> ^Type
					gbAllocator allocator = proc->module->allocator;

					Type *type = type_of_expr(proc->module->info, ce->args[0]);
					Type *ptr_type = make_type_pointer(allocator, type);
					Type *slice_type = make_type_slice(allocator, type);

					i64 s = type_size_of(proc->module->sizes, allocator, type);
					i64 a = type_align_of(proc->module->sizes, allocator, type);

					ssaValue *elem_size  = ssa_make_value_constant(allocator, t_int, make_exact_value_integer(s));
					ssaValue *elem_align = ssa_make_value_constant(allocator, t_int, make_exact_value_integer(a));

					ssaValue *len = ssa_build_expr(proc, ce->args[1]);
					ssaValue *cap = len;
					if (gb_array_count(ce->args) == 3) {
						cap = ssa_build_expr(proc, ce->args[2]);
					}

					Token mul = {Token_Mul};
					ssaValue *slice_size = ssa_emit_arith(proc, mul, elem_size, cap, t_int);

					ssaValue **args = gb_alloc_array(allocator, ssaValue *, 2);
					args[0] = slice_size;
					args[1] = elem_align;
					ssaValue *call = ssa_emit_global_call(proc, "alloc_align", args, 2);

					ssaValue *ptr = ssa_emit_conv(proc, call, ptr_type, true);
					ssaValue *slice = ssa_add_local_generated(proc, slice_type);
					ssa_emit_store(proc, ssa_emit_struct_gep(proc, slice, v_zero32, ptr_type), ptr);
					ssa_emit_store(proc, ssa_emit_struct_gep(proc, slice, v_one32,  t_int),    len);
					ssa_emit_store(proc, ssa_emit_struct_gep(proc, slice, v_two32,  t_int),    cap);
					return ssa_emit_load(proc, slice);
				} break;

				case BuiltinProc_delete: {
					ssa_emit_comment(proc, make_string("delete"));
					// delete :: proc(ptr: ^Type)
					// delete :: proc(slice: []Type)
					gbAllocator allocator = proc->module->allocator;

					ssaValue *value = ssa_build_expr(proc, ce->args[0]);

					if (is_type_slice(ssa_type(value))) {
						Type *etp = get_base_type(ssa_type(value));
						etp = make_type_pointer(allocator, etp->Slice.elem);
						value = ssa_emit(proc, ssa_make_instr_extract_value(proc, value, 0, etp));
					}

					ssaValue **args = gb_alloc_array(allocator, ssaValue *, 1);
					args[0] = ssa_emit_conv(proc, value, t_rawptr, true);
					return ssa_emit_global_call(proc, "dealloc", args, 1);
				} break;


				case BuiltinProc_assert: {
					ssa_emit_comment(proc, make_string("assert"));
					ssaValue *cond = ssa_build_expr(proc, ce->args[0]);
					GB_ASSERT(is_type_boolean(ssa_type(cond)));

					Token eq = {Token_CmpEq};
					cond = ssa_emit_comp(proc, eq, cond, v_false);
					ssaBlock *err  = ssa_add_block(proc, NULL, make_string("builtin.assert.err"));
					ssaBlock *done = ssa__make_block(proc, NULL, make_string("builtin.assert.done"));

					ssa_emit_if(proc, cond, err, done);
					proc->curr_block = err;

					Token token = ast_node_token(ce->args[0]);
					TokenPos pos = token.pos;
					gbString expr = expr_to_string(ce->args[0]);
					defer (gb_string_free(expr));

					isize err_len = pos.file.len + 1 + 10 + 1 + 10 + 1;
					err_len += 20;
					err_len += gb_string_length(expr);
					err_len += 2;
					// HACK(bill): memory leaks
					u8 *err_str = gb_alloc_array(gb_heap_allocator(), u8, err_len);
					err_len = gb_snprintf(cast(char *)err_str, err_len,
					                      "%.*s(%td:%td) Runtime assertion: %s\n",
					                      LIT(pos.file), pos.line, pos.column, expr);
					err_len--;

					ssaValue *array = ssa_add_global_string_array(proc->module, make_exact_value_string(make_string(err_str, err_len)));
					ssaValue *elem = ssa_array_elem(proc, array);
					ssaValue *len = ssa_make_value_constant(proc->module->allocator, t_int, make_exact_value_integer(err_len));
					ssaValue *string = ssa_emit_string(proc, elem, len);

					ssaValue **args = gb_alloc_array(proc->module->allocator, ssaValue *, 1);
					args[0] = string;
					ssa_emit_global_call(proc, "__assert", args, 1);

					ssa_emit_jump(proc, done);
					gb_array_append(proc->blocks, done);
					proc->curr_block = done;

					return NULL;
				} break;

				case BuiltinProc_len: {
					ssa_emit_comment(proc, make_string("len"));
					// len :: proc(v: Type) -> int
					// NOTE(bill): len of an array is a constant expression
					ssaValue *v = ssa_build_expr(proc, ce->args[0]);
					Type *t = get_base_type(ssa_type(v));
					if (t == t_string)
						return ssa_string_len(proc, v);
					else if (t->kind == Type_Slice)
						return ssa_slice_len(proc, v);
				} break;
				case BuiltinProc_cap: {
					ssa_emit_comment(proc, make_string("cap"));
					// cap :: proc(v: Type) -> int
					// NOTE(bill): cap of an array is a constant expression
					ssaValue *v = ssa_build_expr(proc, ce->args[0]);
					Type *t = get_base_type(ssa_type(v));
					return ssa_slice_cap(proc, v);
				} break;
				case BuiltinProc_copy: {
					ssa_emit_comment(proc, make_string("copy"));
					// copy :: proc(dst, src: []Type) -> int
					AstNode *dst_node = ce->args[0];
					AstNode *src_node = ce->args[1];
					ssaValue *dst_slice = ssa_build_expr(proc, dst_node);
					ssaValue *src_slice = ssa_build_expr(proc, src_node);
					Type *slice_type = get_base_type(ssa_type(dst_slice));
					GB_ASSERT(slice_type->kind == Type_Slice);
					Type *elem_type = slice_type->Slice.elem;
					i64 size_of_elem = type_size_of(proc->module->sizes, proc->module->allocator, elem_type);


					ssaValue *dst = ssa_emit_conv(proc, ssa_slice_elem(proc, dst_slice), t_rawptr, true);
					ssaValue *src = ssa_emit_conv(proc, ssa_slice_elem(proc, src_slice), t_rawptr, true);

					ssaValue *len_dst = ssa_slice_len(proc, dst_slice);
					ssaValue *len_src = ssa_slice_len(proc, src_slice);

					Token lt = {Token_Lt};
					ssaValue *cond = ssa_emit_comp(proc, lt, len_dst, len_src);
					ssaValue *len = ssa_emit_select(proc, cond, len_dst, len_src);
					Token mul = {Token_Mul};
					ssaValue *elem_size = ssa_make_value_constant(proc->module->allocator, t_int,
					                                              make_exact_value_integer(size_of_elem));
					ssaValue *byte_count = ssa_emit_arith(proc, mul, len, elem_size, t_int);

					ssaValue **args = gb_alloc_array(proc->module->allocator, ssaValue *, 3);
					args[0] = dst;
					args[1] = src;
					args[2] = byte_count;

					ssa_emit_global_call(proc, "memory_copy", args, 3);

					return len;
				} break;
				case BuiltinProc_append: {
					ssa_emit_comment(proc, make_string("append"));
					// append :: proc(s: ^[]Type, item: Type) -> bool
					AstNode *sptr_node = ce->args[0];
					AstNode *item_node = ce->args[1];
					ssaValue *slice_ptr = ssa_build_expr(proc, sptr_node);
					ssaValue *slice = ssa_emit_load(proc, slice_ptr);

					ssaValue *elem = ssa_slice_elem(proc, slice);
					ssaValue *len  = ssa_slice_len(proc,  slice);
					ssaValue *cap  = ssa_slice_cap(proc,  slice);

					Type *elem_type = type_deref(ssa_type(elem));

					ssaValue *item_value = ssa_build_expr(proc, item_node);
					item_value = ssa_emit_conv(proc, item_value, elem_type, true);

					ssaValue *item = ssa_add_local_generated(proc, elem_type);
					ssa_emit_store(proc, item, item_value);


					// NOTE(bill): Check if can append is possible
					Token lt = {Token_Lt};
					ssaValue *cond = ssa_emit_comp(proc, lt, len, cap);
					ssaBlock *able = ssa_add_block(proc, NULL, make_string("builtin.append.able"));
					ssaBlock *done = ssa__make_block(proc, NULL, make_string("builtin.append.done"));

					ssa_emit_if(proc, cond, able, done);
					proc->curr_block = able;

					// Add new slice item
					i64 item_size = type_size_of(proc->module->sizes, proc->module->allocator, elem_type);
					ssaValue *byte_count = ssa_make_value_constant(proc->module->allocator, t_int,
					                                               make_exact_value_integer(item_size));

					ssaValue *offset = ssa_emit_ptr_offset(proc, elem, len);
					offset = ssa_emit_conv(proc, offset, t_rawptr, true);

					item = ssa_emit_ptr_offset(proc, item, v_zero);
					item = ssa_emit_conv(proc, item, t_rawptr, true);

					ssaValue **args = gb_alloc_array(proc->module->allocator, ssaValue *, 3);
					args[0] = offset;
					args[1] = item;
					args[2] = byte_count;

					ssa_emit_global_call(proc, "memory_copy", args, 3);

					// Increment slice length
					Token add = {Token_Add};
					ssaValue *new_len = ssa_emit_arith(proc, add, len, v_one, t_int);
					ssaValue *gep = ssa_emit_struct_gep(proc, slice_ptr, v_one32, t_int);
					ssa_emit_store(proc, gep, new_len);

					ssa_emit_jump(proc, done);
					gb_array_append(proc->blocks, done);
					proc->curr_block = done;

					return ssa_emit_conv(proc, cond, t_bool, true);
				} break;

				case BuiltinProc_swizzle: {
					ssa_emit_comment(proc, make_string("swizzle"));
					ssaValue *vector = ssa_build_expr(proc, ce->args[0]);
					isize index_count = gb_array_count(ce->args)-1;
					if (index_count == 0) {
						return vector;
					}

					i32 *indices = gb_alloc_array(proc->module->allocator, i32, index_count);
					isize index = 0;
					gb_for_array(i, ce->args) {
						if (i == 0) continue;
						TypeAndValue *tv = type_and_value_of_expression(proc->module->info, ce->args[i]);
						GB_ASSERT(is_type_integer(tv->type));
						GB_ASSERT(tv->value.kind == ExactValue_Integer);
						indices[index++] = cast(i32)tv->value.value_integer;
					}

					return ssa_emit(proc, ssa_make_instr_shuffle_vector(proc, vector, indices, index_count));

				} break;

				case BuiltinProc_ptr_offset: {
					ssa_emit_comment(proc, make_string("ptr_offset"));
					ssaValue *ptr = ssa_build_expr(proc, ce->args[0]);
					ssaValue *offset = ssa_build_expr(proc, ce->args[1]);
					return ssa_emit_ptr_offset(proc, ptr, offset);
				} break;

				case BuiltinProc_ptr_sub: {
					ssa_emit_comment(proc, make_string("ptr_sub"));
					ssaValue *ptr_a = ssa_build_expr(proc, ce->args[0]);
					ssaValue *ptr_b = ssa_build_expr(proc, ce->args[1]);
					Type *ptr_type = get_base_type(ssa_type(ptr_a));
					GB_ASSERT(ptr_type->kind == Type_Pointer);
					isize elem_size = type_size_of(proc->module->sizes, proc->module->allocator, ptr_type->Pointer.elem);
					Token sub = {Token_Sub};
					ssaValue *v = ssa_emit_arith(proc, sub, ptr_a, ptr_b, t_int);
					if (elem_size > 1) {
						Token quo = {Token_Quo};
						ssaValue *ez = ssa_make_value_constant(proc->module->allocator, t_int,
						                                       make_exact_value_integer(elem_size));
						v = ssa_emit_arith(proc, quo, v, ez, t_int);
					}

					return v;
				} break;

				case BuiltinProc_slice_ptr: {
					ssa_emit_comment(proc, make_string("slice_ptr"));
					ssaValue *ptr = ssa_build_expr(proc, ce->args[0]);
					ssaValue *len = ssa_build_expr(proc, ce->args[1]);
					ssaValue *cap = len;

					len = ssa_emit_conv(proc, len, t_int, true);

					if (gb_array_count(ce->args) == 3) {
						cap = ssa_build_expr(proc, ce->args[2]);
						cap = ssa_emit_conv(proc, cap, t_int, true);
					}


					Type *slice_type = make_type_slice(proc->module->allocator, type_deref(ssa_type(ptr)));
					ssaValue *slice = ssa_add_local_generated(proc, slice_type);
					ssa_emit_store(proc, ssa_emit_struct_gep(proc, slice, v_zero32, ssa_type(ptr)), ptr);
					ssa_emit_store(proc, ssa_emit_struct_gep(proc, slice, v_one32,  t_int),         len);
					ssa_emit_store(proc, ssa_emit_struct_gep(proc, slice, v_two32,  t_int),         cap);
					return ssa_emit_load(proc, slice);
				} break;

				case BuiltinProc_min: {
					ssa_emit_comment(proc, make_string("min"));
					ssaValue *x = ssa_build_expr(proc, ce->args[0]);
					ssaValue *y = ssa_build_expr(proc, ce->args[1]);
					Type *t = get_base_type(ssa_type(x));
					Token lt = {Token_Lt};
					ssaValue *cond = ssa_emit_comp(proc, lt, x, y);
					return ssa_emit_select(proc, cond, x, y);
				} break;

				case BuiltinProc_max: {
					ssa_emit_comment(proc, make_string("max"));
					ssaValue *x = ssa_build_expr(proc, ce->args[0]);
					ssaValue *y = ssa_build_expr(proc, ce->args[1]);
					Type *t = get_base_type(ssa_type(x));
					Token gt = {Token_Gt};
					ssaValue *cond = ssa_emit_comp(proc, gt, x, y);
					return ssa_emit_select(proc, cond, x, y);
				} break;

				case BuiltinProc_abs: {
					ssa_emit_comment(proc, make_string("abs"));
					Token lt = {Token_Lt};
					Token sub = {Token_Sub};

					ssaValue *x = ssa_build_expr(proc, ce->args[0]);
					Type *t = ssa_type(x);

					ssaValue *neg_x = ssa_emit_arith(proc, sub, v_zero, x, t);
					ssaValue *cond = ssa_emit_comp(proc, lt, x, v_zero);
					return ssa_emit_select(proc, cond, neg_x, x);
				} break;
				}
			}
		}


		// NOTE(bill): Regular call
		ssaValue *value = ssa_build_expr(proc, ce->proc);
		Type *proc_type_ = get_base_type(ssa_type(value));
		GB_ASSERT(proc_type_->kind == Type_Proc);
		auto *type = &proc_type_->Proc;

		isize arg_index = 0;

		isize arg_count = 0;
		gb_for_array(i, ce->args) {
			AstNode *a = ce->args[i];
			Type *at = get_base_type(type_of_expr(proc->module->info, a));
			if (at->kind == Type_Tuple) {
				arg_count += at->Tuple.variable_count;
			} else {
				arg_count++;
			}
		}
		ssaValue **args = gb_alloc_array(proc->module->allocator, ssaValue *, arg_count);
		b32 variadic = proc_type_->Proc.variadic;
		b32 vari_expand = ce->ellipsis.pos.line != 0;

		gb_for_array(i, ce->args) {
			ssaValue *a = ssa_build_expr(proc, ce->args[i]);
			Type *at = ssa_type(a);
			if (at->kind == Type_Tuple) {
				for (isize i = 0; i < at->Tuple.variable_count; i++) {
					Entity *e = at->Tuple.variables[i];
					ssaValue *v = ssa_emit_struct_ev(proc, a, i, e->type);
					args[arg_index++] = v;
				}
			} else {
				args[arg_index++] = a;
			}
		}

		auto *pt = &type->params->Tuple;

		if (variadic) {
			isize i = 0;
			for (; i < type->param_count-1; i++) {
				args[i] = ssa_emit_conv(proc, args[i], pt->variables[i]->type, true);
			}
			if (!vari_expand) {
				Type *variadic_type = pt->variables[i]->type;
				GB_ASSERT(is_type_slice(variadic_type));
				variadic_type = get_base_type(variadic_type)->Slice.elem;
				for (; i < arg_count; i++) {
					args[i] = ssa_emit_conv(proc, args[i], variadic_type, true);
				}
			}
		} else {
			for (isize i = 0; i < arg_count; i++) {
				args[i] = ssa_emit_conv(proc, args[i], pt->variables[i]->type, true);
			}
		}

		if (variadic && !vari_expand) {
			ssa_emit_comment(proc, make_string("variadic call argument generation"));
			gbAllocator allocator = proc->module->allocator;
			Type *slice_type = pt->variables[type->param_count-1]->type;
			Type *elem_type  = get_base_type(slice_type)->Slice.elem;
			Type *elem_ptr_type = make_type_pointer(allocator, elem_type);
			ssaValue *slice = ssa_add_local_generated(proc, slice_type);
			isize slice_len = arg_count+1 - type->param_count;

			if (slice_len > 0) {
				ssaValue *base_array = ssa_add_local_generated(proc, make_type_array(allocator, elem_type, slice_len));

				for (isize i = type->param_count-1, j = 0; i < arg_count; i++, j++) {
					ssaValue *addr = ssa_emit_struct_gep(proc, base_array, j, elem_type);
					ssa_emit_store(proc, addr, args[i]);
				}

				ssaValue *base_elem  = ssa_emit_struct_gep(proc, base_array, v_zero32, elem_ptr_type);
				ssaValue *slice_elem = ssa_emit_struct_gep(proc, slice,      v_zero32, elem_ptr_type);
				ssa_emit_store(proc, slice_elem, base_elem);
				ssaValue *len = ssa_make_value_constant(allocator, t_int, make_exact_value_integer(slice_len));
				ssa_emit_store(proc, ssa_emit_struct_gep(proc, slice, v_one32, t_int), len);
				ssa_emit_store(proc, ssa_emit_struct_gep(proc, slice, v_two32, t_int), len);
			}


			arg_count = type->param_count;
			args[arg_count-1] = ssa_emit_load(proc, slice);
		}


		return ssa_emit_call(proc, value, args, arg_count);
	case_end;

	case_ast_node(se, SliceExpr, expr);
		return ssa_emit_load(proc, ssa_build_addr(proc, expr).addr);
	case_end;

	case_ast_node(ie, IndexExpr, expr);
		return ssa_emit_load(proc, ssa_build_addr(proc, expr).addr);
	case_end;
	}

	GB_PANIC("Unexpected expression: %.*s", LIT(ast_node_strings[expr->kind]));
	return NULL;
}


ssaValue *ssa_build_expr(ssaProcedure *proc, AstNode *expr) {
	expr = unparen_expr(expr);

	TypeAndValue *tv = map_get(&proc->module->info->types, hash_pointer(expr));
	GB_ASSERT_NOT_NULL(tv);

	if (tv->value.kind != ExactValue_Invalid) {
		if (tv->value.kind == ExactValue_String) {
			ssa_emit_comment(proc, make_string("Emit string constant"));

			// TODO(bill): Optimize by not allocating everytime
			if (tv->value.value_string.len > 0) {
				return ssa_emit_global_string(proc, tv->value);
			} else {
				ssaValue *null_string = ssa_add_local_generated(proc, t_string);
				return ssa_emit_load(proc, null_string);
			}
		}

		return ssa_make_value_constant(proc->module->allocator, tv->type, tv->value);
	}

	ssaValue *value = NULL;
	if (tv->mode == Addressing_Variable) {
		ssaAddr addr = ssa_build_addr(proc, expr);
		value = ssa_lvalue_load(proc, addr);
	} else {
		value = ssa_build_single_expr(proc, expr, tv);
	}

	return value;
}


ssaValue *ssa_add_using_variable(ssaProcedure *proc, Entity *e) {
	GB_ASSERT(e->kind == Entity_Variable && e->Variable.is_using);
	String name = e->token.string;
	Entity *parent = e->using_parent;
	ssaValue *p = NULL;
	if (parent->kind == Entity_Variable && parent->Variable.is_using) {
		p = ssa_add_using_variable(proc, parent);
	}

	Selection sel = lookup_field(parent->type, name, false);
	GB_ASSERT(sel.entity != NULL);
	ssaValue **pv = map_get(&proc->module->values, hash_pointer(parent));
	ssaValue *v = NULL;
	if (pv != NULL) {
		v = *pv;
	} else {
		v = ssa_build_addr(proc, e->using_expr).addr;
	}
	GB_ASSERT(v != NULL);
	ssaValue *var = ssa_emit_deep_field_gep(proc, parent->type, v, sel);
	map_set(&proc->module->values, hash_pointer(e), var);
	return var;
}

ssaAddr ssa_build_addr(ssaProcedure *proc, AstNode *expr) {
	switch (expr->kind) {
	case_ast_node(i, Ident, expr);
		if (ssa_is_blank_ident(expr)) {
			ssaAddr val = {};
			return val;
		}

		Entity *e = entity_of_ident(proc->module->info, expr);
		ssaValue *v = NULL;
		ssaValue **found = map_get(&proc->module->values, hash_pointer(e));
		if (found) {
			v = *found;
		} else if (e->kind == Entity_Variable && e->Variable.is_using) {
			v = ssa_add_using_variable(proc, e);
		}
		if (v == NULL) {
			GB_PANIC("Unknown value: %s, entity: %p\n", expr_to_string(expr), e);
		}

		return ssa_make_addr(v, expr);
	case_end;

	case_ast_node(pe, ParenExpr, expr);
		return ssa_build_addr(proc, unparen_expr(expr));
	case_end;

	case_ast_node(se, SelectorExpr, expr);
		ssa_emit_comment(proc, make_string("SelectorExpr"));
		Type *type = get_base_type(type_of_expr(proc->module->info, se->expr));

		Selection sel = lookup_field(type, unparen_expr(se->selector)->Ident.string, false);
		GB_ASSERT(sel.entity != NULL);

		ssaValue *e = ssa_build_addr(proc, se->expr).addr;
		e = ssa_emit_deep_field_gep(proc, type, e, sel);
		return ssa_make_addr(e, expr);
	case_end;

	case_ast_node(ue, UnaryExpr, expr);
		switch (ue->op.kind) {
		case Token_Pointer: {
			return ssa_build_addr(proc, ue->expr);
		}
		default:
			GB_PANIC("Invalid unary expression for ssa_build_addr");
		}
	case_end;

	case_ast_node(be, BinaryExpr, expr);
		switch (be->op.kind) {
		case Token_as: {
			ssa_emit_comment(proc, make_string("Cast - as"));
			// HACK(bill): Do have to make new variable to do this?
			// NOTE(bill): Needed for dereference of pointer conversion
			Type *type = type_of_expr(proc->module->info, expr);
			ssaValue *v = ssa_add_local_generated(proc, type);
			ssa_emit_store(proc, v, ssa_emit_conv(proc, ssa_build_expr(proc, be->left), type));
			return ssa_make_addr(v, expr);
		}
		case Token_transmute: {
			ssa_emit_comment(proc, make_string("Cast - transmute"));
			// HACK(bill): Do have to make new variable to do this?
			// NOTE(bill): Needed for dereference of pointer conversion
			Type *type = type_of_expr(proc->module->info, expr);
			ssaValue *v = ssa_add_local_generated(proc, type);
			ssa_emit_store(proc, v, ssa_emit_transmute(proc, ssa_build_expr(proc, be->left), type));
			return ssa_make_addr(v, expr);
		}
		default:
			GB_PANIC("Invalid binary expression for ssa_build_addr: %.*s\n", LIT(be->op.string));
			break;
		}
	case_end;

	case_ast_node(ie, IndexExpr, expr);
		ssa_emit_comment(proc, make_string("IndexExpr"));
		Type *t = get_base_type(type_of_expr(proc->module->info, ie->expr));
		gbAllocator a = proc->module->allocator;

		switch (t->kind) {
		case Type_Vector: {
			// HACK(bill): Fix how lvalues for vectors work
			ssaValue *vector = ssa_build_addr(proc, ie->expr).addr;
			ssaValue *index = ssa_emit_conv(proc, ssa_build_expr(proc, ie->index), t_int);
			ssaValue *len = ssa_make_value_constant(a, t_int, make_exact_value_integer(t->Vector.count));
			ssa_array_bounds_check(proc, ast_node_token(expr), index, len);
			return ssa_make_addr_vector(vector, index, expr);
		} break;

		case Type_Array: {
			ssaValue *array = ssa_build_addr(proc, ie->expr).addr;
			Type *et = make_type_pointer(proc->module->allocator, t->Array.elem);
			ssaValue *index = ssa_emit_conv(proc, ssa_build_expr(proc, ie->index), t_int);
			ssaValue *elem = ssa_emit_struct_gep(proc, array, index, et);
			ssaValue *len = ssa_make_value_constant(a, t_int, make_exact_value_integer(t->Vector.count));
			ssa_array_bounds_check(proc, ast_node_token(expr), index, len);
			return ssa_make_addr(elem, expr);
		} break;

		case Type_Slice: {
			ssaValue *slice = ssa_build_expr(proc, ie->expr);
			ssaValue *elem = ssa_slice_elem(proc, slice);
			ssaValue *len = ssa_slice_len(proc, slice);
			ssaValue *index = ssa_emit_conv(proc, ssa_build_expr(proc, ie->index), t_int);
			ssa_array_bounds_check(proc, ast_node_token(expr), index, len);
			ssaValue *v = ssa_emit_ptr_offset(proc, elem, index);
			return ssa_make_addr(v, expr);

		} break;

		case Type_Basic: { // Basic_string
			TypeAndValue *tv = map_get(&proc->module->info->types, hash_pointer(ie->expr));
			ssaValue *elem = NULL;
			ssaValue *len = NULL;
			if (tv->mode == Addressing_Constant) {
				ssaValue *array = ssa_add_global_string_array(proc->module, tv->value);
				elem = ssa_array_elem(proc, array);
				len = ssa_make_value_constant(a, t_int, make_exact_value_integer(tv->value.value_string.len));
			} else {
				ssaValue *str = ssa_build_expr(proc, ie->expr);
				elem = ssa_string_elem(proc, str);
				len = ssa_string_len(proc, str);
			}

			ssaValue *index = ssa_emit_conv(proc, ssa_build_expr(proc, ie->index), t_int);
			ssa_array_bounds_check(proc, ast_node_token(expr), index, len);

			ssaValue *v = ssa_emit_ptr_offset(proc, elem, index);
			return ssa_make_addr(v, expr);
		} break;
		}
	case_end;

	case_ast_node(se, SliceExpr, expr);
		ssa_emit_comment(proc, make_string("SliceExpr"));
		gbAllocator a = proc->module->allocator;
		ssaValue *low  = v_zero;
		ssaValue *high = NULL;
		ssaValue *max  = NULL;

		if (se->low  != NULL)    low  = ssa_build_expr(proc, se->low);
		if (se->high != NULL)    high = ssa_build_expr(proc, se->high);
		if (se->triple_indexed)  max  = ssa_build_expr(proc, se->max);
		ssaAddr base = ssa_build_addr(proc, se->expr);
		Type *type = get_base_type(ssa_addr_type(base));

		// TODO(bill): Cleanup like mad!

		switch (type->kind) {
		case Type_Slice: {
			Type *slice_type = ssa_addr_type(base);

			if (high == NULL) high = ssa_slice_len(proc, ssa_emit_load(proc, base.addr));
			if (max == NULL)  max  = ssa_slice_cap(proc, ssa_emit_load(proc, base.addr));
			GB_ASSERT(max != NULL);
			Token op_sub = {Token_Sub};
			ssaValue *elem = ssa_slice_elem(proc, ssa_emit_load(proc, base.addr));
			ssaValue *len  = ssa_emit_arith(proc, op_sub, high, low, t_int);
			ssaValue *cap  = ssa_emit_arith(proc, op_sub, max,  low, t_int);
			ssaValue *slice = ssa_add_local_generated(proc, slice_type);

			ssaValue *gep0 = ssa_emit_struct_gep(proc, slice, v_zero32, ssa_type(elem));
			ssaValue *gep1 = ssa_emit_struct_gep(proc, slice, v_one32, t_int);
			ssaValue *gep2 = ssa_emit_struct_gep(proc, slice, v_two32, t_int);
			ssa_emit_store(proc, gep0, elem);
			ssa_emit_store(proc, gep1, len);
			ssa_emit_store(proc, gep2, cap);

			return ssa_make_addr(slice, expr);
		}

		case Type_Array: {
			Type *slice_type = make_type_slice(a, type->Array.elem);

			if (high == NULL) high = ssa_array_len(proc, ssa_emit_load(proc, base.addr));
			if (max == NULL)  max  = ssa_array_cap(proc, ssa_emit_load(proc, base.addr));
			GB_ASSERT(max != NULL);
			Token op_sub = {Token_Sub};
			ssaValue *elem = ssa_array_elem(proc, base.addr);
			ssaValue *len  = ssa_emit_arith(proc, op_sub, high, low, t_int);
			ssaValue *cap  = ssa_emit_arith(proc, op_sub, max,  low, t_int);
			ssaValue *slice = ssa_add_local_generated(proc, slice_type);

			ssaValue *gep0 = ssa_emit_struct_gep(proc, slice, v_zero32, ssa_type(elem));
			ssaValue *gep1 = ssa_emit_struct_gep(proc, slice, v_one32, t_int);
			ssaValue *gep2 = ssa_emit_struct_gep(proc, slice, v_two32, t_int);
			ssa_emit_store(proc, gep0, elem);
			ssa_emit_store(proc, gep1, len);
			ssa_emit_store(proc, gep2, cap);

			return ssa_make_addr(slice, expr);
		}

		case Type_Basic:
			return ssa_make_addr(ssa_emit_substring(proc, ssa_emit_load(proc, base.addr), low, high), expr);
		}

		GB_PANIC("Unknown slicable type");
	case_end;

	case_ast_node(de, DerefExpr, expr);
		ssaValue *e = ssa_build_expr(proc, de->expr);
		// TODO(bill): Is a ptr copy needed?
		ssaValue *gep = ssa_emit_zero_gep(proc, e);
		return ssa_make_addr(gep, expr);
	case_end;
	}

	TokenPos token_pos = ast_node_token(expr).pos;
	GB_PANIC("Unexpected address expression\n"
	         "\tAstNode: %.*s @ "
	         "%.*s(%td:%td)\n",
	         LIT(ast_node_strings[expr->kind]),
	         LIT(token_pos.file), token_pos.line, token_pos.column);


	return ssa_make_addr(NULL, NULL);
}

void ssa_build_assign_op(ssaProcedure *proc, ssaAddr lhs, ssaValue *value, Token op) {
	ssaValue *old_value = ssa_lvalue_load(proc, lhs);
	ssaValue *change = ssa_emit_conv(proc, value, ssa_type(old_value));
	ssaValue *new_value = ssa_emit_arith(proc, op, old_value, change, ssa_type(old_value));
	ssa_lvalue_store(proc, lhs, new_value);
}

void ssa_build_cond(ssaProcedure *proc, AstNode *cond, ssaBlock *true_block, ssaBlock *false_block) {
	switch (cond->kind) {
	case_ast_node(pe, ParenExpr, cond);
		ssa_build_cond(proc, pe->expr, true_block, false_block);
		return;
	case_end;

	case_ast_node(ue, UnaryExpr, cond);
		if (ue->op.kind == Token_Not) {
			ssa_build_cond(proc, ue->expr, false_block, true_block);
			return;
		}
	case_end;

	case_ast_node(be, BinaryExpr, cond);
		if (be->op.kind == Token_CmpAnd) {
			ssaBlock *block = ssa_add_block(proc, NULL, make_string("cmp.and"));
			ssa_build_cond(proc, be->left, block, false_block);
			proc->curr_block = block;
			ssa_build_cond(proc, be->right, true_block, false_block);
			return;
		} else if (be->op.kind == Token_CmpOr) {
			ssaBlock *block = ssa_add_block(proc, NULL, make_string("cmp.or"));
			ssa_build_cond(proc, be->left, true_block, block);
			proc->curr_block = block;
			ssa_build_cond(proc, be->right, true_block, false_block);
			return;
		}
	case_end;
	}

	ssaValue *expr = ssa_build_expr(proc, cond);
	ssa_emit_if(proc, expr, true_block, false_block);
}


void ssa_gen_global_type_name(ssaModule *m, Entity *e, String name) {
	ssaValue *t = ssa_make_value_type_name(m->allocator, name, e->type);
	ssa_module_add_value(m, e, t);
	map_set(&m->members, hash_string(name), t);

	Type *bt = get_base_type(e->type);
	if (bt->kind == Type_Record) {
		auto *s = &bt->Record;
		for (isize j = 0; j < s->other_field_count; j++) {
			Entity *field = s->other_fields[j];
			if (field->kind == Entity_TypeName) {
				// HACK(bill): Override name of type so printer prints it correctly
				auto *tn = &field->type->Named;
				String cn = field->token.string;
				isize len = name.len + 1 + cn.len;
				String child = {NULL, len};
				child.text = gb_alloc_array(m->allocator, u8, len);
				isize i = 0;
				gb_memcopy(child.text+i, name.text, name.len);
				i += name.len;
				child.text[i++] = '.';
				gb_memcopy(child.text+i, cn.text, cn.len);
				tn->name = child;
				ssa_gen_global_type_name(m, field, tn->name);
			}
		}
	}

	if (is_type_union(bt)) {
		auto *s = &bt->Record;
		// NOTE(bill): Zeroth entry is null (for `match type` stmts)
		for (isize j = 1; j < s->field_count; j++) {
			Entity *field = s->fields[j];
			if (field->kind == Entity_TypeName) {
				// HACK(bill): Override name of type so printer prints it correctly
				auto *tn = &field->type->Named;
				String cn = field->token.string;
				isize len = name.len + 1 + cn.len;
				String child = {NULL, len};
				child.text = gb_alloc_array(m->allocator, u8, len);
				isize i = 0;
				gb_memcopy(child.text+i, name.text, name.len);
				i += name.len;
				child.text[i++] = '.';
				gb_memcopy(child.text+i, cn.text, cn.len);
				tn->name = child;
				ssa_gen_global_type_name(m, field, tn->name);
			}
		}
	}
}



void ssa_build_stmt_list(ssaProcedure *proc, AstNodeArray stmts) {
	gb_for_array(i, stmts) {
		ssa_build_stmt(proc, stmts[i]);
	}
}

void ssa_build_stmt(ssaProcedure *proc, AstNode *node) {
	u32 prev_stmt_state_flags = proc->module->stmt_state_flags;
	defer (proc->module->stmt_state_flags = prev_stmt_state_flags);

	if (node->stmt_state_flags != 0) {
		u32 in = node->stmt_state_flags;
		u32 out = proc->module->stmt_state_flags;
		defer (proc->module->stmt_state_flags = out);

		if (in & StmtStateFlag_abc) {
			out |= StmtStateFlag_abc;
			out &= ~StmtStateFlag_no_abc;
		} else if (in & StmtStateFlag_no_abc) {
			out |= StmtStateFlag_no_abc;
			out &= ~StmtStateFlag_abc;
		}
	}


	switch (node->kind) {
	case_ast_node(bs, EmptyStmt, node);
	case_end;

	case_ast_node(us, UsingStmt, node);
		AstNode *decl = unparen_expr(us->node);
		if (decl->kind == AstNode_VarDecl) {
			ssa_build_stmt(proc, decl);
		}
	case_end;

	case_ast_node(vd, VarDecl, node);
		if (vd->kind == Declaration_Mutable) {
			if (gb_array_count(vd->names) == gb_array_count(vd->values)) { // 1:1 assigment
				gbArray(ssaAddr)  lvals;
				gbArray(ssaValue *) inits;
				gb_array_init_reserve(lvals, gb_heap_allocator(), gb_array_count(vd->names));
				gb_array_init_reserve(inits, gb_heap_allocator(), gb_array_count(vd->names));
				defer (gb_array_free(lvals));
				defer (gb_array_free(inits));

				gb_for_array(i, vd->names) {
					AstNode *name = vd->names[i];
					ssaAddr lval = ssa_make_addr(NULL, NULL);
					if (!ssa_is_blank_ident(name)) {
						ssa_add_local_for_identifier(proc, name, false);
						lval = ssa_build_addr(proc, name);
						GB_ASSERT(lval.addr != NULL);
					}

					gb_array_append(lvals, lval);
				}
				gb_for_array(i, vd->values) {
					ssaValue *init = ssa_build_expr(proc, vd->values[i]);
					gb_array_append(inits, init);
				}


				gb_for_array(i, inits) {
					ssaValue *v = ssa_emit_conv(proc, inits[i], ssa_addr_type(lvals[i]));
					ssa_lvalue_store(proc, lvals[i], v);
				}

			} else if (gb_array_count(vd->values) == 0) { // declared and zero-initialized
				gb_for_array(i, vd->names) {
					AstNode *name = vd->names[i];
					if (!ssa_is_blank_ident(name)) {
						ssa_add_local_for_identifier(proc, name, true);
					}
				}
			} else { // Tuple(s)
				gbArray(ssaAddr)  lvals;
				gbArray(ssaValue *) inits;
				gb_array_init_reserve(lvals, gb_heap_allocator(), gb_array_count(vd->names));
				gb_array_init_reserve(inits, gb_heap_allocator(), gb_array_count(vd->names));
				defer (gb_array_free(lvals));
				defer (gb_array_free(inits));

				gb_for_array(i, vd->names) {
					AstNode *name = vd->names[i];
					ssaAddr lval = ssa_make_addr(NULL, NULL);
					if (!ssa_is_blank_ident(name)) {
						ssa_add_local_for_identifier(proc, name, false);
						lval = ssa_build_addr(proc, name);
					}

					gb_array_append(lvals, lval);
				}

				gb_for_array(i, vd->values) {
					ssaValue *init = ssa_build_expr(proc, vd->values[i]);
					Type *t = ssa_type(init);
					if (t->kind == Type_Tuple) {
						for (isize i = 0; i < t->Tuple.variable_count; i++) {
							Entity *e = t->Tuple.variables[i];
							ssaValue *v = ssa_emit_struct_ev(proc, init, i, e->type);
							gb_array_append(inits, v);
						}
					} else {
						gb_array_append(inits, init);
					}
				}


				gb_for_array(i, inits) {
					ssaValue *v = ssa_emit_conv(proc, inits[i], ssa_addr_type(lvals[i]));
					ssa_lvalue_store(proc, lvals[i], v);
				}
			}
		}
	case_end;

	case_ast_node(pd, ProcDecl, node);
		if (proc->children == NULL) {
			gb_array_init(proc->children, gb_heap_allocator());
		}


		if (pd->body != NULL) {
			// NOTE(bill): Generate a new name
			// parent$name-guid
			String pd_name = pd->name->Ident.string;
			isize name_len = proc->name.len + 1 + pd_name.len + 1 + 10 + 1;
			u8 *name_text = gb_alloc_array(proc->module->allocator, u8, name_len);
			i32 guid = cast(i32)gb_array_count(proc->children);
			name_len = gb_snprintf(cast(char *)name_text, name_len, "%.*s$%.*s-%d", LIT(proc->name), LIT(pd_name), guid);
			String name = make_string(name_text, name_len-1);

			Entity **found = map_get(&proc->module->info->definitions, hash_pointer(pd->name));
			GB_ASSERT_MSG(found != NULL, "Unable to find: %.*s", LIT(pd->name->Ident.string));
			Entity *e = *found;
			ssaValue *value = ssa_make_value_procedure(proc->module->allocator,
			                                           proc->module, e->type, pd->type, pd->body, name);

			value->Proc.tags = pd->tags;

			ssa_module_add_value(proc->module, e, value);
			gb_array_append(proc->children, &value->Proc);
			ssa_build_proc(value, proc);
		} else {
			// FFI - Foreign function interace
			String original_name = pd->name->Ident.string;
			String name = original_name;
			if (pd->foreign_name.len > 0) {
				name = pd->foreign_name;
			}
			auto *info = proc->module->info;

			Entity *e = *map_get(&info->definitions, hash_pointer(pd->name));
			Entity *f = *map_get(&info->foreign_procs, hash_string(name));
			ssaValue *value = ssa_make_value_procedure(proc->module->allocator,
			                                           proc->module, e->type, pd->type, pd->body, name);

			value->Proc.tags = pd->tags;

			ssa_module_add_value(proc->module, e, value);
			ssa_build_proc(value, proc);
			if (e == f) {
				// NOTE(bill): Don't do mutliple declarations in the IR
				gb_array_append(proc->children, &value->Proc);
			}
		}
	case_end;

	case_ast_node(td, TypeDecl, node);

		// NOTE(bill): Generate a new name
		// parent_proc.name-guid
		String td_name = td->name->Ident.string;
		isize name_len = proc->name.len + 1 + td_name.len + 1 + 10 + 1;
		u8 *name_text = gb_alloc_array(proc->module->allocator, u8, name_len);
		i32 guid = cast(i32)gb_array_count(proc->module->members.entries);
		name_len = gb_snprintf(cast(char *)name_text, name_len, "%.*s.%.*s-%d", LIT(proc->name), LIT(td_name), guid);
		String name = make_string(name_text, name_len-1);

		Entity **found = map_get(&proc->module->info->definitions, hash_pointer(td->name));
		GB_ASSERT(found != NULL);
		Entity *e = *found;
		ssaValue *value = ssa_make_value_type_name(proc->module->allocator,
		                                           name, e->type);
		// HACK(bill): Override name of type so printer prints it correctly
		e->type->Named.name = name;
		ssa_gen_global_type_name(proc->module, e, name);
	case_end;

	case_ast_node(ids, IncDecStmt, node);
		ssa_emit_comment(proc, make_string("IncDecStmt"));
		Token op = ids->op;
		if (op.kind == Token_Increment) {
			op.kind = Token_Add;
		} else if (op.kind == Token_Decrement) {
			op.kind = Token_Sub;
		}
		ssaAddr lval = ssa_build_addr(proc, ids->expr);
		ssaValue *one = ssa_emit_conv(proc, v_one, ssa_addr_type(lval));
		ssa_build_assign_op(proc, lval, one, op);

	case_end;

	case_ast_node(as, AssignStmt, node);
		ssa_emit_comment(proc, make_string("AssignStmt"));
		switch (as->op.kind) {
		case Token_Eq: {
			gbArray(ssaAddr) lvals;
			gb_array_init(lvals, gb_heap_allocator());
			defer (gb_array_free(lvals));

			gb_for_array(i, as->lhs) {
				AstNode *lhs = as->lhs[i];
				ssaAddr lval = {};
				if (!ssa_is_blank_ident(lhs)) {
					lval = ssa_build_addr(proc, lhs);
				}
				gb_array_append(lvals, lval);
			}

			if (gb_array_count(as->lhs) == gb_array_count(as->rhs)) {
				if (gb_array_count(as->lhs) == 1) {
					AstNode *rhs = as->rhs[0];
					ssaValue *init = ssa_build_expr(proc, rhs);
					ssa_lvalue_store(proc, lvals[0], init);
				} else {
					gbArray(ssaValue *) inits;
					gb_array_init_reserve(inits, gb_heap_allocator(), gb_array_count(lvals));
					defer (gb_array_free(inits));

					gb_for_array(i, as->rhs) {
						ssaValue *init = ssa_build_expr(proc, as->rhs[i]);
						gb_array_append(inits, init);
					}

					gb_for_array(i, inits) {
						ssa_lvalue_store(proc, lvals[i], inits[i]);
					}
				}
			} else {
				gbArray(ssaValue *) inits;
				gb_array_init_reserve(inits, gb_heap_allocator(), gb_array_count(lvals));
				defer (gb_array_free(inits));

				gb_for_array(i, as->rhs) {
					ssaValue *init = ssa_build_expr(proc, as->rhs[i]);
					Type *t = ssa_type(init);
					// TODO(bill): refactor for code reuse as this is repeated a bit
					if (t->kind == Type_Tuple) {
						for (isize i = 0; i < t->Tuple.variable_count; i++) {
							Entity *e = t->Tuple.variables[i];
							ssaValue *v = ssa_emit_struct_ev(proc, init, i, e->type);
							gb_array_append(inits, v);
						}
					} else {
						gb_array_append(inits, init);
					}
				}

				gb_for_array(i, inits) {
					ssa_lvalue_store(proc, lvals[i], inits[i]);
				}
			}

		} break;

		default: {
			// NOTE(bill): Only 1 += 1 is allowed, no tuples
			// +=, -=, etc
			Token op = as->op;
			i32 kind = op.kind;
			kind += Token_Add - Token_AddEq; // Convert += to +
			op.kind = cast(TokenKind)kind;
			ssaAddr lhs = ssa_build_addr(proc, as->lhs[0]);
			ssaValue *value = ssa_build_expr(proc, as->rhs[0]);
			ssa_build_assign_op(proc, lhs, value, op);
		} break;
		}
	case_end;

	case_ast_node(es, ExprStmt, node);
		// NOTE(bill): No need to use return value
		ssa_build_expr(proc, es->expr);
	case_end;

	case_ast_node(bs, BlockStmt, node);
		proc->scope_index++;
		ssa_build_stmt_list(proc, bs->stmts);
		ssa_emit_defer_stmts(proc, ssaDefer_Default, NULL);
		proc->scope_index--;
	case_end;

	case_ast_node(ds, DeferStmt, node);
		ssa_emit_comment(proc, make_string("DeferStmt"));
		isize scope_index = proc->scope_index;
		if (ds->stmt->kind == AstNode_BlockStmt)
			scope_index--;
		ssaDefer d = {ds->stmt, scope_index, proc->curr_block};
		gb_array_append(proc->defer_stmts, d);
	case_end;

	case_ast_node(rs, ReturnStmt, node);
		ssa_emit_comment(proc, make_string("ReturnStmt"));
		ssaValue *v = NULL;
		auto *return_type_tuple  = &proc->type->Proc.results->Tuple;
		isize return_count = proc->type->Proc.result_count;
		if (gb_array_count(rs->results) == 1 && return_count > 1) {
			GB_PANIC("ReturnStmt tuple return statement");
		} else if (return_count == 1) {
			Entity *e = return_type_tuple->variables[0];
			v = ssa_emit_conv(proc, ssa_build_expr(proc, rs->results[0]), e->type);
		} else if (return_count == 0) {
			// No return values
		} else {
			// 1:1 multiple return values
			Type *ret_type = proc->type->Proc.results;
			v = ssa_add_local_generated(proc, ret_type);
			gb_for_array(i, rs->results) {
				Entity *e = return_type_tuple->variables[i];
				ssaValue *res = ssa_emit_conv(proc, ssa_build_expr(proc, rs->results[i]), e->type);
				ssaValue *field = ssa_emit_struct_gep(proc, v, i, e->type);
				ssa_emit_store(proc, field, res);
			}
			v = ssa_emit_load(proc, v);
		}
		ssa_emit_ret(proc, v);

	case_end;

	case_ast_node(is, IfStmt, node);
		ssa_emit_comment(proc, make_string("IfStmt"));
		if (is->init != NULL) {
			ssaBlock *init = ssa_add_block(proc, node, make_string("if.init"));
			ssa_emit_jump(proc, init);
			proc->curr_block = init;
			ssa_build_stmt(proc, is->init);
		}
		ssaBlock *then = ssa_add_block(proc, node, make_string("if.then"));
		ssaBlock *done = ssa__make_block(proc, node, make_string("if.done")); // NOTE(bill): Append later
		ssaBlock *else_ = done;
		if (is->else_stmt != NULL) {
			else_ = ssa_add_block(proc, is->else_stmt, make_string("if.else"));
		}

		ssa_build_cond(proc, is->cond, then, else_);
		proc->curr_block = then;

		proc->scope_index++;
		ssa_build_stmt(proc, is->body);
		ssa_emit_defer_stmts(proc, ssaDefer_Default, NULL);
		proc->scope_index--;

		ssa_emit_jump(proc, done);

		if (is->else_stmt != NULL) {
			proc->curr_block = else_;

			proc->scope_index++;
			ssa_build_stmt(proc, is->else_stmt);
			ssa_emit_defer_stmts(proc, ssaDefer_Default, NULL);
			proc->scope_index--;


			ssa_emit_jump(proc, done);
		}
		gb_array_append(proc->blocks, done);
		proc->curr_block = done;
	case_end;

	case_ast_node(fs, ForStmt, node);
		ssa_emit_comment(proc, make_string("ForStmt"));
		if (fs->init != NULL) {
			ssaBlock *init = ssa_add_block(proc, node, make_string("for.init"));
			ssa_emit_jump(proc, init);
			proc->curr_block = init;
			ssa_build_stmt(proc, fs->init);
		}
		ssaBlock *body = ssa_add_block(proc, node, make_string("for.body"));
		ssaBlock *done = ssa__make_block(proc, node, make_string("for.done")); // NOTE(bill): Append later

		ssaBlock *loop = body;

		if (fs->cond != NULL) {
			loop = ssa_add_block(proc, node, make_string("for.loop"));
		}
		ssaBlock *cont = loop;
		if (fs->post != NULL) {
			cont = ssa_add_block(proc, node, make_string("for.post"));

		}
		ssa_emit_jump(proc, loop);
		proc->curr_block = loop;
		if (loop != body) {
			ssa_build_cond(proc, fs->cond, body, done);
			proc->curr_block = body;
		}

		ssa_push_target_list(proc, done, cont, NULL);

		proc->scope_index++;
		ssa_build_stmt(proc, fs->body);
		ssa_emit_defer_stmts(proc, ssaDefer_Default, NULL);
		proc->scope_index--;

		ssa_pop_target_list(proc);
		ssa_emit_jump(proc, cont);

		if (fs->post != NULL) {
			proc->curr_block = cont;
			ssa_build_stmt(proc, fs->post);
			ssa_emit_jump(proc, loop);
		}


		gb_array_append(proc->blocks, done);
		proc->curr_block = done;

	case_end;

	case_ast_node(ms, MatchStmt, node);
		ssa_emit_comment(proc, make_string("MatchStmt"));
		if (ms->init != NULL) {
			ssa_build_stmt(proc, ms->init);
		}
		ssaValue *tag = v_true;
		if (ms->tag != NULL) {
			tag = ssa_build_expr(proc, ms->tag);
		}
		ssaBlock *done = ssa__make_block(proc, node, make_string("match.done")); // NOTE(bill): Append later

		ast_node(body, BlockStmt, ms->body);


		AstNodeArray default_stmts = NULL;
		ssaBlock *default_fall = NULL;
		ssaBlock *default_block = NULL;

		ssaBlock *fall = NULL;
		b32 append_fall = false;

		isize case_count = gb_array_count(body->stmts);
		gb_for_array(i, body->stmts) {
			AstNode *clause = body->stmts[i];
			ssaBlock *body = fall;
			b32 append_body = false;


			ast_node(cc, CaseClause, clause);

			if (body == NULL) {
				append_body = true;
				if (gb_array_count(cc->list)) {
					body = ssa__make_block(proc, clause, make_string("match.dflt.body"));
				} else {
					body = ssa__make_block(proc, clause, make_string("match.case.body"));
				}
			}
			if (append_fall && body == fall) {
				append_fall = false;
				append_body = true;
			}

			fall = done;
			if (i+1 < case_count) {
				append_fall = true;
				fall = ssa__make_block(proc, clause, make_string("match.fall.body"));
			}

			if (gb_array_count(cc->list) == 0) {
				// default case
				default_stmts = cc->stmts;
				default_fall  = fall;
				default_block = body;
				continue;
			}

			ssaBlock *next_cond = NULL;
			Token eq = {Token_CmpEq};
			gb_for_array(j, cc->list) {
				AstNode *expr = cc->list[j];
				next_cond = ssa__make_block(proc, clause, make_string("match.case.next"));

				ssaValue *cond = ssa_emit_comp(proc, eq, tag, ssa_build_expr(proc, expr));
				ssa_emit_if(proc, cond, body, next_cond);
				gb_array_append(proc->blocks, next_cond);
				proc->curr_block = next_cond;
			}
			if (append_body) {
				gb_array_append(proc->blocks, body);
			}
			proc->curr_block = body;
			ssa_push_target_list(proc, done, NULL, fall);
			ssa_build_stmt_list(proc, cc->stmts);
			ssa_pop_target_list(proc);
			ssa_emit_jump(proc, done);
			proc->curr_block = next_cond;
		}

		if (default_block != NULL) {
			ssa_emit_jump(proc, default_block);
			gb_array_append(proc->blocks, default_block);
			proc->curr_block = default_block;
			ssa_push_target_list(proc, done, NULL, default_fall);
			ssa_build_stmt_list(proc, default_stmts);
			ssa_pop_target_list(proc);
		}

		ssa_emit_jump(proc, done);
		gb_array_append(proc->blocks, done);
		proc->curr_block = done;
	case_end;


	case_ast_node(ms, TypeMatchStmt, node);
		ssa_emit_comment(proc, make_string("TypeMatchStmt"));
		gbAllocator allocator = proc->module->allocator;

		ssaValue *parent = ssa_build_expr(proc, ms->tag);
		Type *union_type = type_deref(ssa_type(parent));
		GB_ASSERT(is_type_union(union_type));

		ssa_emit_comment(proc, make_string("get union's tag"));
		ssaValue *tag_index = ssa_emit_struct_gep(proc, parent, v_one32, make_type_pointer(allocator, t_int));
		tag_index = ssa_emit_load(proc, tag_index);

		ssaValue *data = ssa_emit_conv(proc, parent, t_rawptr);

		ssaBlock *start_block = ssa_add_block(proc, node, make_string("type-match.case.first"));
		ssa_emit_jump(proc, start_block);
		proc->curr_block = start_block;

		ssaBlock *done = ssa__make_block(proc, node, make_string("type-match.done")); // NOTE(bill): Append later

		ast_node(body, BlockStmt, ms->body);


		String tag_var_name = ms->var->Ident.string;

		AstNodeArray default_stmts = NULL;
		ssaBlock *default_block = NULL;

		isize case_count = gb_array_count(body->stmts);
		gb_for_array(i, body->stmts) {
			AstNode *clause = body->stmts[i];
			ast_node(cc, CaseClause, clause);

			if (gb_array_count(cc->list) == 0) {
				// default case
				default_stmts = cc->stmts;
				default_block = ssa__make_block(proc, clause, make_string("type-match.dflt.body"));
				continue;
			}


			ssaBlock *body = ssa__make_block(proc, clause, make_string("type-match.case.body"));


			Scope *scope = *map_get(&proc->module->info->scopes, hash_pointer(clause));
			Entity *tag_var_entity = current_scope_lookup_entity(scope, tag_var_name);
			GB_ASSERT_MSG(tag_var_entity != NULL, "%.*s", LIT(tag_var_name));
			ssaValue *tag_var = ssa_add_local(proc, tag_var_entity);
			ssaValue *data_ptr = ssa_emit_conv(proc, data, tag_var_entity->type);
			ssa_emit_store(proc, tag_var, data_ptr);



			Type *base_type = type_deref(tag_var_entity->type);
			ssaValue *index = NULL;
			Type *ut = get_base_type(union_type);
			GB_ASSERT(ut->Record.kind == TypeRecord_Union);
			for (isize field_index = 1; field_index < ut->Record.field_count; field_index++) {
				Entity *f = get_base_type(union_type)->Record.fields[field_index];
				if (are_types_identical(f->type, base_type)) {
					index = ssa_make_value_constant(allocator, t_int, make_exact_value_integer(field_index));
					break;
				}
			}
			GB_ASSERT(index != NULL);

			ssaBlock *next_cond = ssa__make_block(proc, clause, make_string("type-match.case.next"));
			Token eq = {Token_CmpEq};
			ssaValue *cond = ssa_emit_comp(proc, eq, tag_index, index);
			ssa_emit_if(proc, cond, body, next_cond);
			gb_array_append(proc->blocks, next_cond);
			proc->curr_block = next_cond;

			gb_array_append(proc->blocks, body);
			proc->curr_block = body;
			ssa_push_target_list(proc, done, NULL, NULL);
			ssa_build_stmt_list(proc, cc->stmts);
			ssa_pop_target_list(proc);
			ssa_emit_jump(proc, done);
			proc->curr_block = next_cond;
		}

		if (default_block != NULL) {
			ssa_emit_jump(proc, default_block);
			gb_array_append(proc->blocks, default_block);
			proc->curr_block = default_block;
			ssa_push_target_list(proc, done, NULL, NULL);
			ssa_build_stmt_list(proc, default_stmts);
			ssa_pop_target_list(proc);
		}

		ssa_emit_jump(proc, done);
		gb_array_append(proc->blocks, done);
		proc->curr_block = done;
	case_end;

	case_ast_node(bs, BranchStmt, node);
		ssaBlock *block = NULL;
		switch (bs->token.kind) {
		case Token_break: {
			for (ssaTargetList *t = proc->target_list; t != NULL && block == NULL; t = t->prev) {
				block = t->break_;
			}
		} break;
		case Token_continue: {
			for (ssaTargetList *t = proc->target_list; t != NULL && block == NULL; t = t->prev) {
				block = t->continue_;
			}
		} break;
		case Token_fallthrough: {
			for (ssaTargetList *t = proc->target_list; t != NULL && block == NULL; t = t->prev) {
				block = t->fallthrough_;
			}
		} break;
		}
		if (block != NULL && bs->token.kind != Token_fallthrough) {
			ssa_emit_defer_stmts(proc, ssaDefer_Branch, block);
		}
		switch (bs->token.kind) {
		case Token_break:       ssa_emit_comment(proc, make_string("break"));       break;
		case Token_continue:    ssa_emit_comment(proc, make_string("continue"));    break;
		case Token_fallthrough: ssa_emit_comment(proc, make_string("fallthrough")); break;
		}
		ssa_emit_jump(proc, block);
		ssa_emit_unreachable(proc);
	case_end;

	}
}


void ssa_emit_startup_runtime(ssaProcedure *proc) {
	GB_ASSERT(proc->parent == NULL && are_strings_equal(proc->name, make_string("main")));

	ssa_emit(proc, ssa_alloc_instr(proc, ssaInstr_StartupRuntime));
}

void ssa_insert_code_before_proc(ssaProcedure* proc, ssaProcedure *parent) {
	if (parent == NULL) {
		if (are_strings_equal(proc->name, make_string("main"))) {
			ssa_emit_startup_runtime(proc);
		}
	}
}


void ssa_build_proc(ssaValue *value, ssaProcedure *parent) {
	ssaProcedure *proc = &value->Proc;

	proc->parent = parent;

	if (proc->body != NULL) {
		u32 prev_stmt_state_flags = proc->module->stmt_state_flags;
		defer (proc->module->stmt_state_flags = prev_stmt_state_flags);

		if (proc->tags != 0) {
			u32 in = proc->tags;
			u32 out = proc->module->stmt_state_flags;
			defer (proc->module->stmt_state_flags = out);

			if (in & ProcTag_abc) {
				out |= ProcTag_abc;
				out &= ~ProcTag_no_abc;
			} else if (in & ProcTag_no_abc) {
				out |= ProcTag_no_abc;
				out &= ~ProcTag_abc;
			}
		}


		ssa_begin_procedure_body(proc);
		ssa_insert_code_before_proc(proc, parent);
		ssa_build_stmt(proc, proc->body);
		ssa_end_procedure_body(proc);
	}
}
