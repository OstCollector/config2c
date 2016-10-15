/*
 * This file is part of config2c which is relased under Apache License.
 * See LICENSE for full license details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "config2c.h"

#define func_header() \
	do { \
		fprintf(stderr, "entering: %d %s\n", __LINE__, __func__); \
	} while (0)

#define ERROR(fmt, ...) do { fprintf(stderr, fmt, ##__VA_ARGS__); } while (0)

struct node_mapping *mapping;
struct node_type_def_list *ast;
const char *top;

FILE *fp_hdr;
FILE *fp_src;
#define out_fp(fp, file_des, fmt, ...) \
	do { \
		if (0 > fprintf(fp, fmt, ##__VA_ARGS__)) { \
			fprintf(stderr, "failed to write to %s.\n", file_des); \
			exit(EXIT_FAILURE); \
		} \
	} while (0)
#define out_src(fmt, ...) out_fp(fp_src, "source file", fmt, ##__VA_ARGS__)
#define out_hdr(fmt, ...) out_fp(fp_hdr, "source file", fmt, ##__VA_ARGS__)

static indent_hdr(int level)
{
	for (; level; --level) {
		out_hdr("    ");
	}
}

static indent_src(int level)
{
	for (; level; --level) {
		out_src("    ");
	}
}

#define osi(l, fmt, ...) \
	do { \
		indent_src(l); \
		out_fp(fp_src, "source file", fmt, ##__VA_ARGS__); \
	} while (0)

#define ohi(l, fmt, ...) \
	do { \
		indent_hdr(l); \
		out_fp(fp_hdr, "source file", fmt, ##__VA_ARGS__); \
	} while (0)


static const struct node_mapping *lookup_map(const char *type)
{
	const struct node_mapping *cur;
	for (cur = mapping; cur; cur = cur->next) {
		if (!strcmp(cur->name, type)) {
			return cur;
		}
	}
	return NULL;
}

static const struct node_type_def_list *lookup_enum(const char *name)
{
	const struct node_type_def_list *l;
	for (l = ast; l; l = l->next) {
		if (l->type == NODE_TYPE_DEF_ENUM && 
				!strcmp(name, l->enum_def.name)) {
			return l;
		}
	}
	return NULL;
}

static const struct node_type_def_list *lookup_struct(const char *name)
{
	const struct node_type_def_list *l;
	for (l = ast; l; l = l->next) {
		if (l->type == NODE_TYPE_DEF_STRUCT && 
				!strcmp(name, l->struct_def.name)) {
			return l;
		}
	}
	return NULL;
}

static const struct node_type_def_list *lookup_union(const char *name)
{
	const struct node_type_def_list *l;
	for (l = ast; l; l = l->next) {
		if (l->type == NODE_TYPE_DEF_UNION &&
				!strcmp(name, l->union_def.name)) {
			return l;
		}
	}
	return NULL;
}

/* if in top level, name is self, seq is not useful */
/* otherwise, name is the upper level, seq is the number of field within upper level */
static void verify_def_struct(const struct node_member_list *list,
		int top, const char *name, int seq);

static void verify_def_union(const struct node_alter_list *list,
		int top, const char *name, int seq);

static void verify_def_struct(const struct node_member_list *list,
		int top, const char *name, int seq)
{
	long i, j, m, n;
	const struct node_mapping *map;

	const char *unnamed = "union %s has a possible candidate struct "
		"(%ld-th field) that contains an unnamed union (%ld-th "
		"field), which is unsupported\n";
	const char *notexist = "member %s.%s has a unknown type %s\n";
	const char *unmatch = "member %s.%s is unmatched to its type %s\n";

	for (i = 0; list; ++i, list = list->next) {
		switch (list->type) {
		case NODE_MEMBER_DEF_PRIM:
			map = lookup_map(list->type_name);
			if (!map) {
				fprintf(stderr, notexist, name,
						list->in_name,
						list->type_name);
				exit(EXIT_FAILURE);
			}
			m = len_string_list(map->mapped_types);
			n = len_string_list(list->mapped);
			if (m != n) {
				fprintf(stderr, unmatch, name,
						list->in_name,
						list->type_name);
				exit(EXIT_FAILURE);
			}
			break;
		case NODE_MEMBER_DEF_ENUM:
			if (!lookup_enum(list->type_name)) {
				fprintf(stderr, notexist, name,
						list->in_name,
						list->type_name);
				exit(EXIT_FAILURE);
			}
			break;
		case NODE_MEMBER_DEF_STRUCT:
			if (!lookup_struct(list->type_name)) {
				fprintf(stderr, notexist, name,
						list->in_name,
						list->type_name);
				exit(EXIT_FAILURE);
			}
			break;
		case NODE_MEMBER_DEF_UNION:
			if (!lookup_union(list->type_name)) {
				fprintf(stderr, notexist, name,
						list->in_name,
						list->type_name);
				exit(EXIT_FAILURE);
			}
			break;
		case NODE_MEMBER_DEF_UNNAMED_UNION:
			if (!top) {
				fprintf(stderr, unnamed, name,
						seq, i);
				exit(EXIT_FAILURE);
			} else {
				verify_def_union(list->alters,
						0, name, i);
			}
			break;
		}
	}
}

static void verify_def_union(const struct node_alter_list *list,
		int top, const char *name, int seq)
{
	long i, j, m, n;
	const struct node_mapping *map;

	const char *unnamed = "struct %s has an unnamed union (%ld-th "
		"field) that contains an unnamed struct (%ld-th field), "
		"which is unsupported\n";
	const char *multimap = "alternative %s.%s is mapped to "
		"multiple values, which is not supported in union\n";
	const char *vararr = "alternative %s.%s is vararray, "
		"which is not supported in union\n";
	const char *notexist = "member %s.%s has a unknown type %s\n";
	const char *unmatch = "member %s.%s is unmatched to its type %s\n";

	for (i = 0; list; ++i, list = list->next) {
		switch (list->type) {
		case NODE_ALTER_DEF_UNNAMED_STRUCT:
			if (!top) {
				fprintf(stderr, unnamed, seq, i);
				exit(EXIT_FAILURE);
			} else {
				verify_def_struct(list->members,
						0, name, i);
			}
			break;
		case NODE_ALTER_DEF_PRIM:
			map = lookup_map(list->type_name);
			if (!map) {
				fprintf(stderr, notexist, name,
						list->in_name,
						list->type_name);
				exit(EXIT_FAILURE);
			}
			m = len_string_list(map->mapped_types);
			n = len_string_list(list->mapped);
			if (m != n) {
				fprintf(stderr, unmatch, name,
						list->in_name,
						list->type_name);
				exit(EXIT_FAILURE);
			}
			if (m > 1) {
				fprintf(stderr, multimap, name,
						list->in_name);
				exit(EXIT_FAILURE);
			} /* pass through */
			break;
		case NODE_ALTER_DEF_ENUM:
			if (!lookup_enum(list->type_name)) {
				fprintf(stderr, notexist, name,
						list->in_name,
						list->type_name);
				exit(EXIT_FAILURE);
			}
			break;
		case NODE_ALTER_DEF_STRUCT:
			if (!lookup_struct(list->type_name)) {
				fprintf(stderr, notexist, name,
						list->in_name,
						list->type_name);
				exit(EXIT_FAILURE);
			}
			break;
		}
		if (list->type != NODE_ALTER_DEF_UNNAMED_STRUCT) {
			if (list->vec.type == NODE_TYPE_VAR_ARR) {
				fprintf(stderr, vararr, name,
						list->in_name);
				exit(EXIT_FAILURE);
			}
		}
	}
}

static void verify_def_list(const struct node_type_def_list *list)
{
	for (; list; list = list->next) {
		switch (list->type) {
		case NODE_TYPE_DEF_ENUM:
			break;
		case NODE_TYPE_DEF_STRUCT:
			verify_def_struct(list->struct_def.members, 1,
					list->struct_def.name, 0);
			break;
		case NODE_TYPE_DEF_UNION:
			verify_def_union(list->union_def.alters, 1,
					list->union_def.name, 0);
			break;
		}
	}
}

static void decl_enum_list(const struct node_enum_list *el, int l);
static void decl_member_list(const struct node_member_list *ml, int l);
static void decl_alter_list(const struct node_alter_list *al, int l);
static void decl_alias_list(const struct node_alter_list *al, int l);



static void decl_enum_list(const struct node_enum_list *el, int l)
{
	for (; el; el = el->next) {
		ohi(l, "%s,\n", el->name);
	}
}

