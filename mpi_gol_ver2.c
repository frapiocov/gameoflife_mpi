/*
 * Game of Life, versione parallela con OpenMPI
 * Francesco Pio Covino
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <mpi.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

/* rank processo master */
#define MASTER 0

/* stati possibili per una cella */
#define ALIVE 'O'
#define DEAD '.'

/* tag per identificare invio e ricezione */
#define TAG_NEXT 14
#define TAG_PREV 41

/* dimensioni di default della matrice, se non specificate */
#define DEF_ROWS 240
#define DEF_COLS 360
#define DEF_ITERATION 10

/* 
* @brief Mostra una matrice su stdout 
* 
* @param gen numero di generazione mostrata
* @param matrice da mostrare
* @param rows numero di righe della matrice
* @param cols numero di colonne della matrice
*/
void print_matrix(int gen, char *mat, int rows, int cols)
{
    printf("\nGeneration %d:\n", gen);
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            printf("%c", mat[i * cols + j]);
        }
        printf("\n");
    }
}

/* 
* @brief Inizializza una matrice da file
* 
* @param mat matrice da riempire
* @param rows numero di righe della matrice
* @param cols numero di colonne della matrice
* @param file file da cui prendere i dati
*/
void init_from_file(char *mat, int rows, int cols, char *file) {
    /* carattere letto */
    char c; 
    FILE *fptr;
    fptr = fopen(file, "r");
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            fscanf(fptr, "%c ", &c);
            mat[i * cols + j] = c;
        }
    }
    fclose(fptr);
}

void init_test_matrix(char *mat, int rows, int cols) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            mat[i * cols + j] = DEAD;
        }
    }
}

/* 
* @brief Setta il valore di righe e colonne in base al file pattern caricato 
* 
* @param filename path del file scelto
* @param row_size indirizzo variabile in cui memorizzare il numero di righe
* @param col_size indirizzo var in cui memorizzare il numero di colonne
*/
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
        if (c == '\n') { rows++; }
        if (c == '.' || c == 'O') { lines++; }
    }
    /* caso speciale ultima riga */
    if (lines > 0) { rows++; }
    /* setta il valore di righe e colonne */
    *row_size = rows;
    *col_size = lines / rows;
    fclose(file);
}

/*
 * @brief Decide lo stato della cella per la generazione successiva
 *  
 * Controlla le 3 casistiche:
 * una cellula viva con 2 o 3 vicini sopravvive per la prossima generazione
 * se è morta e ha 3 vicini vivi torna in vita nella prossima generazione
 * altrimenti in tutti i restanti casi muore.
 * 
 * @param origin buffer con valori pre-computazione
 * @param result buffer in cui memorizzare i valori post computazione
 * @param index indice della cella target
 * @param live_count vicini vivi nell'intorno della cella target
 */
void life(char *origin, char *result, int index, int live_count) {
    if (origin[index] == ALIVE && (live_count == 2 || live_count == 3)) {
        result[index] = ALIVE;        
    } else if (origin[index] == DEAD && live_count == 3) {
        result[index] = ALIVE; 
    } else {
        result[index] = DEAD; 
    }           
}

/*
* @brief Esegue la computazione utilizzando le celle della riga precedente a quelle date
* 
* Viene calcolato prima il numero di vicini vivi nell'intorno della cella target
* e successivamente deciso lo stato della cella per la generazione successiva.
* Le operazioni sono eseguite per ogni cella.
*
* @param origin_buff buffer da cui prendere i dati
* @param result_buffer buffer su cui memorizzare i risultati
* @param row_size numero di righe della matrice
* @param col_size numero di colonne della matrice
*/
void compute(char* origin_buff, char* result_buffer, int row_size,  int col_size) {
    for (int i = 1; i < row_size - 1; i++) {
            for (int j = 0; j < col_size; j++) {

                /* memorizza i vicini vivi nell'intorno della cella target (i,j) */
                int live_count = 0;
                for (int row = i - 1; row < i + 2; row++) {
                    for (int col = j - 1; col < j + 2; col++) {
                        if (row == i && col == j) {
                            continue;
                        }
                        if (origin_buff[row * col_size + (col % col_size)] == ALIVE) {
                            live_count++;
                        }       
                    }
                }
                
                /* decide lo stato della cella per la generazione successiva */
                life(origin_buff, result_buffer, i * col_size + j, live_count);
            }
        }
}

