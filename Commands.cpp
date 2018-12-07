
#include <fstream>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include "ShellCmd.h"

HelpCmd::HelpCmd()
{
	brief = "Muestra ayuda";
	help = "\thelp [<comando>]\n\nMuestra la ayuda asociada al comando";
	commands["help"] = this;
}
void HelpCmd::run(const std::vector<std::string> &cmd)
{
	if (cmd.size() < 2) {
		bs_print("\n\tComandos de boxShell\n\n");
		for (std::map<std::string, ShellCmd *>::iterator i = commands.begin();
		     i != commands.end(); i++) {
			bs_print("%s\t - %s\n", i->first.c_str(), i->second->brief);
		}
	} else {
		for (int i = 1; i < (int)cmd.size(); i++) {
			if (commands.find(cmd[i]) != commands.end())
				bs_print("\n%s\n\n", commands[cmd[i]]->help);
			else
				bs_print("\nComand '%s' desconocido\n\n", cmd[i].c_str());
		}
	}
	bs_print("\n");
}

QuitCmd::QuitCmd()
{
	brief = "Se cierra la aplicacion";
	help = "\tquit\n\nSe cierra la aplicacion";
	commands["quit"] = this;
	commands["exit"] = this;
}
void QuitCmd::run(const std::vector<std::string> &cmd)
{
	endwin();
	_exit(0);
}

ClearCmd::ClearCmd()
{
	brief = "Limpia la pantalla";
	help = "\tclear\n\nLimpia la pantalla";
	commands["clear"] = this;
}
void ClearCmd::run(const std::vector<std::string> &cmd)
{
	clear();
	move(0,0);
}

WaitCmd::WaitCmd()
{
	brief = "Espera un tiempo determinado";
	help = "\twait [<dseg>]\n\nEspera <dseg> decimas de segundo (por defecto dseg=10)";
	commands["wait"] = this;
}
void WaitCmd::run(const std::vector<std::string> &cmd)
{
	int dseg = 10;
	if (cmd.size() > 1 && atoi(cmd[1].c_str()) > 0)
		dseg = atoi(cmd[1].c_str());
	usleep(dseg*100000);
}

FileCmd::FileCmd()
{
	brief = "Ejecuta los comandos de un fichero";
	help = "\tfile <fichero>\n\nEjecuta los comandos contenidos en <fichero>";
	commands["file"] = this;
}
void FileCmd::run(const std::vector<std::string> &cmd)
{
	if (cmd.size() > 1) {
		std::ifstream ifs(cmd[1].c_str()); std::string line;
		if(!ifs)
		{
			bs_print("Error opening file '%s' : %s\n", cmd[1].c_str(), strerror(errno));
			return;
		}
		while(std::getline(ifs, line))
			dispatch(line);
	}
}

EchoCmd::EchoCmd()
{
	brief = "Escribe en pantalla el texto";
	help = "\techo <texto>\n\nEscribe en pantalla el texto";
	commands["echo"] = this;
}
void EchoCmd::run(const std::vector<std::string> &cmd)
{
	for (int i = 1; i < (int)cmd.size(); i++)
		bs_print("%s ", cmd[i].c_str());
	bs_print("\n");
}
