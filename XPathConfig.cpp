
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <unistd.h>
#include <stdarg.h>
#include "logger.h"
#include "utils.h"
#include "XPathConfig.h"

XPathConfig::XPathConfig(const std::string &s) : doc(NULL), xpathCtx(NULL)
{
	xmlInitParser();
	LIBXML_TEST_VERSION

	if((doc = xmlParseMemory(s.c_str(), s.length())))
		xpathCtx = xmlXPathNewContext(doc);
}

XPathConfig::XPathConfig(const char *fmt, ...) : doc(NULL), xpathCtx(NULL)
{
	char fname[128];
	va_list args;
	va_start( args, fmt );
	vsnprintf( fname, sizeof( fname ), fmt, args );
	va_end( args );
	DLOG("XPathConfig::XPathConfig(%s)", fname);

	xmlInitParser();
	LIBXML_TEST_VERSION

	if (access(fname, R_OK) == 0) {
		if ((doc = xmlParseFile(fname)))
			xpathCtx = xmlXPathNewContext(doc);
	}
}

bool XPathConfig::ok()
{
	return (xpathCtx != NULL);
}

XPathConfig::~XPathConfig()
{
	if (xpathCtx)
		xmlXPathFreeContext(xpathCtx);
	if (doc)
		xmlFreeDoc(doc);
	xmlCleanupParser();
}

int XPathConfig::getValue(const std::string &key, std::string *val, int ins)
{  
    xmlXPathObjectPtr xpathObj; 

    if (!doc || !xpathCtx) {
    	DLOG("XPathConfig::getValue -> File not loaded trying %s(%d)", key.c_str(), ins);
        return 0;
    }

    /* Evaluate xpath expression */
    xpathObj = xmlXPathEvalExpression((const xmlChar*)key.c_str(), xpathCtx);
    if (xpathObj == NULL) {
    	ELOG("XPathConfig::getValue -> Unable to evaluate xpath expression \"%s\"", key.c_str());
        return 0;
    }

    if (xpathObj->nodesetval == NULL) {	//no existe el nodo
        xmlXPathFreeObject(xpathObj);
        return 0;
    }

    int count = xpathObj->nodesetval->nodeNr;
    DLOG("XPathConfig::getValue(%s) -> nodes: %d", key.c_str(), count);
    if (count <= ins) { //no hay nodos
    	xmlXPathFreeObject(xpathObj);
        return 0;
    }

    if (val) {
    	xmlChar *xc = xmlNodeGetContent(xpathObj->nodesetval->nodeTab[ins]);
	    if (!xc || !(*xc)) { //contenido vacio
	    	DLOG("XPathConfig::getValue(%s) -> val: %s", key.c_str(), xc);
	        xmlXPathFreeObject(xpathObj);
	        return 0;
	    }
     	utils::trimmed((*val = (const char*)xc));
    	xmlFree(xc);
    	DLOG("XPathConfig::getValue(%s) -> val: %s", key.c_str(), val->c_str());
    }

    /* Cleanup of XPath data */
    xmlXPathFreeObject(xpathObj);
    return count;
}

int XPathConfig::getValue(const std::string &key, int *val, int ins)
{
	std::string value; int r = getValue(key, &value, ins);
	if (r && val) {
		int ival = atoi(value.c_str());
		if (ival || *value.c_str() == '0') *val = ival;
		DLOG("XPathConfig::getValue_int(%s): %d", key.c_str(), ival);
	}
	return r;
}
int XPathConfig::getValue(const std::string &key, bool *val, int ins)
{
	std::string value; int r = getValue(key, &value, ins);
	if (r && val) {
		if (*value.c_str() == '0' || !strcasecmp(value.c_str(), "false")) *val = false;
		if (*value.c_str() == '1' || !strcasecmp(value.c_str(), "true")) *val = true;
	}
	return r;
}
int XPathConfig::getCount(const std::string &key)
{
	DLOG("XPathConfig::getCount(%s)", key.c_str());
	return getValue(key, (std::string *)NULL, 0);
}
