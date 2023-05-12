/*
* Game of life versione sequenziale
* MPI viene utilizzato solo per il calcolo dei tempi di esecuzione,
* affinchè non ci siano disparità nel conteggio
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <mpi.h>

#define ALIVE 'O'
#define DEAD '.'

/* numero di generazioni/iterazioni da considerare di default */
#define DEF_ITERATIONS 8

/* memorizzano i valori passati da stdin */
int row_size = 0, 
    col_size = 0,
    iterations = 0;

/* funzioni di utility */

/* mostra su stdout la matrice */
void print_matrix(int gen, char *mat) {
    printf("\nGeneration %d:\n", gen);
    for (int i = 0; i < row_size; i++) {
        for (int j = 0; j < col_size; j++) {
            printf("%c", mat[i * col_size + j]);
        }
        printf("\n");
    }
}

/* carica la matrice seed da file */
void load_from_file(char *matrix, char *file) {
    char c; /* carattere letto */
    FILE *fptr;
    fptr = fopen(file, "r");
    for (int i = 0; i < row_size; i++) {
        for (int j = 0; j < col_size; j++) {
            fscanf(fptr, "%c ", &c);
            matrix[i * col_size + j] = c;
        }
    }
    fclose(fptr);
}

/* riempie la matrice in maniera casuale */
void random_initialize(char *matrix) {
    srand(time(NULL));

    for (int i = 0; i < row_size; i++) {
        for (int j = 0; j < col_size; j++) {
            if(rand()%2 == 0){
                matrix[i * col_size + j] = ALIVE;
            } else {
                matrix[i * col_size + j] = DEAD;
            }
            
        }
    }
}

/* setta il valore di righe e colonne in base al file pattern caricato */
void check_matrix_size(char *filename) {
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

/* conta il numero di vicini vivi nell'intorno della cella (r,c) passata */
int count_live_neighbour_cell(char *mat, int r, int c) {
    int i, j, count = 0;
    for (i = r - 1; i <= r + 1; i++) {
        for (j = c - 1; j <= c + 1; j++) {
            if ((i == r && j == c) || (i < 0 || j < 0) || (i >= row_size || j >= col_size)) {
                continue;
            }
            if (mat[i * col_size + j] == ALIVE) {
                count++;
            }
        }
    }
    return count;
}

/* calcola la generazione successiva di mat e la salva in copy */
void life(char *mat, char *copy) {
    int neighbour_live_cell, i, j;
    /* ciclo cella per cella */
    for (i = 0; i < row_size; i++) {
        for (j = 0; j < col_size; j++) {
            /* per ogni cella calcola i suoi vicini vivi */
            neighbour_live_cell = count_live_neighbour_cell(mat, i, j);

            /*
             *  controlla le 3 casistiche:
             *  una cellula viva con 2 o 3 vicini sopravvive per la prossima generazione
             *  se è morta e ha 3 vicini vivi torna in vita nella prossima generazione
             *  altrimenti muore
             */

            if (mat[i * col_size + j] == ALIVE && (neighbour_live_cell == 2 || neighbour_live_cell == 3)) {
                copy[i * col_size + j] = ALIVE;
            }
            else if (mat[i * col_size + j] == DEAD && neighbour_live_cell == 3) {
                copy[i * col_size + j] = ALIVE;
            }
            else {
                copy[i * col_size + j] = DEAD;
            }
        }
    }
}

/* funzione main */
int main(int argc, char *argv[])
{
    char *matrix; /* matrice di partenza */
    char *copy;
    int i, j; 
    char *dir, *filename, *ext, *file; /* vars per lettura file */
    double start, end; /*  per la misurazione del tempo */

    /* inizializzazione ambiente MPI */
    MPI_Init(&argc, &argv);

    /* 
    *  prima opzione di esecuzione 
    *  la matrice seed viene caricata da file di cui viene specificato il nome 
    *  secondo argomento è il numero di iterazioni da svolgere
    */ 
    if (argc == 3) {
        dir = "patterns/";
        filename = argv[1];
        ext = ".txt";
        file = malloc(strlen(dir) + strlen(filename) + strlen(ext) + 1);
        sprintf(file, "%s%s%s", dir, filename, ext);
        
        printf("--Generate matrix seed from %s--\n", file);

        /* controlla le dimensioni della matrice */
        check_matrix_size(file);
        /* istanzia le matrici */
        matrix = calloc(row_size *col_size, sizeof(char));
        copy = calloc(row_size * col_size, sizeof(char));
        /* riempie la matrice di partenza da file */
        load_from_file(matrix, file);
        /* numero di iterazioni da fare */
        iterations = atoi(argv[2]);
    }
    else if(argc == 4 ) {
        /* secondo opzione di esecuzione: l'utente può inserire
        *  numero di righe
        *  numero di colonne
        *  iterazioni da svolgere
        *  la matrice viene riempita in maniera casuale
        */
        row_size = atoi(argv[1]);
        col_size = atoi(argv[2]);
        iterations = atoi(argv[3]);
        /* allocazione delle matrici */
        matrix = calloc(row_size *col_size, sizeof(char));
        copy = calloc(row_size * col_size, sizeof(char));
        /* riempie la matrice in modo casuale */
        random_initialize(matrix);
    }
    else {
        printf("File not valid or bad number of arguments\n");
        MPI_Finalize();
        exit(0);
    }

    /* mostra su stdout la matrice seed */
    printf("Seed matrix:");
    print_matrix(0, matrix);

    /* inizia il calcolo dei tempi */
    start = MPI_Wtime();

    /* ripete il processo per ITERATIONS volte */
    for (int current = 0; current < iterations; current++) {
        /* calcola la generazione successiva e mostra i risultati */
        if (current % 2 == 0) {
            life(matrix, copy);
            print_matrix(current + 1, copy);
        }
        else {
            life(copy, matrix);
            print_matrix(current + 1, matrix);
        }
    }

    /* fine tempo di esecuzione */
    end = MPI_Wtime();
    MPI_Finalize();

    printf("Execution time: %f ms\n", end - start);

    /* libera la memoria dinamica allocata */
    if(argc == 3) {
      free(file);  
    }
    free(matrix);
    free(copy);

    return 0;
}