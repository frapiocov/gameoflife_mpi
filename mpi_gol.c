/*
 *   Game of Life versione parallela con OpenMPI
 *   Francesco Pio Covino
 *   PCPC 2022/2023
 *   Compile with: mpicc -o gol mpi_life.c -lm
 *   Execute with: mpirun -np 4 gol 5 1000 1000
 *
 *   Specificare il numero di iterazioni come primo argomento (obbligatorio)
 *   + Numero di righe e colonne rispettivamente come secondo e terzo argomento
 *   se non specificati verranno utilizzate le dimensioni di default
 */

#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include <math.h>
#include <time.h>

/* rank processo master */
#define MASTER 0

/* stati possibili per una cella */
#define ALIVE 'O'
#define DEAD '.'

#define TAG_NEXT 14
#define TAG_PREV 41

/* dimensioni di default della matrice se non specificate */
#define DEF_ROWS 500
#define DEF_COLS 500
#define DEF_ITERATION 10

/* funzioni di utility */

/* mostra la matrice di input su stdout */
void print_matrix(int gen, char *mat, int rows, int cols)
{
    /* Generazione a cui appartiene la matrice*/
    printf("\nGeneration %d:\n", gen);
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            printf("%c", mat[i * cols + j]);
        }
        printf("\n");
    }
}

/* carica la matrice di partenza da file */
void load_from_file(char *matrix, int rows, int cols, char *file) {
    char c; /* carattere letto */
    FILE *fptr;
    fptr = fopen(file, "r");
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            fscanf(fptr, "%c ", &c);
            matrix[i * cols + j] = c;
        }
    }
    fclose(fptr);
}

/* setta il valore di righe e colonne in base al file pattern caricato */
void check_matrix_size(char *filename, int *row_size, int *col_size) {
    int rows = 0, lines = 0;
    char c;

    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error.\n");
        return;
    }

    /* conteggia righe e colonne del file */
    while ((c = fgetc(file)) != EOF) {
        if (c == '\n') {
            rows++;
        }
        if (c == '.' || c == 'O') {
            lines++;
        }
    }
    /* caso speciale ultima riga */
    if (lines > 0) {
        rows++;
    }
    /* setta il valore di righe e colonne */
    *row_size = rows;
    *col_size = lines / rows;
    fclose(file);
}

/* funzione che decide lo stato della cella per la generazione successiva */
/*
 *  controlla le 3 casistiche:
 *  una cellula viva con 2 o 3 vicini sopravvive per la prossima generazione
 *  se è morta e ha 3 vicini vivi torna in vita nella prossima generazione
 *  altrimenti in tutti i restanti casi muore
 */
void life(char *origin, char *result, int index, int live_count)
{
    if (origin[index] == ALIVE && (live_count == 2 || live_count == 3)) {
        result[index] = ALIVE;        
    } else if (origin[index] == DEAD && live_count == 3) {
        result[index] = ALIVE; 
    } else {
        result[index] = DEAD; 
    }           
}

/**/
void compute_prev_row(MPI_Request* to_wait, char* origin, char* prev_row, char* result, int col_size) {
    MPI_Wait(to_wait, MPI_STATUS_IGNORE);

    for (int j = 0; j < col_size; j++) {
        int live_cell = 0;

        int col_start_index = j - 1;
        int col_end_index = j + 2;

        for (int row = -1; row < 2; row++)
            for (int col = col_start_index; col < col_end_index; col++) {
                /* nel caso dell'item corrente */
                if (row == 0 && col == j)
                    continue;

                /* nel caso della riga precedente */
                if (row == -1) {
                    if (isAlive(prev_row[col % column_size]))
                        live_cell++;
                }
                else {
                    if (isAlive(origin_buff[row * column_size + (col % column_size)]))
                        live_cell++;
                }
            }

        life(origin_buff, result_buff, j, live_cell);
    }
}