static void decl_elem(const char *name, const struct node_vec_def *vec)
{
	switch (vec->type) {
	case NODE_TYPE_SCALE:
		out_hdr("%s;\n", name);
		break;
	case NODE_TYPE_FIX_ARR:
		out_hdr("%s[%ld];\n", name, vec->len_int);
		break;
	case NODE_TYPE_VAR_ARR:
		out_hdr("*%s;\n", name);
		break;
	}
}

static void decl_elem_post(const struct node_vec_def *vec, int l)
{
	switch (vec->type) {
	case NODE_TYPE_VAR_ARR:
		ohi(l, "long %s;\n", vec->len_str);
		break;
	}
}

static void decl_member_list(const struct node_member_list *ml, int l)
{
	const struct string_list *pname, *ptype;
	const struct node_mapping *map;
	const struct node_type_def_list *rel_union;
	for (; ml; ml = ml->next) {
		switch (ml->type) {
		case NODE_MEMBER_DEF_PRIM:
			map = lookup_map(ml->type_name);
			pname = ml->mapped;
			ptype = map->mapped_types;
			for (; pname; pname = pname->next, ptype = ptype->next) {
				ohi(l, "%s ", ptype->str);
				decl_elem(pname->str, &ml->vec);
			}
			decl_elem_post(&ml->vec, l);
			break;
		case NODE_MEMBER_DEF_ENUM:
			pname = ml->mapped;
			ohi(l, "enum %s ", ml->type_name);
			decl_elem(pname->str, &ml->vec);
			decl_elem_post(&ml->vec, l);
			break;
		case NODE_MEMBER_DEF_STRUCT:
			pname = ml->mapped;
			ohi(l, "struct %s ", ml->type_name);
			decl_elem(pname->str, &ml->vec);
			decl_elem_post(&ml->vec, l);
			break;
		case NODE_MEMBER_DEF_UNION:
			rel_union = lookup_union(ml->type_name);
			pname = ml->mapped;
			ohi(l, "union %s ", ml->type_name);
			decl_elem(pname->str, &ml->vec);
			pname = pname->next;
			ohi(l, "enum %s ", rel_union->union_def.enum_name);
			decl_elem(pname->str, &ml->vec);
			decl_elem_post(&ml->vec, l);
			break;
		case NODE_MEMBER_DEF_UNNAMED_UNION:
			ohi(l, "union {\n");
			decl_alter_list(ml->alters, l + 1);
			ohi(l, "};\n");
			ohi(l, "enum {\n");
			decl_alias_list(ml->alters, l + 1);
			ohi(l, "} %s;\n", ml->alt_enum);
			break;
		}
	}
}

static void decl_alter_list(const struct node_alter_list *al, int l)
{
	const struct string_list *pname, *ptype;
	const struct node_mapping *map;

	for (; al; al = al->next) {
		switch (al->type) {
		case NODE_ALTER_DEF_PRIM:
			map = lookup_map(al->type_name);
			pname = al->mapped;
			ptype = map->mapped_types;
			for (; pname; pname = pname->next, ptype = ptype->next) {
				ohi(l,"%s ", ptype->str);
				decl_elem(pname->str, &al->vec);
			}
			break;
		case NODE_ALTER_DEF_ENUM:
			pname = al->mapped;
			ohi(l, "enum %s ", al->type_name);
			decl_elem(pname->str, &al->vec);
			decl_elem_post(&al->vec, l);
			break;
		case NODE_ALTER_DEF_STRUCT:
			pname = al->mapped;
			ohi(l, "struct %s ", al->type_name);
			decl_elem(pname->str, &al->vec);
			decl_elem_post(&al->vec, l);
			break;
		case NODE_ALTER_DEF_UNNAMED_STRUCT:
			ohi(l, "struct {\n");
			decl_member_list(al->members, l + 1);
			ohi(l, "};\n");
			break;
		}
	}
}

static void decl_alias_list(const struct node_alter_list *al, int l)
{
	for (; al; al = al->next) {
		ohi(l, "%s,\n", al->enum_val);
	}
}

static void decl_def_list(const struct node_type_def_list *list)
{
	const char *name, *ename;
	out_hdr("\n");
	for (; list; list = list->next) {
		switch (list->type) {
		case NODE_TYPE_DEF_ENUM:
			name = list->enum_def.name;
			out_hdr("enum %s {\n", name);
			decl_enum_list(list->enum_def.enums, 1);
			out_hdr("};\n");
			out_hdr("\n");
			out_hdr("\n");

			out_src("static int parse__enum_%s(struct pass_to_conv *ctx, "
					"enum %s *value, "
					"const struct node_value *input);\n", 
					name, name);
			out_src("static void dump__enum_%s(put_func func,"
					"struct dump_context *ctx, "
					"const enum %s *value);\n",
					name, name);
			out_src("\n");
			out_src("\n");
			break;
		case NODE_TYPE_DEF_STRUCT:
			name = list->struct_def.name;
			out_hdr("struct %s {\n", name);
			decl_member_list(list->struct_def.members, 1);
			out_hdr("};\n");
			out_hdr("\n");
			out_hdr("\n");

			out_src("static int parse__struct_%s(struct pass_to_conv *ctx, "
					"struct %s *value, "
					"const struct node_value *input);\n",
					name, name);
			out_src("static void free__struct_%s(struct %s *value);\n",
					name, name);
			out_src("static void dump__struct_%s(put_func func, "
					"struct dump_context *ctx, int l, "
					"const struct %s *value);\n", name, name);
			out_src("\n");
			out_src("\n");
			break;
		case NODE_TYPE_DEF_UNION:
			name = list->union_def.name;
			ename = list->union_def.enum_name;
			out_hdr("enum %s {\n", ename);
			decl_alias_list(list->union_def.alters, 1);
			out_hdr("};\n");
			out_hdr("\n");
			out_hdr("union %s {\n", name);
			decl_alter_list(list->union_def.alters, 1);
			out_hdr("};\n");
			out_hdr("\n");
			out_hdr("\n");

			out_src("static int parse__union_%s(struct pass_to_conv *ctx, "
					"union %s *value, enum %s *type, "
					"const struct node_value *input);\n", 
					name, name, ename);
			out_src("static void free__union_%s(union %s *value, enum %s *type);\n",
					name, name, ename);
			out_src("static void dump__union_%s(put_func func, "
					"struct dump_context *ctx, int l, "
					"const union %s *value, const enum %s *type_value);\n",
					name, name, ename);
			out_src("\n");
			out_src("\n");
			break;
		}
	}
}

static void out_str_list(int is_hdr, string prefix, string postfix,
		const struct string_list *list)
{
	for (; list; list = list->next) {
		if (list->next) {
			if (is_hdr) {
				out_hdr("%s%s%s, ", prefix, list->str, postfix);
			} else {
				out_src("%s%s%s, ", prefix, list->str, postfix);
			}
		} else {
			if (is_hdr) {
				out_hdr("%s%s%s", prefix, list->str, postfix);
			} else {
				out_src("%s%s%s", prefix, list->str, postfix);
			}
		}
	}
}

struct type_decl {
	enum {
		TYPE_DECL_PRIM,
		TYPE_DECL_ENUM,
		TYPE_DECL_STRUCT,
		TYPE_DECL_UNION,
	} type;
	string type_name;
};

static void helper_free_scale(const struct type_decl *decl,
		const struct string_list *vars, int l)
{
	string free_func;

	switch (decl->type) {
	case TYPE_DECL_PRIM:
		free_func = lookup_map(decl->type_name)->free_func;
		osi(l, "%s(", free_func);
		out_str_list(0, "&value->", "", vars);
		out_src(");\n");
		break;
	case TYPE_DECL_ENUM:
		break;
	case TYPE_DECL_STRUCT:
		osi(l, "free__struct_%s(&value->%s);\n",
				decl->type_name, vars->str);
		break;
	case TYPE_DECL_UNION:
		osi(l, "free__union_%s(", decl->type_name);
		out_str_list(0, "&value->", "", vars);
		out_src(");\n");
		break;
	}
}

static void helper_free_array(const struct node_vec_def *vec,
		const struct type_decl *decl,
		const struct string_list *vars, int l)
{
	string free_func;
	const struct string_list *var;

	if (vec->type == NODE_TYPE_FIX_ARR) {
		osi(l, "for (i = 0; i < %ld; ++i) {\n",
				vec->len_int);
	} else {
		osi(l, "for (i = 0; i < value->%s; ++i) {\n",
				vec->len_str);
	}

	switch (decl->type) {
	case TYPE_DECL_PRIM:
		free_func = lookup_map(decl->type_name)->free_func;
		osi(l + 1, "%s(", free_func);
		out_str_list(0, "&value->", "[i]", vars);
		out_src(");\n");
		break;
	case TYPE_DECL_ENUM:
		break;
	case TYPE_DECL_STRUCT:
		osi(l + 1, "free__struct_%s(&value->%s[i]);\n",
				decl->type_name, vars->str);
		break;
	case TYPE_DECL_UNION:
		osi(l + 1, "free__union_%s(", decl->type_name);
		out_str_list(0, "&value->", "[i]", vars);
		out_src(");\n");
		break;
	}

	osi(l, "}\n"); /* for */

	if (vec->type == NODE_TYPE_VAR_ARR) {
		for (var = vars; var; var = var->next) {
			osi(l, "free(value->%s);\n", var->str);
		}
	}
}

