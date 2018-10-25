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

int request_not_support_handler(int connfd) {
	char response[256];
	memset(response, '\0', sizeof(response));
	int len = prepare_response_oneline(response, 500, 1, "Unsupported command.");
	if(msocket_write(connfd, response, strlen(response))==-1) {
		return -1;
	}
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
		free(contents[0]);
		free(contents[1]);
		if(msocket_write(connfd, response, strlen(response))==-1) {
			return -1;
		}
	} else {
		int len = prepare_response_oneline(response, 530, 1, "Invalid username.");
		if(msocket_write(connfd, response, strlen(response))==-1) {
			return -1;
		}
	}
	return 0;
}

int pass_handler(int connfd, char* buffer) {
	if(strlen(buffer)<7) {
		return request_not_support_handler(connfd);
	}
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
	for(i=0;i<3;++i) {
		free(contents[i]);
	}
	return 0;
}

int syst_handler(int connfd, char* buffer) {
	if(strlen(buffer)>5) {
		return request_not_support_handler(connfd);
	}
	char response[1024];
	memset(response, '\0', sizeof(response));
	int len = prepare_response_oneline(response, 215, 1, "UNIX Type: L8");
	if(msocket_write(connfd, response, strlen(response))==-1) {
		return -1;
	}
	return 0;
}

int type_handler(int connfd, char* buffer) {
	if(strlen(buffer)<7) {
		return request_not_support_handler(connfd);
	}
	char type_str[256];
	char response[1024];
	memset(type_str, '\0', sizeof(type_str));
	memset(response, '\0', sizeof(response));
	strncpy(type_str, buffer+5, strlen(buffer)-6);
	if(strcmp(type_str, "I")==0) {
		int len = prepare_response_oneline(response, 200, 1, "Type set to I.");
		if(msocket_write(connfd, response, strlen(response))==-1) {
			return -1;
		}
		return 0;
	} else {
		return request_not_support_handler(connfd);
	}
}

