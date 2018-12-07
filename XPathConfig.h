#ifndef XPATHCONFIG_H_
#define XPATHCONFIG_H_

#include <string>

#include <libxml/tree.h>
#include <libxml/xpath.h>

class XPathConfig
{
	xmlDocPtr doc;
	xmlXPathContextPtr xpathCtx;

public:
	XPathConfig(const std::string &s); // for buffers
	XPathConfig(const char *fmt, ...) __attribute__ ((format (printf, 2, 3))); // for files
	~XPathConfig();
	int getValue(const std::string &key, std::string *val, int ins = 0);
	int getValue(const std::string &key, int *value, int ins = 0);
	int getValue(const std::string &key, bool *value, int ins = 0);
	int getCount(const std::string &key);
	bool ok();
};

#endif

