
CC = gcc                 # Imposta il compilatore da usare (GNU Compiler Collection)
CFLAGS = -Wall -Wextra -O3   # Opzioni di compilazione:
                            # -Wall   = attiva tutti gli avvisi comuni
                            # -Wextra = attiva avvisi aggiuntivi
                            # -O3     = livello massimo di ottimizzazione

TARGET = client          # Nome del file eseguibile finale da generare
OBJS = main.o function.o # Lista dei file oggetto necessari per costruire l'eseguibile

all: $(TARGET)           # Target predefinito: eseguito con 'make' senza argomenti
                         # Dipende dal file 'client' (cioè $(TARGET))

$(TARGET): $(OBJS)       # Costruisce il target (client) a partire dai file oggetto
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) # Compila e linka main.o e function.o nell'eseguibile 'client'

main.o: main.c function.h           # Per generare main.o servono main.c e function.h
	$(CC) $(CFLAGS) -c main.c         # Compila main.c in main.o senza eseguire il linking (-c)

function.o: function.c function.h datastructures.h # Per generare function.o servono questi header e il sorgente
	$(CC) $(CFLAGS) -c function.c                 # Compila function.c in function.o

clean:                     # Comando 'make clean' per ripulire il progetto
	rm -f $(OBJS) $(TARGET)  # Rimuove tutti i file oggetto e l'eseguibile