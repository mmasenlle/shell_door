
#ifndef DICTIO_H_
#define DICTIO_H_

#include <map>
#include <vector>
#include <string>
#include "XPathConfig.h"

class dict_t;

class Dictio
{
	std::map<std::string, std::map<std::string, Dictio *> > objects;
	std::map<std::string, dict_t *> entries;
	Dictio *getDictio(const std::string & path, std::string *name = NULL);
protected:
	virtual Dictio *createObject(const std::string &name, const std::string &inst) { return new Dictio; };
	virtual void destroyObject(const std::string &name, Dictio *obj) {};
public:
	void getObjects(const std::string &path, std::vector<const std::string *> *objs);
	void getEntries(const std::string &path, std::vector<const std::string *> *ents);

	void getOptions(const std::string &name, std::vector<const char *> *opts);
	const char *optName(const std::string &name, int i = -1);
	void get(const std::string &name, std::string *str);

	void set(const std::string &name, const std::string &value);
	void set(const std::string &name, const char *value);
	void set(const std::string &name, int value);
	void set(const std::string &name, bool value);

	void set(const std::string &name, std::string *var, bool st = true, bool ld = true);
	void set(const std::string &name, int *var, bool st = true, bool ld = true);
	void set(const std::string &name, bool *var, bool st = true, bool ld = true);

	void newObject(const std::string &name, const std::string &inst, Dictio *dictio = NULL);
	void newObject(const std::string &name, int inst, Dictio *dictio = NULL);
	void rmObject(const std::string &name, const std::string &inst);
	void rmObject(const std::string &name, int inst);
	void newOption(const std::string &name, int *val, const char * (*get_str)(int), int (*get_int)(const char *), bool st = true, bool ld = true);

	void load(XPathConfig *xml, const std::string &xkey);
	void xdump(std::string *sbuf, int tabs);
	void ddump(std::string *sbuf, const std::string &path);

	~Dictio();
};

#define DICTIO_SET(_var) Dictio::set( # _var, & _var )
#define DICTIO_SET_VAL(_var, _val) Dictio::set( # _var, & _var ); _var = _val

#endif /*DICTIO_H_*/
