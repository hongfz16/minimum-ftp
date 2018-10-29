#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>

#include <pthread.h>

#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

#include "socket_utils.h"
#include "server_handler.h"


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
			// memset(root, '\0', sizeof(root));
			root = strcpy(root, argv[i+1]);
			return 0;
		}
	}
	char* default_root = DEFAULT_PATH;
	// memset(root, '\0', sizeof(root));
	root = strcpy(root, default_root);
	return 0;
}

int unpack_message(char* buffer, int login_flag) {
	char order[16];
	memset(order, '\0', sizeof(order));
	int i=0;
	for(i=0;i<4;++i) {
		order[i]=buffer[i];
	}
	int return_code = 100;
	if(strcmp(order, "USER")==0) {
		return_code = 1;
	}
	if(strcmp(order, "PASS")==0) {
		return_code = 2;
	}
	if(strcmp(order, "SYST")==0) {
		return_code = 3;
	}
	if(strcmp(order, "TYPE")==0) {
		return_code = 4;
	}
	if(strcmp(order, "PORT")==0) {
		return_code = 5;
	}
	if(strcmp(order, "RETR")==0) {
		return_code = 6;
	}
	if(strcmp(order, "PASV")==0) {
		return_code = 7;
	}
	if(strcmp(order, "QUIT")==0) {
		return_code = 8;
	}
	if(strcmp(order, "MKD ")==0) {
		return_code = 9;
	}
	if(strcmp(order, "CWD ")==0) {
		return_code = 10;
	}
	if(strcmp(order, "PWD\n")==0) {
		return_code = 11;
	}
	if(strcmp(order, "LIST")==0) {
		return_code = 12;
	}
	if(strcmp(order, "RMD ")==0) {
		return_code = 13;
	}
	if(strcmp(order, "RNFR")==0) {
		return_code = 14;
	}
	if(strcmp(order, "RNTO")==0) {
		return_code = 15;
	}
	if(strcmp(order, "STOR")==0) {
		return_code = 16;
	}
	if(!login_flag && return_code>4 && return_code<100 && return_code!=8) {
		return_code = 101;
	}
	return return_code;
}

struct ftp_thread_struct {
	int connfd;
	char cwd[1024];
	int host_ip[4];
};

