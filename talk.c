#include<sys/types.h>
#include<unistd.h>
#include<string.h>
#include<stdlib.h>
#include<netdb.h>
#include<stdio.h>
#include<errno.h>
#include<signal.h>
#include<sys/select.h>

#define BACK_LOG 5
#define SERVER_PORT 1740
#define BUF_LEN 1024

/* filedescriptor for server socket */
int listen_sock_fd;

/* file descriptor for client side socket */
int client_sock_fd;

/* file descriptor returned by accept() */
int req_sock_fd;

/**********************************************	
 *	Description: this function is called to 
 *	exit the program 'talk' when it behaves 
 *	as server 
 * ********************************************/
void sig_stop_server(int sig){
	
	signal(SIGINT, SIG_DFL);
	close(listen_sock_fd);
	close(req_sock_fd);
	fprintf(stdout, "talk closed...\n");
	exit(0);
}

/**********************************************	
 *	Description: this function is called to 
 *	exit the program 'talk' when it behaves 
 *	as client 
 * ********************************************/
void sig_stop_client(int sig){

	signal(SIGINT, SIG_DFL);
	close(client_sock_fd);
	fprintf(stdout, "talk client closed...\n");
	exit(0);
}

/*	
 *	Description: The readn() function from Steven's Book
 *		     as suggested in lecture slides
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


/*	
 *	Description: The writen() function from Steven's Book
 *	             as suggested in lecture slides
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

/*	Description: This function is called 
 *	when the program has to behave as client
 *	The function stops the program by calling
 *	sig_stop_client() if it receives and EOF
 *	from user or from the socket
 *	
 *	@params 
 *		ip: ip address of the server 
 *	            supplied by the user to
 *	            execute the program in 
 *	            client mode
 *	@params 
 *		return: if some error condition 
 *		occurs then the function displays
 *		an error message and simply returns
 * */

void behave_as_client(char* ip){

	int stdin_fd, err, num_bytes, max_fd;
	/* buffers to hold inputs from standard input and socket repectively */
	char stdin_fd_buf[BUF_LEN], sock_buf[BUF_LEN];
	
	socklen_t server_sock_len;
	struct sockaddr_in server_sock_addr;

	fd_set rset;

	stdin_fd = 0;/* set stdin_fd to standard input */
	
	/* create a TCP socket for client to connect to server */

	client_sock_fd = socket(AF_INET, SOCK_STREAM, 0);

	if( client_sock_fd < 0 ){
		fprintf(stderr, "could not create client socket\n");
		return;
	}
	
	/* fill in the server socket address structure */
	bzero(&server_sock_addr, sizeof(server_sock_addr));
	server_sock_addr.sin_family = AF_INET;
	server_sock_addr.sin_port = htons(SERVER_PORT);
	
	err = inet_aton(ip, &server_sock_addr.sin_addr);
	if( err == 0){
		fprintf(stderr, "could not convert dotted address to network address\n");
		return;
	}
	
	/* connect to the server */
	server_sock_len = sizeof(server_sock_addr);
	err = connect( client_sock_fd, (struct sockaddr *)&server_sock_addr, server_sock_len);
	if( err < 0){
		fprintf(stderr, "could not connect to the server\n");
		return;
	}
	/* set the SIGINT */
	signal(SIGINT, sig_stop_client);

	/* start communication with server after connection establishment */
	while(1){
		
		/* decide to read from socket or keyboard */
			
		FD_ZERO(&rset);
		FD_SET(stdin_fd, &rset);
		FD_SET(client_sock_fd, &rset);
		
		/* choose the file descriptor with maximum value */
		max_fd = (stdin_fd > client_sock_fd) ? stdin_fd : client_sock_fd;
		
		/* call select to monitor the standard input and the client socket */
		err = select(max_fd+10,&rset, NULL, NULL, NULL);
		
		if ( err <= 0 ) { 
			fprintf(stderr, "error in client select()"); 
			continue;
		}
		
		/* check if standard input is ready */
		if ( FD_ISSET( stdin_fd, &rset ) ) {
			
			bzero(stdin_fd_buf,BUF_LEN);
			
			/* read data from standard input */
			num_bytes = read( stdin_fd, stdin_fd_buf, BUF_LEN);
	
			if ( num_bytes < 0) { 

				fprintf(stderr, "error in receiving data from standard input");

			}
			else if (num_bytes == 0) { /* the client entered an EOF ctrl+d */
				
				/* stop the client */
				sig_stop_client(0);
				
			}
			else{ /* send the user input to the server */
						
				err = writen(client_sock_fd, stdin_fd_buf, BUF_LEN);
	
				if( err == -1){

					fprintf(stderr, "could not write data to socket\n");

				}
				else if( err == 0){

					fprintf(stderr, "no data sent to server\n");

				}
			}
		}

		/* if client socket is ready */
		if (FD_ISSET(client_sock_fd,&rset)) {
			/* read data from client socket */			
			num_bytes = readn(client_sock_fd, sock_buf, BUF_LEN);
			
			if( num_bytes < 0 ){
				
				fprintf(stderr,"erro receiving data from server\n");
			
			}
			else if( num_bytes == 0 ){ /* the server sent an EOF so stop the client */
				
				sig_stop_client(0);
			}
			else{

				fprintf(stdout, "SERVER: %s", sock_buf);
			}
		}
	}/* end of while(1) loop */
}


