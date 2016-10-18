/*
 * This file is part of config2c which is relased under Apache License.
 * See LICENSE for full license details.
 */

#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include "parser.h"

typedef void *yyscan_t;
extern void yyset_in(FILE *in_str , yyscan_t yyscanner);
extern void *yy_scan_string (const char *yystr , yyscan_t yyscanner);



struct mem_elem {
	struct mem_elem *next;
	void *data;
};

void mem_pool_init(struct mem_pool *p)
{
	p->list = NULL;
}

void *mem_pool_alloc(struct mem_pool *p, size_t s)
{
	struct mem_elem *q, *r;
	r = p->list;
	q = malloc(sizeof(*q));
	if (!q) {
		return NULL;
	}
	q->data = malloc(s);
	if (!q->data) {
		free(q);
		return NULL;
	}
	q->next = r;
	p->list = q;
	return q->data;
}

void mem_pool_destroy(struct mem_pool *p)
{
	struct mem_elem *q, *r;
	q = p->list;
	while (q) {
		r = q->next;
		free(q->data);
		free(q);
		q = r;
	}
}

const char *make_message(const char *fmt, ...)
{
	int size = 0;
	char *p = NULL;
	va_list ap;

	/* Determine required size */

	va_start(ap, fmt);
	size = vsnprintf(p, size, fmt, ap);
	va_end(ap);

	if (size < 0)
		return NULL;

	size++;		    /* For '\0' */
	p = malloc(size);
	if (p == NULL)
		return NULL;

	va_start(ap, fmt);
	size = vsnprintf(p, size, fmt, ap);
	if (size < 0) {
		free(p);
		return NULL;
	}
	va_end(ap);

	return p;
}

static void rev_str(char *b, char *e)
{
	char t;
	while (b < e) {
		t = *b;
		*b = *e;
		*e = t;
		++b;
		--e;
	}
}

const char *make_msg_loc(const struct node_value *pos, const char *fmt, ...)
{
	const struct node_value *cur;
	long length = 0, seg_len, left_len;
	char *p, *beg, *end;
	va_list ap;

	cur = pos;
	while (cur->parent) {
		if (cur->parent->type == VAL_MEMBERS) {
			length += snprintf(NULL, 0, ".%s", cur->name_copy);
		} else { /* VAL_ELEMS */
			length += snprintf(NULL, 0, ".[%ld]", cur->index);
		}
		cur = cur->parent;
	}
	length += snprintf(NULL, 0, ": ");
	va_start(ap, fmt);
	length += vsnprintf(NULL, 0, fmt, ap) + 1;
	va_end(ap);

	if (length < 0) {
		return NULL;
	}

	length++;
	p = malloc(length);
	if (!p) {
		return NULL;
	}
	
	beg = p;
	end = p;
	left_len = length;
	cur = pos;
	while (cur->parent) {
		if (cur->parent->type == VAL_MEMBERS) {
			seg_len = snprintf(beg, left_len, ".%s", cur->name_copy);
		} else { /* VAL_ELEMS */
			seg_len = snprintf(beg, left_len, ".[%ld]", cur->index);
		}
		if (seg_len >= left_len) {
			goto err;
		}
		end = beg + seg_len - 1;
		rev_str(beg, end);
		beg += seg_len;
		left_len -= seg_len;
		cur = cur->parent;
	}
	rev_str(p, end);

	seg_len = snprintf(beg, left_len, ": ");
	if (seg_len >= left_len) {
		goto err;
	}
	beg += seg_len;
	left_len -= seg_len;

	va_start(ap, fmt);
	seg_len = snprintf(beg, left_len, fmt, ap);
	va_end(ap);
	if (seg_len >= left_len) {
		goto err;
	}

	return p;

err:
	free(p);
	return NULL;
}

void init_pass_to_bison(struct pass_to_bison *ctx, struct mem_pool *pool)
{
	ctx->pool = pool;
	ctx->ok = 1;
	ctx->first_line = 1;
	ctx->first_column = 1;
	ctx->last_line = 1;
	ctx->last_column = 1;
	ctx->myerrno = 0;
	ctx->err_reason = NULL;
	ctx->output = NULL;
}

static const char *msg_conflict = "internal error, got impossible result: "
	"ok: %d, myerror: %d, output: %p";



int yacc_parse_file(const char *path, const char **err_msg,
		struct pass_to_bison *ctx)
{
	FILE *fp;
	int ret;
	yyscan_t scanner;

	*err_msg = NULL;
	fp = fopen(path, "r");
	if (!fp) {
		ret = -errno;
		*err_msg = make_message("failed to open config file %s.\n",
				path);
		goto fail_open;
	}
	ret = yylex_init(&scanner);
	if (ret) {
		ret = -errno;
		*err_msg = make_message("failed to create scanner for lex");
		goto err_yylex_init;
	}
	yyset_in(fp, scanner);
	yyparse(scanner, ctx);

	if ((ctx->ok && (ctx->myerrno || !ctx->output)) ||
			(!ctx->ok && (!ctx->myerrno || ctx->output))) {
		ret = -EINVAL;
		*err_msg = make_message(msg_conflict, ctx->ok,
				ctx->myerrno, ctx->output);
		goto err_yacc;
	}
	if (!ctx->output) {
		ret = ctx->myerrno;
		*err_msg = ctx->err_reason;
		goto err_yacc;
	}

	yylex_destroy(scanner);
	fclose(fp);
	return 0;

err_yacc:
	yylex_destroy(scanner);
err_yylex_init:
	fclose(fp);
fail_open:
	return ret;
}

int yacc_parse_string(const char *str, const char **err_msg,
		struct pass_to_bison *ctx)
{
	yyscan_t scanner;
	void *buffer_state;
	int ret;

	*err_msg = NULL;
	ret = yylex_init(&scanner);
	fprintf(stderr, "at %d(%s), %p\n", __LINE__, __func__, str);
	if (ret) {
		ret = -errno;
		goto err_yylex_init;
	}
	fprintf(stderr, "at %d(%s), %p\n", __LINE__, __func__, str);
	buffer_state = yy_scan_string(str, scanner);
	if (!buffer_state) {
		ret = -ENOMEM;
		goto err_scan_string;
	}

	fprintf(stderr, "at %d(%s), %p\n", __LINE__, __func__, str);
	yyparse(scanner, ctx);
	fprintf(stderr, "at %d(%s), %p\n", __LINE__, __func__, str);
	if ((ctx->ok && (ctx->myerrno || !ctx->output)) ||
			(!ctx->ok && (!ctx->myerrno || ctx->output))) {
		ret = -EINVAL;
		*err_msg = make_message(msg_conflict, ctx->ok,
				ctx->myerrno, ctx->output);
		goto err_yacc;
	}
	fprintf(stderr, "at %d(%s), %p\n", __LINE__, __func__, str);
	if (!ctx->output) {
		ret = ctx->myerrno;
		*err_msg = ctx->err_reason;
		goto err_yacc;
	}
	fprintf(stderr, "at %d(%s), %p\n", __LINE__, __func__, str);
	
	yylex_destroy(scanner);
	return 0;

err_yacc:
err_scan_string:
	yylex_destroy(scanner);
err_yylex_init:
	return ret;
}

