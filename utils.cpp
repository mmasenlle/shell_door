
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termio.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include "logger.h"
#include "utils.h"
#include "globals.h"

int utils::Tty::config(int fd, int baud, int bits, int parity, int stop)
{
	struct termio	tio;
	int cfgVal = (CLOCAL | CREAD);
	
	switch (baud) {
	case 115200: cfgVal |= B115200; break;
	case 57600: cfgVal |= B57600; break;
	case 38400: cfgVal |= B38400; break;
	case 19200: cfgVal |= B19200; break;
	case  9600: cfgVal |= B9600; break;
	case  4800: cfgVal |= B4800; break;
	case  2400: cfgVal |= B2400; break;
	default: cfgVal |= B4800;
	}
	
	switch (bits) {
	case 5: cfgVal |= CS5; break;
	case 6: cfgVal |= CS6; break;
	case 7: cfgVal |= CS7; break;
	case 8: cfgVal |= CS8; break;
	default: cfgVal |= CS8;
	}
	
	switch (parity)	{
	case 0: break;
	case 1: cfgVal |= PARODD;
	case 2: cfgVal |= PARENB; break;
	}
	
	switch (stop) {
	case 1: break;
	case 2: cfgVal |= CSTOPB; break;
	default: cfgVal |= CSTOPB;
	}
	
	tio.c_iflag = tio.c_oflag = tio.c_lflag = tio.c_line = 0;
	tio.c_cflag = cfgVal;
	tio.c_cc[VMIN] = tio.c_cc[VTIME] = 0;
	
	if ((ioctl(fd, TCSETA, &tio)) == -1) {
//		SELOG("utils::Tty::config -> Ioctl TCSETA failed in fd: %d", fd);
		return(-1);
	}
	
	// Flush colas de entrada y salida
	if ((ioctl(fd, TCFLSH, 2)) == -1) {
//		SELOG("utils::Tty::config -> Ioctl TCFLSH failed in fd: %d", fd);
		return(-1);
	}
	usleep(1000);
	
	return 0;
}

#define MAX_RTSFD 32

int utils::Tty::rtsfd[MAX_RTSFD] = { -1 };

int utils::Tty::initRTS(int fd, const char *rtsdev)
{
	if (rtsdev && fd < MAX_RTSFD) {
		if(rtsfd[fd] != -1) close(rtsfd[fd]);
		rtsfd[fd] = open(rtsdev, O_WRONLY);
	}
	int mcs = TIOCM_RTS;
	return ioctl(fd, TIOCMBIS, &mcs);
}

int utils::Tty::writeRTS(int fd, const void *buf, size_t count)
{
	int r, lsr, mcs = TIOCM_RTS;
//	if((r = ioctl(fd, TIOCMBIC, &mcs)) < 0) return r;
	if((r = ioctl(fd, TIOCMBIS, &mcs)) < 0) return r;
	if((r = write(fd, buf, count)) < 0) goto crts;
	if(fd < MAX_RTSFD && rtsfd[fd] != -1 && write(rtsfd[fd], buf, count) == (int)count) return r;
	do
	{
		usleep(1);
		if(ioctl(fd, TIOCSERGETLSR, &lsr) < 0) goto crts;
	}
	while(!(lsr & TIOCSER_TEMT));
	
crts:
//	ioctl(fd, TIOCMBIS, &mcs);
	ioctl(fd, TIOCMBIC, &mcs);
	return r;
}

utils::Timer::Timer()
: unit(second), interval(1000000L)
{
}

void utils::Timer::set(int interv, int u)
{
	unit = u;
	interval = interv;
	update();
}

void utils::Timer::set_tv(struct timeval *tv, int interv, int u)
{
	switch (u) {
	case second: tv->tv_sec = interv;
		tv->tv_usec = 0; break;
	case mili: tv->tv_sec = interv / 1000;
		tv->tv_usec = (interv % 1000) * 1000; break;
	case micro:  tv->tv_sec = interv / 1000000;
		tv->tv_usec = (interv % 1000000); break;
	}
}

