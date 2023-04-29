## Game Of Life - MPI  🕹️🧬
Implementazione del modello matematico *Game of life* (John Conway - 1970) con linguaggio C e OpenMPI.

----
🎓 Università degli Studi di Salerno  
💻 Dipartimento di Informatica  
📚 Programmazione Parallela e Concorrente su Cloud 2022/2023

----
👤 Francesco Pio Covino

## 1. Soluzione proposta
Game of life è un modello matematico creato da John Conway nel 1970.
La seguente è un'implentazione in C con l'utilizzo di OpenMPI. La soluzione proposta si può sintetizzare in cinque parti:

- I processi inizializzano in parallelo una matrice *NxM*. Ogni processo inizializza un insieme di righe, di default saranno *N/numero_di_processi* ma in caso di divisione non equa le righe aggiuntive vengono ridistribuite. Le righe inizializzate vengono poi inviate al processo MASTER attraverso l'uso della routine `MPI_Gatherv`.

- La matrice inizializzata viene suddivisa in modo equo tra tutti i processi, MASTER compreso, attraverso la routine `MPI_Scatterv`.

- La comunicazione fra processi sarà ad anello o toroidale, il successore del processo *n-1* sarà *0* e di conseguenza il predecessore del processo *0* sarà *n-1*.

- L'intero processo sarà ripetuto per un numero di volte scelto dall'utente

- I processi, in modalità asincrona, eseguono le seguenti operazioni:
    - inviano la loro prima riga al loro predecessore e la loro ultima riga al loro successore
    - Si preparano a ricevere la riga di bordo superiore dal loro predecessore e la riga di bordo inferiore dal loro successore
    - Mentre attendono le righe di bordo, iniziano ad eseguire l'algoritmo di GoL nelle zone della matrice in cui gli altri processi non sono coinvolti
    - Ricevute le righe di bordo eseguono l'algoritmo anche con l'utilizzo di quest'ultime

- Le righe risultanti vengono inviate al MASTER attraverso l'uso della routine `MPI_Gatherv`.


### 1.1 Compilazione ed esecuzione
La versione sequenziale utilizza MPI solo affinché il calcolo dei tempi sia fatto nello stesso modo in entrambi gli algoritmi.
Per la compilazione eseguire il seguente comando da terminale:  
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
