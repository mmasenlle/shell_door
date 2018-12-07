
#include "ShellCmd.h"

std::map<std::string, ShellCmd *> ShellCmd::commands;

int (*bs_print) (const char *, ...);
int (*bs_refresh) (void);

void ShellCmd::parse_token(std::string &token, std::vector<std::string> *vtok, std::vector<std::vector<std::string> > *cmds)
{
	if (token.size()) {
		if (token[0]=='$') {
			std::vector<std::vector<std::string> > varl;
			std::string s; dictio.get(token.substr(1), &s);
			strip(s, &varl);
			for (int j = 0; j < (int)varl.size(); j++) {
				if (vtok->size() && j) {
					cmds->push_back(*vtok);
					vtok->clear();
				}
				for (int k = 0; k < (int)varl[j].size(); k++) {
					vtok->push_back(varl[j][k]);
				}
			}
		} else {
			vtok->push_back(token);
		}
		token.clear();
	}
}

void ShellCmd::strip(const std::string &cmd, std::vector<std::vector<std::string> > *cmds)
{
	std::string token;
	std::vector<std::string> vtok;
	bool bs = false, lit = false;
	for (int i = 0; i < (int)cmd.size(); i++) {
		if (bs) { token.push_back(cmd[i]); bs = false; continue; }
		else if (cmd[i]=='\\') { bs = true; continue; }
		if (lit) { if (cmd[i]=='"') { lit = false; } else { token.push_back(cmd[i]); } continue; }
		else if (cmd[i]=='"') { lit = true; continue; }
		if (cmd[i] == '#') break; // comment for files mostly
		if (cmd[i] == ';' || cmd[i] == ',') { // command separator
			parse_token(token, &vtok, cmds);
			if (vtok.size()) {
				cmds->push_back(vtok);
				vtok.clear();
			}
		} else if (isblank(cmd[i]) || cmd[i]=='+' || !isprint(cmd[i])) { // token separator
			parse_token(token, &vtok, cmds);
		} else {
			token.push_back(cmd[i]);
		}
	}
	parse_token(token, &vtok, cmds);
	if (vtok.size())
		cmds->push_back(vtok);
}

void ShellCmd::dispatch(const std::string &cmd)
{
	std::vector<std::vector<std::string> > cmds;
	strip (cmd, &cmds);
	for (int i = 0; i < (int)cmds.size(); i++) {
		if (commands.find(cmds[i][0]) != commands.end()) {
			commands[cmds[i][0]]->run(cmds[i]);
		} else {
			bs_print("Comando '%s' desconocido (help ?)\n", cmds[i][0].c_str());
		}
		bs_refresh();
	}
}

Dictio ShellCmd::dictio;

void ShellCmd::init()
{
	new HelpCmd;
	new QuitCmd;
	new ClearCmd;
	new WaitCmd;
	new FileCmd;
	new EchoCmd;
	new UtilCmd;
	new DictCmd;
//eventos
	new SensorCmd;
	new TarjetaCmd;
	new TeclaCmd;

	new ActCmd;
	new InhCmd;
	new ModeCmd;
	new IfCmd;
	new DisplayCmd;
	new SerialCmd;
	new MonitCmd;
	new ConfCmd;
	new RDicCmd;

	new StateCmd;
	new RetStateCmd;
	new PermCmd;
	new TimetCmd;

	new WsCmd;
	new MCastCmd;

//debug
	new VarInfoCmd;
	new ForceCmd;
	new WVarInfoCmd;
	new WForceCmd;
}
