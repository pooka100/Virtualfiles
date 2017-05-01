#include "libnetfiles.h"

/* Adapted from Computer Systems: A Programmers Perspective book */

/* Should be long enough for any address */
static char *host_name;
static char *port      = "12002";
static int MODE;

int itoa(int total, char *string)
{

	if(total == 0)
	{
		string[0] = '0';
		return 1;
	}
	
	int real = total;
	int i = 0;
	int len = 0;
	if(total < 0)
	{
		total = -(total);
		++len;
		++i;
	}

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
	for(; i < len; ++i)
	{
		
		string[i] = ((total / mult) % 10) + 48;
		mult /= 10;
	}


	if(real < 0)
		string[0] = '-';
	string[len] = '\0';
	return len;
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
			{
				return readmore;
			}
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

int newclientfd(char *hostname, char *port, int addedbitmask)
{
	struct addrinfo hints, *listp, *i;
	int fd;
	int err = -1;


	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_flags = AI_ADDRCONFIG | addedbitmask;
	
	getaddrinfo(hostname, port, &hints, &listp);

	for(i = listp; i != NULL; i = i->ai_next)
	{
		if((fd = socket(i->ai_family, i->ai_socktype, i->ai_protocol)) < 0)
			continue;

		if(connect(fd, i->ai_addr, i->ai_addrlen) != -1)
		{
			err = -2;
			break;
		}

		close(fd);
	}

	freeaddrinfo(listp);
	if(!i)
		return err;
	else
		return fd;
}

void *readbuf_init(int fd)
{
	readbuf *buf = (readbuf *)malloc(sizeof(readbuf));
	buf->fd = fd;
	buf->data = (char *)malloc(sizeof(char) * BUFSIZE);
	buf->curr = BUFSIZE;
	buf->maxread = 0;
	return buf;
}

void readbuf_free(readbuf *buf)
{
	free(buf->data);
	free(buf);
}



ssize_t netopen(const char *pathname, int flags)
{

	if(strcmp("0", host_name) == 0)
	{
		errno = HOST_NOT_FOUND;
		return -1;
	}
	int size_data, size_total, connectedfd, iserror, returnfd;
	char *tmp = (char *)malloc(sizeof(char) * 32);
	char *dataoutput;
	readbuf *buf;


	connectedfd = newclientfd(host_name, _PORT_, AI_NUMERICSERV);
	buf = readbuf_init(connectedfd);
	

	/* Protocol is: size \n Open \n filename \n flags \n \0 */
	size_data = 5 + strlen(pathname) + 1 + itoa(flags, tmp) + 2 + 1 + 2; 
	size_total = size_data + itoa(size_data, tmp);
	dataoutput = (char *)malloc(sizeof(char) * size_total);

	sprintf(dataoutput, "%d\n%s\n%s\n%d\n%d\n", size_data, "Open", pathname, flags, MODE);
	/* Writes the protocol for open */

	serv_writen(connectedfd, dataoutput, size_total);	

	iserror = serv_readln(connectedfd, dataoutput, size_total, buf);
	if(strcmp(dataoutput, "SUCESSFUL") != 0)
	{
		if(iserror >= 1)
		{
			if(strcmp(dataoutput, "UNSUCESSFUL") == 0)
			{
				serv_readln(connectedfd, dataoutput, size_total, buf);
				errno = atoi(dataoutput);
			}
		}
		readbuf_free(buf);
		free(tmp);
		free(dataoutput);
		return -1;
	}

	if(serv_readln(connectedfd, dataoutput, size_total, buf) < 1)
	{
		readbuf_free(buf);
		free(tmp);
		free(dataoutput);
		return -1;
	}

	readbuf_free(buf);
	free(tmp);
	returnfd = -(atoi(dataoutput)) - 10;
	free(dataoutput);
	return returnfd;

}

ssize_t netclose(int fd)
{

	if(strcmp("0", host_name) == 0)
	{
		errno = HOST_NOT_FOUND;
		return -1;
	}

	if(fd >= 0)
	{
		errno = EBADF;
		return -1;
	}

	int size_data, size_total, connectedfd, iserror;
	char *tmp = (char *)malloc(sizeof(char) * 32);
	char *dataoutput;
	readbuf *buf;


	connectedfd = newclientfd(host_name, _PORT_, AI_NUMERICSERV);
	buf = readbuf_init(connectedfd);
	

	/* Protocol is: size \n Close \n FD \n \0 */
	size_data = 6 + 1 + itoa(-(fd) - 10, tmp) + 2 + 1 + 2; 
	size_total = size_data + itoa(size_data, tmp);
	dataoutput = (char *)malloc(sizeof(char) * size_total);

	printf("%d\n", -(fd) - 10);
	sprintf(dataoutput, "%d\n%s\n%d\n%d\n", size_data, "Close", -(fd) - 10, MODE);
	/* Writes the protocol for open */

	serv_writen(connectedfd, dataoutput, size_total);	

	iserror = serv_readln(connectedfd, dataoutput, size_total, buf);
	if(strcmp(dataoutput, "SUCESSFUL") != 0)
	{
		if(iserror >= 1)
		{
			if(strcmp(dataoutput, "UNSUCESSFUL") == 0)
			{
				serv_readln(connectedfd, dataoutput, size_total, buf);
				errno = atoi(dataoutput);
			}
		}
		readbuf_free(buf);
		free(tmp);
		free(dataoutput);
		return -1;
	}

	readbuf_free(buf);
	free(tmp);
	free(dataoutput);
	return 0;
}

ssize_t netread(int fd, void *data, size_t nbyte)
{

	if(strcmp("0", host_name) == 0)
	{
		errno = HOST_NOT_FOUND;
		return -1;
	}

	if(fd >= 0)
	{
		errno = EBADF;
		return -1;
	}

	int size_data, size_total, connectedfd, iserror, numtoread;
	char *tmp = (char *)malloc(sizeof(char) * 32);
	char *tmp2 = (char *)malloc(sizeof(char) * 32);
	char *dataoutput;
	readbuf *buf;


	connectedfd = newclientfd(host_name, _PORT_, AI_NUMERICSERV);

	if(connectedfd == -2)
	{
		errno = ETIMEDOUT;
		free(tmp);
		free(tmp2);
		return -1;
	}
	else if(connectedfd == -1)
	{
		errno = ECONNRESET;
		free(tmp);
		free(tmp2);
		return -1;
	}
	buf = readbuf_init(connectedfd);
	

	/* Protocol is: size \n Read \n FD \n bytes to read \n \0 */
	size_data = 5 + itoa(-(fd) - 10, tmp) + 1 + 2 + itoa(nbyte, tmp2) + 1 + 1;
	size_total = size_data + itoa(size_data, tmp);

	dataoutput = (char *)malloc(sizeof(char) * size_total);

	sprintf(dataoutput, "%d\n%s\n%d\n%d\n", size_data, "Read", -(fd) - 10, (int)nbyte);
	/* Writes the protocol for open */

	serv_writen(connectedfd, dataoutput, size_total);	

	iserror = serv_readln(connectedfd, dataoutput, size_total, buf);
	if(strcmp(dataoutput, "SUCESSFUL") != 0)
	{
		if(iserror >= 1)
		{
			if(strcmp(dataoutput, "UNSUCESSFUL") == 0)
			{
				serv_readln(connectedfd, dataoutput, size_total, buf);
				errno = atoi(dataoutput);
			}
		}
		readbuf_free(buf);
		free(tmp);
		free(tmp2);
		free(dataoutput);
		return -1;
	}

	serv_readln(connectedfd, dataoutput, size_total, buf);



	/* all the chars */

	numtoread = serv_readn(connectedfd, data, atoi(dataoutput), buf);


	readbuf_free(buf);
	free(tmp);
	free(tmp2);
	free(dataoutput);
	return numtoread;
}

ssize_t netwrite(int fildes, const void *data, size_t nbyte)
{

	if(strcmp("0", host_name) == 0)
	{
		errno = HOST_NOT_FOUND;
		return -1;
	}

	if(fildes >= 0)
	{
		errno = EBADF;
		return -1;
	}

	int size_data, size_total, connectedfd, iserror, numtoread;
	char *tmp = (char *)malloc(sizeof(char) * 32);
	char *tmp2 = (char *)malloc(sizeof(char) * 32);
	char *dataoutput;
	readbuf *buf;


	connectedfd = newclientfd(host_name, _PORT_, AI_NUMERICSERV);

	if(connectedfd == -2)
	{
		errno = ETIMEDOUT;
		free(tmp);
		free(tmp2);
		return -1;
	}
	else if(connectedfd == -1)
	{
		errno = ECONNRESET;
		free(tmp);
		free(tmp2);
		return -1;
	}

	buf = readbuf_init(connectedfd);
	

	/* Protocol is: size \n Write \n FD \n bytes to read \n data \n \0 */
	size_data = 6 + itoa(-(fildes) - 10, tmp) + 1 + itoa(nbyte, tmp2) + 1 + 2 + nbyte + 2;
	size_total = size_data + itoa(size_data, tmp);

	dataoutput = (char *)malloc(sizeof(char) * size_total);

	printf("dout: %ld\n", (long)dataoutput);
	sprintf(dataoutput, "%d\n%s\n%d\n%d\n%s\n", size_data, "Write", -(fildes) - 10, (int)nbyte, (const char *)data);
	/* Writes the protocol for open */

	serv_writen(connectedfd, dataoutput, size_total);	

	iserror = serv_readln(connectedfd, dataoutput, size_total, buf);
	if(strcmp(dataoutput, "SUCESSFUL") != 0)
	{
		if(iserror >= 1)
		{
			if(strcmp(dataoutput, "UNSUCESSFUL") == 0)
			{
				serv_readln(connectedfd, dataoutput, size_total, buf);
				errno = atoi(dataoutput);
			}
		}
		readbuf_free(buf);
		free(tmp);
		free(tmp2);
		free(dataoutput);
		return -1;
	}

	printf("dout: %ld\n", (long)dataoutput);
	serv_readln(connectedfd, dataoutput, size_total, buf);
	numtoread = atoi(dataoutput);
	
	
	printf("dout: %ld\n", (long)dataoutput);


	/* all the chars */


	printf("cats in the cradle\n");
	readbuf_free(buf);
	free(tmp);
	free(tmp2);
	free(dataoutput);
	
	printf("cats in the cradle\n");
	
	if(numtoread > nbyte)
		return -1;
	return numtoread;
}


ssize_t netserverinit(char *hostname, int filemode)
{
	int fdgood = newclientfd(hostname, _PORT_, 0);		

	if(fdgood == -1)
	{
		errno = HOST_NOT_FOUND;
		return -1;
	}

	host_name = (char *)malloc(strlen(hostname) + 1);
	strcpy(host_name, hostname);
	//host_name = hostname;
	
	serv_writen(fdgood, "4\n0", 4);
	close(fdgood);	
	if(filemode > 3 || filemode < 0)
		return -1;
	MODE = filemode;
	return 0;
}