static void helper_free(const struct node_vec_def *vec,
		const struct type_decl *decl,
		const struct string_list *vars, int l)
{
	switch (vec->type) {
	case NODE_TYPE_SCALE:
		helper_free_scale(decl, vars, l);
		break;
	default:
		helper_free_array(vec, decl, vars, l);
		break;
	}
}

struct parse_opts {
	enum {
		PARSE_STRUCT,
		PARSE_UNION,
	} mode;
	union {
		struct {
			long idx;
			string opt_var;
			string opt_val;
		} s;
		struct {
			long idx;
			string alt_val;
		} u;
	};
};

static void helper_parse_scale(const struct type_decl *decl,
		string name, const struct string_list *vars,
		const struct parse_opts *opts, int l)
{
	string parse_func;

	osi(l, "if (!strcmp(\"%s\", memb->name)) {\n", name);
	if (opts->mode == PARSE_STRUCT) {
		osi(l + 1, "if (inited[%ld]) {\n", opts->s.idx);
		osi(l + 2, "ctx->node = memb->value;\n");
		osi(l + 2, "ctx->msg = \"member is already defined.\";\n");
		osi(l + 2, "ret = -EINVAL;\n");
		osi(l + 2, "goto error_all;\n");
		osi(l + 1, "}\n"); /* if inited */
	} else {
		osi(l + 1, "if (inited) {\n");
		osi(l + 2, "if (*type_value != %s) {\n", opts->u.alt_val);
		osi(l + 3, "ctx->node = memb->value;\n");
		osi(l + 3, "ctx->msg = \"union is inited with another type.\";\n");
		osi(l + 3, "ret = -EINVAL;\n");
		osi(l + 3, "goto error_all;\n");
		osi(l + 2, "}\n"); /* if */
		osi(l + 2, "if (inited_%s[%ld]) {\n", opts->u.alt_val, opts->u.idx);
		osi(l + 3, "ctx->node = memb->value;\n");
		osi(l + 3, "ctx->msg = \"field is already defined.\";\n");
		osi(l + 3, "ret = -EINVAL;\n");
		osi(l + 3, "goto error_all;\n");
		osi(l + 2, "}\n"); /* if */
		osi(l + 1, "}\n"); /* if */
	}
	switch (decl->type) {
	case TYPE_DECL_PRIM:
		parse_func = lookup_map(decl->type_name)->parse_func;
		osi(l + 1, "ret = %s(ctx", parse_func);
		if (vars) {
			out_src(", ");
		}
		out_str_list(0, "&value->", "", vars);
		out_src(", memb->value);\n");
		break;
	case TYPE_DECL_ENUM:
		osi(l + 1, "ret = parse__enum_%s(ctx, &value->%s, memb->value);\n",
				decl->type_name, vars->str);
		break;
	case TYPE_DECL_STRUCT:
		osi(l + 1, "ret = parse__struct_%s(ctx, &value->%s, memb->value);\n",
				decl->type_name, vars->str);
		break;
	case TYPE_DECL_UNION:
		osi(l + 1, "ret = parse__union_%s(ctx, ", decl->type_name);
		out_str_list(0, "&value->", "", vars);
		out_src(", memb->value);\n");
		break;
		break;
	}
	osi(l + 1, "if (ret) {\n");
	osi(l + 2, "goto error_all;\n");
	osi(l + 1, "}\n"); /* if ret */
	if (opts->mode == PARSE_STRUCT) {
		osi(l + 1, "inited[%ld] = 1;\n", opts->s.idx);
		if (opts->s.opt_var) {
			osi(l + 1, "value->%s = %s;\n", opts->s.opt_var,
					opts->s.opt_val);
		}
	} else {
		osi(l + 1, "inited = 1;\n");
		osi(l + 1, "*type_value = %s;\n", opts->u.alt_val);
		osi(l + 1, "inited_%s[%ld] = 1;\n", opts->u.alt_val,
				opts->u.idx);
	}
	osi(l + 1, "continue;\n");
	osi(l, "}\n"); /* if matches */
}

static void helper_parse_array(const struct node_vec_def *vec,
		const struct type_decl *decl,
		string name, const struct string_list *vars,
		const struct parse_opts *opts, int l)
{
	string parse_func, free_func;
	const struct string_list *cv;

	osi(l, "if (!strcmp(\"%s\", memb->name)) {\n", name);
	if (opts->mode == PARSE_STRUCT) {
		osi(l + 1, "if (inited[%ld]) {\n", opts->s.idx);
		osi(l + 2, "ctx->node = memb->value;\n");
		osi(l + 2, "ctx->msg = \"member is already defined.\";\n");
		osi(l + 2, "ret = -EINVAL;\n");
		osi(l + 2, "goto error_all;\n");
		osi(l + 1, "}\n"); /* if inited */
	} else {
		osi(l + 1, "if (inited) {\n");
		osi(l + 2, "if (*type_value != %s) {\n", opts->u.alt_val);
		osi(l + 3, "ctx->node = memb->value;\n");
		osi(l + 3, "ctx->msg = \"union is inited with another type.\";\n");
		osi(l + 3, "ret = -EINVAL;\n");
		osi(l + 3, "goto error_all;\n");
		osi(l + 2, "}\n"); /* if */
		osi(l + 2, "if (inited_%s[%ld]) {\n", opts->u.alt_val, opts->u.idx);
		osi(l + 3, "ctx->node = memb->value;\n");
		osi(l + 3, "ctx->msg = \"field is already defined.\";\n");
		osi(l + 3, "ret = -EINVAL;\n");
		osi(l + 3, "goto error_all;\n");
		osi(l + 2, "}\n"); /* if */
		osi(l + 1, "}\n"); /* if */
	}
	
	osi(l + 1, "if (memb->value->type != VAL_ELEMS) {\n");
	osi(l + 2, "ctx->node = memb->value;\n");
	osi(l + 2, "ctx->msg = \"invalid type, expecting an array.\";\n");
	osi(l + 2, "ret = -EINVAL;\n");
	osi(l + 2, "goto error_all;\n");
	osi(l + 1, "}\n"); /* if type */

	osi(l + 1, "i = 0;\n");
	if (vec->type == NODE_TYPE_FIX_ARR) {
		osi(l + 1, "if (len_node_elems(memb->value->elems) != %ld) {\n",
				vec->len_int);
		osi(l + 2, "ctx->node = memb->value;\n");
		osi(l + 2, "ctx->msg = \"wrong number of elements.\";\n");
		osi(l + 2, "ret = -EINVAL;\n");
		osi(l + 2, "goto error_all;\n");
		osi(l + 1, "}\n"); /* if len... */
		osi(l + 1, "for (elem = memb->value->elems; i < %ld; "
				"++i, elem = elem->next) {\n", vec->len_int);
	} else {
		osi(l + 1, "len = len_node_elems(memb->value->elems);\n");
		osi(l + 1, "value->%s = len;\n", vec->len_str);
		for (cv = vars; cv; cv = cv->next) {
			osi(l + 1, "value->%s = NULL;\n", cv->str);
		}
		for (cv = vars; cv; cv = cv->next) {
			osi(l + 1, "value->%s = calloc(len, sizeof(*value->%s));\n",
					cv->str, cv->str);
			osi(l + 1, "if (len && !value->%s) {\n", cv->str);
			osi(l + 2, "ctx->node = memb->value;\n");
			osi(l + 2, "ctx->msg = \"memory insufficient.\";\n");
			osi(l + 2, "goto error_%s;\n", name);
			osi(l + 1, "}\n", cv->str);
		}
		osi(l + 1, "for (elem = memb->value->elems; i < len; "
			"++i, elem = elem->next) {\n");
	}

	switch (decl->type) {
	case TYPE_DECL_PRIM:
		parse_func = lookup_map(decl->type_name)->parse_func;
		osi(l + 2, "ret = %s(ctx", parse_func);
		if (vars) {
			out_src(", ");
		}
		out_str_list(0, "&value->", "[i]", vars);
		out_src(", elem->value);\n");
		break;
	case TYPE_DECL_ENUM:
		osi(l + 2, "ret = parse__enum_%s(ctx, &value->%s[i], elem->value);\n",
				decl->type_name, vars->str);
		break;
	case TYPE_DECL_STRUCT:
		osi(l + 2, "ret = parse__struct_%s(ctx, &value->%s[i], elem->value);\n",
				decl->type_name, vars->str);
		break;
	case TYPE_DECL_UNION:
		osi(l + 2, "ret = parse__union_%s(ctx, ", decl->type_name);
		out_str_list(0, "&value->", "[i]", vars);
		out_src(", elem->value);\n");
		break;
		break;
	}
	osi(l + 2, "if (ret) {\n");
	osi(l + 3, "goto error_%s;\n", name);
	osi(l + 2, "}\n"); /* if ret */
	osi(l + 1, "}\n"); /* for */
	if (opts->mode == PARSE_STRUCT) {
		osi(l + 1, "inited[%ld] = 1;\n", opts->s.idx);
		if (opts->s.opt_var) {
			osi(l + 1, "value->%s = %s;\n", opts->s.opt_var,
					opts->s.opt_val);
		}
	} else {
		osi(l + 1, "inited = 1;\n");
		osi(l + 1, "*type_value = %s;\n", opts->u.alt_val);
		osi(l + 1, "inited_%s[%ld] = 1;\n", opts->u.alt_val,
				opts->u.idx);
	}
	osi(l + 1, "continue;\n");
	osi(0, "error_%s:\n", name);
	osi(l + 1, "for (; i > 0; --i) {\n");
	switch (decl->type) {
	case TYPE_DECL_PRIM:
		free_func = lookup_map(decl->type_name)->free_func;
		osi(l + 2, "%s(", free_func);
		out_str_list(0, "&value->", "[i]", vars);
		out_src(");\n");
		break;
	case TYPE_DECL_ENUM:
		break;
	case TYPE_DECL_STRUCT:
		osi(l + 2, "free__struct_%s(&value->%s[i]);\n",
				decl->type_name, vars->str);
		break;
	case TYPE_DECL_UNION:
		osi(l + 2, "free__union_%s(", decl->type_name);
		out_str_list(0, "&value->", "[i]", vars);
		out_src(");\n");
		break;
	}
	osi(l + 1, "}\n"); /* for */
	if (vec->type == NODE_TYPE_VAR_ARR) {
		for (cv = vars; cv; cv = cv->next) {
			osi(l + 1, "free(value->%s);\n", cv->str);
		}
	}
	osi(l + 1, "goto error_all;\n");
	osi(l, "}\n"); /* if matches */
}

