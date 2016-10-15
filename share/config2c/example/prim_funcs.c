/*
 * This file is part of config2c which is relased under Apache License.
 * See LICENSE for full license details.
 */

#include <stdint.h>

#define IP4_ADDR_MAX (15)
#define IP4_PRE_MAX (2)

#define IP6_ADDR_MAX (65) /* should be enough, 16 bytes * 4 */
#define IP6_PRE_MAX (3)

/* return 0 if converted, otherwise return -errno, most likely -ERANGE */
static int to_longlong(long long *out, const char *in)
{
	long long ret;
	errno = 0;
	ret = strtoll(in, NULL, 0);
	if (!errno) {
		*out = ret;
		return 0;
	} else {
		return -errno;
	}
}

/* return 0 if converted, otherwise return -errno, most likely -ERANGE */
static int to_ulonglong(unsigned long long *out, const char *in)
{
	unsigned long long ret;
	if (in[0] == '-') {
		return -EINVAL;
	}
	errno = 0;
	ret = strtoull(in, NULL, 0);
	if (!errno) {
		*out = ret;
		return 0;
	} else {
		return -errno;
	}
}

static int xdigit_to_int(int c)
{ return '0' <= c && c <= '9' ? c - '0' : 
	'a' <= c && c <= 'f' ? c - 'a' : c - 'A'; }

/* return >0 if converted, -EINVAL otherwise */
static int unescape(char *out, const char *in)
{
	if (*in != '\'') {
		*out = *in;
		return 1;
	}
	++in;
	switch (*in) {
	case '\0':
		return -EINVAL;
	case 'a' :
		*out = '\a';
		return 2;
	case 'b':
		*out = '\b';
		return 2;
	case 'f':
		*out = '\f';
		return 2;
	case 'n':
		*out = '\n';
		return 2;
	case 'r':
		*out = '\r';
		return 2;
	case 't':
		*out = '\t';
		return 2;
	case 'v':
		*out = '\v';
		return 2;
	case '\\':
		*out = '\\';
		return 2;
	case '\"':
		*out = '\"';
		return 2;
	case '\'':
		*out = '\'';
		return 2;
	case '\?':
		*out = '\?';
		return 2;
	case 'x':
		if (!isxdigit(in[1]) || !isxdigit(in[2])) {
			return -EINVAL;
		}
		*out = (xdigit_to_int(in[1]) << 4) | xdigit_to_int(in[2]);
		return 4;
	default:
		return -EINVAL;
	}
}

/*
 * Parse values, return 0 if succeed, -errno otherwise
 */
/* return 0 if ok, -errno otherwise */
static int parse_char(struct pass_to_conv *context, char *result, const struct node_value *val)
{
	int ret;
	switch (val->type) {
	case VAL_SCALE_CHAR:
		ret = unescape(result, val->char_str);
		if (ret > 0) {	/* parsed */
			if (!val->char_str[ret]) {	/* still have something */
				context->node = val;
				context->msg = "value cannot be converted to a character.";
				return -EINVAL;
			} else {
				return 0;
			}
		}
		context->node = val;
		context->msg = "value cannot be converted to a character.";
		return -EINVAL;
	default:
		context->node = val;
		context->msg = "wrong type, expect char.";
		return -EINVAL;
	}
}

static int parse_schar(struct pass_to_conv *context, signed char *result, const struct node_value *val)
{
	int ret;
	long long i;
	switch (val->type) {
	case VAL_SCALE_CHAR:
		ret = unescape((char *)result, val->char_str);
		if (ret > 0) {	/* parsed */
			if (!val->char_str[ret]) {	/* still have something */
				context->node = val;
				context->msg = "value cannot be converted to a character.";
				return -EINVAL;
			} else {
				return 0;
			}
		}
		context->node = val;
		context->msg = "value cannot be converted to a character.";
		return -EINVAL;
	case VAL_SCALE_INT:
		ret = to_longlong(&i, val->int_str);
		if (ret) {	/* failed */
			context->node = val;
			context->msg = "overflow occurred.";
			return ret;
		}
		if (i < SCHAR_MIN || i > SCHAR_MAX) {
			context->node = val;
			context->msg = "overflow occurred.";
			return -ERANGE;
		}
		*result = i;
		return 0;
	default:
		context->node = val;
		context->msg = "wrong type, expect char or integer.";
		return -EINVAL;
	}
}

