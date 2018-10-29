#ifndef _SOCKET_UTILS_H_
#define _SOCKET_UTILS_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include <pthread.h>

#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>

#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

void cdebug(char* debug);

void idebug(int i);

void messdebug(char* mess);

int msocket_read(int connfd, char* buffer, int buffer_len);

int msocket_read_file(int connfd, char* buffer, int buffer_len);

int msocket_write(int connfd, char* buffer, int len);

int prepare_response_oneline(char* buffer, int code, int last_line, char* content);

int prepare_response(char* buffer, int code, char** contents, int line_num);

int create_non_blocking_listen_socket(int port, int max_backlog);

int create_blocking_listen_socket(int port, int max_backlog);

int make_socket_non_blocking(int fd);

int single_change_directory(char* test_root, char* single_path);

int change_directory(const char* root, char* path, char* test_root);

#endif