static void helper_parse(const struct node_vec_def *vec,
		const struct type_decl *decl,
		string name, const struct string_list *vars,
		const struct parse_opts *opts, int l)
{
	switch (vec->type) {
	case NODE_TYPE_SCALE:
		helper_parse_scale(decl, name, vars, opts, l);
		break;
	default:
		helper_parse_array(vec, decl, name, vars, opts, l);
		break;
	}
}



static void parse_enum(string name, const struct node_enum_list *list)
{
	const struct node_enum_list *cur;
	osi(0, "static int parse__enum_%s(struct pass_to_conv *ctx, enum %s *value, "
		"const struct node_value *input)\n", name, name);
	osi(0, "{\n");
	osi(1, "if (input->type != VAL_SCALE_IDEN) {\n");
	osi(2, "ctx->node = input;\n");
	osi(2, "ctx->msg = \"invalid type, expecting enum.\";\n");
	osi(2, "return -EINVAL;\n");
	osi(1, "}\n");
	for (cur = list; cur; cur = cur->next) {
		osi(1, "if (!strcmp(input->enum_str, \"%s\")) {\n", cur->name);
		osi(2, "*value = %s;\n", cur->name);
		osi(2, "return 0;\n");
		osi(1, "}\n");
		if (cur->alias) {
			osi(1, "if (!strcmp(input->enum_str, \"%s\")) {\n", cur->alias);
			osi(2, "*value = %s;\n", cur->name);
			osi(2, "return 0;\n");
			osi(1, "}\n");
		}
	}
	osi(1, "ctx->node = input;\n");
	osi(1, "ctx->msg = \"unknown enum value.\";\n");
	osi(1, "ctx-EINVAL;\n");
	osi(0, "}\n"); /* func body */
	osi(0, "\n");
}

static void parse_struct(string name, const struct node_member_list *list)
{
	const struct node_member_list *memb;
	const struct node_alter_list *alt;
	long cnt, idx;
	struct type_decl decl;
	struct parse_opts opts;

	cnt = len_member_list(list);
	opts.mode = PARSE_STRUCT;

	osi(0, "static int parse__struct_%s(struct pass_to_conv *ctx, struct %s *value, "
			"const struct node_value *input)\n", name, name);
	osi(0, "{\n");
	osi(1, "int inited[%ld] = {};\n", cnt);
	osi(1, "int ret;\n");
	osi(1, "long i, len;\n");
	osi(1, "struct node_members *memb;\n");
	osi(1, "struct node_elems *elem;\n");
	osi(1, "if (input->type != VAL_MEMBERS) {\n");
	osi(2, "ctx->node = input;\n");
	osi(2, "ctx->msg = \"invalid type, expecting list of members.\";\n");
	osi(2, "return -EINVAL;\n");
	osi(1, "}\n"); /* if */
	osi(1, "for (memb = input->members; memb; memb = memb->next) {\n");
	for (memb = list, idx = 0; memb; memb = memb->next, ++idx) {
		switch (memb->type) {
		case NODE_MEMBER_DEF_PRIM:
			opts.s.idx = idx;
			opts.s.opt_var = NULL;
			opts.s.opt_val = NULL;
			decl.type = TYPE_DECL_PRIM;
			decl.type_name = memb->type_name;
			helper_parse(&memb->vec, &decl, memb->in_name,
					memb->mapped, &opts, 2);
			break;
		case NODE_MEMBER_DEF_ENUM:
			opts.s.idx = idx;
			opts.s.opt_var = NULL;
			opts.s.opt_val = NULL;
			decl.type = TYPE_DECL_ENUM;
			decl.type_name = memb->type_name;
			helper_parse(&memb->vec, &decl, memb->in_name,
					memb->mapped, &opts, 2);
			break;
		case NODE_MEMBER_DEF_STRUCT:
			opts.s.idx = idx;
			opts.s.opt_var = NULL;
			opts.s.opt_val = NULL;
			decl.type = TYPE_DECL_STRUCT;
			decl.type_name = memb->type_name;
			helper_parse(&memb->vec, &decl, memb->in_name,
					memb->mapped, &opts, 2);
			break;
		case NODE_MEMBER_DEF_UNION:
			opts.s.idx = idx;
			opts.s.opt_var = NULL;
			opts.s.opt_val = NULL;
			decl.type = TYPE_DECL_UNION;
			decl.type_name = memb->type_name;
			helper_parse(&memb->vec, &decl, memb->in_name,
					memb->mapped, &opts, 2);
			break;
		case NODE_MEMBER_DEF_UNNAMED_UNION:
			opts.s.idx = idx;
			opts.s.opt_var = memb->alt_enum;
			for (alt = memb->alters; alt; alt = alt->next) {
				switch (alt->type) {
				case NODE_ALTER_DEF_PRIM:
					opts.s.idx = idx;
					opts.s.opt_val = alt->enum_val;
					decl.type = TYPE_DECL_PRIM;
					decl.type_name = alt->type_name;
					helper_parse(&alt->vec, &decl, alt->in_name,
							alt->mapped, &opts, 2);
					break;
				case NODE_ALTER_DEF_ENUM:
					opts.s.idx = idx;
					opts.s.opt_val = alt->enum_val;
					decl.type = TYPE_DECL_ENUM;
					decl.type_name = alt->type_name;
					helper_parse(&alt->vec, &decl, alt->in_name,
							alt->mapped, &opts, 2);
					break;
				case NODE_ALTER_DEF_STRUCT:
					opts.s.idx = idx;
					opts.s.opt_val = alt->enum_val;
					decl.type = TYPE_DECL_STRUCT;
					decl.type_name = alt->type_name;
					helper_parse(&alt->vec, &decl, alt->in_name,
							alt->mapped, &opts, 2);
					break;
				default:
					fprintf(stderr, "error at %d (%s) : impossible err\n",
							__LINE__, __func__);
					exit(EXIT_FAILURE);
				}
			}
		}
	}
	osi(2, "ctx->node = memb->value;\n");
	osi(2, "ctx->msg = \"unknown member.\";\n");
	osi(2, "ret = -EINVAL;\n");
	osi(2, "goto error_all;\n");
	osi(1, "}\n"); /* for */
	osi(1, "for (i = 0; i < %ld; ++i) {\n", cnt);
	osi(2, "if (!inited[i]) {\n");
	osi(3, "ctx->node = input;\n");
	osi(3, "ctx->msg = \"some field is not initialized.\";\n");
	osi(3, "ret = -EINVAL;\n");
	osi(3, "goto error_all;\n");
	osi(2, "}\n"); /* if */
	osi(1, "}\n"); /* for */
	osi(1, "return 0;\n");
	osi(0, "error_all:\n");
	for (memb = list, idx = 0; memb; memb = memb->next, ++idx) {
		osi(1, "if (inited[%ld]) {\n", idx);
		switch (memb->type) {
		case NODE_MEMBER_DEF_PRIM:
			decl.type = TYPE_DECL_PRIM;
			decl.type_name = memb->type_name;
			helper_free(&memb->vec, &decl, memb->mapped, 2);
			break;
		case NODE_MEMBER_DEF_ENUM:
			decl.type = TYPE_DECL_ENUM;
			decl.type_name = memb->type_name;
			helper_free(&memb->vec, &decl, memb->mapped, 2);
			break;
		case NODE_MEMBER_DEF_STRUCT:
			decl.type = TYPE_DECL_STRUCT;
			decl.type_name = memb->type_name;
			helper_free(&memb->vec, &decl, memb->mapped, 2);
			break;
		case NODE_MEMBER_DEF_UNION:
			decl.type = TYPE_DECL_UNION;
			decl.type_name = memb->type_name;
			helper_free(&memb->vec, &decl, memb->mapped, 2);
			break;
		case NODE_MEMBER_DEF_UNNAMED_UNION:
			for (alt = memb->alters; alt; alt = alt->next) {
				osi(2, "if (value->%s == %s) {\n",
						memb->alt_enum, alt->enum_val);
				switch (alt->type) {
				case NODE_ALTER_DEF_PRIM:
					decl.type = TYPE_DECL_PRIM;
					decl.type_name = alt->type_name;
					helper_free(&alt->vec, &decl, 
							alt->mapped, 3);
					break;
				case NODE_ALTER_DEF_ENUM:
					decl.type = TYPE_DECL_ENUM;
					decl.type_name = alt->type_name;
					helper_free(&alt->vec, &decl, 
							alt->mapped, 3);
					break;
				case NODE_ALTER_DEF_STRUCT:
					decl.type = TYPE_DECL_STRUCT;
					decl.type_name = alt->type_name;
					helper_free(&alt->vec, &decl, 
							alt->mapped, 3);
					break;
				}
				osi(2, "}\n"); /* if */
			}
		}
		osi(1, "}\n"); /* if */
	}
	osi(1, "return ret;\n");
	osi(0, "}\n");
	osi(0, "\n");
}

