## Game Of Life - MPI  🕹️🧬
Implementazione del modello matematico *Game of life* (John Conway - 1970) con linguaggio C e OpenMPI.

----
🎓 Università degli Studi di Salerno  
💻 Dipartimento di Informatica  
📚 Programmazione Parallela e Concorrente su Cloud 2022/2023

----
👤 Francesco Pio Covino

## 1. Soluzione proposta
descrizione della soluzione

### 1.1 Esecuzione versione sequenziale
La versione sequenziale utilizza MPI solo affinché il calcolo dei tempi sia fatto nello stesso modo in entrambi gli algoritmi.
Per la compilazione del file, eseguire il seguente comando da terminale:  
```
mpicc -o gol sequential_gol.c
```  
Per l'esecuzione esistono due varianti:

La prima variante permette di inizializzare la matrice seed con un pattern memorizzato su file. Il file pattern va inserito in formato plain text (.txt) nella directory *patterns/*. Una cella della matrice indicata come viva conterrà il simbolo *O* mentre se indicata come morta conterrà il simbolo *"."*; I seguenti sono due esempi di pattern:

Pulsar pattern             |  Glidergun pattern
:-------------------------:|:-------------------------:
![pulsar](images/pulsar.png)  |  ![glidergun](images/glidergun.png) 

Oltre al nome del file va specificato il numero di iterazioni da eseguire.
```
mpirun -n 1 gol filename it
// per esempio
mpirun -n 1 gol pulsar 10
```  
La seconda variante permette di eseguire l'algoritmo specificando 3 argomenti:
- numero di righe
- numero di colonne
- numero di iterazioni  

La matrice di partenza verrà generata in maniera casuale.
```
mpirun -n 1 gol rows cols it
// per esempio
mpirun -n 1 gol 100 200 8
```  

## 2. Struttura progetto
sintesi codice sottolineando gli aspetti cruciali

## 3. Analisi performance
in termini di scalabilità forte e debole

## 4. Conclusioni
motivazioni performance ottenute
