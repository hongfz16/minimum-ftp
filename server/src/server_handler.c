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

int port_handler(int connfd, char* buffer, int* datafd_arr, int* datafd_counter, struct sockaddr_in* paddr, int* pdatalistenfd) {
	if(strlen(buffer)<7) {
		return request_not_support_handler(connfd);
	}
	int* pdatafd = &(datafd_arr[*datafd_counter]);
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
	*datafd_counter = (*datafd_counter+1)%65536;
	if(datafd_arr[*datafd_counter]>0) {
		close(datafd_arr[*datafd_counter]);
		datafd_arr[*datafd_counter] = -1;
	}
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
	long left = fsize;
	char* data_buffer = (char*)malloc(sizeof(char)*1024*1024);
	int buffer_size = 1024*1024;
	while(left) {
		int tmp_size = buffer_size;
		if(left < buffer_size) {
			tmp_size = left;
			left = 0;
		} else {
			left -= buffer_size;
		}
		if(data_buffer==NULL) {
			fclose(pfile);
			return retr_handler_file_err(connfd, datafd);
		}
		size_t result = fread(data_buffer, 1, tmp_size, pfile);
		// fclose(pfile);
		if(result != tmp_size) {
			fclose(pfile);
			free(data_buffer);
			return retr_handler_file_err(connfd, datafd);
		}
		idebug(fsize);
		if(msocket_write(datafd, data_buffer, tmp_size)==-1) {
			char response[1024];
			memset(response, '\0', sizeof(response));
			int len = prepare_response_oneline(response, 426, 1, "Data transfer connection broken.");
			if(msocket_write(connfd, response, strlen(response))==-1) {
				fclose(pfile);
				free(data_buffer);
				return -1;
			}
			fclose(pfile);
			free(data_buffer);
			return 1;
		}
	}
	fclose(pfile);
	free(data_buffer);
	return 0;
}

struct retr_params {
	int connfd;
	int datafd;
	int* datafd_arr;
	int datafd_counter;
	FILE* pfile;
	long fsize;
};

void* retr_thread_run(void *arg) {
	struct retr_params *params;
	params = (struct retr_params*) arg;
	int connfd = params->connfd;
	int datafd = params->datafd;
	FILE* pfile = params->pfile;
	long fsize = params->fsize;
	int* datafd_arr = params->datafd_arr;
	int datafd_counter = params->datafd_counter;

	char response[1024];
	int *return_val = (int*)malloc(sizeof(int));
	*return_val = -1;
	if(retr_handler_common_file(connfd, datafd, pfile, fsize)!=0) {
		close(datafd);
		fclose(pfile);
		return (void*)return_val;
	}
	fclose(pfile);
	memset(response, '\0', sizeof(response));
	int len = prepare_response_oneline(response, 226, 1, "Finish sending data.");
	if(msocket_write(connfd, response, strlen(response))==-1) {
		close(datafd);
		// datafd_arr[datafd_counter] = -1;
		return (void*)return_val;
	}
	close(datafd);
	datafd_arr[datafd_counter] = -1;
	*return_val = 0;
	return (void*)return_val;
}

int retr_handler(int connfd, char* buffer, int* datafd_arr, int datafd_counter, char* cwd, char* root, int* rest_size, int* rest_flag) {
	int datafd = datafd_arr[datafd_counter];

	if(strlen(buffer)<7) {
		if(datafd>0) {
			close(datafd);
			datafd_arr[datafd_counter] = -1;
		}
		return request_not_support_handler(connfd);
	}
	if(*rest_flag==0) {
		*rest_size = 0;
	}
	char filename[1024];
	memset(filename, '\0', sizeof(filename));
	
	char response[1024];
	memset(response, '\0', sizeof(response));
	if(datafd<0) {
		int len = prepare_response_oneline(response, 425, 1, "No TCP connection was established.");
		if(msocket_write(connfd, response, strlen(response))==-1) {
			close(datafd);
			datafd_arr[datafd_counter] = -1;
			return -1;
		}
	}

	memset(response, '\0', sizeof(response));
	int len = prepare_response_oneline(response, 150, 1, "RETR command confirmed. Trying to send data.");
	if(msocket_write(connfd, response, strlen(response))==-1) {
		close(datafd);
		datafd_arr[datafd_counter] = -1;
		return -1;
	}
	strcpy(filename, cwd);
	if(change_directory(root, buffer+5, filename+strlen(filename))==-1) {
		return retr_handler_file_err(connfd, datafd);
	}
	cdebug(filename);

	FILE* pfile = fopen(filename, "rb");
	if(pfile==NULL) {
		return retr_handler_file_err(connfd, datafd);
	}
	fseek(pfile, 0, SEEK_END);
	long fsize = ftell(pfile);
	rewind(pfile);
	if(fsize > *rest_size) {
		int result = fseek(pfile, *rest_size, SEEK_SET);
		fsize -= *rest_size;
	} else {
		fseek(pfile, 0, SEEK_END);
		fsize = 0;
	}
	struct retr_params param;
	param.connfd = connfd;
	param.datafd = datafd;
	param.pfile = pfile;
	param.fsize = fsize;
	param.datafd_counter = datafd_counter;
	param.datafd_arr = datafd_arr;
	pthread_t id;
	if(pthread_create(&id, NULL, retr_thread_run, &param)) {
		printf("Error creating retr thread.\n");
		return retr_handler_file_err(connfd, datafd);
	}
	pthread_detach(id);
	return 0;
}