static void parse_union(string name, string enum_name,
		const struct node_alter_list *list)
{
	const struct node_member_list *memb;
	const struct node_alter_list *alt;
	long cnt, idx;
	struct type_decl decl;
	struct parse_opts opts;

	opts.mode = PARSE_UNION;

	osi(0, "static int parse__union_%s(struct pass_to_conv *ctx, union %s *value, "
			"enum %s *type_value, const struct node_value *input)\n",
			name, name, enum_name);
	osi(0, "{\n");
	osi(1, "int ret;\n");
	osi(1, "long i, len;\n");
	osi(1, "struct node_members *memb;\n");
	osi(1, "struct node_elems *elem;\n");
	osi(1, "int inited = 0;\n");
	for (alt = list; alt; alt = alt->next) {
		switch (alt->type) {
		case NODE_ALTER_DEF_UNNAMED_STRUCT:
			cnt = len_member_list(alt->members);
			osi(1, "int inited_%s[%ld] = {};\n", alt->enum_val, cnt);
			break;
		default:
			osi(1, "int inited_%s[1] = {};\n", alt->enum_val);
			break;
		}
	}
	osi(1, "if (input->type != VAL_MEMBERS) {\n");
	osi(2, "ctx->node = input;\n");
	osi(2, "ctx->msg = \"invalid type, expecting list of members.\";\n");
	osi(2, "return -EINVAL;\n");
	osi(1, "}\n"); /* for */
	osi(1, "for (memb = input->members; memb; memb = memb->next) {\n");
	for (alt = list; alt; alt = alt->next) {
		switch (alt->type) {
		case NODE_ALTER_DEF_PRIM:
			opts.u.idx = 0;
			opts.u.alt_val = alt->enum_val;
			decl.type = TYPE_DECL_PRIM;
			decl.type_name = alt->type_name;
			helper_parse(&alt->vec, &decl, alt->in_name,
					alt->mapped, &opts, 2);
			break;
		case NODE_ALTER_DEF_ENUM:
			opts.u.idx = 0;
			opts.u.alt_val = alt->enum_val;
			decl.type = TYPE_DECL_ENUM;
			decl.type_name = alt->type_name;
			helper_parse(&alt->vec, &decl, alt->in_name,
					alt->mapped, &opts, 2);
			break;
		case NODE_ALTER_DEF_STRUCT:
			opts.u.idx = 0;
			opts.u.alt_val = alt->enum_val;
			decl.type = TYPE_DECL_STRUCT;
			decl.type_name = alt->type_name;
			helper_parse(&alt->vec, &decl, alt->in_name,
					alt->mapped, &opts, 2);
			break;
		case NODE_ALTER_DEF_UNNAMED_STRUCT:
			for (memb = alt->members, idx = 0; memb;
					memb = memb->next, ++idx) {
				switch (memb->type) {
				case NODE_MEMBER_DEF_PRIM:
					opts.u.idx = idx;
					opts.u.alt_val = alt->enum_val;
					decl.type = TYPE_DECL_PRIM;
					decl.type_name = memb->type_name;
					helper_parse(&memb->vec, &decl, memb->in_name,
							memb->mapped, &opts, 2);
					break;
				case NODE_MEMBER_DEF_ENUM:
					opts.u.idx = idx;
					opts.u.alt_val = alt->enum_val;
					decl.type = TYPE_DECL_ENUM;
					decl.type_name = memb->type_name;
					helper_parse(&memb->vec, &decl, memb->in_name,
							memb->mapped, &opts, 2);
					break;
				case NODE_MEMBER_DEF_STRUCT:
					opts.u.idx = idx;
					opts.u.alt_val = alt->enum_val;
					decl.type = TYPE_DECL_STRUCT;
					decl.type_name = memb->type_name;
					helper_parse(&memb->vec, &decl, memb->in_name,
							memb->mapped, &opts, 2);
					break;
				case NODE_MEMBER_DEF_UNION:
					opts.u.idx = idx;
					opts.u.alt_val = alt->enum_val;
					decl.type = TYPE_DECL_UNION;
					decl.type_name = memb->type_name;
					helper_parse(&memb->vec, &decl, memb->in_name,
							memb->mapped, &opts, 2);
					break;
				}
			}
		}
	}
	osi(2, "ctx->node = memb->value;\n");
	osi(2, "ctx->msg = \"unknown member.\";\n");
	osi(2, "ret = -EINVAL;\n");
	osi(2, "goto error_all;");
	osi(1, "}\n"); /* for */
	osi(1, "if (!inited) {\n");
	osi(2, "ctx->node = memb->value;\n");
	osi(2, "ctx->msg = \"union is not initialized.\";\n");
	osi(2, "return -EINVAL;");
	osi(1, "}\n"); /* if */
	for (alt = list; alt; alt = alt->next) {
		if (alt->type == NODE_ALTER_DEF_UNNAMED_STRUCT) {
			osi(1, "if (*type_value == %s) {\n", alt->enum_val);
			osi(2, "for (i = 0; i < %ld; ++i) {\n",
					len_member_list(alt->members));
			osi(3, "if (!inited_%s[i]) {\n", alt->enum_val);
			osi(4, "ctx->node = input;\n");
			osi(4, "ctx->msg = \"a union member is not initialized.\";\n");
			osi(4, "return -EINVAL;\n");
			osi(4, "goto error_all;\n");
			osi(3, "}\n"); /* if */
			osi(2, "}\n"); /* for */
			osi(1, "}\n"); /* if */
		}
	}
	osi(1, "return 0;\n");
	osi(1, "error_all:\n");
	for (alt = list; alt; alt = alt->next) {
		if (alt->type != NODE_ALTER_DEF_UNNAMED_STRUCT) {
			/* other types won't have partial initialized status */
			continue;
		}
		osi(1, "if (*type_value == %s) {\n", alt->enum_val);
		for (memb = alt->members, idx = 0; memb; memb = memb->next, ++idx) {
			osi(2, "if (inited_%s[%ld]) {\n", alt->enum_val, idx);
			switch (memb->type) {
			case NODE_MEMBER_DEF_PRIM:
				decl.type = TYPE_DECL_PRIM;
				decl.type_name = memb->type_name;
				helper_free(&memb->vec, &decl, memb->mapped, 2);
				break;
			case NODE_MEMBER_DEF_ENUM:
				decl.type = TYPE_DECL_ENUM;
				decl.type_name = memb->type_name;
				helper_free(&memb->vec, &decl, memb->mapped, 2);
				break;
			case NODE_MEMBER_DEF_STRUCT:
				decl.type = TYPE_DECL_STRUCT;
				decl.type_name = memb->type_name;
				helper_free(&memb->vec, &decl, memb->mapped, 2);
				break;
			case NODE_MEMBER_DEF_UNION:
				decl.type = TYPE_DECL_UNION;
				decl.type_name = memb->type_name;
				helper_free(&memb->vec, &decl, memb->mapped, 2);
				break;
			}
			osi(2, "}\n"); /* if */
		}
		osi(2, "}\n"); /* if */
	}
	osi(1, "return ret;\n");
	osi(0, "}\n");
	osi(0, "\n");
}

