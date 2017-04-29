#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <semaphore.h>
#include <unistd.h>
#include <signal.h>

#define _PORT_  "12002"
#define BUFSIZE 128

#define LOCK(X) sem_wait(X)
#define UNLOCK(X)    sem_post(X)

/* SIMPLE STUPID PROTOCOL 
 * 
 * Size (after this \n)
 * Open/Read/Write/Close
 * Size/Filename
 * Text
*/
sem_t mutex;
sigset_t mask;

int *tmpconnfd = NULL;

enum fileop{ OPOPEN, OPCLOSE, OPREAD, OPWRITE };

typedef struct readbuf
{
	char *data;
	int curr;
	int fd;
	ssize_t maxread;
} readbuf;

void *readbuf_init(int fd)
{
	readbuf *buf = (readbuf *)malloc(sizeof(readbuf));
	buf->fd = fd;
	buf->data = (char *)malloc(sizeof(char) * BUFSIZE);
	buf->curr = BUFSIZE;
	buf->maxread = 0;
	return buf;
}


void cleanexit(int sig)
{
	if(tmpconnfd != NULL)
		free(tmpconnfd);
	pthread_exit(NULL);
}
int itoa(int total, char *string)
{
	int len = 0;
	int i;
	int mult = 1;
	int tmp = total;
	/*got the length*/
	while(total > 0)
	{
		total /= 10;
		++len;
		mult *= 10;
	}
	total = tmp;
	mult /= 10;
	for(i = 0; i < len; ++i)
	{
		
		string[i] = ((total / mult) % 10) + 48;
		mult /= 10;
	}

	string[len] = '\0';
	return len;
}

void readbuf_free(readbuf *buf)
{
	free(buf->data);
	free(buf);
}

void ERROR()
{
	printf("You have an error... but you don't know what it is yet..\n");
	exit(-1);
}

int newlistenerfd()
{
	int listenfd;
	int leerror;
	int optval=1;

	struct addrinfo hints, *result, *i;

	memset(&hints, 0, sizeof(struct addrinfo));

	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_NUMERICSERV;
	hints.ai_flags |= AI_ADDRCONFIG;
	hints.ai_flags |= AI_PASSIVE;

	if((leerror = getaddrinfo(NULL, _PORT_, &hints, &result)) != 0)
	{
		printf("%s\n", gai_strerror(leerror));
		ERROR();
	}

	/* Just get the first one that works */
	for(i = result; i != NULL; i = i->ai_next)
	{
		if((listenfd = socket(i->ai_family, i->ai_socktype, i->ai_protocol)) == -1)
			continue;
		//use ai addr ai addrlen
		setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));
		if(bind(listenfd, i->ai_addr, i->ai_addrlen) == 0)
			break;
		close(listenfd);
	}	

	if(!i)
	{
		freeaddrinfo(result);
		return -1;
	}

	freeaddrinfo(result);

	/* Do I really need anymore requests? */
	if((listen(listenfd, 64) != 0))
	{
		close(listenfd);
		return -2;
	}

	return listenfd;
}

ssize_t serv_readn(int fd, void *buf, size_t count, readbuf *tmpbuf)
{
	int i;
	int readmore = 0;
	char *charbuf = (char *)buf;

	while(1)
	{
		if((tmpbuf->curr == tmpbuf->maxread) || (tmpbuf->maxread == 0))
		{
			if((tmpbuf->maxread = read(fd, tmpbuf->data, BUFSIZE)) == 0)
				return readmore;
			if(tmpbuf->maxread == -1)
				return -1;

			tmpbuf->curr = 0;
		}

		for(i = tmpbuf->curr; i < tmpbuf->maxread; ++i, ++readmore, ++(tmpbuf->curr))
		{
			if(readmore < (count - 1))
				charbuf[readmore] = tmpbuf->data[i];
			else
			{
				charbuf[readmore] = tmpbuf->data[i];
				++(tmpbuf->curr);
				return count;
			}
		}

	}
	return -1;
}
/* Reads up to the end of a line unless the max amount of characters specified by count happens REENTRANT */
ssize_t serv_readln(int fd, void *buf, size_t count, readbuf *tmpbuf)
{
	int i;
	int readmore = 0;
	char *charbuf = (char *)buf;

	while(1)
	{
		if((tmpbuf->curr == tmpbuf->maxread) || (tmpbuf->maxread == 0))
		{
			if((tmpbuf->maxread = read(fd, tmpbuf->data, BUFSIZE)) == 0)
				return readmore;
			if(tmpbuf->maxread == -1)
				return -1;

			tmpbuf->curr = 0;
		}

		for(i = tmpbuf->curr; i < tmpbuf->maxread; ++i, ++readmore, ++(tmpbuf->curr))
		{
			if(readmore < (count - 1))
			{
				if(tmpbuf->data[i] == '\n')
				{
					charbuf[readmore] = '\0';
					++readmore;
					++(tmpbuf->curr);
					return readmore;
				}
				charbuf[readmore] = tmpbuf->data[i];
			}
			else
			{
				charbuf[readmore] = '\0';
				++(tmpbuf->curr);
				return count;
			}
		}

	}
	return -1;
}

