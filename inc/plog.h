#ifndef PLOG_H_
#define PLOG_H_

#include <zlog.h>

#define LOG4C_CATEGORY_FTL "FTL"
#define LOG4C_CATEGORY_ERR "ERR"
#define LOG4C_CATEGORY_WRN "WRN"
#define LOG4C_CATEGORY_DBG "DBG"
#define LOG4C_CATEGORY_INF "INF"
#define LOG4C_CATEGORY_NTC "NTC"

zlog_category_t* ftl_log;
zlog_category_t* err_log;
zlog_category_t* wrn_log;
zlog_category_t* dbg_log;
zlog_category_t* inf_log;
zlog_category_t* ntc_log;

void log_printf(unsigned int level, char *fmt, ...);

#endif /* PLOG_H_ */
