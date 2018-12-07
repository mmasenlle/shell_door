#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include "logger.h"

/*__thread*/ Logger *Logger::defaultLogger = NULL;

Logger::Logger(const char *fn) : fd(1), level(START_LEVEL)
{
	if (fn)	{
		if ((fd = open(fn, O_WRONLY | O_CREAT | O_APPEND, 0660)) == -1)	{
			fd = 1;
			log_SYSERR_L("Logger::Logger(%s)-> setting stdout as output", fn);
		}
	}
}

void Logger::log_pre(std::string *s)
{
	struct timeval tv; gettimeofday( &tv, NULL );
	struct tm tm; localtime_r( &tv.tv_sec, &tm );
	char buf[128]; int n = strftime( buf, sizeof( buf ), "%F %k:%M:%S", &tm );
	*s = "["; s->append(buf, n); *s += ",";
	n = sprintf(buf, "%03ld", tv.tv_usec / 1000);
	s->append(buf, n); *s += "] ";
}

void Logger::log_SYSERR_L(const char *fmt, ...)
{
	std::string s; log_pre(&s);
	s += "[SYSERR ";
	char buf[500]; int n = snprintf(buf, sizeof(buf), "%d-", errno);
	s.append(buf, n);
	s += strerror_r(errno, buf, sizeof(buf)); s += "] ";
	va_list args; va_start( args, fmt );
	n = vsnprintf( buf, sizeof( buf ), fmt, args );
	va_end( args ); s.append(buf, n); s += "\n";
	write(fd, s.c_str(), s.length());
#if defined(STD_OUTPUT) && (LOG_COMPILATION_LEVEL >= DEBUG_L)
	if(fd > 1)
		write(1, s->c_str(), s->length());
#elif defined(STD_ERROR)
	if(fd > 2)
		write(2, s.c_str(), s.length());
#endif
}

void Logger::log_ERROR_L(const char *fmt, ...)
{
	std::string s; log_pre(&s);
	s += "[ERROR] ";
	va_list args; va_start( args, fmt ); char buf[500];
	int n = vsnprintf( buf, sizeof( buf ), fmt, args );
	va_end( args ); s.append(buf, n); s += "\n";
	write(fd, s.c_str(), s.length());
#if defined(STD_OUTPUT) && (LOG_COMPILATION_LEVEL >= DEBUG_L)
	if(fd > 1)
		write(1, s->c_str(), s->length());
#elif defined(STD_ERROR)
	if(fd > 2)
		write(2, s.c_str(), s.length());
#endif
}

void Logger::log_WARN_L(const char *fmt, ...)
{
	std::string s; log_pre(&s);
	s += "[WARN ] ";
	va_list args; va_start( args, fmt ); char buf[500];
	int n = vsnprintf( buf, sizeof( buf ), fmt, args );
	va_end( args ); s.append(buf, n); s += "\n";
	write(fd, s.c_str(), s.length());
#if defined(STD_OUTPUT) && (LOG_COMPILATION_LEVEL >= DEBUG_L)
	if(fd > 1)
		write(1, s->c_str(), s->length());
#endif
}

void Logger::log_INFO_L(const char *fmt, ...)
{
	std::string s; log_pre(&s);
	s += "[INFO ] ";
	va_list args; va_start( args, fmt ); char buf[500];
	int n = vsnprintf( buf, sizeof( buf ), fmt, args );
	va_end( args ); s.append(buf, n); s += "\n";
	write(fd, s.c_str(), s.length());
#if defined(STD_OUTPUT) && (LOG_COMPILATION_LEVEL >= DEBUG_L)
	if(fd > 1)
		write(1, s->c_str(), s->length());
#endif
}

void Logger::log_DEBUG_L(const char *fmt, ...)
{
	std::string s; log_pre(&s);
	s += "[DEBUG] ";
	va_list args; va_start( args, fmt ); char buf[500];
	int n = vsnprintf( buf, sizeof( buf ), fmt, args );
	va_end( args ); s.append(buf, n); s += "\n";
	write(fd, s.c_str(), s.length());
#if defined(STD_OUTPUT) && (LOG_COMPILATION_LEVEL >= DEBUG_L)
	if(fd > 1)
		write(1, s->c_str(), s->length());
#endif
}

void Logger::dump(const char *buf, int len) { ::write(fd, buf, len); };

Logger::~Logger()
{
	if (fd > 2)	{
		close(fd);
	}
}

