#include <sys/socket.h>
#include <netinet/in.h>

#include <unistd.h>
#include <errno.h>

#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

#define ROOT_LENGTH 1024
#define DEFAULT_PORT 21
#define DEFAULT_PATH "/tmp"
#define MAX_BACKLOG 10
#define MAX_BUFFER_LEN 10086

void cdebug(char* debug) {
	printf("DEBUG: %s\n", debug);
}

void idebug(int i) {
	printf("DEBUG: %d\n", i);
}

void messdebug(char* mess) {
	printf("FROM SERVER: %s", mess);
}

int parse_port(int default_port, int argc, char** argv) {
	int i = 0;
	char* dest = "-port";
	for(i=0;i<argc;++i) {
		if(strcmp(argv[i], dest)==0) {
			if(i==argc-1) {
				return -1;
			}
			if(argv[i+1][0]>'9' || argv[i+1][0]<'0') {
				return -1;
			}
			int port = atoi(argv[i+1]);
			return port;
		}
	}
	return default_port;
}

int parse_root(char* root, int argc, char** argv) {
	int i=0;
	char* dest = "-root";
	for(i=0;i<argc;++i) {
		if(strcmp(argv[i], dest)==0) {
			if(i==argc-1) {
				return -1;
			}
			memset(root, '\0', sizeof(root));
			root = strcpy(root, argv[i+1]);
			return 0;
		}
	}
	char* default_root = DEFAULT_PATH;
	memset(root, '\0', sizeof(root));
	root = strcpy(root, default_root);
	return 0;
}

int msocket_read(int connfd, char* buffer, int buffer_len) {
	int p = 0;
	while (1) {
		int n = read(connfd, buffer + p, buffer_len - p);
		if (n < 0) {
			printf("Error read(): %s(%d)\n", strerror(errno), errno);
			close(connfd);
			return -1;
		} else if (n == 0) {
			break;
		} else {
			p += n;
			if (buffer[p-1] == '\n') {
				break;
			}
		}
	}
	// buffer[p-1] = '\0';
	return p;
}

int msocket_write(int connfd, char* buffer, int len) {
	int p = 0;
	while (p < len) {
		int n = write(connfd, buffer + p, len - p);
		if (n < 0) {
			printf("Error write(): %s(%d)\n", strerror(errno), errno);
			close(connfd);
			return -1;
 		} else {
			p += n;
		}
	}
	return 0;
}

int prepare_response_oneline(char* buffer, int code, int last_line, char* content) {
	char code_str[4];
	sprintf(code_str, "%d", code);
	buffer = strcpy(buffer, code_str);
	if(last_line) {
		buffer[3] = ' ';
	} else {
		buffer[3] = '-';
	}
	strcpy(buffer+4, content);
	size_t content_len = strlen(content);
	buffer[4+content_len] = '\r';
	buffer[4+content_len+1] = '\n';
	return strlen(buffer);
}

int prepare_response(char* buffer, int code, char** contents, int line_num) {
	char code_str[4];
	sprintf(code_str, "%d", code);
	int i = 0;
	int offset = 0;
	for(i=0;i<line_num;++i) {
		char connect;
		if(i==line_num-1) {
			connect = ' ';
		} else {
			connect = '-';
		}
		strncpy(buffer+offset, code_str, 3);
		buffer[offset+3] = connect;
		offset += 4;
		strncpy(buffer+offset, contents[i], strlen(contents[i]));
		offset += strlen(contents[i]);
		buffer[offset] = '\r';
		buffer[offset+1] = '\n';
		offset += 2;
	}
	return strlen(buffer);
}

int unpack_message(char* buffer) {
	char order[5];
	memset(order, '\0', sizeof(order));
	int i=0;
	for(i=0;i<4;++i) {
		order[i]=buffer[i];
	}
	if(strcmp(order, "USER")==0) {
		return 1;
	}
	if(strcmp(order, "PASS")==0) {
		return 2;
	}
	if(strcmp(order, "SYST")==0) {
		return 3;
	}
	if(strcmp(order, "TYPE")==0) {
		return 4;
	}
	if(strcmp(order, "PORT")==0) {
		return 5;
	}
	if(strcmp(order, "RETR")==0) {
		return 6;
	}
	if(strcmp(order, "PASV")==0) {
		return 7;
	}
	if(strcmp(order, "QUIT")==0) {
		return 8;
	}
	return 100;
}

