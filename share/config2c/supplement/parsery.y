%{

#include <stdlib.h>
#include "parser.h"

#undef PARSE_DEBUG

#if defined PARSE_DEBUG
#define PDBG(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#else
#define PDBG(...) do { ; } while (0)
#endif

struct node_value *create_node_val(struct pass_to_bison *opaque, 
		enum val_type type, const char *val_str)
{
	struct node_value *ret = mem_pool_alloc(opaque->pool, sizeof(*ret));
	PDBG("ok: %d, myerrno: %d, output: %p\n", opaque->ok,
			opaque->myerrno, opaque->output);
	if (!ret) {
		opaque->ok = 0;
		opaque->myerrno = -ENOMEM;
		return NULL;
	}
	ret->parent = NULL;
	switch (type) {
	case VAL_SCALE_IDEN:
		ret->type = VAL_SCALE_IDEN;
		ret->enum_str = val_str;
		PDBG("value:enum:%p, str:%p\n", ret, ret->enum_str);
		break;
	case VAL_SCALE_CHAR:
		ret->type = VAL_SCALE_CHAR;
		ret->char_str = val_str;
		PDBG("value:char:%p, str:%p\n", ret, ret->char_str);
		break;
	case VAL_SCALE_INT:
		ret->type = VAL_SCALE_INT;
		ret->int_str = val_str;
		PDBG("value:int:%p, str:%p\n", ret, ret->int_str);
		break;
	case VAL_SCALE_FLOAT:
		ret->type = VAL_SCALE_FLOAT;
		ret->float_str = val_str;
		PDBG("value:float:%p, str:%p\n", ret, ret->float_str);
		break;
	case VAL_SCALE_STRING:
		ret->type = VAL_SCALE_STRING;
		ret->string_str = val_str;
		PDBG("value:str:%p, str:%p\n", ret, ret->string_str);
		break;
	}
	return ret;
}

%}

%define api.pure full
%define api.value.type { union vvstype }
%define parse.error verbose
%parse-param {void *scanner}
%parse-param {struct pass_to_bison *opaque}
%lex-param {void *scanner}
%lex-param {struct pass_to_bison *opaque}

%token ERROR;
%token <iden_str> IDEN
%token <char_str> CHAR
%token <int_str> INT
%token <float_str> FLOAT
%token <string_str> STRING

%type <members> members
%type <elems> elems
%type <value> value
%type <value> scale

%%

config
	: value {
		PDBG("opaque: %p, ok: %d, myerrno: %d, output: %p\n", opaque,
				opaque->ok, opaque->myerrno, opaque->output);
		opaque->output = $1;
		PDBG("output: %p\n", opaque->output);
	}
	| error
	;

members
	: {
		$$ = NULL;
		PDBG("members:nil\n");
	}
	| members '.' IDEN '=' value ',' {
		if (opaque->ok) {
			struct node_members *ret = mem_pool_alloc(opaque->pool, sizeof(*ret));
			PDBG("ok: %d, myerrno: %d, output: %p\n", opaque->ok,
					opaque->myerrno, opaque->output);
			if (!ret) {
				opaque->ok = 0;
				opaque->myerrno = -ENOMEM;
			} else {
				ret->next = $1;
				ret->name = $3;
				ret->value = $5;
				PDBG("members:cons:%p, name:%p, value:%p, next:%p\n",
						ret, ret->name, ret->value, ret->next);
			}
			$$ = ret;
		} else {
			$$ = NULL;
		}
	}
	;

elems
	: {
		$$ = NULL;
		PDBG("elems:nil\n");
	}
	| elems value ',' {
		if (opaque->ok) {
			struct node_elems *ret =
				mem_pool_alloc(opaque->pool, sizeof(*ret));
			PDBG("ok: %d, myerrno: %d, output: %p\n", opaque->ok,
					opaque->myerrno, opaque->output);
			if (!ret) {
				opaque->ok = 0;
				opaque->myerrno = -ENOMEM;
			} else {
				ret->next = $1;
				ret->value = $2;
				PDBG("elems:cons:%p, value:%p, next:%p\n",
						ret, ret->value, ret->next);
			}
			$$ = ret;
		} else {
			$$ = NULL;
		}
	}
	;

value
	: scale {
		$$ = $1;
	}
	| '{' members '}' {
		if (opaque->ok) {
			struct node_value *ret;
			struct node_members *t;
			ret = mem_pool_alloc(opaque->pool, sizeof(*ret));
			PDBG("ok: %d, myerrno: %d, output: %p\n", opaque->ok,
					opaque->myerrno, opaque->output);
			if (!ret) {
				opaque->ok = 0;
				opaque->myerrno = -ENOMEM;
			} else {
				ret->type = VAL_MEMBERS;
				ret->members = rev_node_members($2);
				set_parent_members(ret->members, ret);
				ret->parent = NULL;
				PDBG("value:members:%p, members:%p\n",
						ret, ret->members);
				PDBG("list: ");
				for (t = ret->members; t; t = t->next) {
					PDBG("%p  ", t);
				}
				PDBG("\n");
			}
			$$ = ret;
		} else {
			$$ = NULL;
		}
	}
	| '[' elems ']' {
		if (opaque->ok) {
			struct node_value *ret;
			struct node_elems *t;
			ret = mem_pool_alloc(opaque->pool, sizeof(*ret));
			PDBG("ok: %d, myerrno: %d, output: %p\n", opaque->ok,
					opaque->myerrno, opaque->output);
			if (!ret) {
				opaque->ok = 0;
				opaque->myerrno = -ENOMEM;
			} else {
				ret->type = VAL_ELEMS;
				ret->elems = rev_node_elems($2);
				set_parent_elems(ret->elems, ret);
				ret->parent = NULL;
				PDBG("value:elems:%p, elems:%p\n",
						ret, ret->elems);
				PDBG("list: ");
				for (t = ret->elems; t; t = t->next) {
					PDBG("%p  ", t);
				}
				PDBG("\n");
			}
			$$ = ret;
		} else {
			$$ = NULL;
		}
	}

	;

scale
	: CHAR {
		$$ = create_node_val(opaque, VAL_SCALE_CHAR, $1);
	}
	| INT {
		$$ = create_node_val(opaque, VAL_SCALE_INT, $1);
	}
	| FLOAT {
		$$ = create_node_val(opaque, VAL_SCALE_FLOAT, $1);
	}
	| IDEN {
		$$ = create_node_val(opaque, VAL_SCALE_IDEN, $1);
	}
	| STRING {
		$$ = create_node_val(opaque, VAL_SCALE_STRING, $1);
	}
	;

%%

int yyerror(void * scanner, struct pass_to_bison *opaque, const char *msg)
{
	opaque->ok = 0;
	opaque->myerrno = -EINVAL;
	opaque->output = NULL;
	opaque->err_reason = make_message("%d:%d : %s",
			opaque->first_line, opaque->first_column, msg);
	PDBG("%s", msg);
}

