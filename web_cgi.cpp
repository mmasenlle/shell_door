
#include <map>
#include <string>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <event.h>
#include <evhttp.h>
#include "config.h"

#define ELOG perror

#define WEBBOX_CGI INSTALL_PATH "web/webbox.sh"

static bool webbox_system(char *cmd[], struct evbuffer *res)
{
	int n, pfd[2];
	if (pipe(pfd) == -1) {
		ELOG("webbox_system - pipe");
		return false;
	}
	switch (fork())	{
	case 0:
		if (close(1) || dup(pfd[1]) != 1) {
			ELOG("webbox_system - close dup");
			_exit(-1);
		}
		close(pfd[0]);
		if(nice(15) == -1) {ELOG("webbox_system - nice 15");}
		execvp(cmd[0], cmd);
		ELOG("webbox_system - execv");
		_exit(-1);
	case -1:
		ELOG("webbox_system - fork");
		close(pfd[0]); close(pfd[1]); return false;
	}
	char buf[1024];
	close(pfd[1]);
	while ((n = read(pfd[0], buf, sizeof(buf))) > 0) {
		evbuffer_add(res, buf, n);
	}
	wait(&n);
	close(pfd[0]);
	return (n == 0);
}

#define MAX_ARGS 32

static const char auth_str[] = "user=admin&pass=admin1";
static std::map<std::string, time_t> auth_hosts;
static void webbox_parse_query(char *uri, char *cmd[MAX_ARGS])
{
	cmd[1] = uri; int i = 2;
//printf("webbox_parse_query(%s)->cmd[0]=%s;cmd[1]=%s\n", uri, cmd[0], cmd[1]);
	while (*uri && i < (MAX_ARGS-2)) {
		if ((i == 2 && *uri == '?') || (i % 2 == 1 && *uri == '=') || (i % 2 == 0 && *uri == '&')) {
			*uri = 0; uri++; if (*uri) cmd[i] = uri; i++;
//printf("webbox_parse_query(%s)->cmd[%d]=%s;cmd[%d]=%s\n", uri, i-2, cmd[i-2], i-1, cmd[i-1]);
		} else {
			uri++;
		}
	}
	cmd[i] = NULL;
}

static const char init_page_redirect[] = "<meta http-equiv=\"REFRESH\" content=\"0;url=index\">";

static void process_request(struct evhttp_request *req, void *arg)
{
//	DLOG("process_request(%s,%s)\n", req->remote_host, evhttp_request_uri(req));
	struct evbuffer *buf = evbuffer_new();
	if (buf == NULL) return;
	char *duri = NULL;
	char *cmd[MAX_ARGS] = { (char*)WEBBOX_CGI, (char*)"/login", NULL };
	if (auth_hosts.find(req->remote_host) != auth_hosts.end()) {
		time_t tt = time(NULL);
		if (auth_hosts[req->remote_host] > tt) {
			auth_hosts[req->remote_host] = tt + 150;
			char *duri = evhttp_decode_uri(evhttp_request_uri(req));
			webbox_parse_query(duri, cmd);
		} else {
			auth_hosts.erase(req->remote_host);
		}
	} else if (!strcmp("/login", evhttp_request_uri(req))) {
		char cbuf[64];
		int n = evbuffer_remove(req->input_buffer, cbuf, sizeof(cbuf));
		if (n ==  sizeof(auth_str)-1 && strncmp(cbuf, auth_str, n) == 0) {
//			ILOG("process_request(%s)->AUTHORIZED\n", req->remote_host);
			auth_hosts[req->remote_host] = time(NULL) + 150;
			evbuffer_add(buf, init_page_redirect, sizeof(init_page_redirect)-1);
			evhttp_send_reply(req, HTTP_OK, "OK", buf);
			evbuffer_free(buf);
			free(duri);
			return;
//			cmd[1] = (char*)"/home";
		}
	}
	if (webbox_system(cmd, buf)) {
		evhttp_send_reply(req, HTTP_OK, "OK", buf);
	} else {
		evhttp_send_error(req, HTTP_NOTFOUND, "");
	}
	evbuffer_free(buf);
	free(duri);
}

int main(int argc, char *argv[])
{
	VERSIONLIT_IN_MAIN;
	int wport = 18088;
	if (argc > 1 && atoi(argv[1]) > 0) wport = atoi(argv[1]);
//	char buf[128];
//	INIT_DEFAULT_LOGGER(CommonConf::getLoggerFile(buf, "webbox"));
//	SET_LOGGER_LEVEL(CommonConf::getLoggerLevel("webbox"));
//	ILOG("webbox port: %d", wport);
    struct event_base *base = NULL;
    struct evhttp *httpd = NULL;
	signal(SIGCHLD, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);
    base = event_init();
    if (base == NULL) return -1;
    httpd = evhttp_new(base);
    if (httpd == NULL) return -1;
    if (evhttp_bind_socket(httpd, "0.0.0.0", wport) != 0) return -1;
    evhttp_set_gencb(httpd, process_request, NULL);
    event_base_dispatch(base);
    return 0;
}

