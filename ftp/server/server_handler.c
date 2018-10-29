#include "server_handler.h"
#include "socket_utils.h"

int request_not_support_handler(int connfd) {
	char response[256];
	memset(response, '\0', sizeof(response));
	int len = prepare_response_oneline(response, 500, 1, "Unsupported command.");
	if(msocket_write(connfd, response, strlen(response))==-1) {
		return -1;
	}
	return 0;
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

int pass_handler(int connfd, char* buffer, int* login_flag) {
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
	*login_flag = 1;
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

int port_handler(int connfd, char* buffer, int* pdatafd, struct sockaddr_in* paddr, int* pdatalistenfd) {
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
	if(*pdatalistenfd>0) {
		close(*pdatalistenfd);
		*pdatalistenfd=-1;
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
	paddr->sin_port = htons(port);
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
	char* data_buffer = (char*)malloc(sizeof(char)*fsize);
	if(data_buffer==NULL) {
		return retr_handler_file_err(connfd, datafd);
	}
	size_t result = fread(data_buffer, 1, fsize, pfile);
	fclose(pfile);
	if(result!=fsize) {
		return retr_handler_file_err(connfd, datafd);
	}
	idebug(fsize);
	if(msocket_write(datafd, data_buffer, fsize)==-1) {
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
	memset(filename, '\0', sizeof(filename));
	
	char response[1024];
	memset(response, '\0', sizeof(response));
	if(datafd<0) {
		int len = prepare_response_oneline(response, 425, 1, "No TCP connection was established.");
		if(msocket_write(connfd, response, strlen(response))==-1) {
			return -1;
		}
	}

	memset(response, '\0', sizeof(response));
	int len = prepare_response_oneline(response, 150, 1, "RETR command confirmed. Trying to send data.");
	if(msocket_write(connfd, response, strlen(response))==-1) {
		return -1;
	}
	strcpy(filename, cwd);
	// strcat(filename, root);
	// int non_sliash = 5;
	// for(non_sliash=5;non_sliash<strlen(buffer);++non_sliash) {
	// 	if(buffer[non_sliash]!='/') {
	// 		break;
	// 	}
	// }
	// filename[strlen(filename)]='/';
	// strncpy(filename+strlen(filename), buffer+non_sliash, strlen(buffer)-non_sliash-1);

	if(change_directory(root, buffer+5, filename+strlen(filename))==-1) {
		return retr_handler_file_err(connfd, datafd);
	}

	// filename[strlen(filename)-1]='\0';
	cdebug(filename);

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

int pasv_handler(int connfd, char* buffer, int* host_ip, int* pdatafd, int* pdatalistenfd) {
	if(strlen(buffer)>5) {
		return request_not_support_handler(connfd);
	}

	if(*pdatafd>0) {
		close(*pdatafd);
		*pdatafd=-1;
	}
	if(*pdatalistenfd>0) {
		close(*pdatalistenfd);
		*pdatalistenfd=-1;
	}
	char response[1024];
	memset(response, '\0', sizeof(response));
	//TODO: consider how to avoid blocking
	int listenfd;
	int port = 8001;
	while(1) {
		*pdatalistenfd = create_non_blocking_listen_socket(port, 10);
		if(*pdatalistenfd!=-1) {
			break;
		} else {
			port += 1;
		}
	}
	sprintf(response, "227 =%d,%d,%d,%d,%d,%d\r\n", host_ip[0], host_ip[1], host_ip[2], host_ip[3], port/256, port%256);
	if(msocket_write(connfd, response, strlen(response))==-1) {
		return -1;
	}
	// if ((*pdatafd = accept(listenfd, NULL, NULL)) == -1) {
	// 	printf("Error accept(): %s(%d)\n", strerror(errno), errno);
	// 	return 0;
	// }
	return 0;
}

int quit_handler(int connfd, char* buffer, int* quit_flag) {
	if(strlen(buffer)>5) {
		return request_not_support_handler(connfd);
	}
	char response[1024];
	memset(response, '\0', sizeof(response));
	int len = prepare_response_oneline(response, 221, 1, "Bye.");
	if(msocket_write(connfd, response, strlen(response))==-1) {
		return -1;
	}
	*quit_flag=1;
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
	if(datafd<0) {
		int len = prepare_response_oneline(response, 425, 1, "No TCP connection was established.");
		if(msocket_write(connfd, response, strlen(response))==-1) {
			return -1;
		}
		cdebug("datafd<0");
		return 0;
	}
	memset(response, '\0', sizeof(response));
	int len = prepare_response_oneline(response, 150, 1, "LIST command confirmed; Trying to send data.");
	if(msocket_write(connfd, response, strlen(response))==-1) {
		cdebug("150 error.");
		return -1;
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
		// cdebug(data_buffer);
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
	// if(strstr(buffer, ".")!=NULL) {
	// 	return request_not_support_handler(connfd);
	// }
	// if(buffer[4]=='/') {
	// 	char modi_buffer[1024];
	// 	memset(modi_buffer, '\0', strlen(modi_buffer));
	// 	sprintf(modi_buffer, "MKD %s", buffer+5);
	// 	return mkd_handler(connfd, modi_buffer, cwd, "/");
	// }
	char response[1024];
	memset(response, '\0', sizeof(response));
	char directname[1024];
	memset(directname, '\0', sizeof(directname));
	strcpy(directname, cwd);
	// strcat(directname, root);
	// int i=0;
	// for(i=4;i<strlen(buffer);++i) {
	// 	if(buffer[i]!='/') {
	// 		break;
	// 	}
	// }
	// if(strcmp(root, "/")!=0) {
	// 	strcat(directname, "/");
	// }
	// strcat(directname, buffer+i);
	if(change_directory(root, buffer+4, directname+strlen(directname))==-1) {
		int len = prepare_response_oneline(response, 550, 1, "Directory creation failed.");
		if(msocket_write(connfd, response, strlen(response))==-1) {
			return -1;
		}
		return 0;
	}
	// directname[strlen(directname)-1] = '\0';
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
	// strcat(directname, root);
	// int i=0;
	// for(i=4;i<strlen(buffer);++i) {
	// 	if(buffer[i]!='/') {
	// 		break;
	// 	}
	// }
	// strcat(directname, buffer+i);
	if(change_directory(root, buffer+4, directname+strlen(directname))==-1) {
		int len = prepare_response_oneline(response, 550, 1, "Directory remove failed.");
		if(msocket_write(connfd, response, strlen(response))==-1) {
			return -1;
		}
		return 0;
	}
	// directname[strlen(directname)-1] = '\0';
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
	strcpy(parameter, buffer+4);
	parameter[strlen(parameter)-1]='\0';
	// int i = 0;
	// int parameter_count = 0;
	// if(buffer[4]=='/') {
	// 	for(i=5;i<strlen(buffer)-1;++i) {
	// 		parameter[parameter_count]=buffer[i];
	// 		parameter_count++;
	// 	}
	// 	if(strlen(parameter)==0) {
	// 		parameter[0]='.';
	// 	}

	// 	char modi_buffer[1024];
	// 	memset(modi_buffer, '\0', sizeof(modi_buffer));
	// 	sprintf(modi_buffer, "CWD %s\n", parameter);
	// 	char modi_root[1024];
	// 	memset(modi_root, '\0', sizeof(modi_root));
	// 	modi_root[0]='/';
	// 	int code = cwd_handler(connfd, modi_buffer, cwd, modi_root);
	// 	if(code==-1) {
	// 		return -1;
	// 	} else if(code==1) {
	// 		return 1;
	// 	} else if(code==0) {
	// 		memset(root, '\0', ROOT_LENGTH);
	// 		strcpy(root, modi_root);
	// 		return 0;
	// 	}
	// }
	// for(i=4;i<strlen(buffer);++i) {
	// 	if(buffer[i]!='/') {
	// 		break;
	// 	}
	// }
	// for(;i<strlen(buffer);++i) {
	// 	if(buffer[i]=='\n') {
	// 		continue;
	// 	}
	// 	parameter[parameter_count]=buffer[i];
	// 	parameter_count++;
	// }
	// for(i=strlen(parameter)-1;i>=0;--i) {
	// 	if(parameter[i]!='/') {
	// 		break;
	// 	}
	// 	parameter[i]='\0';
	// }
	// if(strcmp(parameter, ".")==0) {
	// 	int len = prepare_response_oneline(response, 250, 1, "Okay.");
	// 	if(msocket_write(connfd, response, strlen(response))==-1) {
	// 		return -1;
	// 	}
	// 	return 0;
	// }
	// if(strcmp(parameter, "..")!=0 && strstr(parameter, "..")!=NULL) {
	// 	return request_not_support_handler(connfd);
	// }
	// if(strcmp(root, "/")==0 && strcmp(parameter, "..")==0) {
	// 	char info[1024];
	// 	memset(info, '\0', sizeof(info));
	// 	sprintf(info, "%s: No such file or directory.", parameter);
	// 	int len = prepare_response_oneline(response, 550, 1, info);
	// 	if(msocket_write(connfd, response, strlen(response))==-1) {
	// 		return -1;
	// 	}
	// 	return 1;
	// }
	char test_root[1024];
	memset(test_root, '\0', sizeof(test_root));
	// strcpy(test_root, root);
	// char test_path[1024];
	// memset(test_path, '\0', sizeof(test_path));
	// strcpy(test_path, cwd);
	// if(strcmp(parameter, "..")==0) {
	// 	int root_len = strlen(test_root);
	// 	for(;root_len>=0;--root_len) {
	// 		if(test_root[root_len]!='/') {
	// 			break;
	// 		}
	// 		test_root[root_len]='\0';
	// 	}
	// 	for(;root_len>=0;--root_len) {
	// 		if(test_root[root_len]=='/') {
	// 			break;
	// 		}
	// 		test_root[root_len]='\0';
	// 	}
	// } else {
	// 	if(strcmp(test_root, "/")!=0) {
	// 		strcat(test_root, "/");
	// 	}
	// 	strcat(test_root, parameter);
	// }
	// strcat(test_path, test_root);
	char test_path[2048];
	memset(test_path, '\0', sizeof(test_path));
	strcpy(test_path, cwd);
	// printf("%s\n", test_path);
	// printf("%s\n", root);
	// printf("%s\n", buffer+4);
	if(change_directory(root, buffer+4, test_root)==-1) {
		char info[1024];
		memset(info, '\0', sizeof(info));
		sprintf(info, "%s: No such file or directory.", parameter);
		int len = prepare_response_oneline(response, 550, 1, info);
		if(msocket_write(connfd, response, strlen(response))==-1) {
			return -1;
		}
		return 1;
	}
	// test_root[strlen(test_root)-1]='\0';
	strcat(test_path, test_root);
	// cdebug(test_path);
	DIR* d;
	struct dirent * dir;
	d = opendir(test_path);
	if(d) {
		memset(root, '\0', ROOT_LENGTH);
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
	char* data_buffer = (char*)malloc(sizeof(char)*1024*1024);
	memset(data_buffer, '\0', 1024*1024);
	int datalen = 0;
	if((datalen=msocket_read_file(datafd, data_buffer, 1024*1024))==-1) {
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
	// strcat(filename, root);
	// if(strcmp(root, "/")!=0) {
	// 	strcat(filename, "/");
	// }
	// //TODO: take care of "/test" case
	// strcat(filename, buffer+5);
	if(change_directory(root, buffer + 5, filename+strlen(filename))==-1) {
		len = prepare_response_oneline(response, 451, 1, "Problem saving the file.");
		if(msocket_write(connfd, response, strlen(response))==-1) {
			close(datafd);
			return -1;
		}
		close(datafd);
		return 1;
	}
	// filename[strlen(filename)-1]='\0';
	FILE* pfile = fopen(filename, "wb");
	if(fwrite(data_buffer, sizeof(char), datalen, pfile)==-1) {
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

int login_required_handler(int connfd) {
	char response[1024];
	memset(response, '\0', sizeof(response));
	int len = prepare_response_oneline(response, 530, 1, "Please login first.");
	if(msocket_write(connfd, response, strlen(response))==-1) {
		return -1;
	}
	return 0;
}

int rnfr_handler(int connfd, char* buffer, char* cwd, char* root, int* rnfr_flag, char* move_file_path) {
	char test_root[1024];
	memset(test_root, '\0', sizeof(test_root));
	char response[1024];
	memset(response, '\0', sizeof(response));
	char path[1024];
	memset(path, '\0', sizeof(path));
	int i=4;
	for(;i<strlen(buffer);++i) {
		if(buffer[i]!=' ') {
			break;
		}
	}
	strcpy(path, buffer+i);
	path[strlen(path)-1]='\0';
	if(change_directory(root, path, test_root)==-1) {
		int len = prepare_response_oneline(response, 450, 1, "Illegal file path");
		if(msocket_write(connfd, response, strlen(response))==-1) {
			return -1;
		}
		return 1;
	}
	char filepath[1024];
	memset(filepath, '\0', sizeof(filepath));
	strcpy(filepath, cwd);
	strcat(filepath, test_root);
	FILE* pfile;
	pfile = fopen(filepath, "rb");
	if(pfile==NULL) {
		int len = prepare_response_oneline(response, 450, 1, "No such file.");
		if(msocket_write(connfd, response, strlen(response))==-1) {
			return -1;
		}
		return 1;
	}
	fclose(pfile);
	int len = prepare_response_oneline(response, 350, 1, "Ready to rename the file.");
	if(msocket_write(connfd, response, strlen(response))==-1) {
		return -1;
	}
	strcpy(move_file_path, filepath);
	*rnfr_flag = 2;
	return 0;
}

int same_file(int fd1, int fd2) {
	struct stat stat1, stat2;
	if(fstat(fd1, &stat1)<0) return -1;
	if(fstat(fd2, &stat2)<0) return -1;
	return (stat1.st_dev == stat2.st_dev) && (stat1.st_ino == stat2.st_ino);
}

int rnto_handler(int connfd, char* buffer, char* cwd, char* root, int* rnfr_flag, char* move_file_path) {
	char response[1024];
	memset(response, '\0', sizeof(response));
	if(*rnfr_flag!=1) {
		int len = prepare_response_oneline(response, 503, 1, "RNFR command not specified.");
		if(msocket_write(connfd, response, strlen(response))==1) {
			return -1;
		}
		return 1;
	}
	char test_root[1024];
	memset(test_root, '\0', sizeof(test_root));
	char path[1024];
	memset(path, '\0', sizeof(path));
	int i=4;
	for(;i<strlen(buffer);++i) {
		if(buffer[i]!=' ') {
			break;
		}
	}
	strcpy(path, buffer+i);
	path[strlen(path)-1]='\0';
	if(change_directory(root, path, test_root)==-1) {
		int len = prepare_response_oneline(response, 450, 1, "Illegal file path");
		if(msocket_write(connfd, response, strlen(response))==-1) {
			return -1;
		}
		return 1;
	}
	char filepath[1024];
	memset(filepath, '\0', sizeof(filepath));
	strcpy(filepath, cwd);
	strcat(filepath, test_root);
	FILE *fromfile;
	FILE *tofile;
	fromfile = fopen(move_file_path, "rb");
	tofile = fopen(filepath, "wb");
	if(tofile==NULL || fromfile==NULL) {
		int len = prepare_response_oneline(response, 550, 1,  "Problem renaming the file.");
		if(msocket_write(connfd, response, strlen(response))==-1) {
			return -1;
		}
		if(fromfile) {
			fclose(fromfile);
		}
		if(tofile) {
			fclose(tofile);			
		}
		return 1;
	}

	if(same_file(fileno(fromfile), fileno(tofile))) {
		fclose(fromfile);
		fclose(tofile);
		int len = prepare_response_oneline(response, 250, 1, "Successfully rename the file.");
		if(msocket_write(connfd, response, strlen(response))==-1) {
			return -1;
		}
		return 0;
	}

	fseek(fromfile, 0, SEEK_END);
	long fsize = ftell(fromfile);
	rewind(fromfile);
	unsigned char* file_buffer = (unsigned char*)malloc(sizeof(unsigned char)*fsize);
	size_t result = fread(file_buffer, sizeof(unsigned char), fsize, fromfile);
	if(result!=fsize) {
		int len = prepare_response_oneline(response, 550, 1,  "Problem renaming the file.");
		if(msocket_write(connfd, response, strlen(response))==-1) {
			return -1;
		}
		if(fromfile) {
			fclose(fromfile);
		}
		if(tofile) {
			fclose(tofile);			
		}
		free(file_buffer);
		return 1;
	}
	if(fwrite(file_buffer, sizeof(unsigned char), fsize, tofile)==-1) {
		int len = prepare_response_oneline(response, 550, 1,  "Problem renaming the file.");
		if(msocket_write(connfd, response, strlen(response))==-1) {
			return -1;
		}
		if(fromfile) {
			fclose(fromfile);
		}
		if(tofile) {
			fclose(tofile);			
		}
		free(file_buffer);
		return 1;
	}
	if(fromfile) {
		fclose(fromfile);
	}
	if(tofile) {
		fclose(tofile);			
	}
	free(file_buffer);
	if(remove(move_file_path)==-1) {
		int len = prepare_response_oneline(response, 550, 1,  "Problem renaming the file.");
		if(msocket_write(connfd, response, strlen(response))==-1) {
			return -1;
		}
		return 1;
	}
	int len = prepare_response_oneline(response, 250, 1, "Successfully rename the file.");
	if(msocket_write(connfd, response, strlen(response))==-1) {
		return -1;
	}
	return 0;
}