#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>

#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

#include "socket_utils.h"
#include "server_handler.h"

#define ROOT_LENGTH 1024
#define DEFAULT_PORT 21
#define DEFAULT_PATH "/tmp"
#define MAX_BACKLOG 10
#define MAX_BUFFER_LEN 10086

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

int unpack_message(char* buffer) {
	char order[16];
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
	if(strcmp(order, "MKD ")==0) {
		return 9;
	}
	if(strcmp(order, "CWD ")==0) {
		return 10;
	}
	if(strcmp(order, "PWD\n")==0) {
		return 11;
	}
	if(strcmp(order, "LIST")==0) {
		return 12;
	}
	if(strcmp(order, "RMD ")==0) {
		return 13;
	}
	if(strcmp(order, "RNFR")==0) {
		return 14;
	}
	if(strcmp(order, "RNTO")==0) {
		return 15;
	}
	if(strcmp(order, "STOR")==0) {
		return 16;
	}
	return 100;
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
	int datafd = -1;
	struct sockaddr_in data_addr;
	char cwd[1024];
	memset(cwd, '\0', sizeof(cwd));
	strcpy(cwd, root);
	char ftp_root[1024];
	memset(ftp_root, '\0', sizeof(ftp_root));
	ftp_root[0]='/';

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
		int is_quit=0;
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
			case 3:
				if(syst_handler(connfd, buffer)==-1) {
					return -1;
				}
				break;
			case 4:
				if(type_handler(connfd, buffer)==-1) {
					return -1;
				}
				break;
			case 5:
				if(port_handler(connfd, buffer, &datafd, &data_addr)==-1) {
					return -1;
				}
				break;
			case 6:
				if(retr_handler(connfd, buffer, datafd, cwd, ftp_root)==-1) {
					return -1;
				}
				datafd = -1;
				break;
			case 7:
				if(pasv_handler(connfd, buffer)==-1) {
					return -1;
				}
				break;
			case 8:
				if(quit_handler(connfd, buffer)==-1) {
					return -1;
				}
				is_quit=1;
				break;
			case 9:
				if(mkd_handler(connfd, buffer, cwd, ftp_root)==-1) {
					return -1;
				}
				break;
			case 10:
				if(cwd_handler(connfd, buffer, cwd, ftp_root)==-1) {
					return -1;
				}
				break;
			case 11:
				if(pwd_handler(connfd, buffer, cwd, ftp_root)==-1) {
					return -1;
				}
				break;
			case 12:
				if(list_handler(connfd, buffer, datafd, cwd, ftp_root)==-1) {
					return -1;
				}
				datafd = -1;
				break;
			case 13:
				if(rmd_handler(connfd, buffer, cwd, ftp_root)==-1) {
					return -1;
				}
				break;
			case 16:
				if(stor_handler(connfd, buffer, datafd, cwd, ftp_root)==-1) {
					return -1;
				}
				break;
			default:
				if(request_not_support_handler(connfd)==-1) {
					return -1;
				}
		}
		if(is_quit) {break;}
	}
	close(connfd);
	close(listenfd);
	free(root);
	return 0;
}