#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define BUFF_SIZE 1024

int main(int argc, char *argv[]) {
	int sockfd;
	struct sockaddr_in server;
	struct hostent *hp, *gethostbyname();
	char recv_buf[BUFF_SIZE];
	char send_buf[BUFF_SIZE];
	int data_length;
	int recv_data_length;
	// wczytaj argumenty z linii komend: nr hosta, nr portu
	if( argc < 3 ) {
		printf( "\nUsage: tcp-client <host> <port>\n\n" );
		exit( 1 );
	}
	// Utworz gniazdo (socket)
	sockfd = socket( AF_INET, SOCK_STREAM, 0 );
	if( sockfd < 0 ) {
		perror( "Opening stream socket" );
		exit( 1 );
	}
	// Połącz gniazdo używając nazwy i portu wyspecyfikowanych w lini komend
	server.sin_family = AF_INET;
	hp = gethostbyname( argv[1] );
	if(hp == 0) {
		fprintf(stderr, "%s: unknown host\n", argv[1]);
		exit(2);
	}
	memcpy(&server.sin_addr, hp->h_addr, hp->h_length);
	server.sin_port = htons( atoi(argv[2]) );
	// Połącz się z serwerem
	if( connect( sockfd, (struct sockaddr *)&server, sizeof(server)) < 0 ) {
		perror("connecting stream socket");
		exit(1);
	}
	printf( "Connected to %s\n", argv[1] );
	while( 1 ){
		gets( send_buf );
		if( send_buf[0]=='-' ) break;

		send_buf[strlen( send_buf )+1]=0;
		send_buf[strlen( send_buf )]='\n';
		data_length = strlen( send_buf );
		// wyślij dane
		if( write( sockfd, send_buf, data_length ) < 0) {
			perror("writing on stream socket");
		}
		// odbierz dane
		if(( recv_data_length=read( sockfd, recv_buf, data_length)) < 0) {
			perror("reading on stream socket");
		}
		printf( "Recv: %s - %d bytes\n", recv_buf, recv_data_length );
	}
	close( sockfd );
}