int pasv_handler(int connfd, char* buffer, int* host_ip, int* pdatalistenfd) {
	if(strlen(buffer)>5) {
		return request_not_support_handler(connfd);
	}
	if(*pdatalistenfd>0) {
		close(*pdatalistenfd);
		*pdatalistenfd=-1;
	}
	char response[1024];
	memset(response, '\0', sizeof(response));
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

int list_handler(int connfd, char* buffer, int* datafd_arr, int datafd_counter, char* cwd, char* root) {
	int datafd = datafd_arr[datafd_counter];
	if(strlen(buffer)>5) {
		if(datafd>0) {
			close(datafd);
			datafd_arr[datafd_counter] = -1;
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
		close(datafd);
		datafd_arr[datafd_counter] = -1;
		return -1;
	}
	
	DIR* d;
	struct dirent * dir;
	char dirname[1024];
	strcpy(dirname, cwd);
	strcat(dirname, root);
	d = opendir(dirname);
	memset(response, '\0', sizeof(response));
	if(d) {
		char data_buffer[8192*4];
		memset(data_buffer, '\0', sizeof(data_buffer));
		char path_buffer[1024];
		FILE *command_fp;
		/* Open the command for reading. */
		char ls_command[1024];
		sprintf(ls_command, "/bin/ls -l -a %s", dirname);
		command_fp = popen(ls_command, "r");
		if (command_fp == NULL) {
			len = prepare_response_oneline(response, 451, 1, "Trouble reading directory.");
			if(msocket_write(connfd, response, strlen(response))==-1) {
				close(datafd);
				datafd_arr[datafd_counter] = -1;
				return -1;
			}
			close(datafd);
			datafd_arr[datafd_counter] = -1;
			return 0;
		}
		/* Read the output a line at a time - output it. */
		int skip_first = 1;
		while (fgets(path_buffer, sizeof(path_buffer)-1, command_fp) != NULL) {
			if(skip_first) {
				skip_first = 0;
				continue;
			}
			path_buffer[strlen(path_buffer)-1]='\0';
			strcat(data_buffer, path_buffer);
			strcat(data_buffer, "\r\n");
		}

		/* close */
		pclose(command_fp);
		closedir(d);
		if(msocket_write(datafd, data_buffer, strlen(data_buffer))==-1) {
			len = prepare_response_oneline(response, 426, 1, "TCP connection broken.");
			if(msocket_write(connfd, response, strlen(response))==-1) {
				close(datafd);
				datafd_arr[datafd_counter] = -1;
				return -1;
			}
			close(datafd);
			datafd_arr[datafd_counter] = -1;
			return 0;
		}
		len = prepare_response_oneline(response, 226, 1, "Data successfully transmitted.");
		if(msocket_write(connfd, response, strlen(response))==-1) {
			close(datafd);
			datafd_arr[datafd_counter] = -1;
			return -1;
		}
	} else {
		len = prepare_response_oneline(response, 451, 1, "Trouble reading directory.");
		if(msocket_write(connfd, response, strlen(response))==-1) {
			close(datafd);
			datafd_arr[datafd_counter] = -1;
			return -1;
		}
	}
	close(datafd);
	datafd_arr[datafd_counter] = -1;
	return 0;
}

int mkd_handler(int connfd, char* buffer, char* cwd, char* root) {
	if(strlen(buffer)<6) {
		return request_not_support_handler(connfd);
	}
	char response[1024];
	memset(response, '\0', sizeof(response));
	char directname[1024];
	memset(directname, '\0', sizeof(directname));
	strcpy(directname, cwd);
	if(change_directory(root, buffer+4, directname+strlen(directname))==-1) {
		int len = prepare_response_oneline(response, 550, 1, "Directory creation failed.");
		if(msocket_write(connfd, response, strlen(response))==-1) {
			return -1;
		}
		return 0;
	}
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
	if(change_directory(root, buffer+4, directname+strlen(directname))==-1) {
		int len = prepare_response_oneline(response, 550, 1, "Directory remove failed.");
		if(msocket_write(connfd, response, strlen(response))==-1) {
			return -1;
		}
		return 0;
	}
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
	char test_root[1024];
	memset(test_root, '\0', sizeof(test_root));
	char test_path[2048];
	memset(test_path, '\0', sizeof(test_path));
	strcpy(test_path, cwd);
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
	strcat(test_path, test_root);
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

struct stor_params {
	int connfd;
	int datafd;
	int* datafd_arr;
	int datafd_counter;
	FILE* pfile;
};

void* stor_thread_run(void* args) {
	struct stor_params* params;
	params = (struct stor_params*)args;
	int connfd = params->connfd;
	int datafd = params->datafd;
	int* datafd_arr = params->datafd_arr;
	int datafd_counter = params->datafd_counter;
	FILE* pfile = params->pfile;

	char response[1024];
	memset(response, '\0', sizeof(response));
	int len = 0;

	int* return_val = (int*)malloc(sizeof(int));
	*return_val = -1;

	int datalen=msocket_read_file_large(datafd, pfile);
	if(datalen == -1) {
		len = prepare_response_oneline(response, 426, 1, "Connection broken.");
		if(msocket_write(connfd, response, strlen(response))==-1) {
			fclose(pfile);
			close(datafd);
			return (void*)return_val;
		}
		fclose(pfile);
		close(datafd);
		return (void*)return_val;
	}
	else if(datalen == -2) {
		len = prepare_response_oneline(response, 451, 1, "Problem saving the file.");
		if(msocket_write(connfd, response, strlen(response))==-1) {
			fclose(pfile);
			close(datafd);
			return (void*)return_val;
		}
		fclose(pfile);
		close(datafd);
		datafd_arr[datafd_counter] = -1;
		return (void*)return_val;
	}
	len = prepare_response_oneline(response, 226, 1, "Successfully receive data.");
	if(msocket_write(connfd, response, strlen(response))==-1) {
		fclose(pfile);
		close(datafd);
		// datafd_arr[datafd_counter] = -1;
		return (void*)return_val;
	}
	fclose(pfile);
	close(datafd);
	// datafd_arr[datafd_counter] = -1;
	*return_val = 0;
	return (void*)return_val;
}

int stor_handler(int connfd, char* buffer, int* datafd_arr, int datafd_counter, char* cwd, char* root, char* mode) {
	int datafd = datafd_arr[datafd_counter];
	if(strlen(buffer)<7) {
		if(datafd>0) {
			close(datafd);
			datafd_arr[datafd_counter] = -1;
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
		datafd_arr[datafd_counter] = -1;
		return -1;
	}
	memset(response, '\0', sizeof(response));

	char filename[1024];
	memset(filename, '\0', sizeof(filename));
	strcpy(filename, cwd);

	if(change_directory(root, buffer + 5, filename+strlen(filename))==-1) {
		len = prepare_response_oneline(response, 451, 1, "Problem saving the file.");
		if(msocket_write(connfd, response, strlen(response))==-1) {
			close(datafd);
			datafd_arr[datafd_counter] = -1;
			return -1;
		}
		close(datafd);
		datafd_arr[datafd_counter] = -1;
		return 1;
	}

	FILE* pfile = fopen(filename, mode);
	struct stor_params params;
	params.connfd = connfd;
	params.datafd = datafd;
	params.datafd_arr = datafd_arr;
	params.datafd_counter = datafd_counter;
	params.pfile = pfile;
	pthread_t id;
	if(pthread_create(&id, NULL, stor_thread_run, &params)) {
		printf("Error creating stor thread.\n");
		len = prepare_response_oneline(response, 426, 1, "Connection broken.");
		if(msocket_write(connfd, response, strlen(response))==-1) {
			close(datafd);
			datafd_arr[datafd_counter] = -1;
			return -1;
		}
		close(datafd);
		return 1;
	}
	pthread_detach(id);
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


int rest_handler(int connfd, char* buffer, int* rest_size, int* rest_flag) {
	if(strlen(buffer)<7) {
		return request_not_support_handler(connfd);
	}
	int tmp_rest = 0;
	if(sscanf(buffer, "REST %d\n", &tmp_rest)==EOF) {
		return request_not_support_handler(connfd);
	}
	if(tmp_rest<0) {
		return request_not_support_handler(connfd);
	}
	char response[1024];
	int len = prepare_response_oneline(response, 350, 1, "REST command confirmed.");
	if(msocket_write(connfd, response, strlen(response))==-1) {
		return -1;
	}
	*rest_size = tmp_rest;
	*rest_flag = 2;
	return 0;
}
