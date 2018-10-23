#include <sys/socket.h>
#include <netinet/in.h>

#include <unistd.h>
#include <errno.h>

#include <string.h>
#include <memory.h>
#include <stdio.h>

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

	while(1) {
		char c[200];
		fgets(c, 200, stdin);
		if(c[0]=='r') {
			if((len = msocket_read(sockfd, sentence, BUFFER_SIZE-1))==-1) {
				return -1;
			}
			messdebug(sentence);
		}
		if(c[0]=='w') {
			fgets(sentence, 4096, stdin);
			len = strlen(sentence);
			sentence[len] = '\n';
			sentence[len+1] = '\0';
			// printf("%s", sentence);
			if(msocket_write(sockfd, sentence, len)==-1) {
				return -1;
			}
		}
	}

	close(sockfd);
	return 0;
}
