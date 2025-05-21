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
#include <arpa/inet.h>
//#include <unix.h>

#define MYPORT 8080
#define BACKLOG 1
#define BUFF_SIZE 1024

char data[BUFF_SIZE];
int data_size;

int process_request(char*, char*, int);

int main(int argc, char** argv) {
	short myport;
	int sockfd;
	int new_fd;
	struct sockaddr_in my_addr;
	struct sockaddr_in their_addr;
	int sin_size;
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
			recv_buf[sin_size] = '\0';

			if (sin_size == 0) {
				printf( "Disconnected\n" );
				break;
			}
			if (sin_size != -1) {
				printf( "Recv = %d %s\n", sin_size, recv_buf );
				int send_size = process_request(recv_buf, send_buf, sin_size);
				printf("send size: %d\n", send_size);
				if( send( new_fd, &send_buf, send_size, 0 ) < 0 ) { // odeślij dane
					perror( "Sending data" );
					break;
				}
				break;
			}
		}
		close(new_fd);
	}
	shutdown(sockfd, 2);
	exit(0);
}

int process_request(char* recv_buf, char* send_buf, int sin_size) {
	// client is requesting data
	if (sin_size == 1 && recv_buf[0] == '-') {
		memcpy(send_buf, data, data_size);
		printf("data jego mac: %d\n %s\n", data_size, data);
		return data_size;
	}
	// client is sending data
	memcpy(data, recv_buf, sin_size);
	printf("data saved: length: %d data: %s chuj", strlen(data), data);
	puts("e");
	data_size = sin_size;
	send_buf[0] = 'O';
	send_buf[1] = 'K';
	//sprintf(send_buf, "OK - %d bytes\0", sin_size);
	return 2; // strlen(send_buf);
}