static void parse_type_def_list(const struct node_type_def_list *list)
{
	for (; list; list = list->next) {
		switch (list->type) {
		case NODE_TYPE_DEF_ENUM:
			parse_enum(list->enum_def.name, 
					list->enum_def.enums);
			break;
		case NODE_TYPE_DEF_STRUCT:
			parse_struct(list->struct_def.name,
					list->struct_def.members);
			break;
		case NODE_TYPE_DEF_UNION:
			parse_union(list->union_def.name,
					list->union_def.enum_name,
					list->union_def.alters);
			break;
		}
	}
}


static void free_struct(string name, const struct node_member_list *list)
{
	const struct node_member_list *memb;
	const struct node_alter_list *alt;
	struct type_decl decl;

	osi(0, "static void free__struct_%s(struct %s *value)\n", name, name);
	osi(0, "{\n");
	osi(1, "long i;\n");
	for (memb = list; memb; memb = memb->next) {
		switch (memb->type) {
		case NODE_MEMBER_DEF_PRIM:
			decl.type = TYPE_DECL_PRIM;
			decl.type_name = memb->type_name;
			helper_free(&memb->vec, &decl, memb->mapped, 1);
			break;
		case NODE_MEMBER_DEF_ENUM:
			decl.type = TYPE_DECL_ENUM;
			decl.type_name = memb->type_name;
			helper_free(&memb->vec, &decl, memb->mapped, 1);
			break;
		case NODE_MEMBER_DEF_STRUCT:
			decl.type = TYPE_DECL_STRUCT;
			decl.type_name = memb->type_name;
			helper_free(&memb->vec, &decl, memb->mapped, 1);
			break;
		case NODE_MEMBER_DEF_UNION:
			decl.type = TYPE_DECL_UNION;
			decl.type_name = memb->type_name;
			helper_free(&memb->vec, &decl, memb->mapped, 1);
			break;
		case NODE_MEMBER_DEF_UNNAMED_UNION:
			for (alt = memb->alters; alt; alt = alt->next) {
				osi(1, "if (value->%s == %s) {\n",
						memb->alt_enum, alt->enum_val);
				switch (alt->type) {
				case NODE_ALTER_DEF_PRIM:
					decl.type = TYPE_DECL_PRIM;
					decl.type_name = alt->type_name;
					helper_free(&alt->vec, &decl,
							alt->mapped, 2);
					break;
				case NODE_ALTER_DEF_ENUM:
					decl.type = TYPE_DECL_ENUM;
					decl.type_name = alt->type_name;
					helper_free(&alt->vec, &decl,
							alt->mapped, 2);
					break;
				case NODE_ALTER_DEF_STRUCT:
					decl.type = TYPE_DECL_STRUCT;
					decl.type_name = alt->type_name;
					helper_free(&alt->vec, &decl,
							alt->mapped, 2);
					break;
				}
				osi(1, "}\n");
			}
		}
	}
	osi(0, "}\n");
	osi(0, "\n");
}

static void free_union(string name, string enum_name,
		const struct node_alter_list *list)
{
	const struct node_member_list *memb;
	const struct node_alter_list *alt;
	struct type_decl decl;

	osi(0, "static void free__union_%s(union %s *value, enum %s *type_value)\n",
			name, name, enum_name);
	osi(0, "{\n");
	osi(1, "long i;\n");
	for (alt = list; alt; alt = alt->next) {
		osi(1, "if (*type_value == %s) {\n", alt->enum_val);
		switch (alt->type) {
		case NODE_ALTER_DEF_PRIM:
			decl.type = TYPE_DECL_PRIM;
			decl.type_name = alt->type_name;
			helper_free(&alt->vec, &decl, alt->mapped, 2);
			break;
		case NODE_ALTER_DEF_ENUM:
			decl.type = TYPE_DECL_ENUM;
			decl.type_name = alt->type_name;
			helper_free(&alt->vec, &decl, alt->mapped, 2);
			break;
		case NODE_ALTER_DEF_STRUCT:
			decl.type = TYPE_DECL_STRUCT;
			decl.type_name = alt->type_name;
			helper_free(&alt->vec, &decl, alt->mapped, 2);
			break;
		case NODE_ALTER_DEF_UNNAMED_STRUCT:
			for (memb = alt->members; memb; memb = memb->next) {
				switch (memb->type) {
				case NODE_MEMBER_DEF_PRIM:
					decl.type = TYPE_DECL_PRIM;
					decl.type_name = memb->type_name;
					helper_free(&memb->vec, &decl,
							memb->mapped, 2);
					break;
				case NODE_MEMBER_DEF_ENUM:
					decl.type = TYPE_DECL_ENUM;
					decl.type_name = memb->type_name;
					helper_free(&memb->vec, &decl,
							memb->mapped, 2);
					break;
				case NODE_MEMBER_DEF_STRUCT:
					decl.type = TYPE_DECL_STRUCT;
					decl.type_name = memb->type_name;
					helper_free(&memb->vec, &decl,
							memb->mapped, 2);
					break;
				case NODE_MEMBER_DEF_UNION:
					decl.type = TYPE_DECL_UNION;
					decl.type_name = memb->type_name;
					helper_free(&memb->vec, &decl,
							memb->mapped, 2);
					break;
				}
			}
		}
		osi(1, "}\n"); /* if */
	}
	osi(0, "}\n"); /* func-body */
	osi(0, "\n");
}

static void free_type_def_list(const struct node_type_def_list *list)
{
	for (; list; list = list->next) {
		switch (list->type) {
		case NODE_TYPE_DEF_STRUCT:
			free_struct(list->struct_def.name,
					list->struct_def.members);
			break;
		case NODE_TYPE_DEF_UNION:
			free_union(list->union_def.name,
					list->union_def.enum_name,
					list->union_def.alters);
			break;
		}
	}
}



static void dump_enum(const char *name, const struct node_enum_list *list)
{
	osi(0, "static void dump__enum_%s(put_func func, struct dump_context *ctx, "
			"const enum %s *value)\n", name, name);
	osi(0, "{\n");
	for (; list; list = list->next) {
		osi(1, "if (*value == %s) {\n", list->name);
		osi(2, "func(ctx, \"%s\");\n", list->name);
		osi(1, "}\n");
	}
	osi(0, "}\n");
	osi(0, "\n");
}

static void helper_dump_scale(const struct type_decl *decl,
		string name, const struct string_list *vars, int l)
{
	string dump_func;

	osi(l, "indent(func, ctx, l + 1);\n");
	osi(l, "func(ctx, \".%s = \");\n", name);
	switch (decl->type) {
	case TYPE_DECL_PRIM:
		dump_func = lookup_map(decl->type_name)->dump_func;
		osi(l, "%s(func, ctx", dump_func);
		break;
	case TYPE_DECL_ENUM:
		osi(l, "dump__enum_%s(func, ctx", decl->type_name);
		break;
	case TYPE_DECL_STRUCT:
		osi(l, "dump__struct_%s(func, ctx, l + 1", decl->type_name);
		break;
	case TYPE_DECL_UNION:
		osi(l, "dump__union_%s(func, ctx, l + 1", decl->type_name);
		break;
	}
	if (vars) {
		out_src(", ");
		out_str_list(0, "&value->", "", vars);
	}
	out_src(");\n");
}