static int parse_uchar(struct pass_to_conv *context, unsigned char *result, const struct node_value *val)
{
	int ret;
	unsigned long long i;
	switch (val->type) {
	case VAL_SCALE_CHAR:
		ret = unescape((char *)result, val->char_str);
		if (ret > 0) {	/* parsed */
			if (!val->char_str[ret]) {	/* still have something */
				context->node = val;
				context->msg = "value cannot be converted to a character.";
				return -EINVAL;
			} else {
				return 0;
			}
		}
		context->node = val;
		context->msg = "value cannot be converted to a character.";
		return -EINVAL;
	case VAL_SCALE_INT:
		ret = to_ulonglong(&i, val->int_str);
		if (ret) {	/* failed */
			context->node = val;
			context->msg = "overflow occurred.";
			return ret;
		}
		if (i > UCHAR_MAX) {
			context->node = val;
			context->msg = "overflow occurred.";
			return -ERANGE;
		}
		*result = i;
		return 0;
	default:
		context->node = val;
		context->msg = "wrong type, expect char or integer.";
		return -EINVAL;
	}
}

static void char_dump(put_func func, struct dump_context *ctx, const char *c)
{
	if (0x20 <= *c && *c <= 126) {
		func(ctx, "\'%c\'", *c);
	} else {
		func(ctx, "\'x%hhx\'", (unsigned char)*c);
	}
}

static void dump_char(put_func func, struct dump_context *ctx, const char *c)
{
	char_dump(func, ctx, c);
}

static void dump_uchar(put_func func, struct dump_context *ctx, const signed char *c)
{
	char_dump(func, ctx, (const char *)c);
}

static void dump_schar(put_func func, struct dump_context *ctx, const unsigned char *c)
{
	char_dump(func, ctx, (const char *)c);
}

static void free_char(char *val) {}
static void free_schar(signed char *val) {}
static void free_uchar(unsigned char *val) {}

#define signed_def(def_type, map_type, min, max) \
	static int parse_##def_type(struct pass_to_conv *context, map_type *result, const struct node_value *val) \
	{ \
		int ret; \
		long long i; \
		switch (val->type) { \
		case VAL_SCALE_INT: \
			ret = to_longlong(&i, val->int_str); \
			if (ret) { \
				context->node = val; \
				context->msg = "overflow occurred."; \
				return ret; \
			} \
			if (i < min || i > max) { \
				context->node = val; \
				context->msg = "overflow occurred."; \
				return -ERANGE; \
			} \
			*result = i; \
			return 0; \
		default: \
			context->node = val; \
			context->msg = "wrong type, expect integer."; \
			return -EINVAL; \
		} \
	} \
	\
	static void dump_##def_type(put_func func, struct dump_context *ctx, const map_type *val) \
	{ \
		func(ctx, "%lld", (long long)*val); \
	} \
	\
	static void free_##def_type(map_type *val) {}

#define unsigned_def(def_type, map_type, max) \
	static int parse_##def_type(struct pass_to_conv *context, map_type *result, const struct node_value *val) \
	{ \
		int ret; \
		unsigned long long i; \
		switch (val->type) { \
		case VAL_SCALE_INT: \
			ret = to_longlong(&i, val->int_str); \
			if (ret) { \
				context->node = val; \
				context->msg = "overflow occurred."; \
				return ret; \
			} \
			if (i > max) { \
				context->node = val; \
				context->msg = "overflow occurred."; \
				return -ERANGE; \
			} \
			*result = i; \
			return 0; \
		default: \
			context->node = val; \
			context->msg = "wrong type, expect integer."; \
			return -EINVAL; \
		} \
	} \
	\
	static void dump_##def_type(put_func func, struct dump_context *ctx, const map_type *val) \
	{ \
		func(ctx, "%lld", (long long)*val); \
	} \
	\
	static void free_##def_type(map_type *val) {}

