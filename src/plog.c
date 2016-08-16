#include "plog.h"

void log_printf(unsigned int level, char *fmt, ...){

	char line[8192];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(line, sizeof(line), fmt, ap);
	va_end(ap);

	switch(level){
		case ZLOG_LEVEL_DEBUG:
			zlog_debug(dbg_log, line);
			break;
		case ZLOG_LEVEL_INFO:
			zlog_info(inf_log, line);
			break;
		case ZLOG_LEVEL_NOTICE:
			zlog_notice(ntc_log, line);
			break;
		case ZLOG_LEVEL_WARN:
			zlog_warn(wrn_log, line);
			break;
		case ZLOG_LEVEL_ERROR:
			zlog_error(err_log, line);
			break;
		case ZLOG_LEVEL_FATAL:
			zlog_fatal(ftl_log, line);
			break;
		default:
			break;
	}
}