/*	Description: This function is called 
 *	when the program has to behave as server
 *	The function stops the program by calling
 *	sig_stop_server() if it receives and EOF
 *	from user or from the socket
 *	
 *	@params 
 *		return: if some error condition 
 *		occurs then the function displays
 *		an error message and simply returns
 * */

void behave_as_server(){

	int num_bytes, stdin_fd, max_fd;
	char stdin_fd_buf[BUF_LEN], sock_buf[BUF_LEN];
	fd_set rset;

	int err;

	int buf_len, optval=1;
	char send_buf[BUF_LEN];
	
	socklen_t client_sock_len;

	struct sockaddr_in client_sock_addr, server_sock_addr;

	stdin_fd=0; 

	listen_sock_fd = socket( AF_INET, SOCK_STREAM, 0);

	if( listen_sock_fd < 0){
		fprintf(stderr, "could not create socket\n");
		return;
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
		return;
	}
 
	/* bind the socket */
	err = bind( listen_sock_fd, (struct sockaddr *) &server_sock_addr, sizeof(server_sock_addr));
	if( err < 0 ){
		fprintf(stderr, "could not bind socket\n");
		return;
	}
	
	/* make the socket as server socket */
	err = listen( listen_sock_fd, BACK_LOG);
	if( err < 0 ){

		fprintf(stderr, "socket could not listen\n");
		return;
	}
	/* set the handler SIGINT */
	signal(SIGINT, sig_stop_server);
	
	/* make the server socket wait for incoming connection */
	client_sock_len = sizeof(socklen_t);
	req_sock_fd = accept( listen_sock_fd, (struct sockaddr*) &client_sock_addr, & client_sock_len);
		
	if( req_sock_fd < 0){
		fprintf(stderr, "server could not accept connection\n");
		return;
	}
	
	while( 1 ){
		
		/* decide to read from socket or keyboard */
		
		FD_ZERO(&rset);
		FD_SET(stdin_fd, &rset);
		FD_SET(req_sock_fd, &rset);
		
		/* choose maximum of file descriptors */
		max_fd = ( stdin_fd > req_sock_fd ) ? stdin_fd : req_sock_fd;
		
		num_bytes = select(max_fd+10,&rset, NULL, NULL, NULL);
		
		if (num_bytes<=0) { 

			fprintf(stderr, "error in server select()\n"); 
		}
		
		/* check if the standard input is ready */
		if (FD_ISSET(stdin_fd,&rset)) {

			bzero(stdin_fd_buf,BUF_LEN);
			/* read data from the standard input */
			num_bytes = read(stdin_fd,stdin_fd_buf,BUF_LEN);
			
			if (num_bytes<0) { 
				fprintf(stderr, "error in receiving data from standard input\n");
				continue;
			}

			if (num_bytes==0){ 

				sig_stop_server(0);

			}
			else{
				/* send data to client */
				err = writen(req_sock_fd, stdin_fd_buf, BUF_LEN);
				if( err == -1 ){

					fprintf(stderr, "could not write data to socket\n");

				}
				else if( err == 0 ){

					fprintf( stderr, "no data sent to client\n");

				}
			}

		}

		/* check if the socket is ready */
		if (FD_ISSET(req_sock_fd,&rset)) {
			/* read data from socket */		
			num_bytes = readn(req_sock_fd, sock_buf, BUF_LEN);

			if( num_bytes == -1 ){

				fprintf(stderr, "error receiving data from client\n");

			}
			else if( num_bytes == 0 ){ /* the server received an EOF from client */
				
				/* stop the server */
				sig_stop_server(0);
			}
			else{

				fprintf(stdout, "CLIENT: %s", sock_buf);

			}
		}
	}/* end of while(1) loop */
}


int main(int argc, char* argv[])
{

	if( argc > 2){
		fprintf(stdout, "usage: ./talk <ip address>\n");
		return 0;
	}

	/* server mode starts */
	if( argc == 1){
		behave_as_server();
		
		/* if the function behave_as_server()
		 * returns due to an error then stop
		 * the server properly */
		sig_stop_server(0);
	}
	/* server mode ends */

	/* client mode starts */
	if( argc == 2){
		
		behave_as_client(argv[1]);
				
		/* if the function behave_as_client() 
		 * returns due to an error then stop 
		 * the client properly*/
		sig_stop_client(0);
	}
	/* client mode ends */

	return 0;
}