void utils::Timer::update(int interv)
{
	gettimeofday(&next, NULL);
	sec_org = next.tv_sec;
	if (interv) interval = interv;
	switch (unit) {
	case second: next.tv_sec += interval; break;
	case mili: next.tv_sec += interval / 1000;
	next.tv_usec += (interval % 1000) * 1000; break;
	case micro:  next.tv_sec += interval / 1000000;
	next.tv_usec += (interval % 1000000); break;
	}
	while (next.tv_usec >= 1000000) {
		next.tv_sec++;
		next.tv_usec -= 1000000;
	}
//	DLOG("utils::Timer::update(%d)->next:%d,org:%d)-> ", interval, next.tv_sec, sec_org);
}

bool utils::Timer::isTime()
{
	if (t_out() <= 0) {
		update();
		return true;
	}
	return false;
}

int utils::Timer::t_out()
{
	struct timeval auxtv;
	gettimeofday(&auxtv, NULL);
	if ((auxtv.tv_sec < sec_org) || (auxtv.tv_sec > (next.tv_sec + 100))) {
		WLOG("utils::Timer::t_out(%d, %d, %d)-> clock skew detected !!", (int)next.tv_sec, (int)auxtv.tv_sec, sec_org);
		return 0;
	}
	int to = ((next.tv_sec - auxtv.tv_sec) * 1000) + ((next.tv_usec - auxtv.tv_usec) / 1000);
	if (to <= 0) return 0;
	return to;
}

struct tm *utils::LtDateToTime(const char *date, struct tm *ttm)
{
	ttm->tm_isdst = -1; // find out system
	strptime(date, "%Y-%m-%dT%H:%M:%S", ttm);
	return ttm;
}

const char *utils::timeToLtDate(char *buf, time_t t)
{
	struct tm tmaux;
	strftime( buf, 32, "%FT%H:%M:%S", localtime_r(&t, &tmaux));
	return buf;
}

const char *utils::boolToXml(bool b)
{
	return b ? "true" : "false";
}

bool utils::xmlToBool(const char *s)
{
	return (!strcasecmp(s, "true") || !strcmp(s, "1"));
}

std::string & utils::trimmed(std::string & s)
{
	int p = 0;
	while (p < (int)s.length() && isspace(s[p])) p++;
	if(p > 0) s.erase(0, p); p = s.length();
	while(p > 0 && isspace(s[p - 1])) p--;
	if(p < (int)s.length()) s.erase(p);
	return s;
}

std::string & utils::centered(std::string & s, int room)
{
	std::string aux = s; s.clear();
	while ((int)aux.length() < room) {
		room -= 2;
		s.push_back(' ');
	}
	s += aux;
	return s;
}

int utils::getInterfaceIp(const char *name, std::string *sip)
{
	struct ifreq ifrequest[8];
	struct ifconf ifconfig;
	ifconfig.ifc_len = sizeof(ifrequest);
	ifconfig.ifc_req = ifrequest;
	
	int s = socket(AF_INET, SOCK_DGRAM, 0);
 	if(s == -1)
	{
		return -1;
	}
	if(ioctl(s, SIOCGIFCONF, &ifconfig) == -1)
	{
		close(s);
		return -1;
	}
	close(s);
	int count = ifconfig.ifc_len / sizeof(ifrequest[0]);
	for(int i = 0; i < count; i++)
	{
		if(!strncmp(name, ifrequest[i].ifr_name, strlen(name)))
		{
			*sip = inet_ntoa(((sockaddr_in*)&ifrequest[i].ifr_addr)->sin_addr);
			return 1;
		}
	}
	return 0;
}


