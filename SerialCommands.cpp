
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <poll.h>
#include <fcntl.h>
#include <ncurses.h>
#include "utils.h"
#include "ShellCmd.h"


static char defdev[] = "/dev/ttyS1";
static int baud = 9600, bits = 8, par = 0, bstop = 1;

static int serial_open_conf(int base, const std::vector<std::string> &conf)
{
	const char *dev = defdev;
	if ((base >= (int)conf.size())) {
		if (conf.size() > 2) defdev[9] = conf[2][0];
	} else {
	if (base < (int)conf.size())
		dev = conf[base++].c_str();
	if (base < (int)conf.size())
		baud = atoi(conf[base++].c_str());
	if (base < (int)conf.size())
		bits = atoi(conf[base++].c_str());
	if (base < (int)conf.size())
		par = atoi(conf[base++].c_str());
	if (base < (int)conf.size())
		bstop = atoi(conf[base++].c_str());
	}
	int fd;
	bs_print("Device: %s %d %d %d %d\n", dev, baud, bits, par, bstop);
	if ((fd = open(dev, O_RDWR)) == -1) {
		perror("serial_open_conf -> opening device"); return -1;
	} else if (utils::Tty::config(fd, baud, bits, par, bstop) < 0) {
		perror("serial_open_conf -> setting device configuration");
		close(fd);
		return -1;
	} else if (utils::Tty::initRTS(fd, (std::string(dev) + "_rts").c_str()) < 0) {
		perror("serial_open_conf -> clearing RTS");
		close(fd);
		return -1;
	}
	return fd;
}

SerialCmd::SerialCmd()
{
	brief = "Operaciones de puerto serie";
	help = "\tserial recv <devn> [<device> [<baud> [<bits> [<0|1|2> [<stop>]]]]] \n"
		"Recibe y muestra en hexadecimal\n"
		"Ejemplos:\n\tserial recv 1\n\tserial recv 1 /dev/ttyUSB1 4800 7 0 1\n\n"
			"\tserial send <devn> <string> - Envia <string> por device numero <devn>\n"
			"\tserial sendh <devn> <b1> <b2> ... - Envia bytes (0-ff) por device numero <devn>\n"
			"\tserial sf <devn> <file path> ... - Envia el fichero <file path>\n"
			"\tserial rf <devn> <file path> ... - Recibe el fichero y lo almacena en <file path>\n"
			"\tserial console <devn> ... - Hace la funcion de cliente consola\n"
			;
	commands["serial"] = this;
}
void SerialCmd::run(const std::vector<std::string> &cmd)
{
	int fd;
	if (cmd[1] == "recv") {
		if ((fd = serial_open_conf(3, cmd)) > -1) {
			char c;
			for (;;) {
				int r = read(fd, &c, 1);
				if (r>0) bs_print("%02x ", c);
				else usleep(100000);
				bs_refresh();
			}
			close(fd);
//			system("stty sane"); //FIXME: probar esto
//			system("reset");
//system("echo -e \\033c");
		}
	} else if (cmd[1] == "send") {
		if ((fd = serial_open_conf(10000, cmd)) > -1) {
			std::string sbuf;
			for (int i = 3; i < (int)cmd.size(); i++) { sbuf += cmd[i];sbuf += " "; }
			int r = utils::Tty::writeRTS(fd, sbuf.c_str(), sbuf.size());
			close(fd);
			bs_print("Sent: %d\n", r);
		}
	} else if (cmd[1] == "sendh") {
		if ((fd = serial_open_conf(10000, cmd)) > -1) {
			std::vector<unsigned char> bytes;
			for (int i = 3; i < (int)cmd.size(); i++) { bytes.push_back(strtoul(cmd[i].c_str(), NULL, 16)); }
			int r = utils::Tty::writeRTS(fd, &bytes[0], bytes.size());
			close(fd);
			bs_print("Sent: %d\n", r);
		}
	} else if (cmd[1] == "sf") {
		if ((fd = serial_open_conf(4, cmd)) > -1) {
			char buf[256];
			int rtot = 0, r, fin = open(cmd[3].c_str(), O_RDONLY);
			struct timeval tv0,tv,tvr; gettimeofday(&tv0, NULL);
			if (fin < 0) perror("\r\nopening input file");
			else {
				while((r=read(fin, buf, sizeof(buf))) > 0)
					rtot += utils::Tty::writeRTS(fd, buf, r);
				close(fin);
			}
			close(fd);
			gettimeofday(&tv, NULL); timersub(&tv, &tv0, &tvr);
			bs_print("%d bytes sent from '%s' in %d.%06d s\n", rtot, cmd[3].c_str(), (int)tvr.tv_sec, (int)tvr.tv_usec);
		}
	} else if (cmd[1] == "rf") {
		if ((fd = serial_open_conf(4, cmd)) > -1) {
			char buf[256];
			int rtot=0, c=0, r, fout = open(cmd[3].c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0664);
			struct timeval tv0,tv,tvr; gettimeofday(&tv0, NULL);
			if (fout < 0) perror("\r\nopening output file");
			else {
				while (!rtot || c < 100) {
					if ((r=read(fd, buf, sizeof(buf))) > 0) {
						rtot += write(fout, buf, r);
						c = 0;
					} else { c++; usleep(30000); }
				}
				close(fout);
			}
			close(fd);
			gettimeofday(&tv, NULL); timersub(&tv, &tv0, &tvr);
			bs_print("%d bytes written in '%s' in %d.%06d s\n", rtot, cmd[3].c_str(), (int)tvr.tv_sec, (int)tvr.tv_usec);
		}
	} else if (cmd[1] == "console") {
		if ((fd = serial_open_conf(3, cmd)) > -1) {
			struct pollfd pfd[2];
			pfd[0].events = pfd[1].events = POLLIN;
			pfd[0].fd = fd;
			pfd[1].fd = open("/dev/tty", O_RDWR | O_NOCTTY | O_NONBLOCK);
			bs_print("serial console: hit '^C' for exit\n");bs_refresh();
			char buf[2048]; int r;
			while (poll(pfd, 2, -1) != -1) {
				if (pfd[0].revents == POLLIN) {
					if ((r = read(pfd[0].fd, buf, sizeof(buf))) > 0) {
						write(pfd[1].fd, buf, r);
					}
				}
				if (pfd[1].revents == POLLIN) {
					if ((r = read(pfd[1].fd, buf, sizeof(buf))) > 1) {
						utils::Tty::writeRTS(pfd[0].fd, buf, r);
					} else if (buf[0]=='q') break;
				}
			}
			perror("\n\rwhile (poll)");
			close(pfd[0].fd);
			close(pfd[1].fd);
		}
	} else {
		bs_print("serial: valor de cmd '%s' desconocido\n", cmd[1].c_str());
	}
}
