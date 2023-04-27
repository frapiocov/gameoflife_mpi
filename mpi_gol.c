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

#define TAG_NEXT 99
#define TAG_PREV 98

/* dimensioni di default della matrice se non specificate */
#define DEF_ROWS 500
#define DEF_COLS 500

/* funzioni di utility */

/* mostra la matrice di input su stdout */
void print_matrix(int gen, char **mat, int rows, int cols)
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

/* carica la matrice seed da file */
void load_from_file(char *matrix, int rows, int cols, char *file) {
    char c; /* carattere letto */
    FILE *fptr;
    fptr = fopen(file, "r");
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            fscanf(fptr, "%c ", &c);
            matrix[i * col_size + j] = c;
        }
    }
    fclose(fptr);
}

/* setta il valore di righe e colonne in base al file pattern caricato */
void check_matrix_size(char *filename, int row_size, int col_size) {
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

    row_size = rows;
    col_size = lines / rows;
    printf("Matrix dimension: %d x %d\n", row_size, col_size);

    fclose(file);
}

/* cambia il valore di una cella da morta a viva o viceversa */
void change_state(int *cell)
{
    if (cell == ALIVE)
        *cell = DEAD;
    else
        *cell = ALIVE;
}

/* funzione che decide lo stato della cella per la generazione successiva */
void update_state(int *send_mat, int *recv_mat, int rows, int cols, int index, int alive_count)
{
    int neighbour_live_cell, i, j;
    /* ciclo cella per cella */
    for (i = 0; i < rows; i++)
    {
        for (j = 0; j < cols; j++)
        {
            /* per ogni cella calcola i suoi vicini vivi */
            neighbour_live_cell = count_live_neighbour_cell(send_mat, i, j, rows, cols);

            /*
             *  controlla le 3 casistiche:
             *  una cellula viva con 2 o 3 vicini sopravvive per la prossima generazione
             *  se è morta e ha 3 vicini vivi torna in vita nella prossima generazione
             *  altrimenti muore
             */

            if (send_mat[i][j] == ALIVE && (neighbour_live_cell == 2 || neighbour_live_cell == 3))
            {
                recv_mat[i][j] = ALIVE;
            }
            else if (send_mat[i][j] == DEAD && neighbour_live_cell == 3)
            {
                recv_mat[i][j] = ALIVE;
            }
            else
            {
                recv_mat[i][j] = DEAD;
            }
        }
    }
    return;
}

