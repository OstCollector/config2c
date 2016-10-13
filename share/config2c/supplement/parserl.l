/*
 * This file is part of config2c which is relased under Apache License.
 * See LICENSE for full license details.
 */

O   [0-7]
D   [0-9]
NZ  [1-9]
L   [a-zA-Z_]
A   [a-zA-Z_0-9]
H   [a-fA-F0-9]
HP  (0[xX])
E   ([Ee][+-]?{D}+)
P   ([Pp][+-]?{D}+)
FS  (f|F|l|L)
IS  (((u|U)(l|L|ll|LL)?)|((l|L|ll|LL)(u|U)?))
CP  (u|U|L)
SP  (u8|u|U|L)
ES  (\\(['"\?\\abfnrtv]|x[a-fA-F0-9]{2}))
WS  [ \t\v\n\f]
PM  [+-]

%option reentrant
%option bison-bridge
%option noyywrap

%{

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "parsery.tab.h"

static int advance_char(struct pass_to_bison *opaque, char ch);
static void advance_token(struct pass_to_bison *opaque, const char *token);
static void to_val(union vvstype *lvalp, const char *token,
		struct pass_to_bison *opaque, enum yytokentype type);
static void error(struct pass_to_bison *opaque);

#define YY_DECL yylex(YYSTYPE *yylval_param, yyscan_t yyscanner, struct pass_to_bison *opaque)

#define adv_token advance_token(opaque, yytext)
#define toval(type) to_val(yylval_param, yytext, opaque, type)

%}

%%

{WS}+				{ adv_token; }

"//".*				{ adv_token; /* consume //-comment */ }

{L}{A}*				{ adv_token; toval(IDEN); return IDEN; }

"'"([^'\\\n]|{ES})+"'"		{ adv_token; toval(CHAR); return CHAR; }

{PM}?{HP}{H}+{IS}?		{ adv_token; toval(INT); return INT; }
{PM}?{NZ}{D}*{IS}?		{ adv_token; toval(INT); return INT; }
{PM}?"0"{O}*{IS}?		{ adv_token; toval(INT); return INT; }
{PM}?{D}*"."{D}+{E}?{FS}?	{ adv_token; toval(FLOAT); return FLOAT; }
{PM}?{D}+"."{E}?{FS}?		{ adv_token; toval(FLOAT); return FLOAT; }
{PM}?{HP}{H}+{P}{FS}?		{ adv_token; toval(FLOAT); return FLOAT; }
{PM}?{HP}{H}*"."{H}+{P}{FS}?	{ adv_token; toval(FLOAT); return FLOAT; }
{PM}?{HP}{H}+"."{P}{FS}?	{ adv_token; toval(FLOAT); return FLOAT; }

\"([^"\\\n]|{ES})*\"		{ adv_token; toval(STRING); return STRING; }


","				{ adv_token; return ','; }
"{"				{ adv_token; return '{'; }
"}"				{ adv_token; return '}'; }
"["				{ adv_token; return '['; }
"]"				{ adv_token; return ']'; }
"\."				{ adv_token; return '.'; }
"="				{ adv_token; return '='; }
.				{ adv_token; error(opaque); return ERROR; }

%%

static int advance_char(struct pass_to_bison *opaque, char ch)
{
	if(ch == '\n') {
		opaque->last_line++;
		opaque->last_column = 0;
	}
	else {
		opaque->last_column++;
	}
}

static void advance_token(struct pass_to_bison *opaque, const char *token)
{
	int i;
	if (opaque->ok) {
		opaque->first_line = opaque->last_line;
		opaque->first_column = opaque->last_column;
		for (i = 0; token[i] != '\0'; i++) {
			advance_char(opaque, token[i]);
		}
	}
}

static char *my_strdup(struct pass_to_bison *opaque, const char *str,
		int do_trim)
{
	size_t len = strlen(str);
	char *data;
	if (len >= SIZE_MAX) {
		opaque->ok = 0;
		opaque->myerrno = -EMSGSIZE;
	}
	if (do_trim) {
		len -= 2;
	}
	data = mem_pool_alloc(opaque->pool, len + 1);
	if (!data) {
		opaque->ok = 0;
		opaque->myerrno = -ENOMEM;
	}
	memcpy(data, do_trim ? str + 1 : str, len + 1);
	data[len] = '\0';
	return data;
}

static void to_val(union vvstype *lvalp, const char *token,
		struct pass_to_bison *opaque, enum yytokentype type)
{
	if (opaque->ok) {
		switch (type) {
		case IDEN:
			lvalp->iden_str = my_strdup(opaque, token, 0);
			break;
		case CHAR:
			lvalp->char_str = my_strdup(opaque, token, 1);
			break;
		case INT:
			lvalp->int_str = my_strdup(opaque, token, 0);
			break;
		case FLOAT:
			lvalp->float_str = my_strdup(opaque, token, 0);
			break;
		case STRING:
			lvalp->string_str = my_strdup(opaque, token, 1);
			break;
		}
	}
}

static void error(struct pass_to_bison *opaque) 
{
	opaque->ok = 0;
	opaque->myerrno = -EINVAL;
	opaque->err_reason = make_message("%d:%d : Invalid token",
			opaque->first_line, opaque->first_column);
}