/*
* @brief Esegue la computazione utilizzando le celle della riga precedente a quelle date
* 
* Viene calcolato prima il numero di vicini vivi nell'intorno della cella target
* e successivamente deciso lo stato della cella per la generazione successiva.
* Le operazioni sono eseguite per ogni cella.
*
* @param origin_buff buffer da cui prendere i dati
* @param result_buffer buffer su cui memorizzare i risultati
* @param prev_row riga precedente a quelle processo
* @param col_size numero di colonne della matrice
*/
void compute_prev(char* origin_buff, char* result_buffer, char* prev_row,  int col_size) {
    for (int j = 0; j < col_size; j++) {
        /* memorizza i vicini vivi nell'intorno della cella */
        
        int live_count = 0;
        /* basta controllare la riga precedente e le prime due assegnate al processo corrente */
        for (int row = -1; row < 2; row++) {
            for (int col = j - 1; col < j + 2; col++) {
                
                /* se sto analizzando la cella target salto un giro */
                if (row == 0 && col == j)
                    continue;
                
                /* controlla la riga precedente */
                if (row == -1) {
                    if (prev_row[col % col_size] == ALIVE)
                        live_count++;
                } else { /* altrimenti controlla nella sotto-matrice */
                    if (origin_buff[row * col_size + (col % col_size)] == ALIVE)
                        live_count++;
                }
            }
        }

        /* decide lo stato della cella nella posizione indicata */
        life(origin_buff, result_buffer, j, live_count);
    }
}

/*
* @brief Esegue la computazione utilizzando le celle della riga successiva a quelle date
* 
* Viene calcolato prima il numero di vicini vivi nell'intorno della cella target
* e successivamente deciso lo stato della cella per la generazione successiva.
* Le operazioni sono eseguite per ogni cella.
*
* @param origin_buff buffer da cui prendere i dati
* @param process_buffer buffer in cui memorizzare i risultati della computazione
* @param next_row riga successiva a quelle date
* @param row_size numero di righe della matrice
* @param col_size numero di colonne della matrice
*/
void compute_next(char* origin_buff, char* result_buffer, char* next_row, int row_size, int col_size) {
    for (int j = 0; j < col_size; j++) {
        
        /* memorizza i vicini vivi nell'intorno della cella */
        int live_count = 0;
        for (int row = row_size - 2; row < row_size + 1; row++) {
            for (int col = j - 1; col < j + 2; col++) {
                
                /* se sto analizzando la cella corrente continuo */
                if (row == row_size - 1 && col == j)
                    continue;
                
                /* controlla la riga successiva o la matrice in base alla cella in esame */
                if (row == row_size) {
                    if (next_row[col % col_size] == ALIVE)
                        live_count++;
                } else {
                    if (origin_buff[row * col_size + (col % col_size)] == ALIVE)
                        live_count++;
                }
            }
        }
        /* decide lo stato della cella nella posizione indicata */    
        life(origin_buff, result_buffer, (row_size - 1) * col_size + j, live_count);
    }
}

