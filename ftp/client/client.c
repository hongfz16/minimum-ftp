#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <errno.h>

#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 8192

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
	memset(buffer, '\0', BUFFER_SIZE);
	int p = 0;
	while (1) {
		int n = read(connfd, buffer + p, buffer_len - p);
		idebug(n);
		if (n < 0) {
			printf("Error read(): %s(%d)\n", strerror(errno), errno);
			close(connfd);
			return -1;
		} else if (n == 0) {
			break;
		} else {
			p += n;
			if (buffer[p - 1] == '\n') {
				break;
			}
		}
	}
	return p;
}

int msocket_read_file(int connfd, char* buffer, int buffer_len) {
	memset(buffer, '\0', BUFFER_SIZE);
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

int main(int argc, char **argv) {
	int sockfd;
	struct sockaddr_in addr;
	char sentence[BUFFER_SIZE];
	int len;
	int p;

	//创建socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		printf("Error socket(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	//设置目标主机的ip和port
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = 8000;
	if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) <= 0) {			//转换ip地址:点分十进制-->二进制
		printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	//连接上目标主机（将socket和目标主机连接）-- 阻塞函数
	if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		printf("Error connect(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	int listenfd, datafd;
	struct sockaddr_in data_addr;

	while(1) {
		char c[200];
		fgets(c, 200, stdin);
		if(c[0]=='r') {
			if((len = msocket_read(sockfd, sentence, BUFFER_SIZE-1))==-1) {
				return -1;
			}
			messdebug(sentence);
		}
		if(c[0]=='f') {
			unsigned char data_buffer[2048];
			memset(data_buffer, '\0', sizeof(data_buffer));
			if((len=msocket_read_file(datafd, data_buffer, 2048))==-1) {
				return -1;
			}
			FILE *pfile = fopen("retr.txt", "wb");
			fwrite(data_buffer, sizeof(unsigned char), len, pfile);
			fclose(pfile);
			cdebug("Save to retr.txt.");
		}
		if(c[0]=='s') {
			memset(sentence, '\0', sizeof(sentence));
			sprintf(sentence, "STOR fromclient.txt\r\n");
			if(msocket_write(sockfd, sentence, strlen(sentence))==-1) {
				return -1;
			}
			FILE *pfile = fopen("retr.txt", "rb");
			fseek(pfile, 0, SEEK_END);
			long fsize = ftell(pfile);
			rewind(pfile);
			unsigned char* data_buffer = (unsigned char*)malloc(sizeof(unsigned char)*fsize);
			size_t result = fread(data_buffer, 1, fsize, pfile);
			fclose(pfile);
			msocket_write(datafd, data_buffer, strlen(data_buffer));
		}
		if(c[0]=='w') {
			fgets(sentence, 4096, stdin);
			len = strlen(sentence);
			sentence[len-1] = '\r';
			sentence[len] = '\n';
			if(msocket_write(sockfd, sentence, len+1)==-1) {
				return -1;
			}
		}
		if(c[0]=='p') {
			sprintf(sentence, "PORT 127,0,0,1,31,68\r\n");
			if(msocket_write(sockfd, sentence, strlen(sentence))==-1) {
				return -1;
			}
			if((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
				printf("Error socket(): %s(%d)\n", strerror(errno), errno);
				return -1;
			}

			memset(&data_addr, 0, sizeof(data_addr));
			data_addr.sin_family = AF_INET;
			data_addr.sin_port = 8004;
			data_addr.sin_addr.s_addr = htonl(INADDR_ANY);

			if (bind(listenfd, (struct sockaddr*)&data_addr, sizeof(data_addr)) == -1) {
				printf("Error bind(): %s(%d)\n", strerror(errno), errno);
				return -1;
			}

			if (listen(listenfd, 10) == -1) {
				printf("Error listen(): %s(%d)\n", strerror(errno), errno);
				return -1;
			}

			cdebug("Wait to connect...");
			if ((datafd = accept(listenfd, NULL, NULL)) == -1) {
				printf("Error accept(): %s(%d)\n", strerror(errno), errno);
				return -1;
			}
			cdebug("Successfully connected.");
		}
		if(c[0]=='k') {
			close(datafd);
			close(listenfd);
			cdebug("Close all the ports.");
		}
	}

	close(sockfd);
	return 0;
}
