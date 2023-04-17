/*
Game of life versione sequenziale
*/

#include <stdio.h>
#include <stdlib.h>
// dimensione matrice
#define ROWS 5
#define COLS 5

// numero di generazioni/iterazioni da considerare
#define ITERATIONS 3

// stampa una linea dopo ogni riga
int row_line()
{
    printf("\n");
    for (int i = 0; i < COLS; i++)
    {
        printf(" -----");
    }
    printf("\n");
}

// mostra su stdout la matrice
void print_matrix(int current, int mat[][COLS])
{
    // mostra la nuova generazione
    printf("\n Generazione %d:", current);
    row_line();
    for (int i = 0; i < ROWS; i++)
    {
        printf(":");
        for (int j = 0; j < COLS; j++)
        {
            printf("  %d  :", mat[i][j]);
        }
        row_line();
    }
}

// Ritorna il numero di vicini vivi nell'intorno della cella passata
// point=(r,c)
int count_live_neighbour_cell(int mat[][COLS], int r, int c)
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
            if (mat[i][j] == 1)
            {
                count++;
            }
        }
    }
    return count;
}

// calcola la prossima generazione
void life(int mat[][COLS], int copy[][COLS])
{
    // calcola la nuova generazione in base alla precedente
    int neighbour_live_cell, i, j;

    for (i = 0; i < ROWS; i++)
    {
        for (j = 0; j < COLS; j++)
        {
            // per ogni cella calcola i suoi vicini vivi
            neighbour_live_cell = count_live_neighbour_cell(mat, i, j);

            // controlla le 3 casistiche
            // una cellula viva con 2 o 3 vicini sopravvive per la prossima generazione
            if (mat[i][j] == 1 && (neighbour_live_cell == 2 || neighbour_live_cell == 3))
            {
                copy[i][j] = 1;
            }
            // se è morta e ha 3 vicini vivi torna in vita nella pross generazione
            else if (mat[i][j] == 0 && neighbour_live_cell == 3)
            {
                copy[i][j] = 1;
            }
            // altrimenti muore
            else
            {
                copy[i][j] = 0;
            }
        }
    }
    return;
}

int main(int argc, char *argv[])
{
    int matrix[ROWS][COLS], copy[ROWS][COLS];
    int i, j;

    // riempie la matrice in modo casuale
    // 1 = vivo, 0 = morto
    for (i = 0; i < ROWS; i++)
    {
        for (j = 0; j < COLS; j++)
        {
            matrix[i][j] = rand() % 2;
        }
    }

    // mostra la matrice seed
    printf("Matrice di partenza:");
    print_matrix(0, matrix);

    for (int current = 0; current < ITERATIONS - 1; current++)
    {
        // mostra la nuova generazione
        if (current % 2 == 0)
        {
            life(matrix, copy);
            print_matrix(current+1, copy);
        }
        else
        {
            life(copy, matrix);
            print_matrix(current+1, matrix);
        }
    }

    return 0;
}