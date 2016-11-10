/*
 * This file is part of config2c which is relased under Apache License.
 * See LICENSE for full license details.
 */


D			[0-9]
L			[a-zA-Z_]
IL			[a-zA-Z]
H			[a-fA-F0-9]
E			([Ee][+-]?{D}+)
P                       ([Pp][+-]?{D}+)
FS			(f|F|l|L)
IS                      ((u|U)|(u|U)?(l|L|ll|LL)|(l|L|ll|LL)(u|U))

%option noyywrap



%{

#include <stdlib.h>
#include <string.h>
#include "config2cy.tab.h"

int yycolumn = 1;

static int advance_char(char ch) {
	//fprintf(stderr, "%c", ch);
	if(ch == '\n') {
		yylloc.last_line++;
		yylloc.last_column = 0;
	}
	else {
		yylloc.last_column++;
	}
}

static void advance_token()
{
	int i;
	yylloc.first_line = yylloc.last_line;
	yylloc.first_column = yylloc.last_column;
	for(i = 0; yytext[i] != '\0'; i++) {
		advance_char(yytext[i]);
	}
	//fprintf(stderr, "%d:%d - %d:%d\n",
	//		yylloc.first_line, yylloc.first_column,
	//		yylloc.last_line, yylloc.last_column);
}

void comment(void);
void to_int(void);
void to_str(void);
void to_str2(void);
void to_str3(void);

#define YY_USER_ACTION advance_token();

#if 1
#define LDBG(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#else
#define LDBG(...) do { ; } while (0)
#endif

%}

%%
[ \t\n]		{ }

"/*"		{ comment(); }
"//"[^\n]* 	{ }

"struct"	{ return STRUCT; }
"union"		{ return UNION; }
"enum"          { return ENUM; }
"as"		{ return AS; }

"{"		{ return '{'; }
"}"		{ return '}'; }
"["		{ return '['; }
"]"		{ return ']'; }
"("		{ return '('; }
")"		{ return ')'; }
";"		{ return ';'; }
","		{ return ','; }
":"		{ return ':'; }
"="		{ return '='; }
"~"		{ return '~'; }

{IL}({L}|{D})*	{ to_str(); return IDEN; }

0[xX]{H}+?		{ to_int(); return INTEGER; } 
0[0-7]*?		{ to_int(); return INTEGER; } 
[1-9]{D}*?		{ to_int(); return INTEGER; } 

\'[a-zA-Z][0-9a-zA-Z_\ \*]*\'            { to_str2(); return STRING; }

\"([^"\\\n]|\\[\\\"])*\"		 { to_str3(); return DEFVAL; }


.			{ yyerror("Unknown input"); exit(EXIT_FAILURE);}

%%

void comment(void)
{
	char c, prev = 0;
	while ((c = input()) != 0)      /* (EOF maps to 0) */
	{
		advance_char(c);
		if (c == '/' && prev == '*')
			return;
		prev = c;
	}
	error("unterminated comment");
}

void to_int(void)
{
	errno = 0;
	yylval.int_val = strtol(yytext, NULL, 0);
	if (errno) {
		fprintf(stderr, "integer %s is too large or small at %d:%d\n",
				yytext, yylloc.first_line, yylloc.last_line);
		exit(EXIT_FAILURE);
	}
	LDBG("got integer: %ld\n", yylval.int_val);
}

void to_str(void)
{
	yylval.str_val = strdup(yytext);
	if (!yylval.str_val) {
		fprintf(stderr, "failed to malloc memory for string\n");
		exit(EXIT_FAILURE);
	}
	LDBG("got identifier: %s\n", yylval.str_val);
}

void to_str2(void)
{
	yylval.str_val = strdup(yytext + 1);
	if (!yylval.str_val) {
		fprintf(stderr, "failed to malloc memory for string\n");
		exit(EXIT_FAILURE);
	}
	yylval.str_val[strlen(yylval.str_val) - 1] = '\0';
	LDBG("got string: %s\n", yylval.str_val);
}

void to_str3(void)
{
	yylval.str_val = strdup(yytext);
	if (!yylval.str_val) {
		fprintf(stderr, "failed to malloc memory for string\n");
		exit(EXIT_FAILURE);
	}
	LDBG("got value: %s\n", yylval.str_val);
}

