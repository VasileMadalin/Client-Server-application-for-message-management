#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <stdlib.h>

#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)

#define BUFLEN		1551	// dimensiunea maxima a calupului de date
#define PAYLOAD_LENGTH 1500
#define ID_CLIENT_LENGTH 30
#define IP_CLIENT_LENGTH 30
#define TOPIC_LENGTH 50
#define USEFUL_LENGTH 30
#define TYPE_LENGTH 25
#define STOP_MESSAGE 10
#define TYPE_INT "INT"
#define TYPE_SHORT_REAL "SHORT_REAL"
#define TYPE_FLOAT "FLOAT"
#define TYPE_STRING "STRING"

// structura care reprezinta forma unui mesaj trimis de clientul TCP
typedef struct paket {
	int index_message;
	char id_client[ID_CLIENT_LENGTH];
	char message[BUFLEN];
} t_paket;

// structura care mentine cunoscuta relatia dintre id-ul clientului, 
// ip-ul clinetului,socket-ul si portul pe care acesta s-a conectat
typedef struct info_clients {
	char id_client[ID_CLIENT_LENGTH]; // id-ul clientului
	char ip_client[IP_CLIENT_LENGTH]; // ip-ul clientului
	int port; // port - ul clientului
	int sock; // socket-ul clientului
} t_info_clients;

// structura care stocheaza informatii despre abonati
typedef struct info_subs {
	char topic[TOPIC_LENGTH]; // subiectul
	char id_client[ID_CLIENT_LENGTH]; // id-ul clientului
	char **client_matrix;//[20][2005]; // matricea care contine mesajele 
								// care trebuie trimise clientului
	int number_messages; // numarul de mesaje netrimise
	int sock; // socketul pe care este conectat clientul
	int sf;  // store and forward
	int status; // 1 daca clientul este conectat, 0 in caz contrar
} t_info_subs;

//structura folosita pentru protocolul care incadreaza mesajele
struct __attribute__((__packed__)) protocol {
	char topic[TOPIC_LENGTH]; // topicul
	uint8_t tip_date; // tipul de date
	char payload[PAYLOAD_LENGTH];// payload-ul
};

#endif
