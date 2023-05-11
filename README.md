## Game Of Life - MPI  🕹️🧬
Implementazione del modello matematico *Game of life* (John Conway - 1970) con linguaggio C e OpenMPI.

----
🎓 Università degli Studi di Salerno  
💻 Dipartimento di Informatica  
📚 Programmazione Parallela e Concorrente su Cloud 2022/2023

----
👤 Francesco Pio Covino

## Compilazione ed esecuzione 
Sono presenti due versioni dell'algoritmo, una versione sequenziale ed una parallela, entrambe utilizzano OpenMPI. La versione sequenziale utilizza OpenMPI solo per far in modo che la misurazione dei tempi sia svolta nel medesimo modo.

<details>
  <summary><b>GoL versione sequenziale</b></summary>

Per la compilazione eseguire il seguente comando da terminale:  

```c
mpicc -o gol sequential_gol.c
```
Per l'esecuzione esistono due varianti:

La prima variante permette di inizializzare la matrice seed con un pattern memorizzato su file. Il file pattern va inserito in formato plain text (.txt) nella directory *patterns/*. Una cella della matrice indicata come viva conterrà il simbolo *O* mentre se indicata come morta conterrà il simbolo *"."*; I seguenti sono due esempi di pattern:

Pulsar pattern             |  Glidergun pattern
:-------------------------:|:-------------------------:
![pulsar](images/pulsar.png)  |  ![glidergun](images/glidergun.png) 

Oltre al nome del file va specificato il numero di iterazioni da eseguire:
```c
mpirun -n 1 gol filename it
// per esempio
mpirun -n 1 gol pulsar 10
```  
La seconda variante permette di eseguire l'algoritmo specificando 3 argomenti:
- numero di righe
- numero di colonne
- numero di iterazioni  

N.B. La matrice di partenza verrà generata in maniera casuale.
```c
mpirun -n 1 gol rows cols it
// per esempio
mpirun -n 1 gol 100 200 8
```  
  
</details>

<details>
    <summary><b>GoL versione parallela</b></summary>

Per la compilazione eseguire il seguente comando da terminale:  

```
mpicc -o execname programname.c
// per esempio
mpicc -o mpigol mpi_gol.c
```  
Per l'esecuzione esistono 3 varianti:

La prima variante permette di inizializzare la matrice seed con un pattern memorizzato su file. Il file pattern va inserito in formato plain text (.txt) nella directory *patterns/*. Una cella della matrice indicata come viva conterrà il simbolo *O* mentre se indicata come morta conterrà il simbolo *"."*; I seguenti sono due esempi di pattern:

Pulsar pattern             |  Glidergun pattern
:-------------------------:|:-------------------------:
![pulsar](images/pulsar.png)  |  ![glidergun](images/glidergun.png) 

Oltre al nome del file va specificato il numero di iterazioni da eseguire e il numero di processi da utilizzare:
```
mpirun -n N execname filename it
// per esempio
mpirun -n 5 mpigol pulsar 10
```  
La seconda variante permette di eseguire l'algoritmo specificando 3 argomenti:
- numero di righe
- numero di colonne
- numero di iterazioni  

N.B. La matrice di partenza verrà generata in maniera casuale.
```
mpirun -n N execname rows cols it
// per esempio
mpirun -5  mpigol 100 200 8
```  
La terza variante non richiede alcun argomento aggiuntivo ed eseguirà l'algoritmo utilizzando i parametri di default (una matrice 240x160 su 6 iterazioni), riempendo la matrice con valori casuali.
```
mpirun -n N execname
// per esempio
mpirun -5  mpigol
```  


</details>

## Soluzione proposta
Game of life è un modello matematico creato da John Conway nel 1970.
La seguente è un'implentazione in C con l'utilizzo di OpenMPI. La soluzione proposta si può sintetizzare nei seguenti punti:

N.B. Nella spiegazione verrà preso in esame il caso in cui l'utente ha scelto la dimensione della matrice (NxM) e il numero di iterazioni.

- I processi inizializzano in parallelo una matrice *NxM*. Ogni processo inizializza un insieme di righe, di default saranno *N/numero_di_processi* ma in caso di divisione non equa le righe in più vengono ridistribuite. Le righe inizializzate vengono poi inviate al processo MASTER attraverso l'uso della routine `MPI_Gatherv`.

- La matrice inizializzata viene suddivisa in modo equo tra tutti i processi, MASTER compreso, attraverso la routine `MPI_Scatterv`.

- La comunicazione fra processi sarà ad anello o toroidale, il successore del processo con rank *n-1* avrà rank *0* e di conseguenza il predecessore del processo con rank *0* avrà rank *n-1*.

- I processi, in modalità asincrona, eseguono le seguenti operazioni:
    - calcolano il rank del proprio successore e predecessore nell'anello.
    - inviano (`MPI_Isend`) la loro prima riga al loro predecessore e la loro ultima riga al loro successore.
    - Si preparano a ricevere (`MPI_Irecv`) la riga di bordo superiore dal loro predecessore e la riga di bordo inferiore dal loro successore.
    - Mentre attendono le righe di bordo, iniziano ad eseguire l'algoritmo di GoL nelle zone della matrice in cui gli altri processi non sono coinvolti.
    - Ricevute le righe di bordo, i processi eseguono l'algoritmo anche sulle celle che necessitano dell'utilizzo di quest'ultime.

- Le righe risultanti vengono inviate al MASTER attraverso l'uso della routine `MPI_Gatherv`.

## Struttura progetto
sintesi codice sottolineando gli aspetti cruciali

## Analisi performance
in termini di scalabilità forte e debole

## Conclusioni
motivazioni performance ottenute