/* Forces a full write */
ssize_t serv_writen(int connfd, void *buf, size_t count)
{
	int written = 0;
	int tmp;
	while(written < count)
	{
		tmp = write(connfd, buf, count - written);
		if(tmp == -1) return written;
		buf = ((char *)buf) + tmp;
		written += tmp;
	}
	return written;
}


void serv_error(int error, int connfd)
{

	char *buf = (char *)malloc(sizeof(char) * 11);
	itoa(error, buf);
	serv_writen(connfd, "UNSUCESSFUL\n", sizeof(char) * 12);
	serv_writen(connfd, buf, strlen(buf));
	free(buf);
}

void serv_client_error(int connfd)
{
	serv_writen(connfd, "UNSUCESSFUL\n", sizeof(char) * 12);
	serv_writen(connfd, "USERERROR\n", sizeof(char) * 10);
}

/* Have to block for file operations because of errno */
int blockedcall(int connfd, char *param1, int param2, char *param3, enum fileop operation)
{
	int misc;
	int error = 0;
	char *tmp, *size;
	LOCK(&mutex);	
	switch (operation)
	{
		case OPOPEN:
			misc = open(param1, param2);
			break;
		case OPCLOSE:
			misc = close(param2);
			break;
		case OPREAD:
			tmp = (char *)malloc(sizeof(char) * atoi(param1));
			size = (char *)malloc(sizeof(char) * BUFSIZE);
			printf("shit\n");
			misc = read(param2, tmp, atoi(param1));
			printf("shit\n");
			if(misc == -1)
			{
				free(tmp);
				break;
			}
			sprintf(size, "SUCESSFUL\n%d\n", misc);
			serv_writen(connfd, size, strlen(size)); 

			int shit = serv_writen(connfd, tmp, misc);
			free(tmp);
			free(size);
			break;
		case OPWRITE:
			tmp = (char *)malloc(sizeof(char) * BUFSIZE);	
			misc = write(param2, param3, atoi(param1));
			printf("%d\n", misc);
			if(misc == -1)
			{
				free(tmp);
				break;
			}
			sprintf(tmp, "SUCESSFUL\n%d\n", misc);
			printf("PLEASE PRINT THIS: %s\n", tmp);
			serv_writen(connfd, tmp, strlen(tmp));
			free(tmp);
			break;
		default:
			break;

	}
	
	if(misc == -1)
	{
		error = errno;
		UNLOCK(&mutex);
		serv_error(error, connfd);
		return -1;
	}

	UNLOCK(&mutex);
	return misc;
}

void serv_open(int connfd, char *lines, int size, readbuf *buf)
{
	int charsread, fakefd;
	char *writebuf = (char *)malloc(sizeof(char *) * 64);
	char *name, *flags;

	/* next line should be name */
	if((charsread = serv_readln(connfd, lines, size, buf)) == -1)
	{
		serv_client_error(connfd);
		return;
	}
	else if(charsread == 0)
	{
		serv_client_error(connfd);
		return;
	}

	name = lines;
	lines = ((char *)lines) + charsread;

	/* Should be flags */
	if((charsread = serv_readln(connfd, lines, size, buf)) == -1)
	{
		serv_client_error(connfd);
		return;
	}
	else if(charsread == 0)
	{
		serv_client_error(connfd);
		return;
	}

	flags = lines;
	/* MAKE SURE TO ADD A \n AT THE END OF SENDING MESSAGE FROM CLIENT */

	if((fakefd = blockedcall(connfd, name, atoi(flags), NULL, OPOPEN)) == -1)
	{
		free(writebuf);
		return;
	}


	free(writebuf);
	writebuf = (char *)malloc(sizeof(char) * 128);

	printf("The fakefd: %d\n", fakefd);

	int total = sprintf(writebuf, "%s\n%d\n", "SUCESSFUL", fakefd);

	serv_writen(connfd, writebuf, total + 1);
	free(writebuf);
	
	printf("Open Successful\n");

}


void serv_close(int connfd, char *lines, int size, readbuf *buf)
{
	int charsread;
	char *writebuf = (char *)malloc(sizeof(char *) * 64);
	char *fd_char;

	/* next line should be fd */

	if((charsread = serv_readln(connfd, lines, size, buf)) == -1)
	{
		serv_client_error(connfd);
		return;
	}
	else if(charsread == 0)
	{
		serv_client_error(connfd);
		return;
	}

	fd_char = lines;
	lines = ((char *)lines) + charsread;

	if(blockedcall(connfd, NULL, atoi(fd_char), NULL, OPCLOSE) == -1)
	{
		free(writebuf);
		return;
	}


	free(writebuf);
	writebuf = (char *)malloc(sizeof(char) * 128);

	int total = sprintf(writebuf, "%s\n", "SUCESSFUL");

	serv_writen(connfd, writebuf, total + 1);
	free(writebuf);

	printf("Close successful\n");

}
	

