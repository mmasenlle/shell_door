
#ifndef __LOGGER_H_
#define __LOGGER_H_

#include <string>

#define SYSERR_L	1
#define ERROR_L		2
#define WARN_L 		3
#define INFO_L 		4
#define DEBUG_L		5

/*
 * Configuration of the logger behavior
 * LOG_COMPILATION_LEVEL usefull with macros
 * STD_OUTPUT also logs over stdout when level == DEBUG
 * STD_ERROR also logs over stderr when level == ERROR
 */
#define START_LEVEL 0
#ifndef LOG_COMPILATION_LEVEL
#	define LOG_COMPILATION_LEVEL	DEBUG_L
#	undef START_LEVEL
#	define START_LEVEL DEBUG_L
#endif
//#define STD_OUTPUT
//#define STD_ERROR


class Logger
{
	int fd;
	void log_pre(std::string *str);
public:
	int level;
	static /*__thread*/ Logger *defaultLogger;
	Logger(const char *fn = NULL);
	~Logger();
	void log_SYSERR_L(const char *fmt, ...)  __attribute__ ((format (printf, 2, 3)));
	void log_ERROR_L(const char *fmt, ...) __attribute__ ((format (printf, 2, 3)));
	void log_WARN_L(const char *fmt, ...) __attribute__ ((format (printf, 2, 3)));
	void log_INFO_L(const char *fmt, ...) __attribute__ ((format (printf, 2, 3)));
	void log_DEBUG_L(const char *fmt, ...) __attribute__ ((format (printf, 2, 3)));
	void dump(const char *buf, int len);
};

#define INIT_DEFAULT_LOGGER if(!Logger::defaultLogger) Logger::defaultLogger = new Logger
#define END_DEFAULT_LOGGER delete Logger::defaultLogger; Logger::defaultLogger = NULL
#define SET_LOGGER_LEVEL(l) if(Logger::defaultLogger) Logger::defaultLogger->level = l

#define __PRE_LOG(lev) do { if(__builtin_expect(!!(Logger::defaultLogger), 1) && __builtin_expect((Logger::defaultLogger->level >= lev), 0)) Logger::defaultLogger->log_ ## lev

#define SELOG(fmt, args...) __PRE_LOG(SYSERR_L) (fmt, ## args) ; } while(0)
#define ELOG(fmt, args...)  __PRE_LOG(ERROR_L)  (fmt, ## args) ; } while(0)
#define WLOG(fmt, args...)
#define ILOG(fmt, args...)
#define DLOG(fmt, args...)
#define DUMPLOG(buf, len)

#if LOG_COMPILATION_LEVEL >= WARN_L
#	undef WLOG
#	define WLOG(fmt, args...) __PRE_LOG(WARN_L) (fmt, ## args) ; } while(0)
#	if LOG_COMPILATION_LEVEL >= INFO_L
#		undef ILOG
#		define ILOG(fmt, args...) __PRE_LOG(INFO_L) (fmt, ## args) ; } while(0)
#		if LOG_COMPILATION_LEVEL >= DEBUG_L
#			undef DLOG
#			define DLOG(fmt, args...) __PRE_LOG(DEBUG_L) (fmt, ## args) ; } while(0)
#			undef DUMPLOG
#			define DUMPLOG(buf, len) do { if(Logger::defaultLogger) Logger::defaultLogger->dump(buf, len); } while(0)
#		endif
#	endif
#endif

#endif

