#!/bin/bash

echo "Number of clients to start:"
read n_client

if ! [[ "$n_client" =~ ^[0-9]+$ ]] || [ "$n_client" -lt 1 ]; then
    echo "Try again, enter a valid number of clients!"
    exit 1
fi

if [[ "$OSTYPE" == "darwin"* ]]; then
    docker-compose up --build --scale client=$n_client

elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    if command -v gnome-terminal >/dev/null 2>&1; then
        gnome-terminal -- bash -c "docker-compose up --build --scale client=$n_client; exec bash"

    elif command -v konsole >/dev/null 2>&1; then
        konsole --hold -e "docker-compose up --build --scale client=$n_client"

    elif command -v xfce4-terminal >/dev/null 2>&1; then
        xfce4-terminal --hold -e "docker-compose up --build --scale client=$n_client"
        
    elif command -v xterm >/dev/null 2>&1; then
        xterm -hold -e "docker-compose up --build --scale client=$n_client"
    fi
fi
