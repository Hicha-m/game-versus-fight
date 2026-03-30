#include <stdarg.h>
#include <stdio.h>

#include "utils/log.h"

static void log_vwrite(const char* level, const char* fmt, va_list args)
{
	if (!fmt) {
		return;
	}

	fprintf(stderr, "[%s] ", level);
	vfprintf(stderr, fmt, args);
	fputc('\n', stderr);
}

void log_info(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	log_vwrite("INFO", fmt, args);
	va_end(args);
}

void log_warn(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	log_vwrite("WARN", fmt, args);
	va_end(args);
}

void log_error(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	log_vwrite("ERROR", fmt, args);
	va_end(args);
}
