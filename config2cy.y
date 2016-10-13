%code requires {

/*
 * This file is part of config2c which is relased under Apache License.
 * See LICENSE for full license details.
 */


#include <stdlib.h>
#include <stdio.h>
#include "config2c.h"

static inline void valid_or_fail(void *ret)
{
	if (!ret) {
		fprintf(stderr, "insufficient memory\n");
		exit(EXIT_FAILURE);
	}
}

#if 0
#define PDBG(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#else
#define PDBG(...) do { ; } while (0)
#endif

}

%union {
	char *str_val;
	long  int_val;
	struct node_type_def_list	*type_def;
	int				exported;
	struct node_mapping		*mapping;
	struct node_enum_list		*enum_list;
	struct node_member_list		*members;
	struct node_alter_list		*alters;
	struct node_vec_def		vec_def;
	struct string_list		*str_list;
}

%token STRUCT UNION ENUM AS

%token <str_val> IDEN
%token <int_val> INTEGER
%token <str_val> STRING

%type <mapping>			mapping_def
%type <enum_list>		enum_list_t
%type <enum_list>		enum_list
%type <enum_list>		enum_entry
%type <type_def>		type_def
%type <exported>		exported
%type <members>			member_list
%type <members>			member_def
%type <alters>			alter_list
%type <alters>			alter_def
%type <vec_def>			vec_def
%type <str_list>		string_list_t
%type <str_list>		string_list
%type <str_list>		iden_list_t
%type <str_list>		iden_list

%error-verbose

%start config_spec

%locations

%%
config_spec
	: mapping_def_list type_def_list {
		ast = rev_type_def_list(ast);
		mapping = rev_mapping(mapping);
	}

mapping_def_list
	: {
		mapping = NULL;
	}
	| mapping_def_list mapping_def {
		$2->next = mapping;
		mapping = $2;
	}
	;

mapping_def
	: IDEN ':' IDEN IDEN IDEN '(' string_list_t ')' ';' {
		struct node_mapping *ret = malloc(sizeof(*ret));
		valid_or_fail(ret);
		ret->next = NULL;
		ret->name = $1;
		ret->parse_func = $3;
		ret->dump_func = $4;
		ret->free_func = $5;
		ret->mapped_types = rev_string_list($7);
		PDBG("mapping_def:%p, name:%p, parse:%p, dump:%p, "
				"free:%p, mapped:%p\n", ret, ret->name, 
				ret->parse_func, ret->dump_func,
				ret->free_func, ret->mapped_types);
		$$ = ret;
	}
	;

type_def_list
	: {
		ast = NULL;
	}
	| type_def_list type_def {
		$2->next = ast;
		ast = $2;
	}
	;

type_def
	: ENUM IDEN '{' enum_list_t '}' ';' {
		struct node_type_def_list *ret = malloc(sizeof(*ret));
		valid_or_fail(ret);
		ret->type = NODE_TYPE_DEF_ENUM;
		ret->enum_def.name = $2;
		ret->enum_def.enums = rev_enum_list($4);
		PDBG("type_def:enum:%p, name:%p, list:%p\n",
				ret, ret->enum_def.name,
				ret->enum_def.enums);
		$$ = ret;
	}
	| STRUCT IDEN '{' member_list '}' exported ';' {
		struct node_type_def_list *ret = malloc(sizeof(*ret));
		valid_or_fail(ret);
		ret->type = NODE_TYPE_DEF_STRUCT;
		ret->struct_def.name = $2;
		ret->struct_def.members = rev_member_list($4);
		ret->struct_def.exported = $6;
		PDBG("type_def:struct:%p, name:%p, list:%p\n",
				ret, ret->struct_def.name,
				ret->struct_def.members);
		$$ = ret;
	}
	| UNION IDEN ':' IDEN '{' alter_list '}' ';' {
		struct node_type_def_list *ret = malloc(sizeof(*ret));
		valid_or_fail(ret);
		ret->type = NODE_TYPE_DEF_UNION;
		ret->union_def.name = $2;
		ret->union_def.enum_name = $4;
		ret->union_def.alters = rev_alter_list($6);
		PDBG("type_def:union:%p, name:%p, list:%p\n",
				ret, ret->union_def.name,
				ret->union_def.enum_name,
				ret->union_def.alters);
		$$ = ret;
	}
	;