void waitAndExecuteNext(MPI_Request* to_wait, char* origin_buff, char* next_row, char* result_buff, int my_row_size, int column_size) {
    MPI_Wait(to_wait, MPI_STATUS_IGNORE);
  for (int j = 0; j < column_size; j++) {
            int live_count = 0;

            int col_start_index = j - 1;
            int col_end_index = j + 2;

            for (int row = my_row_size - 2; row < my_row_size + 1; row++)
                for (int col = col_start_index; col < col_end_index; col++) {
                    // If I'm on the current item
                    if (row == my_row_size - 1 && col == j)
                        continue;

                    // If I want to inspect the next row
                    if (row == my_row_size) { 
                        if (isAlive(next_row[col % column_size]))
                            live_count++;
                    }
                    else {
                        if (isAlive(origin_buff[row * column_size + (col % column_size)]))
                            live_count++;
                    }
                }

            decideFate(origin_buff, result_buff, (my_row_size - 1) * column_size + j, live_count);
        }
    executeNext(origin_buff, next_row, result_buff, my_row_size, column_size);
}


/* funzione main */
int main(int argc, char **argv)
{
    int rank,       /* rank processo */
        num_proc,   /* size communicator */
        row_size,       /* righe matrice */
        col_size,       /* colonne matrice */
        iterations, /* numero di generazioni */
        prev,       /* rank processo precedente al corrente */
        next;       /* rank processo successivo al corrente */
    double start,
        end;
    int *rows_for_proc, /* memorizza il numero di righe da assegnare ad ogni processo */
        *displacement;  /* memorizza il displacement per ogni processo */
    char *sub_matrix_proc,     /* buffer per l'invio della sotto matrice */
        *matrix;        /* matrice di partenza */

    char *result_buff,  /* buffer usato per memorizzare i risultati del gioco su un processore */
        *recv_buff, /* per lo scatter dal nodo master */
        *prev_row, /* riga precedente a quelle del processo corrente */
        *next_row; /* riga successiva a quelle del processo corrente */

    char *dir, *filename, *ext, *file; /* variabili per la lettura da file */

    MPI_Request send_request = MPI_REQUEST_NULL; /* Request per l'invio di dati */
    MPI_Request prev_request = MPI_REQUEST_NULL; /* Request per la ricezione dal processo precedente */
    MPI_Request next_request = MPI_REQUEST_NULL; /* Request per la ricezione dal processo successivo */
    MPI_Status status;                           /* lo stato di un'operazione di recv */
    MPI_Datatype mat_row;                        /* datatype per la riga della matrice */

    /* inizializzazione ambiente MPI */
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_proc);

    if (argc == 3) { /* l'utente ha indicato un pattern da file */
        dir = "patterns/";
        filename = argv[1];
        ext = ".txt";
        file = malloc(strlen(dir) + strlen(filename) + strlen(ext) + 1);
        sprintf(file, "%s%s%s", dir, filename, ext);
        
        printf("--Generate matrix seed from %s--\n", file);

        /* controlla le dimensioni della matrice */
        check_matrix_size(file);
        /* riempie la matrice di partenza da file */
        //load_from_file(matrix, file);
        /* numero di iterazioni da fare */
        iterations = atoi(argv[2]);
    }
    else if (argc == 4) { /* le dimensioni sono scelte dall'utente */
        row_size = atoi(argv[1]);
        col_size = atoi(argv[2]);
        iterations = atoi(argv[3]);
    }
    else { /* altrimenti il numero degli argomenti non è corretto */
        printf("ERROR: Insert the correct number of argument\n");
        MPI_Finalize();
        exit(1);
    }

    /* crea un nuovo tipo di dato MPI replicando MPI_CHAR col_size volte in posizioni contigue */
    MPI_Type_contiguous(col_size, MPI_CHAR, &mat_row);
    MPI_Type_commit(&mat_row);

    /*
     *  divisione della matrice per righe
     *  nel caso di divisione non equa, i primi resto processi
     *  riceveranno una riga in più
     */

    /* righe per processo */
    rows_for_proc = calloc(num_proc, sizeof(int));
    /* displacement da applicare ad ogni processo */
    displacement = calloc(num_proc, sizeof(int));
    /* base di partenza divisione */
    int base = row_size / num_proc;
    /* righe in più da distribuire */
    int rest = row_size % num_proc;
    /* righe già assegnate */
    int assigned = 0;

    /* calcolo delle righe in più per processo */
    for (int i = 0; i < num_proc; i++) {
        displacement[i] = assigned;
        /* nel caso di righe in più viene aggiunta al processo i */
        if (rest > 0) {
            rows_for_proc[i] = base + 1;
            rest--;
        } 
        else {
            rows_for_proc[i] = base;
        }
        /* aggiorna il numero di righe già assegnate */
        assigned += rows_for_proc[i];
    }

    /* processo master */
    if (rank == MASTER) {
        printf("Settings: iterations %d\trows %d\tcolumns %d\n", iterations, row_size, col_size);
    
        start = MPI_Wtime();
        /* alloca la matrice di partenza */
        matrix = calloc(row_size *col_size, sizeof(char));
    }

    /* ogni processo alloca la sua porzione di matrice */
    sub_matrix_proc = calloc(rows_for_proc[rank] * col_size, sizeof(char));
    
    /* e la inizializza con valori casuali */
    srand(time(NULL) + rank);
    for(int i = 0; i < rows_for_proc[rank] * col_size; i++) {
        if (rand() % 2 == 0) {
            sub_matrix_proc[i] = ALIVE; 
        } else {
            sub_matrix_proc[i] = DEAD;
        }
    }
    /*
    sendbuf
        indirizzo iniziale del buffer di invio
    sendcount
        numero di elementi nel buffer di invio (intero)
    sendtype
        tipo di dati degli elementi del buffer di invio (handle)
    recvcounts
        array intero contenente il numero di elementi ricevuti da ogni processo
    displs
        array intero. La voce i specifica lo spostamento rispetto a recvbuf in cui collocare i dati in arrivo dal processo i
    recvtype
        tipo di dati degli elementi del buffer recv(handle)
    root
        rank del processo ricevente (intero)
    comm
        comunicatore (handle)
    */    
    /* raccoglie i dati da tutti i processi del communicator e li concatena nel buffer del processo master */
    /* A differenza di MPI_Gather, tuttavia, MPI_Gatherv consente ai messaggi ricevuti di avere lunghezze diverse e di essere memorizzati in posizioni arbitrarie nel buffer del processo principale. */
    MPI_Gatherv(sub_matrix_proc, rows_for_proc[rank], mat_row, matrix, rows_for_proc, displacement, mat_row, MASTER, MPI_COMM_WORLD);

    /* calcolo rank del processo successivo e precedente al corrente */
    prev = (rank - 1 + num_proc) % num_proc;
    next = (rank + 1) % num_proc;

    /* alloca i buffer per le operazioni di scatter */
    recv_buff = calloc(rows_for_proc[rank] * col_size, sizeof(char));
    result_buff = calloc(rows_for_proc[rank] * col_size, sizeof(char));
    /* alloca la riga precedente e successiva alla sottomatrice del processo rank */
    prev_row = calloc(col_size, sizeof(char));
    next_row = calloc(col_size, sizeof(char));

    /* il processo viene eseguito iterations volte */
    for(int it = 0; it < iterations; it++) {

        /* la matrice divisa per righe viene inviata ai sotto processi */
        /*
        sendbuf
            indirizzo send buffer
        sendcounts
            array di interi (size righe) specifica il numero di elementi da inviare ad ogni processo
        displs
            array di interi. La voce i specifica lo spostamento (rispetto a sendbuf) da cui prendere i dati in uscita al processo i
        sendtype
            data type degli elementi del send buffer (handle)
        recvcount
            numero degli elements in receive buffer (integer)
        recvtype
            data type elementi receive buffer (handle)
        root
            rank del processo che invia (integer)
        comm
            communicator (handle)
        */
        MPI_Scatterv(matrix, rows_for_proc, displacement, mat_row, recv_buff, rows_for_proc[rank], mat_row, MASTER, MPI_COMM_WORLD);
        
        /* numero di righe del processo rank */
        int sub_rows_size = rows_for_proc[rank];

        /* rank invia la sua prima riga al processo precedente */
        MPI_Isend(recv_buff, 1, mat_row, prev, TAG_NEXT, MPI_COMM_WORLD, &send_request);
        MPI_Request_free(&send_request);

        /* rank riceve la riga precedente alle sue dal suo predecessore */
        MPI_Irecv(prev_row, 1, mat_row, prev, TAG_PREV, MPI_COMM_WORLD, &prev_request);

        /* rank invia la sua ultima riga al suo successore */
        MPI_Isend(recv_buff + (col_size * (sub_rows_size - 1)), 1, mat_row, next, TAG_PREV, MPI_COMM_WORLD, &send_request);
        MPI_Request_free(&send_request);

        /* rank riceve la riga successiva alle sue dal suo predecessore */
        MPI_Irecv(next_row, 1, mat_row, next, TAG_NEXT, MPI_COMM_WORLD, &next_request);

        /* calcola i valori delle celle che non interferiscono con gli altri processi */
        for (int i = 1; i < sub_rows_size - 1; i++) {
            for (int j = 0; j < col_size; j++) {
                int live_count = 0;

                int col_start_index = j - 1;
                int col_end_index = j + 2;

                for (int row = i - 1; row < i + 2; row++) {
                    for (int col = col_start_index; col < col_end_index; col++) {
                        if (row == i && col == j) {
                            continue;
                        }

                        if (recv_buff[row * col_size + (col % col_size)]) {
                            live_count++;
                        }       
                    }
                }
                life(recv_buff, result_buff, i * col_size + j, live_count);
            }
        }

        /* attende il completamento delle comunicazioni */
        MPI_Request to_wait[] = {prev_request, next_request};
        int handle_index;
        MPI_Waitany(
            2, */* numero di richieste */
            to_wait, /* array di request da attendere */
            &handle_index, // Index of handle for operation that completed
            &status
        );

        /* nel caso la next_request viene completata prima */
        if(status.MPI_TAG == TAG_NEXT) {
            /* calcola i valori sulla riga successiva */
            executeNext(recv_buff, next_row, result_buff, sub_rows_size, col_size);
            // wait for the prev_row and then perform logic
            waitAndExecutePrev(&prev_request, recv_buff, prev_row, result_buff, col_size);
        } 
        else { /* nel caso viene completata prima la prev_request */
            /* calcola i valori sulla riga precedente */
            executePrev(recv_buff, prev_row, result_buff, col_size);
            /* attende la riga successiva ed esegue l'algoritmo */
            waitAndExecuteNext(&next_request, recv_buff, next_row, result_buff, sub_rows_size, col_size);
        }

        MPI_Gatherv(result_buff, rows_for_proc[rank], mat_row, matrix, rows_for_proc, displacement, mat_row, MASTER, MPI_COMM_WORLD);

    }

    /* libera la memoria dinamica allocata */
    free(recv_buff);
    free(result_buff);
    free(next_row);
    free(prev_row);

    /* sincronizza tutti i processi */
    MPI_Barrier(MPI_COMM_WORLD);

    /* il processo master mostra il tempo di esecuzione */
    if(rank == MASTER) {
        free(matrix);
        end = MPI_Wtime();
        printf("Execution Time: %f ms\n", end - start);
    }

    MPI_Finalize();
    return 0;
}