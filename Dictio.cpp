
#include "utils.h"
#include "Dictio.h"

class dict_t
{
public:
	struct {
		unsigned vtype:8;
		unsigned save:1;
		unsigned load:1;
		unsigned created:1;
	} mode;
	void *var;
	dict_t() { mode.vtype = 0; mode.save = 1; mode.load = 1; mode.created = 0; var = 0; };
	~dict_t();
};

enum { dic_type_empty, dic_type_string, dic_type_int, dic_type_bool, dic_type_enum, last_dic_type };

struct dict_enum_t
{
	int *actual;
	const char * (*get_str)(int);
	int (*get_int)(const char *);
};

dict_t::~dict_t()
{
	if (mode.created) {
		switch (mode.vtype) {
		case dic_type_string: delete (std::string *)var; break;
		case dic_type_int: delete (int*)var; break;
		case dic_type_bool: delete (bool*)var; break;
		case dic_type_enum:	delete (dict_enum_t*)var; break;
		}
	}
}

void Dictio::getObjects(const std::string &path, std::vector<const std::string *> *objs)
{
	std::string inst; Dictio *d = getDictio(path, &inst);
	if (d->objects.find(inst) != d->objects.end()) {
		for (std::map<std::string, Dictio *>::iterator i = d->objects[inst].begin(); i != d->objects[inst].end(); i++) {
			objs->push_back(&i->first);
		}
	} else {
		for (std::map<std::string, std::map<std::string, Dictio *> >::iterator i = d->objects.begin(); i != d->objects.end(); i++) {
			objs->push_back(&i->first);
		}
	}
}
void Dictio::getEntries(const std::string &path, std::vector<const std::string *> *ents)
{
	std::string name; Dictio *d = getDictio(path, &name);
	for (std::map<std::string, dict_t *>::iterator i = d->entries.begin(); i != d->entries.end(); i++) {
		ents->push_back(&i->first);
	}
}
void Dictio::getOptions(const std::string &path, std::vector<const char *> *opts)
{
	std::string name; Dictio *d = getDictio(path, &name);
	std::map<std::string, dict_t *>::iterator it = d->entries.find(name);
	if (it != d->entries.end()) {
		if (it->second->mode.vtype == dic_type_enum) {
			dict_enum_t *denum = (dict_enum_t *)it->second->var;
			opts->push_back(denum->get_str(0));
			for (int i = 1; denum->get_str(i) && (opts->back() != denum->get_str(i)); i++) {
				opts->push_back(denum->get_str(i));
			}
		} else if (it->second->mode.vtype == dic_type_bool) {
			opts->push_back("true");opts->push_back("false");
		}
	}
}
const char *Dictio::optName(const std::string &path, int i)
{
	std::string name; Dictio *d = getDictio(path, &name);
	std::map<std::string, dict_t *>::iterator it = d->entries.find(name);
	if (it != d->entries.end()) {
		if (it->second->mode.vtype == dic_type_enum) {
			dict_enum_t *denum = (dict_enum_t *)it->second->var;
			if (i >= 0) {
				return denum->get_str(i);
			} else {
				return denum->get_str(*denum->actual);
			}
		}
	}
	return NULL;
}
void Dictio::get(const std::string &path, std::string *str)
{
	std::string name; Dictio *d = getDictio(path, &name);
	std::map<std::string, dict_t *>::iterator it = d->entries.find(name);
	if (it != d->entries.end()) {
		switch (it->second->mode.vtype) {
		case dic_type_string: *str = *(std::string *)it->second->var; break;
		case dic_type_int: char buf[32]; *str = utils::toString(buf, *(int*)it->second->var); break;
		case dic_type_bool: *str = (*(bool*)it->second->var)?"true":"false"; break;
		case dic_type_enum: { int n = 0; if (((dict_enum_t*)it->second->var)->actual) n = *((dict_enum_t*)it->second->var)->actual;
			*str = ((dict_enum_t*)it->second->var)->get_str(n); } break;
		}
	}
}
void Dictio::set(const std::string &path, const char *value)
{
	std::string name; Dictio *d = getDictio(path, &name);
	std::map<std::string, dict_t *>::iterator it = d->entries.find(name);
	if (it != d->entries.end()) {
		switch (it->second->mode.vtype) {
		case dic_type_string: *(std::string *)it->second->var = value; break;
		case dic_type_int: *(int*)it->second->var = atoi(value); break;
		case dic_type_bool: *(bool*)it->second->var = utils::xmlToBool(value); break;
		case dic_type_enum:	*((dict_enum_t*)it->second->var)->actual = ((dict_enum_t*)it->second->var)->get_int(value); break;
		}
	} else {
		dict_t *entry = new dict_t;
		entry->mode.vtype = dic_type_string;
		entry->mode.created = 1;
		entry->var = new std::string(value);
		d->entries[name] = entry;
	}
}
void Dictio::set(const std::string &name, const std::string &value)
{
	set(name, value.c_str());
}
void Dictio::set(const std::string &path, int value)
{
	std::string name; Dictio *d = getDictio(path, &name);
	std::map<std::string, dict_t *>::iterator it = d->entries.find(name);
	if (it != d->entries.end()) {
		switch (it->second->mode.vtype) {
		case dic_type_int: *(int*)it->second->var = value; break;
		case dic_type_enum:	*((dict_enum_t*)it->second->var)->actual = value; break;
		}
	} else {
		dict_t *entry = new dict_t;
		entry->mode.vtype = dic_type_int;
		entry->mode.created = 1;
		entry->var = new int(value);
		d->entries[name] = entry;
	}
}
void Dictio::set(const std::string &path, bool value)
{
	std::string name; Dictio *d = getDictio(path, &name);
	std::map<std::string, dict_t *>::iterator it = d->entries.find(name);
	if (it != d->entries.end()) {
		if (it->second->mode.vtype == dic_type_bool) {
			*(bool*)it->second->var = value;
		}
	} else {
		dict_t *entry = new dict_t;
		entry->mode.vtype = dic_type_bool;
		entry->mode.created = 1;
		entry->var = new bool(value);
		d->entries[name] = entry;
	}
}
void Dictio::set(const std::string &path, std::string *var, bool sv, bool ld)
{
	std::string name; Dictio *d = getDictio(path, &name);
	std::map<std::string, dict_t *>::iterator it = d->entries.find(name);
	if (it == d->entries.end()) {
		dict_t *entry = new dict_t;
		entry->mode.vtype = dic_type_string;
		entry->mode.save = sv;
		entry->mode.load = ld;
		entry->var = var;
		d->entries[name] = entry;
	}
}
void Dictio::set(const std::string &path, int *var, bool sv, bool ld)
{
	std::string name; Dictio *d = getDictio(path, &name);
	std::map<std::string, dict_t *>::iterator it = d->entries.find(name);
	if (it == d->entries.end()) {
		dict_t *entry = new dict_t;
		entry->mode.vtype = dic_type_int;
		entry->mode.save = sv;
		entry->mode.load = ld;
		entry->var = var;
		d->entries[name] = entry;
	}
}
void Dictio::set(const std::string &path, bool *var, bool sv, bool ld)
{
	std::string name; Dictio *d = getDictio(path, &name);
	std::map<std::string, dict_t *>::iterator it = d->entries.find(name);
	if (it == d->entries.end()) {
		dict_t *entry = new dict_t;
		entry->mode.vtype = dic_type_bool;
		entry->mode.save = sv;
		entry->mode.load = ld;
		entry->var = var;
		d->entries[name] = entry;
	}
}
void Dictio::newObject(const std::string &name, int inst, Dictio *dictio)
{
	char buf[16]; newObject(name, utils::toString(buf, inst), dictio);
}
void Dictio::newObject(const std::string &path, const std::string &inst, Dictio *dictio)
{
	std::string name;
	Dictio *d = getDictio(path, &name);
	if (!dictio) {
		dictio = d->createObject(name, inst);
	}
	if (dictio) d->objects[name][inst] = dictio;
}
void Dictio::rmObject(const std::string &path, int inst)
{
	char buf[16]; rmObject(path, utils::toString(buf, inst));
}
void Dictio::rmObject(const std::string &path, const std::string &inst)
{
	std::string name;
	Dictio *d = getDictio(path, &name);
	if (d->objects.find(name) != d->objects.end() && d->objects[name].find(inst) != d->objects[name].end()) {
		destroyObject(name, d->objects[name][inst]);
		d->objects[name].erase(inst);
	}
}
void Dictio::newOption(const std::string &path, int *val, const char * (*get_str)(int), int (*get_int)(const char *), bool sv, bool ld)
{
	std::string name; Dictio *d = getDictio(path, &name);
	std::map<std::string, dict_t *>::iterator it = d->entries.find(name);
	if (it == d->entries.end()) {
		dict_t *entry = new dict_t;
		entry->mode.vtype = dic_type_enum;
		entry->mode.save = sv;
		entry->mode.load = ld;
		dict_enum_t *denum = new dict_enum_t;
		entry->var = denum;
		denum->actual = val;
		denum->get_str = get_str;
		denum->get_int = get_int;
		d->entries[name] = entry;
	}
}