exported
	: { $$ = 0; }
	| IDEN {
		if (strcmp($1, "export")) {
			yyerror("expecting ';' or 'export'");
		}
		$$ = 1;
	}
	;

enum_list_t
	: enum_list {
		$$ = $1;
	}
	| enum_list ',' {
		$$ = $1;
	}
	;

enum_list
	: enum_entry {
		PDBG("enum_list: entry:%p, name:%p, alias:%p, next:%p\n",
				$1, $1->name, $1->alias, $1->next);
		$$ = $1;
	}
	| enum_list ',' enum_entry {
		$3->next = $1;
		PDBG("enum_list: entry:%p, name:%p, alias:%p, next:%p\n",
				$3, $3->name, $3->alias, $3->next);
		$$ = $3;
	}
	;

enum_entry
	: IDEN {
		struct node_enum_list *ret = malloc(sizeof(*ret));
		valid_or_fail(ret);
		ret->next = NULL;
		ret->name = $1;
		ret->alias = NULL;
		$$ = ret;
	}
	| IDEN AS IDEN {
		struct node_enum_list *ret = malloc(sizeof(*ret));
		valid_or_fail(ret);
		ret->next = NULL;
		ret->name = $1;
		ret->alias = $3;
		$$ = ret;
	}
	;

member_list
	: {
		$$ = NULL;
		PDBG("member_list:nil\n");
	}
	| member_list member_def {
		$2->next = $1;
		PDBG("member_list:cons:%p, def:%p, next:%p\n",
				ret, ret->def, ret->next);
		$$ = $2;
	}
	;

member_def
	: IDEN IDEN '(' iden_list_t ')' vec_def ';' {
		struct node_member_list *ret = malloc(sizeof(*ret));
		valid_or_fail(ret);
		ret->type = NODE_MEMBER_DEF_PRIM;
		ret->type_name = $1;
		ret->in_name = $2;
		ret->mapped = rev_string_list($4);
		ret->vec = $6;
		$$ = ret;
	}
	| IDEN IDEN vec_def ';' {
		struct node_member_list *ret = malloc(sizeof(*ret));
		struct string_list *list = malloc(sizeof(*ret));
		valid_or_fail(ret);
		valid_or_fail(list);
		ret->type = NODE_MEMBER_DEF_PRIM;
		ret->type_name = $1;
		ret->in_name = $2;
		ret->mapped = list;
		list->str = $2;
		list->next = NULL;
		ret->vec = $3;
		$$ = ret;
	}
	| ENUM IDEN IDEN vec_def ';' {
		struct node_member_list *ret = malloc(sizeof(*ret));
		struct string_list *list = malloc(sizeof(*ret));
		valid_or_fail(ret);
		valid_or_fail(list);
		ret->type = NODE_MEMBER_DEF_ENUM;
		ret->type_name = $2;
		ret->in_name = $3;
		ret->mapped = list;
		list->str = $3;
		list->next = NULL;
		ret->vec = $4;
		$$ = ret;
	}
	| STRUCT IDEN IDEN vec_def ';' {
		struct node_member_list *ret = malloc(sizeof(*ret));
		struct string_list *list = malloc(sizeof(*ret));
		valid_or_fail(ret);
		valid_or_fail(list);
		ret->type = NODE_MEMBER_DEF_STRUCT;
		ret->type_name = $2;
		ret->in_name = $3;
		ret->mapped = list;
		list->str = $3;
		list->next = NULL;
		ret->vec = $4;
		$$ = ret;
	}
	| UNION IDEN IDEN vec_def AS IDEN ';' {
		struct node_member_list *ret = malloc(sizeof(*ret));
		struct string_list *list1 = malloc(sizeof(*ret));
		struct string_list *list2 = malloc(sizeof(*ret));
		valid_or_fail(ret);
		valid_or_fail(list1);
		valid_or_fail(list2);
		ret->type = NODE_MEMBER_DEF_UNION;
		ret->type_name = $2;
		ret->in_name = $3;
		ret->mapped = list1;
		list1->str = $3;
		list1->next = list2;
		list2->str = $6;
		list2->next = NULL;
		ret->vec = $4;
		$$ = ret;
	}
	| UNION '{' alter_list '}' ':' IDEN ';' {
		struct node_member_list *ret = malloc(sizeof(*ret));
		valid_or_fail(ret);
		ret->type = NODE_MEMBER_DEF_UNNAMED_UNION;
		ret->alters = rev_alter_list($3);
		ret->alt_enum = $6;
		$$ = ret;
	}
	;

