#ifndef UTILS_H_
#define UTILS_H_

#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <string>

#if __GNUC__ < 4
#	undef __thread
#	define __thread
#endif

#define LIKELY(exp)   __builtin_expect (!!(exp), 1)
#define UNLIKELY(exp) __builtin_expect (!!(exp), 0)

#define COUNTOF(array) (int)(sizeof (array) / sizeof ((array)[0]))
#define CHECKTT(_to, _tt, _max) (((_to < _tt) || (_to > (_tt + _max))))

#define MS_GET_STR(ss, v) \
	if(v < (int)COUNTOF(ss)) return ss[v]; \
	return ss[(COUNTOF(ss) - 1)];

#define MS_GET_VAL(ss, s) \
	for(int i = 0; i < (int)COUNTOF(ss); i++) \
		if(!strcmp(s, ss[i])) return i; \
	return (COUNTOF(ss) - 1);

enum
{
	TOSTR_UNSIGNED,
	TOSTR_SIGNED,
	TOSTR_HEX,
};

namespace utils
{
	class Tty
	{
		static int rtsfd[];
	public:
		static int config(int fd, int baud, int bits, int parity, int stop);
		static int initRTS(int fd, const char *rtsdev = NULL);
		static int writeRTS(int fd, const void *buf, size_t count);
	};
	class Timer
	{
		int unit;
		int interval;
		struct timeval next;
		int sec_org; // catch skews
	public:
		enum { second, mili, micro };
		Timer();
		void set(int interv, int u = mili);
		void update(int interv = 0);
		bool isTime();
		int t_out(); /* ms */
		int *get_intervp() { return &interval; };
		static void set_tv(struct timeval *tv, int interv, int u = mili);
	};
	class GrowBuffer
	{
		void *buffer;
		int size, len;
		void grow(int min);
		GrowBuffer(const GrowBuffer &);
		GrowBuffer & operator=(const GrowBuffer &);
	public:
		GrowBuffer() : buffer(NULL), size(0), len(0) {};
		~GrowBuffer() { free(buffer); };
		void *ptr() { return buffer; };
		void *wptr() { if(UNLIKELY(size <= len)) grow(0); return (char*)buffer + len; };
		int length() { return len; };
		int room() { if(UNLIKELY(size <= len)) grow(0); return (size - len); };
		void put(int n) { len += n; };
		void clear() { len = 0; };
		void put(const void *data, int n) { if(UNLIKELY(len + n > size)) grow(n); memcpy(wptr(), data, n); put(n); };
	};

	const char *toString(char *buf, int var, int m = TOSTR_SIGNED);
	const char *toStringll(char *buf, unsigned long long var);
	struct tm  *LtDateToTime(const char *date, struct tm *ttm);
	const char *timeToLtDate(char *buf, time_t t);
	const char *timeToLtDateTm(char *buf, struct tm *ttm);
	const char *boolToXml(bool b);
	bool xmlToBool(const char *s);
	bool isZero(const char *s);
	std::string & trimmed(std::string & s);
	std::string & centered(std::string & s, int room);
	
	void putTag(std::string *s, int level, const char *tag, const char *value);
	void putTag(std::string *s, int level, const char *tag, int value);
	void putTag(std::string *s, int level, const char *tag, bool value);

	int getInterfaceIp(const char *name, std::string *sip);

	//I.I -> v7.2.23
	int time_t2int(time_t fecha_sistema);
	time_t int2time_t(int fecha);

	int count(const char* szStr,char charToFind);

	void save_file(const std::string &s, const char *fmt, ...)  __attribute__ ((format (printf, 2, 3)));
}


#endif /*UTILS_H_*/