signed_def(short, short, SHRT_MIN, SHRT_MAX)
unsigned_def(ushort, unsigned short, USHRT_MAX)
signed_def(int, int, INT_MIN, INT_MAX)
unsigned_def(uint, unsigned, UINT_MAX)
signed_def(long, long, LONG_MIN, LONG_MAX)
unsigned_def(ulong, unsigned long, ULONG_MAX)
signed_def(llong, long long, LLONG_MIN, LLONG_MAX)
unsigned_def(ullong, unsigned long long, ULLONG_MAX)

signed_def(s8, int8_t, INT8_MIN, INT8_MAX)
unsigned_def(u8, uint8_t, UINT8_MAX)
signed_def(s16, int16_t, INT16_MIN, INT16_MAX)
unsigned_def(u16, uint16_t, UINT16_MAX)
signed_def(s32, int32_t, INT32_MIN, INT32_MAX)
unsigned_def(u32, uint32_t, UINT32_MAX)
signed_def(s64, int64_t, INT64_MIN, INT64_MAX)
unsigned_def(u64, uint64_t, UINT64_MAX)

#define fp_gen(func_name, out_type, convert_func) \
	static int func_name(struct pass_to_conv *context, out_type *result, const struct node_value *val) \
	{ \
		out_type conved; \
		const char *value; \
		switch (val->type) { \
		case VAL_SCALE_INT: \
			value = val->int_str; \
			break; \
		case VAL_SCALE_FLOAT: \
			value = val->float_str; \
			break; \
		default: \
			context->node = val; \
			context->msg = "wrong type, expect integer and float."; \
			return -EINVAL; \
		} \
		errno = 0; \
		conved = convert_func(value, NULL); \
		if (errno) { \
			context->node = val; \
			context->msg = "overflow or underflow occurred."; \
			return -errno; \
		} else { \
			*result = conved; \
			return 0; \
		} \
	}

#define fp_dump(func_name, in_type, fmt) \
	static void func_name(put_func func, struct dump_context *ctx, const in_type *val) \
	{ \
		func(ctx, fmt, *val); \
	}

fp_gen(parse_float, float, strtof)
fp_dump(dump_float, float, "%f");
fp_gen(parse_double, double, strtod)
fp_dump(dump_double, double, "%f");
fp_gen(parse_ldouble, long double, strtold)
fp_dump(dump_ldouble, long double, "%Lf");
static void free_float(float *net) {}
static void free_double(double *ret) {}
static void free_ldouble(long double *ret) {}

static int parse_string(struct pass_to_conv *context, const char **result,
		const struct node_value *val)
{
	size_t len;
	int t;
	char *malloced, *dst;
	const char *src;
	if (val->type != VAL_SCALE_STRING) {
		context->node = val; \
		context->msg = "wrong type, expect string."; \
		return -EINVAL;
	}
	len = strlen(val->string_str);
	if (len >= SIZE_MAX) {
		context->node = val; \
		context->msg = "string is too long."; \
		return -EFBIG;
	}
	malloced = malloc(len + 1);
	if (!malloced) {
		context->node = val; \
		context->msg = "memory insufficient."; \
		return -ENOMEM;
	}
	src = val->string_str;
	dst = malloced;
	while (*src) {
		t = unescape(dst, src);
		if (t < 0) {
			context->node = val; \
			context->msg = "is not a valid string."; \
			free(malloced);
			return t;
		}
		src += t;
		++dst;
	}
	*dst = '\0';
	*result = (const char *)malloced;
	return 0;
}

static void dump_string(put_func func, struct dump_context *ctx, const char * const*val)
{
	const char *c = *val;
	func(ctx, "\"");
	while (*c) {
		if (*c == '"' || *c == '\\') {
			func(ctx, "\\%c", *c);
		}else if (0x20 <= *c && *c <= 126) {
			func(ctx, "%c", *c);
		} else {
			func(ctx, "x%hhx", (unsigned char)*c);
		}
		++c;
	}
	func(ctx, "\"");
}

