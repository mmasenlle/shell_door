
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "ShellCmd.h"


MCastCmd::MCastCmd()
{
	brief = "Lanza comandos multicast (IDC)";
	help = "\tmcast sniff [<str> [<ip> [<puerto>]]] - leer trafico multicast filtrado por str\n\n"
			"\tmcast [<n> [<dest> [<ip> [<puerto>]]] [<cmd...>]]\n\n"
		"Envia comando multicast <n> (simulando un IDC):\n"
		" 3: Info\n"
		" 4: Reset\n"
		" 5: Reinit\n"
		" 6: Status\n"
		" 7: Info\n"
		" 8: Command\n"
			;
	commands["mcast"] = this;
}
void MCastCmd::run(const std::vector<std::string> &cmd)
{
	int port = 10101, n = 7, len = 0;
	const char *ip = "226.0.0.1", *dest = NULL;
	if (cmd.size() > 4 && atoi(cmd[4].c_str()) > 0) port = atoi(cmd[4].c_str());
	if (cmd.size() > 3 && cmd[3].size() > 6) ip = cmd[3].c_str();
	if (cmd.size() > 1 && cmd[1] == "sniff") {
		if (cmd.size() > 2 && cmd[2].size() > 3) dest = cmd[2].c_str();
		int sd = socket(AF_INET, SOCK_DGRAM, 0);
		{int reuse = 1;
		setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse));
		struct sockaddr_in localSock;
		struct ip_mreq group;
		memset((char *) &localSock, 0, sizeof(localSock));
		memset((char *) &group, 0, sizeof(group));
		localSock.sin_family = AF_INET;
		localSock.sin_port = htons(port);
		localSock.sin_addr.s_addr = INADDR_ANY;
		if (bind(sd, (struct sockaddr*)&localSock, sizeof(localSock)) < 0) return;
		group.imr_multiaddr.s_addr = inet_addr(ip);
		group.imr_interface.s_addr = INADDR_ANY;//inet_addr("192.168.241.233");
		if (setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&group, sizeof(group)) < 0) return;
		} for (;;) {
			char rbuf[2048];
			len = read(sd, rbuf, sizeof(rbuf));
			if (len > 0) {
				rbuf[len] = 0;
				if (!dest || strstr(rbuf, dest)) {
					bs_print("%s\n", rbuf);bs_refresh();
				}
			}
		}
		close(sd);
	} else {
		char bdest[128] = "HOSTNAME=", cbuf[800] = "", buf[1024];
		dest = bdest;
		gethostname(bdest+strlen(bdest), sizeof(bdest)+strlen(bdest));
		if (cmd.size() > 2) dest = cmd[2].c_str();
		if (cmd.size() > 1) n = atoi(cmd[1].c_str());
		if (strlen(dest) < 2) dest = "*"; //for all
		if (n == 8) {
			char buf[1024];
			for (int i = 5; i < (int)cmd.size(); i++)
				len += snprintf(buf+len, sizeof(buf)-len, "%s ", cmd[i].c_str());
		}
		len = snprintf(buf, sizeof(buf), "(%s)(boxShell)(0%d)(%02x)(00)(00)%s",	dest, dest[0]=='*'?2:3, n, cbuf);
		bs_print("%s\n", buf);
		struct sockaddr_in conndat;
		conndat.sin_family = AF_INET;
		conndat.sin_port = htons(port);
		inet_aton(ip, &conndat.sin_addr);
		int sd = socket(AF_INET, SOCK_DGRAM, 0);
		u_char ttl = 25;
		setsockopt(sd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));
		sendto(sd, buf, len, 0, (struct sockaddr*)&conndat, sizeof(conndat));
/*		if (dest[0]!='*') {
			while ((len = read(sd, buf, sizeof(buf))) > 0)
				bs_print("%s", buf);
		}
*/		close(sd);
//		bs_print("\n");
	}
}