int port_handler(int connfd, char* buffer, int* pdatafd, struct sockaddr_in* paddr) {
	if(strlen(buffer)<7) {
		return request_not_support_handler(connfd);
	}
	int port = -1;
	char addr[256];
	char response[1024];
	memset(addr, '\0', sizeof(addr));
	memset(response, '\0', sizeof(response));
	int parse[6];
	char tmp[16];
	memset(tmp, '\0', sizeof(tmp));
	int i=0;
	int comma_count=0;
	int tmp_count=0;
	for(i=5;buffer[i]!='\0';++i) {
		if(buffer[i]==',' || buffer[i]=='\n') {
			if(tmp_count==0) {
				return request_not_support_handler(connfd);
			} else {
				if(comma_count>5) {
					return request_not_support_handler(connfd);
				}
				// sprintf(tmp, "%d", parse[comma_count]);
				parse[comma_count] = atoi(tmp);
			}
			comma_count+=1;
			tmp_count=0;
			memset(tmp, '\0', 16);
		} else if(buffer[i]<='9' && buffer[i]>='0') {
			tmp[tmp_count]=buffer[i];
			tmp_count+=1;
		} else {
			return request_not_support_handler(connfd);
		}
	}
	if(comma_count!=6) {
		return request_not_support_handler(connfd);
	}
	for(i=0;i<6;++i) {
		if(parse[i]<0 || parse[i]>255) {
			return request_not_support_handler(connfd);
		}
	}

	if(*pdatafd>0) {
		close(*pdatafd);
	}

	int len = prepare_response_oneline(response, 200, 1, "PORT command successful.");
	if(msocket_write(connfd, response, strlen(response))==-1) {
		return -1;
	}

	sprintf(addr, "%d.%d.%d.%d", parse[0], parse[1], parse[2], parse[3]);
	port = parse[4]*256+parse[5];

	cdebug(addr);
	idebug(port);

	if ((*pdatafd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		printf("Error socket(): %s(%d)\n", strerror(errno), errno);
		*pdatafd = -1;
		return -1;
	}

	memset(paddr, 0, sizeof(*paddr));
	paddr->sin_family = AF_INET;
	paddr->sin_port = port;
	if (inet_pton(AF_INET, addr, &(paddr->sin_addr)) <= 0) {
		printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
		return -1;
	}

	cdebug("Trying to connect...");
	if (connect(*pdatafd, (struct sockaddr*)paddr, sizeof(*paddr)) < 0) {
		printf("Error connect(): %s(%d)\n", strerror(errno), errno);
		*pdatafd = -1;
		return 0;
	}
	cdebug("Successfully connected.");
	return 0;
}

int retr_handler_file_err(int connfd, int datafd) {
	char response[1024];
	memset(response, '\0', sizeof(response));
	int len = prepare_response_oneline(response, 451, 1, "Trouble reading the file.");
	if(msocket_write(connfd, response, strlen(response))==-1) {
		return -1;
	}
	close(datafd);
	return 0;
}

int retr_handler_common_file(int connfd, int datafd, FILE* pfile, long fsize) {
	unsigned char* data_buffer = (unsigned char*)malloc(sizeof(unsigned char)*fsize);
	if(data_buffer==NULL) {
		return retr_handler_file_err(connfd, datafd);
	}
	size_t result = fread(data_buffer, 1, fsize, pfile);
	fclose(pfile);
	if(result!=fsize) {
		return retr_handler_file_err(connfd, datafd);
	}
	idebug(strlen(data_buffer));
	if(msocket_write(datafd, data_buffer, strlen(data_buffer))==-1) {
		char response[1024];
		memset(response, '\0', sizeof(response));
		int len = prepare_response_oneline(response, 426, 1, "Data transfer connection broken.");
		if(msocket_write(connfd, response, strlen(response))==-1) {
			free(data_buffer);
			return -1;
		}
	}
	free(data_buffer);
	return 0;
}

int retr_handler(int connfd, char* buffer, int datafd, char* cwd, char* root) {
	if(strlen(buffer)<7) {
		if(datafd>0) {
			close(datafd);
		}
		return request_not_support_handler(connfd);
	}
	char filename[1024];
	char response[1024];
	memset(filename, '\0', sizeof(filename));
	memset(response, '\0', sizeof(response));
	int len = prepare_response_oneline(response, 150, 1, "RETR command confirmed. Trying to send data.");
	if(msocket_write(connfd, response, strlen(response))==-1) {
		return -1;
	}
	memset(response, '\0', sizeof(response));
	if(datafd<0) {
		int len = prepare_response_oneline(response, 425, 1, "No TCP connection was established.");
		if(msocket_write(connfd, response, strlen(response))==-1) {
			return -1;
		}
	}
	strcpy(filename, cwd);
	strcat(filename, root);
	int non_sliash = 5;
	for(non_sliash=5;non_sliash<strlen(buffer);++non_sliash) {
		if(buffer[non_sliash]!='/') {
			break;
		}
	}
	filename[strlen(filename)]='/';
	strncpy(filename+strlen(filename), buffer+non_sliash, strlen(buffer)-non_sliash-1);

	FILE* pfile = fopen(filename, "rb");
	if(pfile==NULL) {
		return retr_handler_file_err(connfd, datafd);
	}
	fseek(pfile, 0, SEEK_END);
	long fsize = ftell(pfile);
	rewind(pfile);
	if(retr_handler_common_file(connfd, datafd, pfile, fsize)==-1) {
		return -1;
	}
	memset(response, '\0', sizeof(response));
	len = prepare_response_oneline(response, 226, 1, "Finish sending data.");
	if(msocket_write(connfd, response, strlen(response))==-1) {
		return -1;
	}
	close(datafd);
	return 0;
}

int pasv_handler(int connfd, char* buffer) {
	if(strlen(buffer)>5) {
		return request_not_support_handler(connfd);
	}
	char response[1024];
	memset(response, '\0', sizeof(response));
	//TODO: consider how to avoid blocking
	return 0;
}

int quit_handler(int connfd, char* buffer) {
	if(strlen(buffer)>5) {
		return request_not_support_handler(connfd);
	}
	char response[1024];
	memset(response, '\0', sizeof(response));
	int len = prepare_response_oneline(response, 221, 1, "Bye.");
	if(msocket_write(connfd, response, strlen(response))==-1) {
		return -1;
	}
	return 0;
}

int pwd_handler(int connfd, char* buffer, char* cwd, char* root) {
	if(strlen(buffer)>4) {
		return request_not_support_handler(connfd);
	}
	char response[1024];
	memset(response, '\0', sizeof(response));
	char path[1024];
	memset(path, '\0', sizeof(path));
	sprintf(path, "\"%s\" ", root);
	int len = prepare_response_oneline(response, 257, 1, path);
	if(msocket_write(connfd, response, strlen(response))==-1) {
		return -1;
	}
	return 0;
}

int list_handler(int connfd, char* buffer, int datafd, char* cwd, char* root) {
	if(strlen(buffer)>5) {
		if(datafd>0) {
			close(datafd);
		}
		return request_not_support_handler(connfd);
	}
	char response[1024];
	memset(response, '\0', sizeof(response));
	int len = prepare_response_oneline(response, 150, 1, "LIST command confirmed; Trying to send data.");
	if(msocket_write(connfd, response, strlen(response))==-1) {
		return -1;
	}
	memset(response, '\0', sizeof(response));
	if(datafd<0) {
		len = prepare_response_oneline(response, 425, 1, "No TCP connection was established.");
		if(msocket_write(connfd, response, strlen(response))==-1) {
			return -1;
		}
		return 0;
	}
	DIR* d;
	struct dirent * dir;
	char dirname[1024];
	strcpy(dirname, cwd);
	strcat(dirname, root);
	d = opendir(dirname);
	if(d) {
		char data_buffer[1024];
		memset(data_buffer, '\0', sizeof(data_buffer));
		while((dir=readdir(d))!=NULL) {
			if(strcmp(root, "/")==0) {
				if(strcmp(dir->d_name, "..")==0) {
					continue;
				}
			}
			strcat(data_buffer, dir->d_name);
			strcat(data_buffer, "\r\n");
		}
		closedir(d);
		if(msocket_write(datafd, data_buffer, strlen(data_buffer))==-1) {
			len = prepare_response_oneline(response, 426, 1, "TCP connection broken.");
			if(msocket_write(connfd, response, strlen(response))==-1) {
				return -1;
			}
			return 0;
		}
		len = prepare_response_oneline(response, 226, 1, "Data successfully transmitted.");
		if(msocket_write(connfd, response, strlen(response))==-1) {
			return -1;
		}
	} else {
		len = prepare_response_oneline(response, 451, 1, "Trouble reading directory.");
		if(msocket_write(connfd, response, strlen(response))==-1) {
			return -1;
		}
	}
	close(datafd);
	return 0;
}

int mkd_handler(int connfd, char* buffer, char* cwd, char* root) {
	if(strlen(buffer)<6) {
		return request_not_support_handler(connfd);
	}
	if(strpbrk(buffer, ".")!=NULL) {
		return request_not_support_handler(connfd);
	}
	if(buffer[4]=='/') {
		char modi_buffer[1024];
		memset(modi_buffer, '\0', strlen(modi_buffer));
		sprintf(modi_buffer, "MKD %s", buffer+5);
		return mkd_handler(connfd, modi_buffer, cwd, "/");
	}
	char response[1024];
	memset(response, '\0', sizeof(response));
	char directname[1024];
	memset(directname, '\0', sizeof(directname));
	strcpy(directname, cwd);
	strcat(directname, root);
	int i=0;
	for(i=4;i<strlen(buffer);++i) {
		if(buffer[i]!='/') {
			break;
		}
	}
	if(strcmp(root, "/")!=0) {
		strcat(directname, "/");
	}
	strcat(directname, buffer+i);
	directname[strlen(directname)-1] = '\0';
	cdebug("Making directory in:");
	cdebug(directname);
	if(mkdir(directname, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)==-1) {
		int len = prepare_response_oneline(response, 550, 1, "Directory creation failed.");
		if(msocket_write(connfd, response, strlen(response))==-1) {
			return -1;
		}
		return 0;
	}
	int len = prepare_response_oneline(response, 250, 1, "Directory created.");
	if(msocket_write(connfd, response, strlen(response))==-1) {
		return -1;
	}
	return 0;
}

int rmd_handler(int connfd, char* buffer, char* cwd, char* root) {
	if(strlen(buffer)<6) {
		return request_not_support_handler(connfd);
	}
	char response[1024];
	memset(response, '\0', sizeof(response));
	char directname[1024];
	memset(directname, '\0', sizeof(directname));
	strcpy(directname, cwd);
	strcat(directname, root);
	int i=0;
	for(i=4;i<strlen(buffer);++i) {
		if(buffer[i]!='/') {
			break;
		}
	}
	strcat(directname, buffer+i);
	directname[strlen(directname)-1] = '\0';
	if(rmdir(directname)==-1) {
		int len = prepare_response_oneline(response, 550, 1, "Directory remove failed.");
		if(msocket_write(connfd, response, strlen(response))==-1) {
			return -1;
		}
		return 0;
	}
	int len = prepare_response_oneline(response, 250, 1, "Directory removed.");
	if(msocket_write(connfd, response, strlen(response))==-1) {
		return -1;
	}
	return 0;
}

int cwd_handler(int connfd, char* buffer, char* cwd, char* root) {
	if(strlen(buffer)<6) {
		return request_not_support_handler(connfd);
	}
	char response[1024];
	memset(response, '\0', sizeof(response));
	char parameter[1024];
	memset(parameter, '\0', sizeof(parameter));
	int i = 0;
	int parameter_count = 0;
	if(buffer[4]=='/') {
		for(i=5;i<strlen(buffer)-1;++i) {
			parameter[parameter_count]=buffer[i];
			parameter_count++;
		}
		if(strlen(parameter)==0) {
			parameter[0]='.';
		}

		char modi_buffer[1024];
		memset(modi_buffer, '\0', sizeof(modi_buffer));
		sprintf(modi_buffer, "CWD %s\n", parameter);
		char modi_root[1024];
		memset(modi_root, '\0', sizeof(modi_root));
		modi_root[0]='/';
		int code = cwd_handler(connfd, modi_buffer, cwd, modi_root);
		if(code==-1) {
			return -1;
		} else if(code==1) {
			return 1;
		} else if(code==0) {
			memset(root, '\0', sizeof(root));
			strcpy(root, modi_root);
			return 0;
		}
	}
	for(i=4;i<strlen(buffer);++i) {
		if(buffer[i]!='/') {
			break;
		}
	}
	for(;i<strlen(buffer);++i) {
		if(buffer[i]=='\n') {
			continue;
		}
		parameter[parameter_count]=buffer[i];
		parameter_count++;
	}
	for(i=strlen(parameter)-1;i>=0;--i) {
		if(parameter[i]!='/') {
			break;
		}
		parameter[i]='\0';
	}
	if(strcmp(parameter, ".")==0) {
		int len = prepare_response_oneline(response, 250, 1, "Okay.");
		if(msocket_write(connfd, response, strlen(response))==-1) {
			return -1;
		}
		return 0;
	}
	if(strcmp(parameter, "..")!=0 && strpbrk(parameter, "..")!=NULL) {
		return request_not_support_handler(connfd);
	}
	if(strcmp(root, "/")==0 && strcmp(parameter, "..")==0) {
		char info[1024];
		memset(info, '\0', sizeof(info));
		sprintf(info, "%s: No such file or directory.", parameter);
		int len = prepare_response_oneline(response, 550, 1, info);
		if(msocket_write(connfd, response, strlen(response))==-1) {
			return -1;
		}
		return 1;
	}
	char test_root[1024];
	memset(test_root, '\0', sizeof(test_root));
	strcpy(test_root, root);
	char test_path[1024];
	memset(test_path, '\0', sizeof(test_path));
	strcpy(test_path, cwd);
	if(strcmp(parameter, "..")==0) {
		int root_len = strlen(test_root);
		for(;root_len>=0;--root_len) {
			if(test_root[root_len]!='/') {
				break;
			}
			test_root[root_len]='\0';
		}
		for(;root_len>=0;--root_len) {
			if(test_root[root_len]=='/') {
				break;
			}
			test_root[root_len]='\0';
		}
	} else {
		if(strcmp(test_root, "/")!=0) {
			strcat(test_root, "/");
		}
		strcat(test_root, parameter);
	}
	strcat(test_path, test_root);
	cdebug(test_path);
	DIR* d;
	struct dirent * dir;
	d = opendir(test_path);
	if(d) {
		memset(root, '\0', sizeof(root));
		strcpy(root, test_root);
		int len = prepare_response_oneline(response, 250, 1, "Okay.");
		if(msocket_write(connfd, response, strlen(response))==-1) {
			return -1;
		}
		closedir(d);
	} else {
		char info[1024];
		memset(info, '\0', sizeof(info));
		sprintf(info, "%s: No such file or directory.", parameter);
		int len = prepare_response_oneline(response, 550, 1, info);
		if(msocket_write(connfd, response, strlen(response))==-1) {
			return -1;
		}
		return 1;
	}
	return 0;
}

int stor_handler(int connfd, char* buffer, int datafd, char* cwd, char* root) {
	if(strlen(buffer)<7) {
		if(datafd>0) {
			close(datafd);
		}
		return request_not_support_handler(connfd);
	}
	char response[1024];
	memset(response, '\0', sizeof(response));
	if(datafd<0) {
		int len = prepare_response_oneline(response, 425, 1, "No TCP connection created.");
		if(msocket_write(connfd, response, strlen(response))==-1) {
			return -1;
		}
	}
	int len = prepare_response_oneline(response, 150, 1, "STOR command confirmed; Start receiving data.");
	if(msocket_write(connfd, response, strlen(response))==-1) {
		close(datafd);
		return -1;
	}
	memset(response, '\0', sizeof(response));
	unsigned char data_buffer[2048];
	int datalen = 0;
	if((datalen=msocket_read(datafd, data_buffer, 2048))==-1) {
		len = prepare_response_oneline(response, 426, 1, "Connection broken.");
		if(msocket_write(connfd, response, strlen(response))==-1) {
			close(datafd);
			return -1;
		}
		close(datafd);
		return 1;
	}
	char filename[1024];
	memset(filename, '\0', sizeof(filename));
	strcpy(filename, cwd);
	strcat(filename, root);
	if(strcmp(root, "/")!=0) {
		strcat(filename, "/");
	}
	//TODO: take care of "/test" case
	strcat(filename, buffer+5);
	filename[strlen(filename)-1]='\0';
	FILE* pfile = fopen(filename, "wb");
	if(fwrite(data_buffer, sizeof(unsigned char), datalen, pfile)==-1) {
		len = prepare_response_oneline(response, 451, 1, "Problem saving the file.");
		if(msocket_write(connfd, response, strlen(response))==-1) {
			close(datafd);
			return -1;
		}
		fclose(pfile);
		close(datafd);
		return 1;
	}
	len = prepare_response_oneline(response, 226, 1, "Successfully receive data.");
	if(msocket_write(connfd, response, strlen(response))==-1) {
		fclose(pfile);
		close(datafd);
		return -1;
	}
	fclose(pfile);
	close(datafd);
	return 0;
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