int main(int argc, char **argv)
{
    int rank,       /* rank processo corrente */
        num_proc,   /* size communicator */
        row_size,       /* righe matrice */
        col_size,       /* colonne matrice */
        generations, /* numero di generazioni */
        prev_rank,       /* rank del processo precedente al corrente */
        next_rank;       /* rank del processo successivo al corrente */
    
    double start_time, end_time; /* per la misurazione dei tempi */
    
    int *rows_for_proc, /* memorizza il numero di righe assegnate ad ogni processo */
        *displ_for_proc;  /* memorizza il displacement per ogni processo */
    
    char *game_matrix;        /* matrice di gioco */
    
    char *process_buffer,  /* buffer usato dal singolo processore per memorizzare le righe della propria computazione */
        *result_buffer, /* buffer usato dal singolo processore per memorizzare il risultato della propria computazione */
        *prev_row, /* riga precedente alle proprie */
        *next_row; /* riga successiva alle proprie */
    
    char *dir, *filename, *ext, *file; /* variabili per la lettura da file */
    bool is_file = false, is_test = false; /* indica che la matrice è stata riempita da file */

    MPI_Request send_request = MPI_REQUEST_NULL; /* Request per l'invio di dati fra i processori */
    MPI_Request prev_request = MPI_REQUEST_NULL; /* Request per la ricezione dal processo precedente */
    MPI_Request next_request = MPI_REQUEST_NULL; /* Request per la ricezione dal processo successivo */
    MPI_Status request_status;  /* lo stato di un'operazione di invio */
    MPI_Datatype row_data;    /* datatype che indica una riga della matrice */

    /* inizializzazione ambiente MPI */
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &num_proc);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    switch (argc) {  
    case 3: /* l'utente ha indicato un pattern da file */
        is_file = true;
        if (rank == MASTER) {
            /* preparazione file */
            dir = "patterns/";
            filename = argv[1];
            ext = ".txt";
            file = malloc(strlen(dir) + strlen(filename) + strlen(ext) + 1);
            sprintf(file, "%s%s%s", dir, filename, ext);
            printf("--Generate game matrix seed from %s--\n", file);
            /* il processo master calcola le dimensioni della matrice in base al file */
            check_matrix_size(file, &row_size, &col_size);
        }
        /* MASTER invia la size della matrice a tutti i processi */
        MPI_Bcast(&row_size, 1, MPI_INT, MASTER, MPI_COMM_WORLD);
        MPI_Bcast(&col_size, 1, MPI_INT, MASTER, MPI_COMM_WORLD);
        generations = atoi(argv[2]);
        break;    
    case 4: /* le dimensioni sono scelte dall'utente */
        row_size = atoi(argv[1]);
        col_size = atoi(argv[2]);
        generations = atoi(argv[3]);
        break;
    case 5: /* le dimensioni sono scelte dall'utente e la matrice viene stampata ad ogni iterazione */
        if(strcmp(argv[4], "test") == 0) {
           is_test = true; 
        }
        row_size = atoi(argv[1]);
        col_size = atoi(argv[2]);
        generations = atoi(argv[3]);
        break;    
    case 1: /* configurazioni di default */
        row_size = DEF_ROWS;
        col_size = DEF_COLS;
        generations = DEF_ITERATION;
        break;
    default:
        printf("Error, check the number of arguments.\n");
        MPI_Finalize();
        return 0;
        break;
    }

    /* crea un nuovo tipo di dato MPI replicando MPI_CHAR col_size volte in posizioni contigue */
    MPI_Type_contiguous(col_size, MPI_CHAR, &row_data);
    MPI_Type_commit(&row_data);

    /* ogni cella i memorizza il numero di righe assegnate al processo i-esimo */
    rows_for_proc = calloc(num_proc, sizeof(int));
    /* ogni cella i memorizza il displacement da applicare al processo i-esimo */
    displ_for_proc = calloc(num_proc, sizeof(int));
    
    /* divisione delle righe */
    int base = (int)row_size / num_proc;
    int rest = row_size % num_proc;
    /* righe già assegnate */
    int assigned = 0;

    /* calcolo righe e displacement per ogni processo */
    for (int i = 0; i < num_proc; i++) {
        displ_for_proc[i] = assigned;
        /* nel caso di resto presente, i primi resto processi ricevono una riga in più*/
        if (rest > 0) {
            rows_for_proc[i] = base + 1;
            rest--;
        } else {
            rows_for_proc[i] = base;
        }
        assigned += rows_for_proc[i];
    }

    if(rank == MASTER) {    
        /* nel caso di file presente, solo master inizializza la matrice */
        start_time = MPI_Wtime();
        if(is_file) {
            /* viene allocata la matrice di gioco */ 
            game_matrix = calloc(row_size*col_size, sizeof(char));
            /* inizializzata da file */
            init_from_file(game_matrix, row_size, col_size, file);
        }
        if(is_test) {
            /* viene allocata la matrice di gioco per mostrare i risultati delle varie operazioni */ 
            game_matrix = calloc(row_size*col_size, sizeof(char));
            init_test_matrix(game_matrix, row_size, col_size);
        }
        printf("Settings: generations %d \trows %d \tcolumns %d\n", generations, row_size, col_size);
    }

    /* ogni processo alloca la sua porzione di righe */
    process_buffer = calloc(rows_for_proc[rank] * col_size, sizeof(char));
    
    /* se non è presente file, ogni processo inizializza la sua porzione con valori casuali */
    if(!is_file) {
        srand(time(NULL) + rank);
        for(int i = 0; i < rows_for_proc[rank] * col_size; i++) {
            if (rand() % 2 == 0) {
                process_buffer[i] = ALIVE;
            } else {
                process_buffer[i] = DEAD;
            }
        }  
    }

    /* 
    raccoglie i dati da tutti i processi del communicator e li concatena nel buffer del processo master 
    MPI_Gatherv consente ai messaggi ricevuti di avere lunghezze diverse e di essere memorizzati
    in posizioni arbitrarie nel buffer del processo MASTER. 
    
    Utilizzata solo nella fase di test per stampare la matrice a video
    */
    
    if(is_test) {
        MPI_Gatherv(process_buffer, rows_for_proc[rank], row_data, game_matrix, rows_for_proc, displ_for_proc, row_data, MASTER, MPI_COMM_WORLD); 
    }

        
    /* in caso di test o di file, il processo MASTER mostra su stdout la matrice di partenza */
    if(rank == MASTER) {
        if(is_file || is_test) {
            print_matrix(0, game_matrix, row_size, col_size);
        }
    }

    /* calcolo rank processi successivo e precedente al corrente (tenendo conto del toroide) */
    prev_rank = (rank - 1 + num_proc) % num_proc;
    next_rank = (rank + 1) % num_proc;

    /* alloca, per ogni processo, i buffer per memorizzare il risultato della computazione e le righe da ricevere */
    result_buffer = calloc(rows_for_proc[rank] * col_size, sizeof(char));
    char *temp; /* per lo scambio di puntatori */
    prev_row = calloc(col_size, sizeof(char));
    next_row = calloc(col_size, sizeof(char));

    for(int gen = 0; gen < generations; gen++) {
        
        /* scambia i puntatori */
        if(gen > 0) {
            if( gen%2 == 0) {
                temp = result_buffer;
                result_buffer = process_buffer;
                process_buffer = temp;
            } else {
                temp = process_buffer;
                process_buffer = result_buffer;
                result_buffer = temp;
            }
        }
        

        /* la matrice inizializzata da file viene divisa ed inviata, per righe, agli altri processi */
        if(is_file) {
            MPI_Scatterv(game_matrix, rows_for_proc, displ_for_proc, row_data, process_buffer, rows_for_proc[rank], row_data, MASTER, MPI_COMM_WORLD);
        }
            
        /* invio e ricezione delle righe di bordo in modalità non bloccante*/
        /* rank invia la sua prima riga al processo precedente */
        MPI_Isend(process_buffer, 1, row_data, prev_rank, TAG_PREV, MPI_COMM_WORLD, &send_request);
        MPI_Request_free(&send_request);

        /* rank riceve la riga precedente dal suo predecessore */
        MPI_Irecv(prev_row, 1, row_data, prev_rank, TAG_NEXT, MPI_COMM_WORLD, &prev_request);

        /* rank invia la sua ultima riga al suo successore */
        MPI_Isend(process_buffer + (col_size * (rows_for_proc[rank] - 1)), 1, row_data, next_rank, TAG_NEXT, MPI_COMM_WORLD, &send_request);
        MPI_Request_free(&send_request);

        /* rank riceve la riga successiva dal suo successore */
        MPI_Irecv(next_row, 1, row_data, next_rank, TAG_PREV, MPI_COMM_WORLD, &next_request);
        
        /* calcola i valori delle celle che non necessitano di aiuto da altri processi quindi escluse la prima e l'ultima riga di quelle possedute */
        compute(process_buffer, result_buffer, rows_for_proc[rank], col_size);

        MPI_Request to_wait[] = {prev_request, next_request};
        int handle_index;
        /* attende il completamento delle comunicazioni */
        MPI_Waitany(
            2, /* numero di richieste */
            to_wait, /* array di request da attendere */
            &handle_index,
            &request_status
        );

        /* nel caso la next_request venga completata prima */
        if(request_status.MPI_TAG == TAG_PREV) {
            /* 
            calcola i valori con l'utilizzo della riga successiva,
            attende il completamento della ricezione della riga precedente
            e computa le celle con l'ausilio della riga precedente
            */
            compute_next(process_buffer, result_buffer, next_row, rows_for_proc[rank], col_size);
            MPI_Wait(&prev_request, MPI_STATUS_IGNORE);
            compute_prev(process_buffer, result_buffer, prev_row, col_size);
        } else if(request_status.MPI_TAG == TAG_NEXT) { /* nel caso viene completata prima la prev_request */
            /* 
            calcola i valori sulla riga precedente, 
            attende la riga successiva
            e calcola i valori usando la riga successiva
            */
            compute_prev(process_buffer, result_buffer, prev_row, col_size);
            MPI_Wait(&next_request, MPI_STATUS_IGNORE);
            compute_next(process_buffer, result_buffer, next_row, rows_for_proc[rank], col_size);
        }

        /* 
            le righe appena calcolate vengono reinviate al master e memorizzate in game_matrix
            nel caso di test e file per permettere di mostrare la matrice a video
        */
        if (is_test || is_file)
            MPI_Gatherv(result_buffer, rows_for_proc[rank], row_data, game_matrix, rows_for_proc, displ_for_proc, row_data, MASTER, MPI_COMM_WORLD);

        /* nel caso di file o di test viene mostrata la matrice dopo ogni iterazione */
        if(rank == MASTER) {
            if(is_file || is_test) {
                print_matrix(gen + 1, game_matrix, row_size, col_size);
            }
        }
    }
    
    /* sincronizza tutti i processi affinchè arrivino tutti al medesimo punto */
    MPI_Barrier(MPI_COMM_WORLD);

    /* libera la memoria dinamica allocata */
    free(result_buffer);
    free(process_buffer);
    free(next_row);
    free(prev_row);

    /* il processo master mostra il tempo di esecuzione */
    if(rank == MASTER) {
        if(is_file || is_test){
          free(game_matrix);  
        }
        end_time = MPI_Wtime();
        printf("\nExecution Time: %f ms\n", end_time - start_time);
    }
    MPI_Finalize();
    return 0;
}