Dictio *Dictio::getDictio(const std::string &path, std::string *name)
{
	if (name) *name = path;
	Dictio *d = this;
	std::size_t type1 = path.find("::");
	if (type1 != std::string::npos && path.size() > type1 + 2) {
		std::string type = path.substr(0, type1);
		std::size_t inst1 = path.substr(type1 + 2).find_first_of('.');
		std::string next = path.substr(type1 + 2);
		std::string inst = next;
		if (inst1 != std::string::npos && path.size() > inst1 + type1 + 3) {
			inst = path.substr(type1 + 2, inst1);
			next = path.substr(inst1 + type1 + 3);
		}
		if (objects.find(type) != objects.end() && objects[type].find(inst) != objects[type].end()) {
			d = objects[type][inst]->getDictio(next, name);
		}
	}
	return d;
}

void Dictio::load(XPathConfig *xml, const std::string &xkey)
{
	int n = xml->getCount(xkey + "/var/@id");
	for (int i = 0; i < n; i++) {
		std::string name, val;
		xml->getValue(xkey + "/var/@id", &name, i);
		if (xml->getValue(xkey + "/var[@id=\"" + name + "\"]", &val))
			set(name, val);
	}
	n = xml->getCount(xkey + "/obj/@type");
	for (int i = 0; i < n; i++) {
		std::string type;
		xml->getValue(xkey + "/obj/@type", &type, i);
		int m = xml->getCount(xkey + "/obj[@type=\"" + type + "\"]/@id");
		for (int j = 0; j < m; j++) {
			std::string name;
			if (xml->getValue(xkey + "/obj[@type=\"" + type + "\"]/@id", &name, j)) {
				objects[type];
				if (objects[type].find(name) == objects[type].end()) {
					Dictio *dictio = createObject(type, name);
					if (dictio) objects[type][name] = dictio;
				}
				if (objects[type].find(name) != objects[type].end()) {
					objects[type][name]->load(xml, xkey + "/obj[@type=\"" + type + "\" and @id=\"" + name + "\"]");
				}
			}
		}
	}
}