static void helper_dump_array(const struct node_vec_def *vec,
		const struct type_decl *decl,
		string name, const struct string_list *vars, int l)
{
	string dump_func;
	osi(l, "indent(func, ctx, l + 1);\n");
	osi(l, "func(ctx, \".%s = [\\n\");\n", name);
	if (vec->type == NODE_TYPE_FIX_ARR) {
		osi(l, "for (i = 0; i < %ld; ++i) {\n", vec->len_int);
	} else {
		osi(l, "for (i = 0; i < value->%s; ++i) {\n", vec->len_str);
	}
	osi(l + 1, "indent(func, ctx, l + 2);\n");
	switch (decl->type) {
	case TYPE_DECL_PRIM:
		dump_func = lookup_map(decl->type_name)->dump_func;
		osi(l + 1, "%s(func, ctx", dump_func);
		break;
	case TYPE_DECL_ENUM:
		osi(l + 1, "dump__enum_%s(func, ctx", decl->type_name);
		break;
	case TYPE_DECL_STRUCT:
		osi(l + 1, "dump__struct_%s(func, ctx, l + 2", decl->type_name);
		break;
	case TYPE_DECL_UNION:
		osi(l + 1, "dump__union_%s(func, ctx, l + 2", decl->type_name);
		break;
	}
	if (vars) {
		out_src(", ");
		out_str_list(0, "&value->", "[i]", vars);
	}
	out_src(");\n");
	osi(l + 1, "func(ctx, \",\\n\");\n");
	osi(l, "}\n"); /* for */
	osi(l, "indent(func, ctx, l + 1);\n");
	osi(l, "func(ctx, \"]\");\n");
}

static void helper_dump(const struct node_vec_def *vec,
		const struct type_decl *decl,
		string name, const struct string_list *vars, int l)
{
	switch (vec->type) {
	case NODE_TYPE_SCALE:
		helper_dump_scale(decl, name, vars, l);
		break;
	default:
		helper_dump_array(vec, decl, name, vars, l);
		break;
	}
}

static void dump_struct(string name, const struct node_member_list *list)
{
	const struct node_member_list *memb;
	const struct node_alter_list *alt;
	struct type_decl decl;

	osi(0, "static void dump__struct_%s(put_func func, struct dump_context *ctx, "
			"int l, const struct %s *value)\n", name, name);
	osi(0, "{\n");
	osi(1, "long i;\n");
	osi(1, "func(ctx, \"{\\n\");\n");
	for (memb = list; memb; memb = memb->next) {
		switch (memb->type) {
		case NODE_MEMBER_DEF_PRIM:
			decl.type = TYPE_DECL_PRIM;
			decl.type_name = memb->type_name;
			helper_dump(&memb->vec, &decl, memb->in_name,
					memb->mapped, 1);
			break;
		case NODE_MEMBER_DEF_ENUM:
			decl.type = TYPE_DECL_ENUM;
			decl.type_name = memb->type_name;
			helper_dump(&memb->vec, &decl, memb->in_name,
					memb->mapped, 1);
			break;
		case NODE_MEMBER_DEF_STRUCT:
			decl.type = TYPE_DECL_STRUCT;
			decl.type_name = memb->type_name;
			helper_dump(&memb->vec, &decl, memb->in_name,
					memb->mapped, 1);
			break;
		case NODE_MEMBER_DEF_UNION:
			decl.type = TYPE_DECL_UNION;
			decl.type_name = memb->type_name;
			helper_dump(&memb->vec, &decl, memb->in_name,
					memb->mapped, 1);
			break;
		case NODE_MEMBER_DEF_UNNAMED_UNION:
			for (alt = memb->alters; alt; alt = alt->next) {
				osi(1, "if (value->%s == %s) {\n",
						memb->alt_enum, alt->enum_val);
				switch (alt->type) {
				case NODE_ALTER_DEF_PRIM:
					decl.type = TYPE_DECL_PRIM;
					decl.type_name = alt->type_name;
					helper_dump(&alt->vec, &decl, alt->in_name,
							alt->mapped, 2);
					break;
				case NODE_ALTER_DEF_ENUM:
					decl.type = TYPE_DECL_ENUM;
					decl.type_name = alt->type_name;
					helper_dump(&alt->vec, &decl, alt->in_name,
							alt->mapped, 2);
					break;
				case NODE_ALTER_DEF_STRUCT:
					decl.type = TYPE_DECL_STRUCT;
					decl.type_name = alt->type_name;
					helper_dump(&alt->vec, &decl, alt->in_name,
							alt->mapped, 2);
					break;
				}
				osi(1, "}\n"); /* if */
			}
		}
		osi(1, "func(ctx, \",\\n\");\n");
	}
	osi(1, "indent(func, ctx, l);\n");
	osi(1, "func(ctx, \"}\");\n");
	osi(0, "}\n"); /* func body */
	osi(0, "\n");
}

static void dump_union(string name, string enum_name,
		const struct node_alter_list *list)
{
	const struct node_member_list *memb;
	const struct node_alter_list *alt;
	struct type_decl decl;

	osi(0, "static void dump__union_%s(put_func func, struct dump_context *ctx, "
			"int l, const union %s *value, "
			"const enum %s *type_value)\n", name, name, enum_name);
	osi(0, "{\n");
	osi(1, "long i;\n");
	osi(1, "func(ctx, \"{\\n\");\n");
	for (alt = list; alt; alt = alt->next) {
		osi(1, "if (*type_value == %s) {", alt->enum_val);
		switch (alt->type) {
		case NODE_ALTER_DEF_PRIM:
			decl.type = TYPE_DECL_PRIM;
			decl.type_name = alt->type_name;
			helper_dump(&alt->vec, &decl, alt->in_name, alt->mapped, 2);
			osi(1, "func(ctx, \",\\n\");\n");
			break;
		case NODE_ALTER_DEF_ENUM:
			decl.type = TYPE_DECL_ENUM;
			decl.type_name = alt->type_name;
			helper_dump(&alt->vec, &decl, alt->in_name, alt->mapped, 2);
			osi(1, "func(ctx, \",\\n\");\n");
			break;
		case NODE_ALTER_DEF_STRUCT:
			decl.type = TYPE_DECL_STRUCT;
			decl.type_name = alt->type_name;
			helper_dump(&alt->vec, &decl, alt->in_name, alt->mapped, 2);
			osi(1, "func(ctx, \",\\n\");\n");
			break;
		case NODE_ALTER_DEF_UNNAMED_STRUCT:
			for (memb = alt->members; memb; memb = memb->next) {
				switch (memb->type) {
				case NODE_MEMBER_DEF_PRIM:
					decl.type = TYPE_DECL_PRIM;
					decl.type_name = memb->type_name;
					helper_dump(&memb->vec, &decl, memb->in_name,
							memb->mapped, 2);
					break;
				case NODE_MEMBER_DEF_ENUM:
					decl.type = TYPE_DECL_ENUM;
					decl.type_name = memb->type_name;
					helper_dump(&memb->vec, &decl, memb->in_name,
							memb->mapped, 2);
					break;
				case NODE_MEMBER_DEF_STRUCT:
					decl.type = TYPE_DECL_STRUCT;
					decl.type_name = memb->type_name;
					helper_dump(&memb->vec, &decl, memb->in_name,
							memb->mapped, 2);
					break;
				case NODE_MEMBER_DEF_UNION:
					decl.type = TYPE_DECL_UNION;
					decl.type_name = memb->type_name;
					helper_dump(&memb->vec, &decl, memb->in_name,
							memb->mapped, 2);
					break;
				}
				osi(1, "func(ctx, \",\\n\");\n");
			}
		}
		osi(1, "}\n");
	}
	osi(1, "indent(func, ctx, l);\n");
	osi(1, "func(ctx, \"}\");\n");
	osi(0, "}\n"); /* func body */
	osi(0, "\n");
}

static void dump_type_def_list(const struct node_type_def_list *list)
{
	while (list) {
		switch (list->type) {
		case NODE_TYPE_DEF_ENUM:
			dump_enum(list->enum_def.name,
					list->enum_def.enums);
			break;
		case NODE_TYPE_DEF_STRUCT:
			dump_struct(list->struct_def.name,
					list->struct_def.members);
			break;
		case NODE_TYPE_DEF_UNION:
			dump_union(list->union_def.name,
					list->union_def.enum_name,
					list->union_def.alters);
			break;
		}
		list = list->next;
	}
}


const char *spec_path;
const char *prim_path;
const char *prelude_path;
const char *hdr_path;
const char *src_path;
const char *include_guard;

