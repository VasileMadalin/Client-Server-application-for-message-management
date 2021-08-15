#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include "helpers.h"

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_address server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	int sockfd, n, ret;
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN];
	fd_set readFds;
	fd_set tmpFds;

	int fdMax;

	// se goleste multimea de descriptori de citire (read_fds) si 
	// multimea temporara (tmp_fds)
	FD_ZERO(&tmpFds);
	FD_ZERO(&readFds);

	if (argc < 3) {
		usage(argv[0]);
	}
	// se creeaza un socket nou pentru clietnii TCP
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	// se dezactiveaza algoritmul lui Neagle
	int flag = 1;
	int result = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *) &flag,
		sizeof(int));

	if ( result < 0 ) {
		printf("Neagle's algorithm disabling error");
	}

	// verificare erori
	DIE(sockfd < 0, "socket");

	// se adauga noul file descriptor (socketul pe care se asculta conexiuni)
	// tcp
	FD_SET(sockfd, &readFds);
	fdMax = sockfd;
	FD_SET(0, &readFds);

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");

	// se trimite pachetul 0
	t_paket m;
	m.index_message = 0;
	strcpy(m.id_client, argv[1]);
	strcpy(m.message, "undefined"); 

	n = send(sockfd, &m, sizeof(m), 0);

	// verificare erori
	DIE(n < 0, "SEND FAILED");
	
	int index = 1;
	int active = 1;
	while (active == 1) {
		tmpFds = readFds; 
		ret = select(fdMax + 1, &tmpFds, NULL, NULL, NULL);
		DIE(ret < 0, "Nu putem selecta un socket");

		if (FD_ISSET(0, &tmpFds)) {
			// se citeste mesajul, si se trimite la server
			memset(buffer, 0, BUFLEN);
			fgets(buffer, BUFLEN - 1, stdin);
			int length = strlen(buffer);
			buffer[length - 1] = '\0';

			t_paket m;
			m.index_message = index;
			// se seteaza id-ul clientului 
			// ca fiind al doilea argument
			strcpy(m.id_client, argv[1]);
			strcpy(m.message, buffer); 
			// se trimite mesajul
			n = send(sockfd, &m, sizeof(m), 0);
			//verificare eroare
			DIE(n < 0, "SEND FAILED");
			index++;

		} else {
			char test[BUFLEN];
			//recv(sockfd, buffer, BUFLEN, 0);
			int received_size = 0;
			int n = -1;
			// se primesc date de la server
			while(BUFLEN - received_size > 0 && n != 0) {
				n = recv(sockfd, test + received_size, BUFLEN - received_size, 0);
				if (n == -1) {
					printf("Error at receiving message");
				}
				received_size = received_size + n;
			}
			// dace se primeste mesajul "stop", se inchide conexiunea
			if (strcmp(test, "stop") == 0) {
				break;
			} // altfel, se afiseaza mesajul
			else {
				printf("%s\n", test);
			}
		}
	}
	close(sockfd);

	return 0;
}