void Dictio::xdump(std::string *sbuf, int n)
{
	char buf[128] = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
	for (std::map<std::string, std::map<std::string, Dictio *> >::iterator i = objects.begin(); i != objects.end(); i++) {
		for (std::map<std::string, Dictio *>::iterator j = i->second.begin(); j != i->second.end(); j++) {
			sbuf->append(buf, n);  *sbuf += "<obj type=\"" + i->first + "\" id=\"" + j->first + "\">\n";
			j->second->xdump(sbuf, n + 1);
			sbuf->append(buf, n);  *sbuf += "</obj>\n";
		}
	}
	for (std::map<std::string, dict_t *>::iterator i = entries.begin(); i != entries.end(); i++) {
		if (i->second->mode.save && (i->second->mode.vtype != dic_type_enum || ((dict_enum_t*)i->second->var)->actual)) {
			sbuf->append(buf, n); *sbuf += "<var id=\"" + i->first + "\">";
			switch (i->second->mode.vtype) {
			case dic_type_string: *sbuf += *(std::string *)i->second->var; break;
			case dic_type_int: char buf[32]; *sbuf += utils::toString(buf, *(int*)i->second->var); break;
			case dic_type_bool: *sbuf += (*(bool*)i->second->var)?"true":"false"; break;
			case dic_type_enum: *sbuf += ((dict_enum_t*)i->second->var)->get_str(*((dict_enum_t*)i->second->var)->actual); break;
			}
			*sbuf += "</var>\n";
		}
	}
}

void Dictio::ddump(std::string *sbuf, const std::string &path)
{
	char buf[32];
	for (std::map<std::string, dict_t *>::iterator i = entries.begin(); i != entries.end(); i++) {
		int n = snprintf(buf, sizeof(buf), "[%d%d%d%d] ", i->second->mode.vtype,
		                 i->second->mode.save, i->second->mode.load, i->second->mode.created);
		sbuf->append(buf, n); *sbuf += path + i->first + ": ";
		switch (i->second->mode.vtype) {
			case dic_type_string: *sbuf += *(std::string *)i->second->var; break;
			case dic_type_int: char buf[32]; *sbuf += utils::toString(buf, *(int*)i->second->var); break;
			case dic_type_bool: *sbuf += (*(bool*)i->second->var)?"true":"false"; break;
			case dic_type_enum: {
				dict_enum_t *denum = (dict_enum_t*)i->second->var;
				if (denum->actual) {
					*sbuf += denum->get_str(*denum->actual);
				} else {
					*sbuf += "{"; *sbuf += denum->get_str(0); *sbuf += ",";
					for (int i = 1; denum->get_str(i) && strcmp(denum->get_str(i-1), denum->get_str(i)); i++) {
						*sbuf += denum->get_str(i); *sbuf += ",";
					}
					*sbuf += "\b}";
				}
			} break;
		}
		*sbuf += "\n";
	}
	for (std::map<std::string, std::map<std::string, Dictio *> >::iterator i = objects.begin(); i != objects.end(); i++) {
		for (std::map<std::string, Dictio *>::iterator j = i->second.begin(); j != i->second.end(); j++) {
			j->second->ddump(sbuf, path + i->first + "::" + j->first + ".");
		}
	}
}

Dictio::~Dictio()
{
	for (std::map<std::string, dict_t *>::iterator i = entries.begin(); i != entries.end(); i++) {
		delete i->second;
	}
	entries.clear();
	for (std::map<std::string, std::map<std::string, Dictio *> >::iterator i = objects.begin(); i != objects.end(); i++) {
		for (std::map<std::string, Dictio *>::iterator j = i->second.begin(); j != i->second.end(); j++) {
			j->second->~Dictio();
		}
	}
	objects.clear();
}