alter_list
	: alter_def {
		$1->next = NULL;
		$$ = $1;
	}
	| alter_list alter_def {
		$2->next = $1;
		$$ = $2;
	}
	;

alter_def
	: IDEN IDEN vec_def AS IDEN ';' {
		struct node_alter_list *ret = malloc(sizeof(*ret));
		struct string_list *list = malloc(sizeof(*ret));
		valid_or_fail(ret);
		valid_or_fail(list);
		ret->type = NODE_ALTER_DEF_PRIM;
		ret->type_name = $1;
		ret->in_name = $2;
		ret->mapped = list;
		list->str = $2;
		list->next = NULL;
		ret->vec = $3;
		ret->enum_val = $5;
		$$ = ret;
	}
	| ENUM IDEN IDEN vec_def AS IDEN ';'  {
		struct node_alter_list *ret = malloc(sizeof(*ret));
		struct string_list *list = malloc(sizeof(*ret));
		valid_or_fail(ret);
		valid_or_fail(list);
		ret->type = NODE_ALTER_DEF_ENUM;
		ret->type_name = $2;
		ret->in_name = $3;
		ret->mapped = list;
		list->str = $3;
		list->next = NULL;
		ret->vec = $4;
		ret->enum_val = $6;
		$$ = ret;
	}
	| STRUCT IDEN IDEN vec_def AS IDEN ';'  {
		struct node_alter_list *ret = malloc(sizeof(*ret));
		struct string_list *list = malloc(sizeof(*ret));
		valid_or_fail(ret);
		valid_or_fail(list);
		ret->type = NODE_ALTER_DEF_STRUCT;
		ret->type_name = $2;
		ret->in_name = $3;
		ret->mapped = list;
		list->str = $3;
		list->next = NULL;
		ret->vec = $4;
		ret->enum_val = $6;
		$$ = ret;
	}
	| STRUCT '{' member_list '}' AS IDEN ';' {
		struct node_alter_list *ret = malloc(sizeof(*ret));
		valid_or_fail(ret);
		ret->type = NODE_ALTER_DEF_UNNAMED_STRUCT;
		ret->members = rev_member_list($3);
		ret->enum_val = $6;
		$$ = ret;
	}
	;


vec_def
	: {
		$$.type = NODE_TYPE_SCALE;
	}
	| '[' INTEGER ']' {
		$$.type = NODE_TYPE_FIX_ARR;
		$$.len_int = $2;
	}
	| '[' IDEN ']' {
		$$.type = NODE_TYPE_VAR_ARR;
		$$.len_str = $2;
	}
	;

string_list_t
	: string_list { $$ = $1; }
	| string_list ',' { $$ = $1; }
	;

string_list
	: STRING {
		struct string_list *ret = malloc(sizeof(*ret));
		valid_or_fail(ret);
		ret->next = NULL;
		ret->str = $1;
		PDBG("string_list:single: %p, next: %p, type:%p\n",
				ret, ret->next, ret->str);
		$$ = ret;
	}
	| string_list ',' STRING {
		struct string_list *ret = malloc(sizeof(*ret));
		valid_or_fail(ret);
		ret->next = $1;
		ret->str = $3;
		PDBG("string_list:conv: %p, next: %p, type:%p\n",
				ret, ret->next, ret->str);
		$$ = ret;
	}
	;

iden_list_t
	: iden_list { $$ = $1; }
	| iden_list ',' { $$ = $1; }
	;

iden_list
	: IDEN {
		struct string_list *ret = malloc(sizeof(*ret));
		valid_or_fail(ret);
		ret->next = NULL;
		ret->str = $1;
		PDBG("string_list:single: %p, next: %p, type:%p\n",
				ret, ret->next, ret->str);
		$$ = ret;
	}
	| iden_list ',' IDEN {
		struct string_list *ret = malloc(sizeof(*ret));
		valid_or_fail(ret);
		ret->next = $1;
		ret->str = $3;
		PDBG("string_list:conv: %p, next: %p, type:%p\n",
				ret, ret->next, ret->str);
		$$ = ret;
	}
	;



%%

int yyerror(const char *msg)
{
	fprintf(stderr, "%d:%d %s\n", yylloc.first_line,
			yylloc.first_column, msg);
	exit(EXIT_FAILURE);
}
