#include "socket_utils.h"

void cdebug(char* debug) {
	printf("DEBUG: %s\n", debug);
}

void idebug(int i) {
	printf("DEBUG: %d\n", i);
}

void messdebug(char* mess) {
	printf("FROM SERVER: %s", mess);
}

int msocket_read(int connfd, char* buffer, int buffer_len) {
	int p = 0;
	while (1) {
		int n = read(connfd, buffer + p, buffer_len - p);
		if (n < 0) {
			if(errno==EWOULDBLOCK) {
				return 0;
			}
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
	if(p>=2) {
		buffer[p-1]='\0';
		buffer[p-2]='\n';
	}
	return p-1;
}

int msocket_read_file(int connfd, char* buffer, int buffer_len) {
	int p = 0;
	while (1) {
		int n = read(connfd, buffer + p, buffer_len - p);
		cdebug(buffer);
		idebug(n);
		if (n < 0) {
			printf("Error read(): %s(%d)\n", strerror(errno), errno);
			close(connfd);
			return -1;
		} else if (n == 0) {
			break;
		} else {
			p += n;
		}
	}
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

int make_socket_non_blocking(int fd) {
	int flags;
	if((flags = fcntl(fd, F_GETFL))==-1) {
		printf("Error fcntl(): Could not get flags on TCP socket\n");
		return -1;
	}

	if(fcntl(fd, F_SETFL, flags | O_NONBLOCK)==-1) {
		printf("Error fcntl(): Could not set TCP socket to be non-blocking\n");
		return -1;
	}
	return 0;
}

int create_non_blocking_listen_socket(int port, int max_backlog) {
	int listenfd;
	struct sockaddr_in addr;
	int flags;

	if((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		printf("Error socket(): %s(%d)\n", strerror(errno), errno);
		return -1;
	}

	if((flags = fcntl(listenfd, F_GETFL))==-1) {
		printf("Error fcntl(): Could not get flags on TCP listening socket\n");
		return -1;
	}

	if(fcntl(listenfd, F_SETFL, flags | O_NONBLOCK)==-1) {
		printf("Error fcntl(): Could not set TCP listening socket to be non-blocking\n");
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(listenfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		printf("Error bind(): %s(%d)\n", strerror(errno), errno);
		return -1;
	}

	if (listen(listenfd, max_backlog) == -1) {
		printf("Error listen(): %s(%d)\n", strerror(errno), errno);
		return -1;
	}

	return listenfd;
}

int create_blocking_listen_socket(int port, int max_backlog) {
	int listenfd;
	struct sockaddr_in addr;

	if ((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		printf("Error socket(): %s(%d)\n", strerror(errno), errno);
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(listenfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		printf("Error bind(): %s(%d)\n", strerror(errno), errno);
		return -1;
	}

	if (listen(listenfd, max_backlog) == -1) {
		printf("Error listen(): %s(%d)\n", strerror(errno), errno);
		return -1;
	}

	return listenfd;
}
