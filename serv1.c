#include<sys/types.h>
#include<unistd.h>
#include<string.h>
#include<stdlib.h>
#include<netdb.h>
#include<stdio.h>
#include<errno.h>
#include<signal.h>

#define BACK_LOG 5
#define SERVER_PORT 1740
#define BUF_LEN 10

/* filedescriptor for server socket */
int listen_sock_fd;

void sig_stop_server(int sig){
	
	signal(SIGINT, SIG_DFL);
	close(listen_sock_fd);
	fprintf(stdout, "serv1 closed...\n");
	exit(0);
}

/*	Reads n bytes from the descriptor
 *	the readn() is taken from Steven's Book
 *	as advised in the lecture slides
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

/* 	Write "n" bytes to a descriptor. 
 *  	writen() taken from Steven's Book
 *  	as advised in the lecuture slides
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
	int req_sock_fd, err;

	int counter=0, buf_len, optval=1;
	char send_buf[BUF_LEN];
	
	socklen_t client_sock_len;

	struct sockaddr_in client_sock_addr, server_sock_addr;

	/* create a TCP socket that will be used as server socket */
	listen_sock_fd = socket( AF_INET, SOCK_STREAM, 0);

	if( listen_sock_fd < 0){
		fprintf(stderr, "could not create socket\n");
		return 0;
	}

	/* fill in the server socket address structure */
	bzero( & server_sock_addr, sizeof(server_sock_addr));
	server_sock_addr.sin_family = AF_INET;
	server_sock_addr.sin_addr.s_addr= htonl( INADDR_ANY);
	server_sock_addr.sin_port= htons(SERVER_PORT);
	
	/* allow the program to reuse the port */
	err = setsockopt( listen_sock_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	
	if( err < 0){
		fprintf(stderr, "error in setsockopt()\n");
		return 0;
	}

	/* bind the socket */
	err = bind( listen_sock_fd, (struct sockaddr *) &server_sock_addr, sizeof(server_sock_addr));
	if( err < 0 ){
		fprintf(stderr, "could not bind socket\n");
		return 0;
	}
	
	/* make the socket as server socket */
	err = listen( listen_sock_fd, BACK_LOG);
	if( err < 0 ){
		fprintf(stderr, "socket could not listen\n");
		return 0;
	}
	/* set signal handler for the SIGINT */
	signal(SIGINT, sig_stop_server);

	while( 1 ){
		
		/* make the server socket wait for incoming connection */
		client_sock_len = sizeof(socklen_t);
		req_sock_fd = accept( listen_sock_fd, (struct sockaddr*) &client_sock_addr, & client_sock_len);

		if( req_sock_fd < 0){
			fprintf(stderr, "accept could create socket\n");
			return 0;
		}

		/* ----------------------------- treat request here ---------------------- */
		/* increment the counter */
		counter++;
		sprintf(send_buf, "%d", counter);
		
		/* send the value of counter to the client */
		buf_len = strlen(send_buf);
		err = writen( req_sock_fd, send_buf, buf_len+1);
		
		if( err == -1){
			fprintf(stderr, "error writing to socket\n");
			counter--;
		}

		/* close connection with the client */
		close( req_sock_fd);
	}
	
	return 0;
}
