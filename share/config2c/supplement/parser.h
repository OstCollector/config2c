#ifndef PARSER_H
#define PARSER_H

/*
 * This file is part of config2c which is relased under Apache License.
 * See LICENSE for full license details.
 */

#include <stddef.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

/* type for numerical types, pre-convert */

struct malloc_list;
struct node_value;
struct node_members;
struct node_elems;

enum val_type {
	VAL_SCALE_CHAR,
	VAL_SCALE_INT,
	VAL_SCALE_FLOAT,
	VAL_SCALE_IDEN,
	VAL_SCALE_STRING,
	VAL_MEMBERS,
	VAL_ELEMS,
};

struct node_value {
	union {
		const char *char_str;
		const char *int_str;
		const char *float_str;
		const char *enum_str;
		const char *string_str;
		struct node_members *members;
		struct node_elems *elems;
	};
	enum val_type type;
	const struct node_value *parent;
	union {
		const char *name_copy;
		long index;
	};
};

struct node_members {
	struct node_members *next;
	const char *name;
	struct node_value *value;
};

struct node_elems {
	struct node_elems *next;
	struct node_value *value;
};

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

rev_list(rev_node_members, struct node_members, next)
rev_list(rev_node_elems, struct node_elems, next)

#define len_list(name, type, pnext) \
	static inline long name(type *p) \
	{ \
		int i = 0; \
		while (p) { \
			++i; \
			p = p->pnext; \
		} \
		return i; \
	}

len_list(len_node_members, struct node_members, next)
len_list(len_node_elems, struct node_elems, next)

static inline void set_parent_members(struct node_members *p, struct node_value *parent)
{
	while (p) {
		p->value->parent = parent;
		p->value->name_copy = p->name;
		p = p->next;
	}
}

static inline void set_parent_elems(struct node_elems *p, struct node_value *parent)
{
	long i = 0;
	while (p) {
		p->value->parent = parent;
		p->value->index = i;
		p = p->next;
		++i;
	}
}

struct mem_elem;
struct mem_pool {
	struct mem_elem *list;
};

extern void mem_pool_init(struct mem_pool *p);
extern void *mem_pool_alloc(struct mem_pool *p, size_t s); 
extern void mem_pool_destroy(struct mem_pool *p);

struct pass_to_conv {
	struct mem_pool *pool;
	const struct node_value *node;
	const char *msg;
};

struct pass_to_bison {
	struct mem_pool *pool;
	int ok;
	
	int first_line;
	int first_column;
	int last_line;
	int last_column;

	int myerrno;
	const char *err_reason;
	
	struct node_value *output;
};

extern void init_pass_to_bison(struct pass_to_bison *ctx, 
		struct mem_pool *pool);

extern const char *make_message(const char *fmt, ...);

union vvstype {
	const char *iden_str;
	const char *char_str;
	const char *int_str;
	const char *float_str;
	const char *string_str;
	struct node_value *value;
	struct node_members *members;
	struct node_elems *elems;
};

extern const char *make_msg_loc(const struct node_value *pos, const char *fmt, ...);

extern int yacc_parse_file(const char *filename, const char **err_msg, 
		struct pass_to_bison *ctx);
extern int yacc_parse_string(const char *str, const char **err_msg,
		struct pass_to_bison *ctx);

#endif
