#include <stdio.h>
#include <stdarg.h>
#include "demo_0-converter.h"
#include "errno.h"

struct cfg config;
struct dump_context {};
void put_func_impl(struct dump_context *context, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

int main(int argc, char **argv)
{
	const char *err_msg;
	int ret;
	if (ret = config_parse_cfg(&config, argv[1], &err_msg)) {
		ret = -ret;
		errno = ret;
		fprintf(stderr, "failed to parse: reason: %d(%m): %s\n",
				ret, err_msg ? err_msg : "");
		free((char *)err_msg);
		return;
	}
	config_dump_cfg(put_func_impl, NULL, &config);
	config_free_cfg(&config);
}