void* start_one_ftp_worker(void *pftp_thread_struct) {
	struct ftp_thread_struct* pworker_info = (struct ftp_thread_struct*) pftp_thread_struct;
	int connfd = pworker_info->connfd;
	char* cwd = pworker_info->cwd;
	int* host_ip = pworker_info->host_ip;
	
	int len;
	char buffer[MAX_BUFFER_LEN];

	int datafd = -1;
	struct sockaddr_in data_addr;
	int datalistenfd = -1;

	char ftp_root[1024];
	memset(ftp_root, '\0', sizeof(ftp_root));
	ftp_root[0]='/';

	int* returnvalue = (int*)malloc(sizeof(int));
	*returnvalue=-1;

	len = prepare_response_oneline(buffer, 220, 1, "hongfz FTP server ready.");
	if(msocket_write(connfd, buffer, len)==-1) {
		return (void*)returnvalue;
	}
	
	int quit_flag=0;
	int login_flag=0;
	int rnfr_flag=0;
	char move_file_path[1024];

	while(1) {
		if(datalistenfd>0 && datafd<0) {
			if((datafd = accept(datalistenfd, NULL, NULL))==-1) {
				if(errno != EWOULDBLOCK) {
					printf("Error accept(): %s(%d)\n", strerror(errno), errno);
					close(datalistenfd);
					datalistenfd = -1;
				}
			}
			if(datafd>0) {
				close(datalistenfd);
				datalistenfd=-1;
			}
		}

		memset(buffer, '\0', MAX_BUFFER_LEN);
		if((len=msocket_read(connfd, buffer, MAX_BUFFER_LEN-1))==-1) {
			return (void*)returnvalue;
		}
		if(len==0) {
			continue;
		}
		messdebug(buffer);
		int order = unpack_message(buffer, login_flag);
		idebug(order);
		switch (order){
			case 1:
				if(user_handler(connfd, buffer)==-1) {
					return (void*)returnvalue;
				}
				break;
			case 2:
				if(pass_handler(connfd, buffer, &login_flag)==-1) {
					return (void*)returnvalue;
				}
				break;
			case 3:
				if(syst_handler(connfd, buffer)==-1) {
					return (void*)returnvalue;
				}
				break;
			case 4:
				if(type_handler(connfd, buffer)==-1) {
					return (void*)returnvalue;
				}
				break;
			case 5:
				if(port_handler(connfd, buffer, &datafd, &data_addr, &datalistenfd)==-1) {
					return (void*)returnvalue;
				}
				break;
			case 6:
				if(retr_handler(connfd, buffer, datafd, cwd, ftp_root)==-1) {
					return (void*)returnvalue;
				}
				if(datafd>0) {
					close(datafd);
					datafd = -1;
				}
				if(datalistenfd>0) {
					close(datalistenfd);
					datalistenfd = -1;
				}
				break;
			case 7:
				if(pasv_handler(connfd, buffer, host_ip, &datafd, &datalistenfd)==-1) {
					return (void*)returnvalue;
				}
				break;
			case 8:
				if(quit_handler(connfd, buffer, &quit_flag)==-1) {
					return (void*)returnvalue;
				}
				break;
			case 9:
				if(mkd_handler(connfd, buffer, cwd, ftp_root)==-1) {
					return (void*)returnvalue;
				}
				break;
			case 10:
				if(cwd_handler(connfd, buffer, cwd, ftp_root)==-1) {
					return (void*)returnvalue;
				}
				break;
			case 11:
				if(pwd_handler(connfd, buffer, cwd, ftp_root)==-1) {
					return (void*)returnvalue;
				}
				break;
			case 12:
				if(list_handler(connfd, buffer, datafd, cwd, ftp_root)==-1) {
					return (void*)returnvalue;
				}
				if(datafd>0) {
					close(datafd);
					datafd = -1;
				}
				if(datalistenfd>0) {
					close(datalistenfd);
					datalistenfd = -1;
				}
				break;
			case 13:
				if(rmd_handler(connfd, buffer, cwd, ftp_root)==-1) {
					return (void*)returnvalue;
				}
				break;
			case 14:
				memset(move_file_path, '\0', sizeof(move_file_path));
				if(rnfr_handler(connfd, buffer, cwd, ftp_root, &rnfr_flag, move_file_path)==-1) {
					return (void*)returnvalue;
				}
				break;
			case 15:
				if(rnto_handler(connfd, buffer, cwd, ftp_root, &rnfr_flag, move_file_path)==-1) {
					return (void*)returnvalue;
				}
				break;
			case 16:
				if(stor_handler(connfd, buffer, datafd, cwd, ftp_root)==-1) {
					return (void*)returnvalue;
				}
				if(datafd>0) {
					close(datafd);
					datafd = -1;
				}
				if(datalistenfd>0) {
					close(datalistenfd);
					datalistenfd = -1;
				}
				break;
			case 100:
				if(request_not_support_handler(connfd)==-1) {
					return (void*)returnvalue;
				}
				break;
			case 101:
				if(login_required_handler(connfd)==-1) {
					return (void*)returnvalue;
				}
				break;
		}

		if(rnfr_flag>0) {
			rnfr_flag-=1;
		}

		if(quit_flag) {
			break;
		}
	}
	if(datafd>0) {
		close(datafd);
		datafd = -1;
	}
	if(datalistenfd>0) {
		close(datalistenfd);
		datalistenfd = -1;
	}
	close(connfd);
	*returnvalue=0;
	return (void*)returnvalue;
}

int main(int argc, char** argv) {
	char* root = (char*)malloc(ROOT_LENGTH);
	memset(root, '\0', ROOT_LENGTH);
	int port = 21;
	if((port = parse_port(DEFAULT_PORT, argc, argv))==-1) {
		printf("Error parse_port(): Please specify the port you want to listen to after -port flag!\n");
		return -1;
	}
	if(parse_root(root, argc, argv)==-1) {
		printf("Error parse_root(): Please specify the root path you want to serve after -root flag!\n");
		return -1;
	}

	int listenfd, connfd;
	pthread_t thread_id;

	if((listenfd = create_non_blocking_listen_socket(port, MAX_BACKLOG))==-1) {
		return -1;
	}

	printf("listening on %d\n", port);

	struct ftp_thread_struct worker_info;
	worker_info.connfd = -1;
	worker_info.host_ip[0] = 127;
	worker_info.host_ip[1] = 0;
	worker_info.host_ip[2] = 0;
	worker_info.host_ip[3] = 1;
	memset(worker_info.cwd, '\0', sizeof(worker_info.cwd));
	strcpy(worker_info.cwd, root);

	while(1) {
		if ((connfd = accept(listenfd, NULL, NULL)) == -1) {
			if(errno == EWOULDBLOCK) {
				// cdebug("No pendding TCP connection. Sleeping for one second.");
				// sleep(0.01);
				continue;
			} else {
				printf("Error accept(): %s(%d)\n", strerror(errno), errno);
				return -1;
			}
		}
		if(make_socket_non_blocking(connfd)==-1) {
			return -1;
		}
		worker_info.connfd = connfd;
		if(pthread_create(&thread_id, NULL, start_one_ftp_worker, (void*)&worker_info)) {
			printf("Error pthread_create(): Thread not created.\n");
			close(connfd);
			continue;
		}
		cdebug("New connection.");
		pthread_detach(thread_id);
	}

	close(listenfd);
	free(root);
	return 0;
}