void serv_read(int connfd, char *lines, int size, readbuf *buf)
{
	int charsread;
	char *writebuf = (char *)malloc(sizeof(char *) * 64);
	char *fd_char, *data;

	 /*next line should be fd*/ 
	if((charsread = serv_readln(connfd, lines, size, buf)) == -1)
	{
		serv_client_error(connfd);
		return;
	}
	else if(charsread == 0)
	{
		serv_client_error(connfd);
		return;
	}

	fd_char = lines;
	lines = ((char *)lines) + charsread;

	/* bytes to read */
	if((charsread = serv_readln(connfd, lines, size, buf)) == -1)
	{
		serv_client_error(connfd);
		return;
	}
	else if(charsread == 0)
	{
		serv_client_error(connfd);
		return;
	}

	data = lines;

	if(blockedcall(connfd, data, atoi(fd_char), NULL, OPREAD) == -1)
	{
		free(writebuf);
		return;
	}


	free(writebuf);

	printf("Successfully completed a Read\n");
}

void serv_write(int connfd, char *lines, int size, readbuf *buf)
{
	int charsread;
	char *writebuf = (char *)malloc(sizeof(char *) * 64);
	char *fd_char, *size_char, *data;

	printf("BLACK PENIS\n");
	/* GET FD */
	if((charsread = serv_readln(connfd, lines, size, buf)) == -1)
	{
		serv_client_error(connfd);
		return;
	}
	else if(charsread == 0)
	{
		serv_client_error(connfd);
		return;
	}

	printf("BLACK PENIS\n");
	fd_char = lines;
	lines = ((char *)lines) + charsread;

	/* GET CHARS TO READ */
	if((charsread = serv_readln(connfd, lines, size, buf)) == -1)
	{
		serv_client_error(connfd);
		return;
	}
	else if(charsread == 0)
	{
		serv_client_error(connfd);
		return;
	}

	printf("BLACK PENIS\n");
	size_char = lines;
	lines = ((char *)lines) + charsread;

	/* GET DATAAAA */

	printf("%s\n", size_char);
	if((charsread = serv_readn(connfd, lines, atoi(size_char), buf)) == -1)
	{
		serv_client_error(connfd);
		return;
	}
	else if(charsread == 0)
	{
		serv_client_error(connfd);
		return;
	}

	printf("BLACK PENIS\n");
	data = lines;

	if(blockedcall(connfd, size_char, atoi(fd_char), data, OPWRITE) == -1)
	{
		free(writebuf);
		return;
	}
	free(writebuf);
	printf("DONE HERE HAHA\n");

}

void fileio(int connfd)
{
	int size = 0;
	readbuf *buf = readbuf_init(connfd);
	char *lines = (char *)malloc(sizeof(char) * 64);
	/* Gets the size */
	serv_readln(connfd, lines, 64, buf);
	printf("%s\n", lines);
	size = atoi(lines);
	free(lines);
	lines = (char *)malloc(sizeof(char) * size);	

	serv_readln(connfd, lines, size, buf);

	size = size - strlen(lines);
	printf("%s\n", lines);
	
	if(strcmp("Open", lines) == 0)
	{
		printf("Got an open\n");
		serv_open(connfd, lines, size, buf);
	}
	else if(strcmp("Close", lines) == 0)
	{
		printf("Got a close\n");
		serv_close(connfd, lines, size, buf);
	}
	else if(strcmp("Read", lines) == 0)
	{
		printf("Got a read\n");
		serv_read(connfd, lines, size, buf);
	}
	else if(strcmp("Write", lines) == 0)
	{
		printf("Got a write\n");
		serv_write(connfd, lines, size, buf);
	}
	else
	{
		printf("Error none matching\n");
	}

	free(lines);
	readbuf_free(buf);

}

void *fileio_init(void *_connfd)
{
	int connfd = *((int *) _connfd);
	pthread_detach(pthread_self());
	free(_connfd);
	printf("WOWIE GOT A CONNECTION\n");
	fileio(connfd);
	close(connfd);
	return NULL;
}


int main(int argc, char **argv)
{
	struct sockaddr_storage addr;
	socklen_t addr_len;
	pthread_t tid;
	int listenfd;


	listenfd = newlistenerfd();
	sem_init(&mutex, 0, 1);
	
	/* Dont want to overuse mutex so creating SIGPIPE block here, should not be concerned */
	sigemptyset(&mask);
	sigaddset(&mask, SIGPIPE);
	sigprocmask(SIG_SETMASK, &mask, NULL);
	signal(SIGINT, cleanexit);	
	while(1)
	{
		addr_len = sizeof(struct sockaddr_storage);	
		tmpconnfd = (int *)malloc(sizeof(int));
		*tmpconnfd = accept(listenfd, (struct sockaddr *)&addr, &addr_len);
		printf("stop this shit %d\n", *tmpconnfd);
		pthread_create(&tid, NULL, fileio_init, tmpconnfd);
		tmpconnfd = NULL;
	}

	return 0;
}
