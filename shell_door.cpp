
#include <string>
#include <vector>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include "ShellCmd.h"


static std::string command;
static std::vector<std::string> history;
static int idx = 0, p = 0;
static std::string temp_cmd;

static void up_command()
{
	if (idx > 0) {
		if (idx == (int)history.size())
			temp_cmd = command; /* save temporal command */
		idx--;
		command = history[idx];
		p = command.size();
	}
}

static void down_command()
{
	if (idx < (int)history.size()) {
		idx++;
		if (idx == (int)history.size())
			command = temp_cmd;
		else
			command = history[idx];
		p = command.size();
	}
}

static void search_command()
{
	for (int i = history.size() - 1; i >= 0; i--) {
		if (history[i].size() > command.size()
				&& strncmp(command.c_str(), history[i].c_str(), command.size()) == 0) {
			idx = i;
			command = history[idx];
			p = command.size();
			return;
		}
	}
}

static void insert_in_command(int c)
{
	if (p == (int)command.size())
		command.push_back(c);
	else
		command.insert(p, 1, c);
	p++;
}

static void get_command()
{
	command.clear(); p = 0;
	for (;;) {
		if (p >= 1000) {
			bs_print("Comando inaceptablemente largo\n");
			return;
		}
		int c = getch();
//bs_print(" 0x%x",c);continue;
		switch (c) {
		case '\n': bs_print("\n"); return;
		case '\t': search_command(); break;
		case KEY_DOWN: down_command(); break;		
		case KEY_UP: up_command(); break;
		case KEY_LEFT: if (p > 0) p--; break;
		case KEY_RIGHT: if (p < (int)command.size()) p++; else insert_in_command(' '); break;
		case KEY_HOME: case 0x14b: p = 0; break;
		case KEY_BACKSPACE: case 0x8: if (p > 0) command.erase(--p, 1); break;
		case KEY_END: p = command.size(); break;
		case KEY_DC: if (p >= 0 && p < (int)command.size()) command.erase(p, 1); break;
		case KEY_NPAGE: if (idx < (int)history.size()) { command = temp_cmd; idx = history.size(); p = command.size(); } break;
		case KEY_EXIT: endwin(); _exit(0);
		default: if (isprint(c)) insert_in_command(c);
		}
		int y, x;            // to store where you are
		getyx(stdscr, y, x); // save current pos
		move(y, 0);          // move to begining of line
		clrtoeol();          // clear line
		bs_print("boxShell> %s", command.c_str());
		move(y, p + 10);
	}
}

static void dispatch_cmd()
{
	if (!command.size()) return;
	history.push_back(command); idx = history.size();
	ShellCmd::dispatch(history[idx-1]);
}

int _bs_refresh () { return fflush(stdout); }

int main(int argc, char *argv[])
{
	VERSIONLIT_IN_MAIN;

	nice(15);
	
	ShellCmd::init();
	if (argc > 1) {
		for (int i = 1; i < argc; i++) {
			command += argv[i];
			command += " ";
		}
		bs_print = printf;
		bs_refresh = _bs_refresh;
		dispatch_cmd();
	} else {
		setenv("TERM", "linux", 1);
		initscr();
		noecho();
		keypad(stdscr, TRUE);
		scrollok(stdscr, TRUE);
		bs_print = (int (*) (const char *, ...))printw;
		bs_refresh = refresh;
		for (;;) {
			bs_print("boxShell> ");
			get_command();
			dispatch_cmd();
		}
		endwin();
	}
}

