#ifndef _SERVER_HANDLER_H_
#define _SERVER_HANDLER_H_

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

int request_not_support_handler(int connfd);

int user_handler(int connfd, char* buffer);

int pass_handler(int connfd, char* buffer, int* login_flag);

int syst_handler(int connfd, char* buffer);

int type_handler(int connfd, char* buffer);

int port_handler(int connfd, char* buffer, int* pdatafd, struct sockaddr_in* paddr, int* pdatalistenfd);

int retr_handler_file_err(int connfd, int datafd);

int retr_handler_common_file(int connfd, int datafd, FILE* pfile, long fsize);

int retr_handler(int connfd, char* buffer, int datafd, char* cwd, char* root);

int pasv_handler(int connfd, char* buffer, int* host_ip, int* pdatafd, int* pdatalistenfd);

int quit_handler(int connfd, char* buffer, int* quit_flag);

int pwd_handler(int connfd, char* buffer, char* cwd, char* root);

int list_handler(int connfd, char* buffer, int datafd, char* cwd, char* root);

int mkd_handler(int connfd, char* buffer, char* cwd, char* root);

int rmd_handler(int connfd, char* buffer, char* cwd, char* root);

int cwd_handler(int connfd, char* buffer, char* cwd, char* root);

int stor_handler(int connfd, char* buffer, int datafd, char* cwd, char* root);

int login_required_handler(int connfd);

int rnfr_handler(int connfd, char* buffer, char* cwd, char* root, int* rnfr_flag, char* move_file_path);

int rnto_handler(int connfd, char* buffer, char* cwd, char* ftp_root, int* rnfr_flag, char* move_file_path);

#endif