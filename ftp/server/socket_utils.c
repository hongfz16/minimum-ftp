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
		idebug(n);
		cdebug(buffer);
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