#ifndef SHELLCMD_H_
#define SHELLCMD_H_

#include <map>
#include <string>
#include <vector>
#include "Dictio.h"

class ShellCmd
{
	static void parse_token(std::string &token, std::vector<std::string> *vtok, std::vector<std::vector<std::string> > *cmds);
	static void strip(const std::string &cmd, std::vector<std::vector<std::string> > *cmds);
protected:
	static std::map<std::string, ShellCmd *> commands;
	static Dictio dictio;
public:
	static void init();
	static void dispatch(const std::string &cmd);
	
	const char *brief;
	const char *help;
	virtual void run(const std::vector<std::string> &cmd) = 0;
};

extern int (*bs_print) (const char *, ...) __attribute__ ((format (printf, 1, 2)));
extern int (*bs_refresh) (void);

#define SHELLCMD_CLASS(_class) \
class _class : public ShellCmd {public: _class();\
virtual void run(const std::vector<std::string> &cmd);}

SHELLCMD_CLASS(HelpCmd);
SHELLCMD_CLASS(QuitCmd);
SHELLCMD_CLASS(ClearCmd);
SHELLCMD_CLASS(WaitCmd);
SHELLCMD_CLASS(FileCmd);
SHELLCMD_CLASS(EchoCmd);
SHELLCMD_CLASS(UtilCmd);
SHELLCMD_CLASS(DictCmd);
//eventos
SHELLCMD_CLASS(SensorCmd);
SHELLCMD_CLASS(TarjetaCmd);
SHELLCMD_CLASS(TeclaCmd);
//actions
SHELLCMD_CLASS(ActCmd);
SHELLCMD_CLASS(InhCmd);
SHELLCMD_CLASS(WsCmd);
SHELLCMD_CLASS(ModeCmd);
SHELLCMD_CLASS(IfCmd);
SHELLCMD_CLASS(DisplayCmd);
SHELLCMD_CLASS(SerialCmd);
SHELLCMD_CLASS(ConfCmd);
SHELLCMD_CLASS(RDicCmd);
//management
SHELLCMD_CLASS(MonitCmd);
SHELLCMD_CLASS(MCastCmd);
//data
SHELLCMD_CLASS(StateCmd);
SHELLCMD_CLASS(RetStateCmd);
SHELLCMD_CLASS(PermCmd);
SHELLCMD_CLASS(TimetCmd);
//debug
SHELLCMD_CLASS(VarInfoCmd);
SHELLCMD_CLASS(ForceCmd);
SHELLCMD_CLASS(WVarInfoCmd);
SHELLCMD_CLASS(WForceCmd);

#endif
