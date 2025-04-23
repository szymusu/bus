#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
//#include <unix.h>

#define MYPORT 8080
#define BACKLOG 1
#define BUFF_SIZE 1024

int main(int argc, char** argv) {
	short myport;
	int sockfd;
	int new_fd;
	struct sockaddr_in my_addr;
	struct sockaddr_in their_addr;
	int sin_size;
	int out_size;
	int recv_data;
	char send_buf[BUFF_SIZE];
	char recv_buf[BUFF_SIZE];

	// wczytaj argumenty z linii komend: nr portu
	if( argc >= 2 ) {
		myport = atoi( argv[1] );
	}
	else {
		myport = MYPORT;
	}
	signal( SIGCHLD, SIG_IGN );
	// utwórz gniazdo
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}
	// ustaw parametry gniazda
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons( myport );
	my_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(my_addr.sin_zero), 8);
	if (bind(sockfd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr))== -1) {
		perror("bind");
		exit(1);
	}
	// czekaj na połączenie
	if (listen( sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}
	// odbierz połączenie
	while(1) {
		sin_size = sizeof(struct sockaddr_in);
		new_fd = accept(sockfd, (struct sockaddr *) &their_addr, &sin_size);
		if(new_fd == -1) {
			perror("accept");
			continue;
		}
		printf("server: got connection from %s\n", inet_ntoa(their_addr.sin_addr));

		for( ;; ) {
			sin_size = recv(new_fd, &recv_buf, sizeof(recv_buf), 0); // odbierz dane

			if (sin_size == 0) {
				printf( "Disconnected\n" );
				break;
			}
			if (sin_size != -1) {
				printf( "Recv = %d %s\n", sin_size, recv_buf );
				if( send( new_fd, &send_buf, sin_size, 0 )<0 ) { // odeślij dane
					perror( "Sending data" );
					break;
				}
			}
		}
		close(new_fd);
	}
	shutdown(sockfd, 2);
	exit(0);
}
