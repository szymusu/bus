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
ssize_t data_size;

ssize_t process_request(char*, char*, ssize_t);

int main(int argc, char** argv) {
	short myport;
	int my_socket;
	int incoming_socket;
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
	if ((my_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}
	// ustaw parametry gniazda
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons( myport );
	my_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(my_addr.sin_zero), 8);
	if (bind(my_socket, (struct sockaddr *) &my_addr, sizeof(struct sockaddr))== -1) {
		perror("bind");
		exit(1);
	}
	// czekaj na połączenie
	if (listen( my_socket, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}
	// odbierz połączenie
	while(1) {
		sin_size = sizeof(struct sockaddr_in);
		incoming_socket = accept(my_socket, (struct sockaddr *) &their_addr, &sin_size);
		if(incoming_socket == -1) {
			perror("accept");
			continue;
		}
		printf("server: got connection from %s\n", inet_ntoa(their_addr.sin_addr));

		while(1) {
			sin_size = recv(incoming_socket, &recv_buf, sizeof(recv_buf), 0); // odbierz dane
			recv_buf[sin_size] = '\0';

			if (sin_size == 0) {
				printf("Disconnected\n");
				break;
			}
			if (sin_size != -1) {
				ssize_t send_size = process_request(recv_buf, send_buf, sin_size);

				printf("send size: %ld\n", send_size);
				send_buf[send_size] = 0;
				ssize_t bytes_sent = send(incoming_socket, send_buf, send_size, 0);
				if( bytes_sent < 0 ) { // odeślij dane
					perror( "Sending data" );
					break;
				}
				printf("Successfully sent %ld bytes\n", bytes_sent);
				break;
			}
		}
		close(incoming_socket);
	}
	shutdown(my_socket, 2);
	exit(0);
}

ssize_t process_request(char* recv_buf, char* send_buf, ssize_t sin_size) {
	// client is requesting data
	if (sin_size == 1 && recv_buf[0] == '-') {
		memcpy(send_buf, data, data_size);
		puts("Got request to send data");
		printf("size: %ld\n", data_size);
		printf("data: %s\n", data);
		printf("send buffer: %s\n", send_buf);
		return data_size;
	}
	// client is sending data
	printf("Received data: %s\n", recv_buf);
	printf("Size: %ld\n", sin_size);
	memcpy(data, recv_buf, sin_size);
	data[sin_size] = 0;
	printf("Saved data: %s\n", data);
	printf("Strlen: %ld\n", strlen(data));
	data_size = sin_size;
	send_buf[0] = 'O';
	send_buf[1] = 'K';
	return 2;
}
