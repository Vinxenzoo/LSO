# Tris – Gioco Multiplayer Client/Server

Questo progetto implementa un gioco multiplayer di Tris sviluppato in linguaggio C,
strutturato secondo l'architettura client/server e gestito tramite container Docker.

## Requisiti

- [Docker](https://www.docker.com/)
- [Docker Compose](https://docs.docker.com/compose/)
- Terminale compatibile con bash (es. Linux, macOS o WSL su Windows)

## Struttura del progetto

- `server/` — Codice sorgente del server
- `client/` — Codice sorgente del client
- `docker-compose.yml` — File di orchestrazione dei container
- `scriptunix.sh` — Script per avviare il server e un numero arbitrario di client su sistemi Unix-like
- `scriptwindows.ps1` — Script PowerShell per avviare il server e un numero arbitrario di client su ambienti Windows

## Come avviare il progetto

### Su Linux/macOS

1. Assicurarsi di aver installato sul proprio computer docker

2. Apri il terminale e posizionati nella cartella principale del progetto attraverso il comando  "cd path/to/projectname"

3. Dare i permessi di esecuzione allo script attraverso il comando "chmod +x ./scriptname"

4. Eseguire quindi lo script attraverso il comando "./scriptname"

5. Scegliere quanti client vogliamo lanciare

6. Aprire tanti nuovi terminali quanti sono i client lanciati e all'interno di ogni terminale 
   lanciare il comando "docker attach tris-lso-client-i" dove i va sostituito con il numero di quel preciso client 

### Su Windows 

1. Assicurarsi di aver installato sul proprio computer docker

2. Apri la powershell e posizionati nella cartella principale del progetto attraverso il comando  "cd path/to/projectname"

3. Dare i permessi di esecuzione allo script attraverso il comando "Set-ExecutionPolicy -ExecutionPolicy RemoteSigned" 
   lanciando la powershell da admin

4. Eseguire quindi lo script e scegliere quanti client vogliamo lanciare

4. Eseguire quindi lo script attraverso il comando "./scriptname"

5. Scegliere quanti client vogliamo lanciare

6. Aprire tanti nuovi terminali quanti sono i client lanciati e all'interno di ogni terminale 
   lanciare il comando "docker attach tris-lso-client-i" dove i va sostituito con il numero di quel preciso client 

## Problemi avviamento tramite script

Nel caso in cui si dovessero riscontrare delle problematiche con i file .sh si puo comunque avviare il progetto in maniera 'manuale' andando 
a scrivere sul terminale il seguente comando "docker-compose up --scale client=n" che avvierà  il server e un numero di client scelti 
dall'utente sostituendo alla n il numero desiderato.