int user_handler(int connfd, char* buffer) {
	char username[256];
	char response[1024];
	memset(username, '\0', sizeof(username));
	memset(response, '\0', sizeof(response));

	strncpy(username, buffer+5, strlen(buffer)-6);
	if(strcmp(username, "anonymous")==0) {
		char* contents[2];
		contents[0] = (char*)malloc(256);
		contents[1] = (char*)malloc(256);
		strcpy(contents[0], "Guest login ok;");
		strcpy(contents[1], "Send your complete e-mail address as password.");
		int len = prepare_response(response, 331, contents, 2);
		if(msocket_write(connfd, response, strlen(response))==-1) {
			return -1;
		}
		return 0;
	} else {
		int len = prepare_response_oneline(response, 530, 1, "Invalid username.");
		if(msocket_write(connfd, response, strlen(response))==-1) {
			return -1;
		}
		return 0;
	}
}

int pass_handler(int connfd, char* buffer) {
	char password[256];
	char response[1024];
	memset(password, '\0', sizeof(password));
	memset(response, '\0', sizeof(response));

	strncpy(password, buffer+5, strlen(buffer)-6);
	cdebug(password);
	char* contents[3];
	int i = 0;
	for(i=0;i<3;++i) {
		contents[i] = (char*)malloc(256);
	}
	strcpy(contents[0], "");
	strcpy(contents[1], "Welcome to hongfz FTP;");
	strcpy(contents[2], "Guest login ok, access restrictions apply.");
	int len = prepare_response(response, 230, contents, 3);
	if(msocket_write(connfd, response, strlen(response))==-1) {
		return -1;
	}
	return 0;
}

int syst_handler(int connfd) {
	char response[1024];
	memset(response, '\0', sizeof(response));
	int len = prepare_response_oneline(response, 215, )
}

int request_not_support_handler(int connfd) {
	char response[256];
	memset(response, '\0', sizeof(response));
	int len = prepare_response_oneline(response, 500, 1, "Unsupported command.");
	if(msocket_write(connfd, response, strlen(response))==-1) {
		return -1;
	}
}

int main(int argc, char** argv) {
	char* root = (char*)malloc(ROOT_LENGTH);
	int port = 0;
	if((port = parse_port(DEFAULT_PORT, argc, argv))==-1) {
		printf("Error parse_port(): Please specify the port you want to listen to after -port flag!\n");
		return -1;
	}
	if(parse_root(root, argc, argv)==-1) {
		printf("Error parse_root(): Please specify the root path you want to serve after -root flag!\n");
		return -1;
	}

	int listenfd, connfd;
	struct sockaddr_in addr;
	int len;
	char buffer[MAX_BUFFER_LEN];

	if((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		printf("Error socket(): %s(%d)\n", strerror(errno), errno);
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = port;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(listenfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		printf("Error bind(): %s(%d)\n", strerror(errno), errno);
		return -1;
	}

	if (listen(listenfd, MAX_BACKLOG) == -1) {
		printf("Error listen(): %s(%d)\n", strerror(errno), errno);
		return -1;
	}

	if ((connfd = accept(listenfd, NULL, NULL)) == -1) {
		printf("Error accept(): %s(%d)\n", strerror(errno), errno);
		return -1;
	}

	len = prepare_response_oneline(buffer, 220, 1, "hongfz FTP server ready.");
	if(msocket_write(connfd, buffer, len)==-1) {
		return -1;
	}

	while(1) {
		memset(buffer, '\0', MAX_BUFFER_LEN);
		if((len=msocket_read(connfd, buffer, MAX_BUFFER_LEN-1))==-1) {
			return -1;
		}
		messdebug(buffer);
		int order = unpack_message(buffer);
		idebug(order);
		switch (order){
			case 1:
				if(user_handler(connfd, buffer)==-1) {
					return -1;
				}
				break;
			case 2:
				if(pass_handler(connfd, buffer)==-1) {
					return -1;
				}
				break;
			default:
				if(request_not_support_handler(connfd)==-1) {
					return -1;
				}
		}
	}
	return 0;
}