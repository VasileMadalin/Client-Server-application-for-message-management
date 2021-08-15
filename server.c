#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <byteswap.h>
#include <netinet/tcp.h>

#include "helpers.h"

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}
// aceasta functie afiseaza string-ul reprezentat de impartiea 
// unui numar natural la o putere a lui 10
void convert_float_to_string(uint32_t number, int power, char *aux, 
	uint8_t sign) {
	char number_string[USEFUL_LENGTH];
	sprintf(number_string, "%d", number);
	int number_length = strlen(number_string);
	int length = 0;
	if (sign == 1) {
		aux[length++] = '-';
	}
	// daca puterea este mai mica decat numarul de cifre a lui n
	// atunci punctul(virgula) se adauga in interiorul numarului
	if (power < number_length) {
		for(int j = 0; j < number_length; j++) {
			aux[length] = number_string[j];
			length++;
			if (j == number_length - power - 1 && j != number_length - 1) {
				aux[length++] = '.';
			}
		}
		aux[length] = '\0';	
	}
	else {
		// altfel, punctul(virgula) se adauga la inceput
		aux[length++] = '0';
		aux[length++] = '.';
		for(int j = 0; j < power - number_length; j++) {
			aux[length++] = '0';
		}
		for(int j = 0; j <= number_length; j++) {
			aux[length++] = number_string[j];
		}
	}
}
int main(int argc, char *argv[])
{
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	int sockfd_udp, sockfd_tcp, newsockfd, portno;
	char buffer[BUFLEN];
	struct sockaddr_in serv_addr, cli_addr;
	int n, i, ret_tcp, ret_udp;
	socklen_t clilen;

	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar
	int fdmax;			// valoare maxima fd din multimea read_fds

	if (argc < 2) {
		usage(argv[0]);
	}

	// se goleste multimea de descriptori de citire (read_fds) si 
	// multimea temporara (tmp_fds)
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	// se creeaza un socket nou pentru clietnii TCP
	sockfd_tcp = socket(AF_INET, SOCK_STREAM, 0);

	// se dezactiveaza algoritmul lui Neagle
	int flag = 1;
	int result = setsockopt(sockfd_tcp, IPPROTO_TCP, TCP_NODELAY,
		(char *) &flag, sizeof(int));
	if (result < 0) {
		printf("Neagle's algorithm disabling error");
	}

	// se creeaza un socket nou pentru clientii UDP
	sockfd_udp = socket(AF_INET, SOCK_DGRAM, 0);

	// verificari erori
	DIE(sockfd_tcp < 0, "socket");
	DIE(sockfd_udp < 0, "socket");

	// se seteaza port-ul pe care se face conexiunea
	portno = atoi(argv[1]);
	DIE(portno == 0, "atoi");

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	// udp
	struct sockaddr_in from_station;
	socklen_t socklen = sizeof(from_station); 

	// se creeaza legatura pentru udp si tcp
	ret_tcp = bind(sockfd_tcp, (struct sockaddr *) &serv_addr,
		sizeof(struct sockaddr));
	ret_udp = bind(sockfd_udp, (struct sockaddr*)&serv_addr,
		sizeof(struct sockaddr));

	// verificari erori
	DIE(ret_tcp < 0, "bind");
	DIE(ret_udp < 0, "bind");

	// numarul initial de clienti este 1, acesta creste la conectarea 
	// altor clienti
	int number_clients = 1;

	ret_tcp = listen(sockfd_tcp, number_clients);
	DIE(ret_tcp < 0, "listen");

	// se adauga noul file descriptor (socketul pe care se asculta conexiuni)
	// tcp
	FD_SET(sockfd_tcp, &read_fds);
	// se adauga noul file descriptor (socketul pe care se asculta conexiuni)
	// udp
	FD_SET(sockfd_udp, &read_fds);
	// se adauga noul file descriptor (socketul pe care se asculta conexiuni)
	// keyboard
	FD_SET(0, &read_fds);


	fdmax = 0;
	// se alege maxim-ul dintre socket-ul tcp si udp
	if (sockfd_tcp > sockfd_udp) {
		fdmax = sockfd_tcp;
	}
	else {
		fdmax = sockfd_udp;
	}
	// se aloca dinamic memorie pentru vecotrul de clienti
	t_info_clients *all_the_clients = 
		(t_info_clients *)malloc(sizeof(t_info_clients));
	if (all_the_clients == NULL) {
		printf("Allocation Error");
	}
	int length_clients = 0;

	// se aloca dinamic memorie pentru vectorul de abonati
	t_info_subs *subscriptions = 
		(t_info_subs *)malloc(sizeof(t_info_subs));
	if (subscriptions == NULL) {
		printf("Allocation Error");
	}
	int length_subs = 0;
	int active = 1;
	// se primesc mesaje cat timp server-ul este activ
	while (active == 1) {
		tmp_fds = read_fds; 
		ret_tcp = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret_tcp < 0, "select");
		// se parcurge fiecare socket
		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				// daca socket-ul este cel de keyboard
				if (i == 0) {
					char test_server[USEFUL_LENGTH];
					scanf("%s", test_server);
					if (strcmp(test_server, "exit") == 0) {
						active = 0;
						// se trimite mesajul "stop" la toti clientii
						for(int j = 0; j <= fdmax; j++) {
							if (j != sockfd_udp && j != sockfd_tcp && j != 0) {
								char stop[STOP_MESSAGE];
								strcpy(stop, "stop");
								send(j, stop, BUFLEN, 0);
							}
						}
						// se elibereaza memoria folosita
						free(all_the_clients);
						for(int j = 0; j < length_subs; j++) {
							for(int p = 0; p < 
								subscriptions[j].number_messages; p++) {
								free(subscriptions[j].client_matrix[p]);
							}
							free(subscriptions[j].client_matrix);
						}
						free(subscriptions);
						close(sockfd_tcp);
						close(sockfd_udp);
					}
					
				} else {
					if (i == sockfd_udp) {
						char topic[TOPIC_LENGTH], type[TYPE_LENGTH];
						// se primeste mesaj de la clientul UDP
						int bytesread = recvfrom(sockfd_udp, buffer, BUFLEN, 0, 
							(struct sockaddr*) &serv_addr, &socklen);
						if (bytesread == -1) {
							printf("Error at receiving message");
						}
						// initialiez propriul protocol de incadrare de mesaje
						struct protocol *my_protocol=(struct protocol *)buffer;
						strcpy(topic, my_protocol->topic);
						char sendto[BUFLEN];
						// in functie de tipul de topic trimis, 
						// se creeaza mesajul corespunzator
						if (my_protocol->tip_date == 0) {
							strcpy(type, TYPE_INT);
							// se extrage semnul int-ului
							uint8_t sign = *((uint8_t*)(my_protocol->payload));
							// se extrage valoarea in big endian
							uint32_t unsigned_value_be = 
								*((uint32_t*)(my_protocol->payload+1));
							// se converteste in little endian
							uint32_t unsigned_value = ntohl(unsigned_value_be);
							uint32_t value;
							if (sign == 1) {
								value = 0 - unsigned_value;
							}
							else {
								value = unsigned_value;
							}
							// se construieste mesajul
							sprintf(sendto, "%s:%d - %s - %s - %d", 
								inet_ntoa(serv_addr.sin_addr), 
									ntohs(serv_addr.sin_port), topic, type, value);
						}
						if (my_protocol->tip_date == 1) {
							strcpy(type, TYPE_SHORT_REAL);
							// se extrage int-ul care reprezinta short-ul 
							// inmultit cu 100
							uint16_t short_real_aux = 
								*((uint16_t*)(my_protocol->payload));
							// se fromeaza short-ul
							float short_real = 
								((float) (ntohs(short_real_aux)) / 100);
							//se construieste mesaj-ul
							sprintf(sendto, "%s:%d - %s - %s - %.2f", 
								inet_ntoa(serv_addr.sin_addr), 
									ntohs(serv_addr.sin_port), 
										topic, type, short_real);
						}
						if (my_protocol->tip_date == 2) {
							strcpy(type, TYPE_FLOAT);
							// se extrage semn-ul float-ului
							uint8_t semn = *((uint8_t*)(my_protocol->payload));
							// se extrage floatul inmultit cu o putere de a 
							// lui 10, in big endian
							uint32_t unsigned_value_be = 
								*((uint32_t*)(my_protocol->payload + 1));
							// se converteste in little endian
							uint32_t unsigned_value = 
								ntohl(unsigned_value_be);
							// se extrage puterea lui 10 care trebuie 
							// inmultita cu numarul
							uint8_t modul_putere =  
								*((uint8_t*)(my_protocol->payload + 5));
							char my_float[USEFUL_LENGTH];
							// se copiaza intr-un string numarul intreg 
							// impartit la puterea lui 10
							convert_float_to_string(unsigned_value, 
								modul_putere, my_float, semn);
							// se construieste mesaj-ul
							sprintf(sendto, "%s:%d - %s - %s - %s", 
								inet_ntoa(serv_addr.sin_addr), 
									ntohs(serv_addr.sin_port), 
										topic, type, my_float);
						}

						if (my_protocol->tip_date == 3) {
							strcpy(type, TYPE_STRING);
							// se construieste mesaj-ul 
							sprintf(sendto, "%s:%d - %s - %s - %s", 
								inet_ntoa(serv_addr.sin_addr), 
									ntohs(serv_addr.sin_port), 
										topic, type, my_protocol->payload);
						}
						// se parcurg toti abonatii si se trimite celui 
						// care este abonat la topic-ul respectiv
						for(int j = 0; j < length_subs; j++) {
							if (strcmp(subscriptions[j].topic, topic) == 0) {
								// daca abonatul este activ, se trimite mesajul
								if (subscriptions[j].status == 1) {
									send(subscriptions[j].sock, 
										sendto, BUFLEN, 0);
								} else {//daca nu este activ, se verifica sf-ul
									// pentru sf 1, se adauga mesajele in
									// matricea client-ului pentru a putea fi
									// transmie cand acesta devine activ 
									if (subscriptions[j].sf == 1) {
										int length = 
											subscriptions[j].number_messages;
										subscriptions[j].client_matrix[length] = 
											(char*)malloc(BUFLEN*sizeof(char));
										if (subscriptions[j].client_matrix[length] 
											== NULL) {
											printf("Allocation Error");
										}
										strcpy(
										subscriptions[j].client_matrix[length], 
										sendto);

										subscriptions[j].number_messages 
											= length + 1;
										// se realoca numar-ul de mesaje 
										// din matrice
										int size = 
										sizeof(char *)*
										(subscriptions[j].number_messages + 1);

										char **new_client_matrix = 
										(char **)
										realloc(subscriptions[j].client_matrix, 
											size);
										if (new_client_matrix == NULL) {
											printf("Reallocation Error");
										}
										subscriptions[j].client_matrix = 
											new_client_matrix;
									}
								}
							}
						}
					} else {
						if (i == sockfd_tcp) {
							// a venit o cerere de conexiune pe socketul 
							// inactiv (cel cu listen),
							// pe care serverul o accepta
							clilen = sizeof(cli_addr);
							newsockfd = accept(sockfd_tcp, 
								(struct sockaddr *) &cli_addr, &clilen);
							DIE(newsockfd < 0, "accept");
							number_clients++;
							ret_tcp = listen(sockfd_tcp, number_clients);
							DIE(ret_tcp < 0, "listen");

							// se adauga noul socket intors de accept() 
							// la multimea descriptorilor de citire
							FD_SET(newsockfd, &read_fds);
							if (newsockfd > fdmax) { 
								fdmax = newsockfd;
							}
							// adaug socket-ul in vectorul de clientii
							all_the_clients[length_clients].sock = newsockfd;
							// adauga in vector toate informatiile 
							strcpy(all_the_clients[length_clients].ip_client, 
								inet_ntoa(cli_addr.sin_addr));
							all_the_clients[length_clients].port = 
								ntohs(cli_addr.sin_port);
							strcpy(all_the_clients[length_clients].id_client, 
								"unknown");
							length_clients++;
							// se realoca memorie pentru vector
							t_info_clients *new_all_the_clients = 
								(t_info_clients *)realloc(all_the_clients, 
									sizeof(t_info_clients)*(length_clients+1));
							if (new_all_the_clients == NULL) {
								printf("Reallocation Error");
							} 
							all_the_clients = new_all_the_clients;

						} else {
							// s-au primit date pe unul din socketii de client,
							// serverul le receptioneaza
							t_paket m;
							int received_size = 0;
							n = -1;
							while(sizeof(m) - received_size > 0 && n != 0) {
								n = recv(i, (&m) + received_size, 
									sizeof(m) - received_size, 0);
								if (n == -1) {
									printf("Error at receiving message");
								}
								received_size = received_size + n;
							}
							DIE(n < 0, "recv");
							if (n == 0) {
								// conexiunea s-a inchis
								close(i);
								// se scoate din multimea de citire 
								// socketul inchis 
								FD_CLR(i, &read_fds);
							} else {
								if (m.index_message == 0) {
									// se verifica daca clientul exista deja, 
									// ca sa nu mai fie adaugat o data
									int check_client = 0;
									for(int j = 0; j < length_clients; j++) {
										if(strcmp(all_the_clients[j].id_client,
												m.id_client) == 0) {
											check_client = 1;
											break;
										}
									}
									if (check_client == 0) {
										// se cauta clientul in vectorul de 
										// abonati,daca exista si are sf 1, se
										// trimit toate mesajele netrimise
										int j;
										for (j = 0; j < 
											length_subs; j++) {
											if(strcmp(subscriptions[j].id_client
												,m.id_client) == 0) {

												subscriptions[j].sock = i;
												// se seteaza status-ul pe 1
												subscriptions[j].status = 1;
												if (subscriptions[j].sf == 1) {
													int length = 
													subscriptions[j].number_messages;
													for(int l=0;l<length;l++) {
														char test[BUFLEN];
														strcpy(test, 
														subscriptions[j].
														     client_matrix[l]);
														int m = send(i,test,BUFLEN,0);
														DIE(m < 0, "SEND FAILED");
													}
												}
											}
										}
										// caut socketul de pe care sa 
										// iau informatiile
										for(j = 0; j < length_clients; j++) {
											if (all_the_clients[j].sock == i) 
												break;
										}
										// se afiseaza mesajul de client nou
										strcpy(all_the_clients[j].id_client, 
											m.id_client);
										printf(
										"New client %s connected from %s:%d.\n"
										,m.id_client,
										all_the_clients[j].ip_client,
										all_the_clients[j].port);
									}
									// clientul exista, se trimite notificare 
									// cu stop, pentru a fi oprit
									else {
										printf("Client %s already connected.\n"
											, m.id_client);
										char buffer[BUFLEN];
										strcpy(buffer, "stop");
										int m = send(i, buffer, BUFLEN, 0);
										DIE(m < 0, "SEND FAILED");
									}
								} else { // clientul se aboneaza la un topic 
									if (strncmp(m.message,"subscribe",9)==0) {
										int topic_length = 0;
										// se extrage topic-ul si sf-ul 
										// din continutul mesajului
										char topic[TOPIC_LENGTH];
										int message_length = strlen(m.message);
										for(int j = 10; j < message_length - 2;
											j++) {
											topic[topic_length] = m.message[j];
											topic_length++;
										}
										int sf = 
											m.message[message_length - 1]-'0';
										topic[topic_length] = '\0';
										strcpy(subscriptions[length_subs].id_client, 
												m.id_client);

										strcpy(subscriptions
										[length_subs].topic, topic);
										subscriptions[length_subs].sock = i;
										subscriptions[length_subs].sf = sf;
										subscriptions[length_subs].number_messages
											= 0;
										subscriptions[length_subs].status = 1;
										// se aloca memorie pentru matricea 
										// de mesaje netrimise
										if (sf == 1) {
											// se aloca memorie pentru 
											// matrice doar daca sf este 1 
											// (abonatul primeste mesaje)
											subscriptions[length_subs].client_matrix
											= (char **)malloc(sizeof(char *));
											if (subscriptions[length_subs].client_matrix
												== NULL) {
												printf("Allocation Error");
											}
										}
										length_subs++;
										// se realoca memorie 
										// pentru vectorul de abonati
										t_info_subs *new_subscriptions = 
										(t_info_subs *)realloc(subscriptions, 
											sizeof(t_info_subs)*(length_subs + 1));
										if (new_subscriptions == NULL) {
											printf("Reallocation Error");
										}
										subscriptions = new_subscriptions;
										// se trimite mesaj de abonare
										strcpy(buffer, "Subscribed to topic.");
										int m = send(i, buffer, BUFLEN, 0);
										DIE(m < 0, "SEND FAILED");
									} // clientul se dezaboneaza de la un topic
									if (strncmp(m.message, "unsubscribe", 11) == 0) {
										// se extrage topic-ul
										int topic_length = 0;
										char topic[TOPIC_LENGTH];
										int message_length = strlen(m.message);
										int j;
										for(int j = 12; j < message_length; j++) {
											topic[topic_length] = m.message[j];
											topic_length++;
										}
										topic[topic_length] = '\0';
										// se trimite mesaj de dezabonare 
										strcpy(buffer, "Unsubscribed from topic.");
										int m = send(i, buffer, BUFLEN, 0);
										DIE(m < 0, "SEND FAILED");
										for(j = 0; j < length_subs; j++) {
											if (subscriptions[j].sock==i && 
											strcmp(subscriptions[j].topic,topic)==0) 
												break;
										} 
										// se elemina topic-ul respectiv
										for (int k = j; k < length_subs - 1; k++) {
											memcpy(subscriptions + k, 
												subscriptions + k + 1, 
													sizeof(t_info_subs));
										}
										length_subs = length_subs - 1;
									}
									if (strcmp(m.message, "exit") == 0) {
										// se scoate clientul din vectorul de clienti
										//setam status-ul client-ului pe 0 in structura cu abonati
										int j;
										for(j = 0; j < length_subs; j++) {
											if (strcmp(subscriptions[j].id_client, 
													m.id_client) == 0) {
												subscriptions[j].status = 0;
											}	 
										}
										for(j = 0; j < length_clients; j++) {
											if (strcmp(all_the_clients[j].id_client, 
												m.id_client) == 0) {
												break;
											}
										}
										// se elimina clientul din vectorul de clienti
										for (int k = j; k < length_clients - 1; k++) {
											memcpy(all_the_clients + k, 
												all_the_clients + k + 1, 
													sizeof(t_info_clients));
										}
										length_clients = length_clients - 1;
										// se afiseaza mesajul de deconectare
										printf("Client %s disconnected.\n", 
											m.id_client);
										char buffer[BUFLEN];
										// se trimite mesajul "stop" la client, 
										// pentru a se deconecta
										strcpy(buffer, "stop");
										int m = send(i, buffer, BUFLEN, 0);
										DIE(m < 0, "SEND FAILED");
									}	
								}
							}
						}
					}
				}
			}
		}
	}
	return 0;
}
