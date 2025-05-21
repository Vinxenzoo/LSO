#!/bin/bash

# Chiedi all'utente il numero di client da avviare
echo "Number of clients to start:"
read n_client

# Verifica che l'input sia un numero intero maggiore o uguale a 1
if ! [[ "$n_client" =~ ^[0-9]+$ ]] || [ "$n_client" -lt 1 ]; then
    echo "Try again, enter a valid number of clients!"
    exit 1
fi

# Esegui docker compose nel terminale corrente (compatibile con WSL)
echo "[INFO] Launching Docker Compose for $n_client clients..."
docker compose up --build --scale client=$n_client