/*
 * Free corresponding resources
 */
static void free_string(const char **val)
{
	free((char *)(*val));
	*val = NULL;
}


static int parse_inet4(struct pass_to_conv *context, struct in_addr *result,
		const struct node_value *val)
{
	switch (val->type) {
	case VAL_SCALE_STRING:
		if (inet_pton(AF_INET, val->string_str, result)) {
			return 0;
		} else {
			context->node = val; \
			context->msg = "is not a valid ipv4 address string."; \
			return -EINVAL;
		}
	default:
		context->node = val; \
		context->msg = "wrong type, expect string (in inet4)."; \
		return -EINVAL;
	}
}

static void dump_inet4(put_func func, struct dump_context *ctx, const struct in_addr *val)
{
	char addr_str[IP4_ADDR_MAX + 1];
	if (!inet_ntop(AF_INET, val, addr_str, IP4_ADDR_MAX)) {
		fprintf(stderr, "%s: buffer length requirement estimation error.\n", __func__);
		return;
	}
	func(ctx, "\"%s\"", addr_str);
}

static int pasrse_inet6(struct pass_to_conv *context, struct in6_addr *result,
		const struct node_value *val)
{
	switch (val->type) {
	case VAL_SCALE_STRING:
		if (inet_pton(AF_INET6, val->string_str, result)) {
			return 0;
		} else {
			context->node = val; \
			context->msg = "is not a valid ipv6 address string."; \
			return -EINVAL;
		}
	default:
		context->node = val; \
		context->msg = "wrong type, expect string (in inet6)."; \
		return -EINVAL;
	}
}

static void dump_inet6(put_func func, struct dump_context *ctx, const struct in6_addr *val)
{
	char addr_str[IP6_ADDR_MAX + 1];
	if (!inet_ntop(AF_INET6, val, addr_str, IP6_ADDR_MAX)) {
		fprintf(stderr, "%s: buffer length requirement estimation error.\n", __func__);
		return;
	}
	func(ctx, "\"%s\"", addr_str);
}

static int parse_inet4wp(struct pass_to_conv *context, struct in_addr *ip,
		int *prefix, const struct node_value *val)
{
	char *slash;
	char addr_str[IP4_ADDR_MAX + 1];
	char prefix_str[IP4_PRE_MAX + 1], *prefix_end;
	long my_prefix;
	struct in_addr my_ip;
	if (val->type != VAL_SCALE_STRING) {
		context->node = val;
		context->msg = "wrong type, expect string (in inet4 with prefix)."; 
		return -EINVAL;
	}

	slash = strstr(val->string_str, "/");
	if (!slash) {
		context->node = val;
		context->msg = "missing slash.";
		return -EINVAL;
	}

	if (slash - val->string_str > IP4_ADDR_MAX) {
		context->node = val;
		context->msg = "address part is invalid.";
		return -EINVAL;
	}
	memcpy(addr_str, val->string_str, slash - val->string_str);
	addr_str[slash - val->string_str] = '\0';
	++slash;
	if (strlen(slash) > IP4_PRE_MAX) {
		context->node = val;
		context->msg = "prefix part is invalid.";
		return -EINVAL;
	}
	strcpy(prefix_str, slash);
	
	if (!inet_pton(AF_INET, addr_str, &my_ip)) {
		context->node = val;
		context->msg = "addr part is invalid.";
		return -EINVAL;
	}
	errno = 0;
	my_prefix = strtol(prefix_str, &prefix_end, 10);
	if (my_prefix < 0 || my_prefix > 32 || *prefix_end) {
		context->node = val;
		context->msg = "prefix part is invalid.";
		return -EINVAL;
	}
	*ip = my_ip;
	*prefix = my_prefix;
	return 0;
}

