/**********************************************
To compile -  gcc mssv.c -o mssv 
To run - ./mssv <text file name> <delay>
	./mssv solution.txt 3
**********************************************/


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define GRID_SIZE 9	// Size of the sudoku grid
#define SUBGRID_SIZE 3	// Size of the subgrid
#define NUM_THREADS 4	// Number of threads being used

int Sol[GRID_SIZE][GRID_SIZE];	// 2D array for sudoku solution
int Row[GRID_SIZE] = {0}; // Array Row used to keep track of numbers in each row
int Col[GRID_SIZE] = {0}; // Array Column used to keep track of numbers in each column
int Sub[GRID_SIZE] = {0}; // Array Sub-grid used to keep track of numbers in each subgrid
int Counter = 0;	// Keeping track of number of valid, rows, columns and sub-grids



// For Synchronization
pthread_mutex_t lock;	// Only one thread can access a shared resource at a time
pthread_cond_t cond;	// Thread waits until a contion becomes true



/**************************************************************************************************
Purpose = Checks for valid sub-grids and rows Checks if number is between (1-9). 
Row validation also happens in the same way.
Column validation also happens in the same way.
After validation updates counter and signals parent thread when all sub-grid and rows have passed
***************************************************************************************************/


void *validateSubGridsAndRows(void *arg) {
    int threadID = *(int *)arg;

    // Validate sub-grids
    for (int i = (threadID - 1) * 3; i < threadID * 3; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            int num = Sol[i][j];
            int subgrid = (i / SUBGRID_SIZE) * SUBGRID_SIZE + (j / SUBGRID_SIZE);
            if (Sub[subgrid] == 0 && num >= 1 && num <= 9) {
                Sub[subgrid] = 1;
            }
        }
    }

    // Validate rows
    for (int i = (threadID - 1) * 3; i < threadID * 3; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            int num = Sol[i][j];
            if (Row[i] == 0 && num >= 1 && num <= 9) {
                Row[i] = 1;
            }
        }
    }

    // Sleep to simulate validation delay
    int delay = *((int *)arg + 1);
    sleep(delay);

    // Update counter and signal parent thread
    pthread_mutex_lock(&lock);
    Counter += (SUBGRID_SIZE + SUBGRID_SIZE); // Each thread increments by 6
    if (Counter >= 27) {
        printf("Thread ID-%d is the last thread.\n", threadID);
        pthread_cond_signal(&cond);
    }
    pthread_mutex_unlock(&lock);

    pthread_exit(NULL);
}

// Validate columns
void *validateColumns(void *arg) {
    
    for (int col = 0; col < GRID_SIZE; col++) {
        for (int row = 0; row < GRID_SIZE; row++) {
            int num = Sol[row][col];
            if (Col[col] == 0 && num >= 1 && num <= 9) {
                Col[col] = 1;
            }
        }
    }

    // Sleep to simulate validation delay
    int delay = *(int *)arg;
    sleep(delay);

    // Update counter and signal parent thread
    pthread_mutex_lock(&lock);
    Counter += GRID_SIZE; // Thread 4 increments by 9
    if (Counter == 27) {
        printf("Thread ID-4 is the last thread.\n");
        pthread_cond_signal(&cond);
    }
    pthread_mutex_unlock(&lock);

    pthread_exit(NULL);
}

/****************************************************************************************
Purpose = Takes command line arguments and initialize the mutex and condtion variables.
Reads from the file the sudoku solution and stores it in the 2D array 'Sol'.
4 threads are created, 3 to validate sub-grids and rows and 1 to validate columns.
Waits for all threads to finish their tasks and signals the parent thread. 
Prints the results and destroys the mutex and condtion
*****************************************************************************************/
int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Command: %s solution delay\n", argv[0]);
        return 1;
    }

    char *solution_file = argv[1];
    int delay = atoi(argv[2]);

    // Initialize mutex and condition variable
    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&cond, NULL);

    // Read Sudoku solution from file
    FILE *file = fopen(solution_file, "r");
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            fscanf(file, "%d", &Sol[i][j]);
        }
    }
    fclose(file);

    // Create threads
    pthread_t threads[NUM_THREADS];
    int threadIDs[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        threadIDs[i] = i + 1;
        if (i < NUM_THREADS - 1) {
            pthread_create(&threads[i], NULL, validateSubGridsAndRows, &threadIDs[i]);
        } else {
            pthread_create(&threads[i], NULL, validateColumns, &delay);
        }
    }

    // Wait for all threads to finish
    pthread_mutex_lock(&lock);
    pthread_cond_wait(&cond, &lock);
    pthread_mutex_unlock(&lock);

    // Print results
    printf("Thread ID-1: %s\n", (Row[0] && Row[1] && Row[2]) ? "valid" : "row 1, row 2 are invalid");
    printf("Thread ID-2: %s\n", (Row[3] && Row[4] && Row[5]) ? "valid" : "row 4, row 5 are invalid");
    printf("Thread ID-3: %s\n", (Row[6] && Row[7] && Row[8]) ? "valid" : "row 7, row 8 are invalid");
    printf("Thread ID-4: %s\n", (Col[4]) ? "valid" : "column 5 is invalid");

    int validSubGrids = 0;
    for (int i = 0; i < GRID_SIZE; i++) {
        if (Sub[i]) {
            validSubGrids++;
        }
    }
    printf("There are %d valid sub-grids, and thus the solution is %s.\n", validSubGrids, validSubGrids == 9 ? "valid" : "invalid");

    // Clean up
    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cond);
    pthread_exit(NULL);

    return 0;
}


