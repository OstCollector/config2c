/*
 * This file is part of config2c which is relased under Apache License.
 * See LICENSE for full license details.
 */

#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include "parser.h"

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


