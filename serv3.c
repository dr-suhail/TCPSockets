#include<sys/types.h>
#include<unistd.h>
#include<string.h>
#include<stdlib.h>
#include<netdb.h>
#include<stdio.h>
#include<errno.h>
#include<signal.h>
#include<sys/ipc.h>
#include<errno.h>
#include<sys/shm.h>
#include<sys/sem.h>

#define BACK_LOG 5
#define SERVER_PORT 1740
#define BUF_LEN 30
#define NB_PROC 10

/* 	DEBUG 1 means include the code for debugging in compilation 
 *	DEBUG 0 means do not include the code for debugging in compilation
 * */
#define DEBUG 1



		/* Global variables section */

/* filedescriptor for server socket */
int listen_sock_fd;

/* this will hold the shared memory id */
int shmid;

/* pointer to the shared memory */
int* counter;

/* this will hold the semaphore id */
int my_sem;

		/* Global variables section end */

void sig_stop_server(int sig){
	
	signal(SIGINT, SIG_DFL);

	/* close server socket */
	close(listen_sock_fd);

	/* detach the shared memroy */
	shmdt((void*) counter);
	
	/* destroy the shared memory */
	shmctl(shmid, IPC_RMID, 0);

	/* destroy the semaphore */
	semctl(my_sem, 0, IPC_RMID);

	fprintf(stdout, "serv3 closed...\n");

	exit(0);
}


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

/* Write "n" bytes to a descriptor. */
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

void recv_request( ){

	int req_sock_fd, err, buf_len;
	
	char send_buf[BUF_LEN];
	
	socklen_t client_sock_len;
	struct sockaddr_in client_sock_addr;

	struct sembuf up = {0, 1, 0};
	struct sembuf down = {0, -1, 0};

	while( 1 ){

		/* call DOWN() on semaphore */
		semop(my_sem, &down, 1);

		/* make the server socket wait for incoming connection */
		client_sock_len = sizeof(socklen_t);
		req_sock_fd = accept( listen_sock_fd, (struct sockaddr*) &client_sock_addr, & client_sock_len);
		
		if( req_sock_fd < 0){
			fprintf(stderr, "accept could not create socket\n");
			exit(0);
		}

		/* increment the counter */
		*counter = (*counter) + 1;

#if DEBUG == 0 
		/* send only the value of counter to client */
		sprintf(send_buf, "%d", *counter);

#elif DEBUG == 1
		/* send the value of counter along with sending process id in a formatted message */	
		sprintf(send_buf, "%d from process: %d", *counter, getpid());
		
#endif

		/* send the value of counter to the client */
		buf_len = strlen(send_buf);
		err = writen( req_sock_fd, send_buf, buf_len+1);
		
		if( err == -1){
			fprintf(stderr, "error writing to socket\n");
			/* decrement counter because the client request was not fulfilled */
			*counter = (*counter) - 1;
		}

		/* close connection with the client */
		close( req_sock_fd);
		
		/* call UP() on the semaphore */
		semop(my_sem, &up, 1);
	}
	/* detatch the shared memory from child process */
	shmdt((void*) counter);
}

int main(int argc, char* argv[])
{
	int req_sock_fd, err, i;

	pid_t pid;

	int buf_len, optval=1;
	char send_buf[BUF_LEN];
	
	socklen_t client_sock_len;

	struct sockaddr_in client_sock_addr, server_sock_addr;

	shmid = shmget( IPC_PRIVATE, sizeof(int), 0600);
	counter = (int*) shmat(shmid, 0, 0);
	
	my_sem = semget(IPC_PRIVATE, 1, 0600);
	semctl(my_sem, 0, SETVAL, 1);

	/* initialize counter */
	*counter=0;
	
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

	/* set handler for SIGINT */
	signal(SIGINT, sig_stop_server);

	/* prefork()ing */
	for(i=0; i<NB_PROC; i++){
	
		pid = fork();
		if( pid == -1){
			fprintf(stderr, "could not start prefork()ing\n");
			return 0;
		}
		else if( pid  == 0 ){

			recv_request();
		}
	}

	while(1) pause();

	return 0;
}