/* funzione main */
int main(int argc, char **argv)
{
    int rank,       /* rank processo */
        num_proc,   /* size communicator */
        rows,       /* righe matrice */
        cols,       /* colonne matrice */
        iterations, /* numero di generazioni */
        prev,       /* rank processo precedente al corrente */
        next;       /* rank processo successivo al corrente */
    double start,
        end;
    int *rows_for_proc, /* memorizza il numero di righe da assegnare ad ogni processo */
        *displacement;  /* memorizza il displacement per ogni processo */
    char *recv_buff,    /* buffer per la ricezione */
        *send_matrix,     /* buffer per l'invio della sotto matrice */
        *matrix;        /* matrice di partenza */

    char *result_buff,  /* buffer usato per memorizzare i risultati del gioco su un processore */
        *recv_scatt_buff, /* per lo scatter dal nodo master */
        *prev_row, /* riga precedente a quelle del processo corrente */
        *next_row; /* riga successiva a quelle del processo corrente */

    MPI_Request send_request = MPI_REQUEST_NULL; /* Request per l'invio di dati */
    MPI_Request prev_request = MPI_REQUEST_NULL; /* Request per la ricezione dal processo precedente */
    MPI_Request next_request = MPI_REQUEST_NULL; /* Request per la ricezione dal processo successivo */
    MPI_Status status;                           /* lo stato di un'operazione di recv */
    MPI_Datatype mat_row;                        /* datatype per la riga della matrice */

    /* inizializzazione ambiente MPI */
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_proc);

    /* se viene specificato solo il numero di iterazioni */
    if (argc == 2) {
        iterations = atoi(argv[1]);
        /* la grandezza della matrice è quella di default */
        rows = DEF_ROWS;
        cols = DEF_COLS;
    }
    else if (argc == 3) { /* l'utente ha indicato un pattern da file */
        iterations = atoi(argv[1]);
        /* lettura da file */
        //rows = atoi(argv[2]);
        //cols = atoi(argv[3]);
    }
    else if (argc == 4) { /* le dimensioni sono scelte dall'utente */
        iterations = atoi(argv[1]);
        rows = atoi(argv[2]);
        cols = atoi(argv[3]);
    }
    else
    { /* altrimenti il numero degli argomenti non è corretto */
        printf("ERROR: Insert the correct number of argument: #iteration #rows #cols or #iteration only\n");
        MPI_Finalize();
        exit(1);
    }

    /* crea un nuovo tipo di dato MPI replicando MPI_CHAR cols volte in posizioni contigue */
    MPI_Type_contiguous(cols, MPI_CHAR, &mat_row);
    MPI_Type_commit(&mat_row);

    /*
     *  divisione della matrice per righe
     *  nel caso di divisione non equa, i primi resto processi
     *  riceveranno una riga in più
     */

    /* righe per processo */
    rows_for_proc = calloc(num_proc, sizeof(int));
    /* memorizza il displacement da applicare ad ogni processo */
    displacement = calloc(num_proc, sizeof(int));
    /* base di partenza divisione*/
    int base = rows / num_proc;
    /* righe in più da distribuire */
    int rest = rows % num_proc;
    /* righe assegnate */
    int assigned = 0;

    /* calcolo delle righe in più per processo */
    for (int i = 0; i < num_proc; i++)
    {
        displacement[i] = assigned;
        /* nel caso di righe in più viene aggiunta al processo i */
        if (rest > 0)
        {
            rows_for_proc[i] = base + 1;
            rest--;
        }
        else /* divisione senza resto */
        {
            rows_for_proc[i] = base;
        }
        /* aggiorna il numero di righe già assegnate */
        assigned += rows_for_proc[i];
    }

    /* processo master */
    if (rank == MASTER)
    {
        printf("Settings:\n");
        printf("iterations:%d\trows:%d\tcolumns:%d\n", iterations, rows, cols);
    
        start = MPI_Wtime();
        /* alloca la matrice */
        matrix = calloc(rows *cols, sizeof(char));
    }

    /* ogni processo inizializza la sua porzione di righe */
    send_matrix = calloc(rows_for_proc[rank] * cols, sizeof(char));
    srand(time(null) + rank);

    /* ogni cella viene inizializzata in modo casuale */
    for(int i = 0; i < rows_for_proc[rank] * cols; i++)
        if (rand() % 2 == 0)
            result_buff[i] = ALIVE;
        else
            result_buff[i] = DEAD;

    /* raccoglie i dati da tutti i processi del communicator e li concatena nel buffer del processo master */
    /* A differenza di MPI_Gather, tuttavia, MPI_Gatherv consente ai messaggi ricevuti di avere lunghezze diverse e di essere memorizzati in posizioni arbitrarie nel buffer del processo principale. */
    MPI_Gatherv(send_matrix, rows_for_proc[rank], mat_row, matrix, rows_for_proc, displacement, mat_row, MASTER, MPI_COMM_WORLD);

    /* calcolo rank del processo successivo e precedente al corrente */
    prev = (rank - 1 + num_proc) % num_proc;
    next = (rank + 1) % num_proc;

    /*  */
    recv_scatt_buff = calloc(rows_for_proc[rank] * cols, sizeof(char));
    /*  */
    result_buff = calloc(rows_for_proc[rank] * cols, sizeof(char));
    
    prev_row = calloc(cols, sizeof(char));
    next_row = calloc(cols, sizeof(char));

    /* il gioco viene eseguito iterations volte */
    for(int it = 0; it < iterations; it++) {

        /* la matrice divisa per righe viene inviata ai sotto processi */
        MPI_Scatterv(matrix, rows_for_proc, displacement, mat_row, recv_scatt_buff, rows_for_proc[rank], mat_row, MASTER, MPI_COMM_WORLD);
        
        /* numero di righe del processo rank */
        int sub_rows = rows_for_proc[rank];

        /* rank invia la sua prima riga al processo precedente */
        MPI_Isend(recv_scatt_buff, 1, mat_row, prev, TAG_NEXT, MPI_COMM_WORLD, &send_request);
        MPI_Request_free(&send_request);

        /* rank riceve la riga precedente alle sue dal suo predecessore */
        MPI_Irecv(prev_row, 1, mat_row, prev, TAG_PREV, MPI_COMM_WORLD, &prev_request);

        /* rank invia la sua ultima riga al suo successore */
        MPI_Isend(recv_scatt_buff + (cols * (sub_rows - 1)), 1, mat_row, next, TAG_PREV, MPI_COMM_WORLD, &send_request);
        MPI_Request_free(&send_request);

        /* rank riceve la riga successiva alle sue dal suo predecessore */
        MPI_Irecv(next_row, 1, mat_row, next, TAG_NEXT, MPI_COMM_WORLD, &next_request);

        /* calcola i valori delle celle che non interferiscono con gli altri processi */
        for (int i = 1; i < sub_rows - 1; i++) {
            for (int j = 0; j < cols; j++) {
                int live_count = 0;

                int col_start_index = j - 1;
                int col_end_index = j + 2;

                for (int row = i - 1; row < i + 2; row++) {
                    for (int col = col_start_index; col < col_end_index; col++) {
                        if (row == i && col == j) {
                            continue;
                        }

                        if (recv_scatt_buff[row * cols + (col % cols)]) {
                            live_count++;
                        }       
                    }
                }
                decideFate(recv_scatt_buff, result_buff, i * cols + j, live_count);
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
            executeNext(receive_buff, next_row, result_buff, my_row_size, column_size);
            // wait for the prev_row and then perform logic
            waitAndExecutePrev(&prev_request, receive_buff, prev_row, result_buff, column_size);
        } 
        else { /* nel caso viene completata prima la prev_request */
            /* calcola i valori sulla riga precedente */
            executePrev(receive_buff, prev_row, result_buff, column_size);
            // wait for the next_row and then perform logic
            waitAndExecuteNext(&next_request, receive_buff, next_row, result_buff, my_row_size, column_size);
        }

        MPI_Gatherv(result_buff, send_count[rank], game_row, game_matrix, send_count, displacement, game_row, MASTER_NODE, MPI_COMM_WORLD);

    }

    /* libera la memoria dinamica allocata */
    free(recv_scatt_buff);
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