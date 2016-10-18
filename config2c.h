#ifndef CONFIG2C_H 
#define CONFIG2C_H

/*
 * This file is part of config2c which is relased under Apache License.
 * See LICENSE for full license details.
 */


typedef const char *string;
struct string_list {
	string str;
	struct string_list *next;
};

struct node_mapping {
	string name;
	string parse_func;
	string free_func;
	string dump_func;
	struct string_list *mapped_types;
	struct node_mapping *next;
};

struct node_type_def_list {
	struct node_type_def_list *next;
	enum {
		NODE_TYPE_DEF_ENUM,
		NODE_TYPE_DEF_STRUCT,
		NODE_TYPE_DEF_UNION,
	} type;
	union {
		struct {
			const char *name;
			struct node_enum_list *enums;
		} enum_def;
		struct {
			const char *name;
			struct node_member_list *members;
			int exported;
		} struct_def;
		struct {
			const char *name;
			const char *enum_name;
			struct node_alter_list *alters;
		} union_def;
	};
};

struct node_enum_list {
	struct node_enum_list *next;
	const char *name;
	const char *alias;
};

struct node_vec_def {
	enum {
		NODE_TYPE_SCALE,
		NODE_TYPE_FIX_ARR,
		NODE_TYPE_VAR_ARR,
	} type;
	union {
		long len_int;
		string len_str;
	};
};

struct node_member_list {
	struct node_member_list *next;
	enum {
		NODE_MEMBER_DEF_PRIM,
		NODE_MEMBER_DEF_ENUM,
		NODE_MEMBER_DEF_STRUCT,
		NODE_MEMBER_DEF_UNION,
		NODE_MEMBER_DEF_UNNAMED_UNION,
	} type;
	union {
		struct {
			string type_name;
			string in_name;
			struct string_list *mapped;
			struct node_vec_def vec;
			string default_val;
		};
		struct {
			struct node_alter_list  *alters;
			string alt_enum;
		};
	};
};

struct node_alter_list {
	struct node_alter_list *next;
	enum {
		NODE_ALTER_DEF_PRIM,
		NODE_ALTER_DEF_ENUM,
		NODE_ALTER_DEF_STRUCT,
		NODE_ALTER_DEF_UNNAMED_STRUCT,
	} type;
	union {
		struct {
			string type_name;
			string in_name;
			struct string_list *mapped;
			struct node_vec_def vec;
		};
		struct node_member_list *members;
	};
	string enum_val;
};

extern struct node_mapping *mapping;
extern struct node_type_def_list *ast;

#define rev_list(name, type, pnext) \
	static inline type *name(type *p) \
	{ \
		type *q = NULL; \
		type *r; \
		while (p) { \
			r = p; \
			p = p->pnext; \
			r->pnext = q; \
			q = r; \
		} \
		return q; \
	}

rev_list(rev_type_def_list, struct node_type_def_list, next);
rev_list(rev_enum_list, struct node_enum_list, next);
rev_list(rev_member_list, struct node_member_list, next);
rev_list(rev_alter_list, struct node_alter_list, next);
rev_list(rev_mapping, struct node_mapping, next);
rev_list(rev_string_list, struct string_list, next);

#define len_list(name, type, pnext) \
	static inline long name(const type *p) \
	{ \
		long i = 0; \
		while (p) { \
			++i; \
			p = p->pnext; \
		} \
		return i; \
	}
len_list(len_type_def_list, struct node_type_def_list, next);
len_list(len_enum_list, struct node_enum_list, next);
len_list(len_member_list, struct node_member_list, next);
len_list(len_alter_list, struct node_alter_list, next);
len_list(len_mapping, struct node_mapping, next);
len_list(len_string_list, struct string_list, next);



#endif
