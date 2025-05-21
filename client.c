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
	int is_read;
	// wczytaj argumenty z linii komend: nr hosta, nr portu
	if( argc < 4 ) {
		printf( "\nUsage: tcp-client <host> <port> <type>\n\n" );
		exit( 1 );
	}

	if (argv[3][0] == 'r') {
		is_read = 1;
	}
	else if (argv[3][0] == 'w') {
		is_read = 0;
	}
	else {
		perror("type should be 'r' or 'w'");
		exit(-1);
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

	while( 1 ){
		if (is_read) {
			puts("Press enter to request current data");
			send_buf[0] = '-';
			send_buf[1] = 0;
			data_length = 1;
			getchar();
		}
		else {
			gets( send_buf );
			if( send_buf[0]=='-' ) break;
			data_length = strlen( send_buf );
			send_buf[data_length + 1] = 0;
			send_buf[data_length] = '\n';
		}

		// Utworz gniazdo (socket)
		sockfd = socket( AF_INET, SOCK_STREAM, 0 );
		if( sockfd < 0 ) {
			perror( "Opening stream socket" );
			exit( 1 );
		}

		// Połącz się z serwerem
		if( connect( sockfd, (struct sockaddr *)&server, sizeof(server)) < 0 ) {
			perror("connecting stream socket");
			exit(1);
		}
		printf( "Connected to %s\n", argv[1] );

		// wyślij dane
		if( write( sockfd, send_buf, data_length ) < 0) {
			perror("writing on stream socket");
		}
		// odbierz dane
		if(( recv_data_length = read( sockfd, recv_buf, data_length)) < 0) {
			perror("reading on stream socket");
		}
		recv_buf[recv_data_length] = 0;
		printf( "Recv: %s - %d bytes\n", recv_buf, recv_data_length );
		close( sockfd );
	}
}
