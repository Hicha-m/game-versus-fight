#include "utils.h"

static LogLevel g_log_level = LOG_LEVEL_INFO;

void log_set_level(LogLevel level) {
	g_log_level = level;
}

void log_write(LogLevel level, const char *message) {
	(void)level;
	(void)message;
	(void)g_log_level;
}
