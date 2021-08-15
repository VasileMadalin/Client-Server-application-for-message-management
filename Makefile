# Tema2PC
# Makefile

CFLAGS = -Wall -g

# Portul pe care asculta serverul (de completat)
PORT = 8080

# Adresa IP a serverului (de completat)
IP_SERVER = 127.0.0.1

all: server subscriber

# Compileaza server.c
server: server.c

# Compileaza subscriber.c
subscriber: subscriber.c

.PHONY: clean run_server run_subscriber

# Ruleaza serverul
run_server:
	./server ${PORT}

# Ruleaza subscriber-ul
run_subscriber:
	./subscriber ${IP_SERVER} ${PORT}

clean:
	rm -f server subscriber
