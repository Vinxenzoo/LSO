
# ==========================================
# Sezione: Compilatore e opzioni
# ==========================================
CC      := gcc                   # Imposta il compilatore da usare (gcc)
CFLAGS  := -Wall -Wextra -O3     # Opzioni di compilazione:
                                 # -Wall   = mostra tutti gli avvisi standard
                                 # -Wextra = mostra avvisi aggiuntivi
                                 # -O3     = ottimizzazione massima

# ==========================================
# Sezione: Nome dell'eseguibile
# ==========================================
TARGET  := server                # Nome del file eseguibile risultante

# ==========================================
# Sezione: Sorgenti, header e oggetti
# ==========================================
SOURCES := main.c server.c       # File sorgente .c del progetto
HEADERS := server.h datastructures.h  # Header richiesti per la compilazione
OBJECTS := $(SOURCES:.c=.o)      # Converte main.c server.c => main.o server.o

# ==========================================
# Target principale (default)
# ==========================================
all: $(TARGET)                   # Quando si esegue 'make' viene invocato questo target

# ==========================================
# Regola per creare l'eseguibile
# ==========================================
$(TARGET): $(OBJECTS)            # Per creare il server servono i file oggetto
	@echo "Linking..."              # Stampa 'Linking...' (con @ per non mostrare il comando stesso)
	$(CC) $(CFLAGS) -o $@ $^        # Compila e linka gli oggetti in un eseguibile:
                                 # $@ = nome del target (server)
                                 # $^ = tutti i prerequisiti (main.o server.o)

# ==========================================
# Regola generica per compilare qualsiasi .c in .o
# ==========================================
%.o: %.c $(HEADERS)              # Per ogni file .c, se esistono gli header
	@echo "Compiling $<..."         # Mostra quale file si sta compilando ($< = sorgente)
	$(CC) $(CFLAGS) -c $< -o $@     # Compila il sorgente in oggetto:
                                 # -c = solo compilazione, senza linking
                                 # $< = primo prerequisito (il .c)
                                 # -o $@ = output nel file oggetto corrispondente

# ==========================================
# Pulizia dei file generati
# ==========================================
clean:
	rm -f $(TARGET) $(OBJECTS)     # Elimina eseguibile e file oggetto