struct option opts[] = {
	{ "spec_path", required_argument, 0, 0 },
	{ "prim_path", required_argument, 0, 0 },
	{ "prelude_path", required_argument, 0, 0 },
	{ "hdr_path", required_argument, 0, 0 },
	{ "src_path", required_argument, 0, 0 },
	{ "include_guard", required_argument, 0, 0 },
	{ 0, 0, 0, 0},
};

#define ERR(fmt, ...) \
	do {\
		fprintf(stderr, fmt, ##__VA_ARGS__); \
		exit(EXIT_FAILURE); \
	} while (0)

#define ARGCASE(n, var) \
	case n: \
		if (var) { \
			ERR("arg `" #var "' defined twice\n"); \
		} \
		var = optarg; \
		break

#define ARGDEFINED(var) \
	do { \
		if (!var) { \
			ERR("arg `" #var "' is not defined\n"); \
		} \
	} while (0)

int parse_opts(int argc, char **argv)
{
	int c, opt_index;
	while (1) {
		c = getopt_long(argc, argv, "", opts, &opt_index);
		if (c == -1) { /* no more args */
			break;
		} else if (c == 0) {
			switch (opt_index) {
				ARGCASE(0, spec_path);
				ARGCASE(1, prim_path);
				ARGCASE(2, prelude_path);
				ARGCASE(3, hdr_path);
				ARGCASE(4, src_path);
				ARGCASE(5, include_guard);
			}
		} else {
			ERR("unknown argument: %s\n", argv[optind - 1]);
		}
	}
	ARGDEFINED(spec_path);
	ARGDEFINED(prim_path);
	ARGDEFINED(prelude_path);
	ARGDEFINED(hdr_path);
	ARGDEFINED(src_path);
	ARGDEFINED(include_guard);
}

extern FILE *yyin;
FILE *fp_prelude;
FILE *fp_prim;

void copy_file(FILE *dst, FILE *src)
{
	int c;
	while (1) {
		c = fgetc(src);
		if (c == EOF) {
			break;
		}
		if (putc(c, dst) == EOF) {
			ERR("failed to write to file.\n");
		}
	}
}

const char indent_dump_fmt[] = 
"static inline indent(put_func put_func, struct dump_context *ctx, int indents)\n"
"{\n"
"        int i;\n"
"        for (i = 0; i < indents; ++i) {\n"
"                put_func(ctx, \"  \");\n"
"        }\n"
"}\n"
"\n";

const char parser_func_fmt[] =
"int config_parse_%s(struct %s *value, const char *path, const char **err_msg)\n"
"{\n"
"        FILE *fp;\n"
"        void *scanner;\n"
"        struct pass_to_bison opaque;\n"
"        struct mem_pool pool;\n"
"        struct pass_to_conv context;\n"
"        int ret;\n"
"\n"
"        const char *msg_conflict = \"internal error, got impossible result: \"\n"
"                \"ok: %%d, myerrno: %%d, output: %%pn\";\n"
"\n"
"        fp = fopen(path, \"r\");\n"
"        if (!fp) {\n"
"                ret = -errno;\n"
"                *err_msg = make_message(\"failed to open file %%s for openn\",\n"
"                                path);\n"
"                goto err_fopen;\n"
"        }\n"
"\n"
"        mem_pool_init(&pool);\n"
"        opaque.pool = &pool;\n"
"        opaque.ok = 1;\n"
"        opaque.first_line = opaque.first_column = 1;\n"
"        opaque.last_line = opaque.last_column = 1;\n"
"        opaque.myerrno = 0;\n"
"        opaque.err_reason = \"no error occurred\";\n"
"        opaque.output = NULL;\n"
"\n"
"        ret = yylex_init(&scanner);\n"
"        if (ret) {\n"
"                ret = -errno;\n"
"                goto err_yylex_init;\n"
"        }\n"
"\n"
"        yyset_in(fp, scanner);\n"
"        yyparse(scanner, &opaque);\n"
"        if (opaque.ok && (opaque.myerrno || !opaque.output)) {\n"
"                ret = -EINVAL;\n"
"                *err_msg = make_message(msg_conflict, opaque.ok,\n"
"                                opaque.myerrno, opaque.output);\n"
"                goto err_yacc;\n"
"        }\n"
"        if (!opaque.ok && (!opaque.myerrno || opaque.output)) {\n"
"                ret = -EINVAL;\n"
"                *err_msg = make_message(msg_conflict, opaque.ok,\n"
"                                opaque.myerrno, opaque.output);\n"
"                goto err_yacc;\n"
"        }\n"
"        if (!opaque.output) {\n"
"                ret = opaque.myerrno;\n"
"                *err_msg = opaque.err_reason;\n"
"                goto err_yacc;\n"
"        }\n"
"\n"
"        ret = parse__struct_%s(&context, value, opaque.output);\n"
"        if (ret) {\n"
"                if (context.msg) {\n"
"                        *err_msg = make_msg_loc(context.node, context.msg);\n"
"                } else {\n"
"                        *err_msg = make_msg_loc(context.node, \"\");\n"
"                }\n"
"                goto err_toconfig;\n"
"        }\n"
"\n"
"        mem_pool_destroy(&pool);\n"
"        yylex_destroy(scanner);\n"
"        fclose(fp);\n"
"        return 0;\n"
"\n"
"err_toconfig:\n"
"err_yacc:\n"
"        mem_pool_destroy(&pool);\n"
"        yylex_destroy(scanner);\n"
"err_yylex_init:\n"
"        fclose(fp);\n"
"err_fopen:\n"
"        return ret;\n"
"}\n"
"\n";

const char config_dump[] =
"void config_dump_%s(put_func func, struct dump_context *context, const struct %s *value)\n"
"{\n"
"        dump__struct_%s(func, context, 0, value);\n"
"        func(context, \"\\n\");\n"
"}\n"
"\n";

const char config_free[] =
"void config_free_%s(struct %s *value)\n"
"{\n"
"        free__struct_%s(value);\n"
"}\n"
"\n";

int main(int argc, char **argv)
{
	const char *header_filename;
	const struct node_type_def_list *list;

	parse_opts(argc, argv);


	yyin = fopen(spec_path, "r");
	if (!yyin) {
		ERR("failed to open spec file %s to read\n", spec_path);
	}

	fp_prelude = fopen(prelude_path, "r");
	if (!fp_prelude) {
		ERR("failed to open prelude file %s to read\n", spec_path);
	}
	fp_prim = fopen(prim_path, "r");
	if (!fp_prim) {
		ERR("failed to open primitive types file %s to read\n", prim_path);
	}

	yyparse();

	fprintf(stderr, "spec file parsed\n");

	fp_hdr = fopen(hdr_path, "w");
	if (!fp_hdr) {
		ERR("failed to open header file %s to write\n", hdr_path);
	}
	fp_src = fopen(src_path, "w");
	if (!fp_src) {
		ERR("failed to open source file %s to write\n", src_path);
	}

	verify_def_list(ast);

	out_hdr("#ifndef %s\n", include_guard);
	out_hdr("#define %s\n", include_guard);
	
	header_filename = strrchr(hdr_path, '/');
	if (!header_filename) {
		header_filename = hdr_path;
	}
	out_src("#include \"%s\"\n", header_filename);
	out_src("#include \"parser.h\"\n", header_filename);

	copy_file(fp_hdr, fp_prelude);
	copy_file(fp_src, fp_prim);

	out_src(indent_dump_fmt);	
	decl_def_list(ast);
	parse_type_def_list(ast);
	free_type_def_list(ast);
	dump_type_def_list(ast);

	out_hdr("struct dump_context;\n");
	out_hdr("typedef void (*put_func)(struct dump_context *ctx, "
			"const char *fmt, ...);\n");
	for (list = ast; list; list = list->next) {
		if (list->type == NODE_TYPE_DEF_STRUCT &&
				list->struct_def.exported) {
			out_src(parser_func_fmt, list->struct_def.name,
					list->struct_def.name,
					list->struct_def.name);
			out_src(config_dump, list->struct_def.name,
					list->struct_def.name,
					list->struct_def.name);
			out_src(config_free, list->struct_def.name,
					list->struct_def.name,
					list->struct_def.name);
		
			out_hdr("extern void config_free_%s(struct %s *);\n",
					list->struct_def.name,
					list->struct_def.name);
			out_hdr("extern int config_parse_%s(struct %s *value, "
					"const char *path, const char **err_msg);\n",
					list->struct_def.name,
					list->struct_def.name);
			out_hdr("extern void config_dump_%s(put_func, struct dump_context *ctx, "
					"const struct %s *value);\n",
					list->struct_def.name,
					list->struct_def.name);
		}
	}

	out_hdr("#endif\n");
	return 0;
}
