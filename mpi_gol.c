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

/* dimensioni di default della matrice se non specificate */
#define DEF_ROWS 5
#define DEF_COLS 5

/* funzioni di utility */

/* stampa una linea dopo ogni riga */ 
int print_line(int cols)
{
    printf("\n");
    for (int i = 0; i < cols; i++)
    {
        printf(" -----");
    }
    printf("\n");
}

/* mostra la matrice di input su stdout */
void print_matrix(int current, int **mat, int rows, int cols)
{
    /* Generazione a cui appartiene la matrice*/
    printf("\n Generation #%d:", current);
    print_line();
    for (int i = 0; i < rows; i++)
    {
        printf(":");
        for (int j = 0; j < cols; j++)
        {
            printf("  %d  :", mat[i][j]);
        }
        print_line();
    }
}

/* cambia il valore di una cella da morta a viva o viceversa 
*  1 = cellula viva
*  0 = cellula morta
*/
void change_state(int *cell)
{
    if(*cell == 1)
        *cell = 0;
    else
        *cell = 1;    
}

/* controlla se una cella è viva */
int is_alive(int cell)
{
    return cell == 1;
}

/* funzione che decide lo stato della cella per la generazione successiva */
void get_state(int *send_mat, int *recv_mat, int index, int alive_count)
{
    if(is_alive())
}

/* funzione main */
int main(int argc, char **argv)
{
    int rank, num_proc; // P numero di processi
    int rows, cols, iterations;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);     // rank processo
    MPI_Comm_size(MPI_COMM_WORLD, &num_proc);

    // specificato solo il numero di iterazioni
    if (argc == 2)
    {
        iterations = atoi(argv[1]);
        rows = DEF_ROWS;
        cols = DEF_COLS;
    }
    else if (argc == 4)
    {
        iterations = atoi(argv[1]);
        rows = atoi(argv[2]);
        cols = atoi(argv[3]);
    } else {
        printf("ERROR: Insert the correct number of arguments: #iteration #rows #cols or #iteration only\n");
        MPI_Finalize();
        exit(1);
    }

    int matrix[rows][cols];

    if (rank == MASTER)
    {
        printf("Settings:\n");
        printf("iterations:%d\trows:%d\tcolumns:%d\n", iterations, rows, cols);
    }
    
    MPI_Finalize();
    return 0;
}