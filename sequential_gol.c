/*
Game of life versione sequenziale
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ALIVE 'O'
#define DEAD '.'

/* dimensione matrice di default*/
#define ROWS 15
#define COLS 42

/* numero di generazioni/iterazioni da considerare di default */
#define ITERATIONS 8

/* funzioni di utility */

/* mostra su stdout la matrice */
void print_matrix(int current, char mat[][COLS])
{
    printf("\nGenerazione %d:\n", current);
    for (int i = 0; i < ROWS; i++)
    {
        for (int j = 0; j < COLS; j++)
        {
            printf("%c", mat[i][j]);
        }
        printf("\n");
    }
}

/* carica la matrice seed da file */
void load_from_file(char matrix[][COLS], char *file)
{
    char c; /* carattere letto */
    FILE *fptr;
    fptr = fopen(file, "r");
    for (int i = 0; i < ROWS; i++)
    {
        for (int j = 0; j < COLS; j++)
        {
            fscanf(fptr, "%c ", &c);
            matrix[i][j] = c;
        }
    }
    fclose(fptr);
}

/* setta il valore di righe e colonne in base al pattern caricato */
void check_matrix_size(char *filename)
{
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

    printf("Il file ha %d righe e %d colonne.\n", ROWS, COLS);

    fclose(file);
}


/* conta il numero di vicini vivi nell'intorno della cella passata */
int count_live_neighbour_cell(char mat[][COLS], int r, int c)
{
    int i, j, count = 0;
    for (i = r - 1; i <= r + 1; i++)
    {
        for (j = c - 1; j <= c + 1; j++)
        {
            if ((i == r && j == c) || (i < 0 || j < 0) || (i >= ROWS || j >= COLS))
            {
                continue;
            }
            if (mat[i][j] == ALIVE)
            {
                count++;
            }
        }
    }
    return count;
}

/* calcola la generazione successiva di mat e la salva in copy */
void life(char mat[][COLS], char copy[][COLS])
{
    int neighbour_live_cell, i, j;
    /* ciclo cella per cella */
    for (i = 0; i < ROWS; i++)
    {
        for (j = 0; j < COLS; j++)
        {
            /* per ogni cella calcola i suoi vicini vivi */
            neighbour_live_cell = count_live_neighbour_cell(mat, i, j);

            /*
             *  controlla le 3 casistiche:
             *  una cellula viva con 2 o 3 vicini sopravvive per la prossima generazione
             *  se è morta e ha 3 vicini vivi torna in vita nella prossima generazione
             *  altrimenti muore
             */

            if (mat[i][j] == ALIVE && (neighbour_live_cell == 2 || neighbour_live_cell == 3))
            {
                copy[i][j] = ALIVE;
            }
            else if (mat[i][j] == DEAD && neighbour_live_cell == 3)
            {
                copy[i][j] = ALIVE;
            }
            else
            {
                copy[i][j] = DEAD;
            }
        }
    }
    return;
}

/* funzione main */
int main(int argc, char *argv[])
{
    char matrix[ROWS][COLS]; /* matrice di partenza */
    char copy[ROWS][COLS];
    int i, j;
    char *dir, *filename, *ext, *file;

    if (argc == 2) /* la matrice seed viene caricata da file */
    {
        dir = "patterns/";
        filename = argv[1];
        ext = ".txt";
        file = malloc(strlen(dir) + strlen(filename) + strlen(ext) + 1);
        sprintf(file, "%s%s%s", dir, filename, ext);
        
        printf("--Generata dal file %s--\n", file);

        /* controlla le dimensioni della matrice */
        // check_matrix_size(file);
        /* riempie la matrice da file */
        load_from_file(matrix, file);
    }
    else if (argc == 1) /* se non viene specificato il file */
    {
        /*
         * riempie la matrice in modo casuale
         * O = vivo, . = morto
         */
        printf("--Generata in modo casuale--\n");
        for (i = 0; i < ROWS; i++)
        {
            for (j = 0; j < COLS; j++)
            {
                if(rand() % 2 == 0)
                    matrix[i][j] = '.';
                else
                    matrix[i][j] = 'O';    
            }
        }
    }
    else
    {
        exit(1);
    }

    /* mostra su stdout la matrice seed */
    printf("Matrice di partenza:");
    print_matrix(0, matrix);

    /* ripete il processo per ITERATIONS volte */
    for (int current = 0; current < ITERATIONS - 1; current++)
    {
        /* calcola la generazione successiva e mostra i risultati */
        if (current % 2 == 0)
        {
            life(matrix, copy);
            print_matrix(current + 1, copy);
        }
        else
        {
            life(copy, matrix);
            print_matrix(current + 1, matrix);
        }
    }

    free(file);
    return 0;
}