void utils::GrowBuffer::grow(int min)
{
	size = size ? size * 2 : 4096;
	while(len + min > size) size *= 2;
	buffer = realloc(buffer, size);
	if(!buffer) ELOG("utils::GrowBuffer::grow(%d) -> out of memory !!!!!!! (size: %d, len: %d)", min, size, len);
	assert(buffer);
}
//I.I. 7.2.23
int utils::time_t2int(time_t fecha_sistema)
{
	struct  tm  *fh_tm = localtime(&fecha_sistema);	// fecha sistema
	return ((((fh_tm->tm_year-100) * 100) + (fh_tm->tm_mon+1)) * 100) + fh_tm->tm_mday;
}

time_t utils::int2time_t(int aammdd)
{
	if (aammdd < 100) return 0;
	int year = aammdd / 10000;
	int day = aammdd % 100;
	int month = (aammdd - year) / 100;
	if (!year) { //aamm  (assuming not day)
		year = aammdd / 100;
		month = aammdd % 100;
		day = 31;
		if (month == 4 || month == 6 || month == 9 || month == 11)
			day = 30;
		else if (month == 2)
			day = (year % 4 == 0) ? 29 : 28;
	}
	struct tm fechaTemp;
	fechaTemp.tm_sec = 59;
	fechaTemp.tm_min = 59;
	fechaTemp.tm_hour = 23;
	fechaTemp.tm_mday = day;
	fechaTemp.tm_mon = month - 1;
	fechaTemp.tm_year = year + 100;

	return mktime(&fechaTemp);
}

int utils::count(const char * szStr, char charToFind)
{
	int cnt = 0;
	for(char *ptr=(char*)szStr;*ptr!='\0';ptr++){
		if(*ptr == charToFind)
			cnt++;
	}
	DLOG("utils::count(%s,%c)=%d",szStr,charToFind,cnt);
	return cnt;
}

const char *utils::toString(char *buf, int var, int m)
{
	switch(m)
	{
	case TOSTR_UNSIGNED:	snprintf(buf, 15, "%u", (unsigned)var); break;
	case TOSTR_SIGNED: 		snprintf(buf, 15, "%d", var); break;
	case TOSTR_HEX:  		snprintf(buf, 15, "%x", var); break;
	}
	return buf;
}

const char *utils::toStringll(char *buf, unsigned long long var)
{
	snprintf(buf, 31, "%llu", var);
	return buf;
}

void utils::save_file(const std::string &sbuf, const char *fmt, ...)
{
	char fname[128]; va_list args; va_start( args, fmt );
	vsnprintf( fname, sizeof( fname ), fmt, args ); va_end( args );
	DLOG("utils::save_file(%s)", fname);

	int fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (fd == -1) {
		SELOG("utils::save_file:open(%s)", fname);
		return;
	}
	if (write(fd, sbuf.c_str(), sbuf.length()) != (int)sbuf.length()) {
		SELOG("utils::save_file:write(%s)", fname);
	}
	close(fd);
}

void utils::putTag(std::string *s, int level, const char *tag, const char *value)
{
	char buf[256] = "\t\t\t\t\t\t\t";
	int n = snprintf(buf+level, sizeof(buf)-level, "<%s>%s</%s>\n", tag, value, tag);
	s->append(buf, n + level);
}
void utils::putTag(std::string *s, int level, const char *tag, int value)
{
	char buf[256] = "\t\t\t\t\t\t\t";
	int n = snprintf(buf+level, sizeof(buf)-level, "<%s>%d</%s>\n", tag, value, tag);
	s->append(buf, n + level);
}
void utils::putTag(std::string *s, int level, const char *tag, bool value)
{
	char buf[256] = "\t\t\t\t\t\t\t";
	int n = snprintf(buf+level, sizeof(buf)-level, "<%s>%c</%s>\n", tag, value?'1':'0', tag);
	s->append(buf, n + level);
}
bool utils::isZero(const char *s)
{
	return !(atoi(s) || *s != '0');
}
