# Compilatore e opzioni
CC      := gcc
CFLAGS  := -Wall -Wextra -O3

# Nome dell'eseguibile
TARGET  := server

# Lista dei file oggetto
SOURCES := main.c server.c
HEADERS := server.h datastructure.h
OBJECTS := $(SOURCES:.c=.o)

# Regola principale
all: $(TARGET)

# Regola per creare l'eseguibile
$(TARGET): $(OBJECTS)
	@echo "Linking..."
	$(CC) $(CFLAGS) -o $@ $^

# Regola implicita per compilare i .c in .o
%.o: %.c $(HEADERS)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Pulizia
clean:
	rm -f $(TARGET) $(OBJECTS)
