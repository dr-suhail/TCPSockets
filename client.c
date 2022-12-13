/*	Program name: client.c
 *
 * */

#include<sys/types.h>
#include<unistd.h>
#include<sys/socket.h>
#include<stdio.h>
#include<sys/wait.h>
#include<signal.h>
#include<string.h>
#include<stdlib.h>
#include<netdb.h>
#include<errno.h>

  
#define BACK_LOG 5
#define SERVER_PORT 1740
#define BUF_SIZE 30

/*	Reads n bytes from a descriptor
 *	the readn() function from Steven's book
 *	as advised in the lecture slides
 *
 * */
ssize_t readn(int fd, void *vptr, size_t n)
{
	size_t	nleft;
	ssize_t	nread;
	char	*ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
		if ( (nread = read(fd, ptr, nleft)) < 0) {
			if (errno == EINTR)
				nread = 0;		/* and call read() again */
			else
				return(-1);
		} else if (nread == 0)
			break;				/* EOF */

		nleft -= nread;
		ptr   += nread;
	}
	return(n - nleft);		/* return >= 0 */
}
/* end readn */

/* 	Write "n" bytes to a descriptor. */
/*	the writen() function from Steven's book as 
 *	advised in the lecture slides
 * */
ssize_t writen(int fd, const void *vptr, size_t n)
{
	size_t		nleft;
	ssize_t		nwritten;
	const char	*ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
		if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
			if (nwritten < 0 && errno == EINTR)
				nwritten = 0;		/* and call write() again */
			else
				return(-1);			/* error */
		}

		nleft -= nwritten;
		ptr   += nwritten;
	}
	return(n);
}
/* end writen */


int main(int argc, char* argv[])
{
	int client_sock_fd, err, num_bytes;
	char counter_buf[BUF_SIZE];
	socklen_t server_sock_len;
	struct sockaddr_in server_sock_addr;

	if( argc != 2){
		fprintf(stderr, "usage: ./client <domain name>\n");
		return 0;
	}
	
	/* create a TCP socket for client to communicate with the server */
	client_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if( client_sock_fd < 0 ){
		fprintf(stderr, "could not create client socket\n");
		return 0;
	}
	
	/* fill in the server socket address structure */
	bzero(&server_sock_addr, sizeof(server_sock_addr));
	
	server_sock_addr.sin_family = AF_INET;
	server_sock_addr.sin_port = htons(SERVER_PORT);
	
	err = inet_aton(argv[1], &server_sock_addr.sin_addr);
	if( err == 0){
		fprintf(stderr, "could not conver dotted address to network address\n");
		return 0;
	}
	
	server_sock_len = sizeof(server_sock_addr);

	/* connect to the server */
	err = connect( client_sock_fd, (struct sockaddr *)&server_sock_addr, server_sock_len);
	if( err < 0){
		fprintf(stderr, "could not connect to the server\n");
		return 0;
	}

	/* read data from server */
	err = readn( client_sock_fd, counter_buf, BUF_SIZE);
	if( err == -1){
		fprintf(stderr, "could not read data from socket\n");
	}
	else if( err == 0){
		fprintf(stderr, "no data received\n");
	}
	else{
		fprintf(stdout, "I received: %s\n", counter_buf);
	}

	/* close the client socket */
	close( client_sock_fd);
		
	return 0;
}