static void dump_inet4wp(put_func func, struct dump_context *ctx, const struct in_addr *val, const int *prefix)
{
	char addr_str[IP4_ADDR_MAX + 1];
	if (!inet_ntop(AF_INET, val, addr_str, IP4_ADDR_MAX)) {
		fprintf(stderr, "%s: buffer length requirement estimation error.\n", __func__);
		return;
	}
	func(ctx, "\"%s/%d\"", addr_str, *prefix);
}

static int parse_inet6wp(struct pass_to_conv *context, struct in6_addr *ip,
		int *prefix, const struct node_value *val)
{
	char *slash;
	char addr_str[IP6_ADDR_MAX + 1];
	char prefix_str[IP6_PRE_MAX + 1], *prefix_end;
	long my_prefix;
	struct in6_addr my_ip;
	if (val->type != VAL_SCALE_STRING) {
		context->node = val; \
		context->msg = "wrong type, expect string (in inet6 with prefix)."; \
		return -EINVAL;
	}

	slash = strstr(val->string_str, "/");
	if (!slash) {
		context->node = val;
		context->msg = "missing slash.";
		return -EINVAL;
	}

	if (slash - val->string_str > IP6_ADDR_MAX) {
		context->node = val;
		context->msg = "address part is invalid.";
		return -EINVAL;
	}
	memcpy(addr_str, val->string_str, slash - val->string_str);
	addr_str[slash - val->string_str] = '\0';
	++slash;
	if (strlen(slash) > IP6_PRE_MAX) {
		context->node = val;
		context->msg = "prefix part is invalid.";
		return -EINVAL;
	}
	strcpy(prefix_str, slash);
	
	if (!inet_pton(AF_INET6, addr_str, &my_ip)) {
		context->node = val;
		context->msg = "addr part is invalid.";
		return -EINVAL;
	}
	errno = 0;
	my_prefix = strtol(prefix_str, &prefix_end, 10);
	if (my_prefix < 0 || my_prefix > 128 || *prefix_end) {
		context->node = val;
		context->msg = "prefix part is invalid.";
		return -EINVAL;
	}
	*ip = my_ip;
	*prefix = my_prefix;
	return 0;
}

static void dump_inet6wp(put_func func, struct dump_context *ctx, const struct in6_addr *val, const int *prefix)
{
	char addr_str[IP6_ADDR_MAX + 1];
	if (!inet_ntop(AF_INET6, val, addr_str, IP6_ADDR_MAX)) {
		fprintf(stderr, "%s: buffer length requirement estimation error.\n", __func__);
		return;
	}
	func(ctx, "\"%s/%d\"", addr_str, *prefix);
}

static void free_inet4(struct in_addr *net) {}
static void free_inet6(struct in6_addr *net) {}
static void free_inet4wp(struct in_addr *net, int *val) {}
static void free_inet6wp(struct in6_addr *net, int *val) {}

static int parse_eth_mac(struct pass_to_conv *context, struct eth_mac *mac, const struct node_value *val)
{
	int i;
	char t;
	if (val->type != VAL_SCALE_STRING) {
		context->node = val;
		context->msg = "wrong type, expect string (in mac address)."; \
		return -EINVAL;
	}
	i = sscanf(val->string_str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx%c",
			&mac->a[0], &mac->a[1], &mac->a[2], &mac->a[3],
			&mac->a[4], &mac->a[5], &t);
	if (i == 6) {
		return 0;
	}
	i = sscanf(val->string_str, "%hhx-%hhx-%hhx-%hhx-%hhx-%hhx%c",
			&mac->a[0], &mac->a[1], &mac->a[2], &mac->a[3],
			&mac->a[4], &mac->a[5], &t);
	if (i == 6) {
		return 0;
	}
	context->node = val;
	context->msg = "is not a valid mac address string."; \
	return -EINVAL;
}

static int dump_eth_mac(put_func func, struct dump_context *ctx, const struct eth_mac *val)
{
	func(ctx, "\"%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\"",
			val->a[0], val->a[1], val->a[2], val->a[3], val->a[4], val->a[5]);
}

static int free_eth_mac(struct eth_mac *val) {}

