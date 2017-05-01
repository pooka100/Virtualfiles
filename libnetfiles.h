#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>

#ifndef _LIBNETFILES_H_
#define _LIBNETFILES_H_

#define BUFSIZE 128
#define _PORT_ "12002"

#define M_UNRESTRICTED 0
#define M_EXCLUSIVE    1
#define M_TRANSACTION  2
typedef struct readbuf
{
	char *data;
	int curr;
	int fd;
	ssize_t maxread;
} readbuf;

int itoa(int total, char *string);
ssize_t serv_writen(int connfd, void *buf, size_t count);
ssize_t serv_readln(int fd, void *buf, size_t count, readbuf *tmpbuf);
ssize_t serv_readn(int fd, void *buf, size_t count, readbuf *tmpbuf);
int newclientfd(char *hostname, char *port, int addedbitmask);
void *readbuf_init(int fd);
void readbuf_free(readbuf *buf);

/* Main functions for execution */
ssize_t netserverinit(char *hostname, int filemode);

ssize_t netopen(const char *pathname, int flags);
ssize_t netread(int fd, void *data, size_t nbyte);
ssize_t netwrite(int fildes, const void *data, size_t nbyte);
ssize_t netclose(int fd);


#endif
