#include "utils.h"
#include "logging.h"

static LogLevel g_log_level = LOG_LEVEL_INFO;

void log_set_level(LogLevel level) {
	g_log_level = level;
}

void log_write(LogLevel level, const char *message) {
	(void)level;
	(void)message;
	(void)g_log_level;
}


static const char *const g_error_strings[] = {
	"ok",                 /* GAME_OK */
	"invalid argument",  /* GAME_ERROR_INVALID_ARGUMENT */
	"invalid state",     /* GAME_ERROR_INVALID_STATE */
	"out of memory",     /* GAME_ERROR_OUT_OF_MEMORY */
	"out of bounds",     /* GAME_ERROR_OUT_OF_BOUNDS */
	"not found",         /* GAME_ERROR_NOT_FOUND */
	"unsupported",       /* GAME_ERROR_UNSUPPORTED */
	"timeout",           /* GAME_ERROR_TIMEOUT */
	"io error",          /* GAME_ERROR_IO */
	"internal error",    /* GAME_ERROR_INTERNAL */
};

const char *game_error_string(GameError error)
{
	size_t idx = (size_t)error;
	size_t count = sizeof g_error_strings / sizeof *g_error_strings;
	if (idx < count) {
		return g_error_strings[idx];
	}
	return "unknown error";
}
