/**
 * @file main.c
 * @author Shresth Mishra
 * @brief Contains main function that splits base station and seafloor sensors into different MPI communicators
 * @version 0.1
 * @date 2022-10-18
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "base.h"
#include "sensor_node.h"
#include "tools.h"

int termination_flag = 0, num_seafloor_sensors, balloon_front = -1;

// Size of report_data. 5 for NODE_RANK, CART_RANK, COUNTER_OVER_THRESHOLD, ITER_NUM ans STATUS
const int report_data_len = INFO_LEN * NBR_NUM * NBR_PROCESS_LEN + 5;
float *report_data;         // An array holding report data. Used in base station and all sensors.
pthread_mutex_t g_Mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * Gets user specified number of rows and columns. Splits base station and seafloor sensors into different MPI communicators.
 * 
 * @param argc a int holding the number of command line arguments
 * @param argv an array holding command line arguments
 * @return int 0 for no issue
 */
int main(int argc, char *argv[]){
    // Variable declaration
    MPI_Comm new_comm;                      // A new MPI communicator to be splitted into
    int rank, size, provided;
    int nrows = 0, ncols = 0;

    // =============== Initialisation ===============
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);              // Set MPI Thread environments
    MPI_Comm_size(MPI_COMM_WORLD, &size);                                       // Get MPI size
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);                                       // Get corresponding MPI rank
    

    // If user specified the numbers for row and column
    if (argc == 3)
    {
        nrows = atoi(argv[1]);                           // Get user's wanted row 
        ncols = atoi(argv[2]);                           // Get user's wanted column 
        
        // If user specified wrong numbers for row and/or column
        if ((nrows * ncols + 1) != size)
        {
            // print appropriate message and terminate
            if (rank == 0){
                printf("ERROR: nrows*ncols)=%d * %d = %d != %d\n", nrows, ncols, nrows * ncols, size);
            }

            MPI_Finalize();
            return 0;
        }
    }

    // Allocate memory for report array
    report_data = (float *) malloc( report_data_len * sizeof(float));

    // Split the communicator and start the program
    MPI_Comm_split(MPI_COMM_WORLD, rank == size - 1, 0, &new_comm);

    if (rank == size - 1) {
        num_seafloor_sensors = size -1;
        base_station_io();                              // Run base station 
    }
    else
    {
        sensor_node_io(new_comm, nrows, ncols);         // Run seafloor sensors
    }
    
    free(report_data);                                  // Deallocate memory
    MPI_Comm_free(&new_comm);                           // free the MPI communicator
    MPI_Finalize();
    return 0;
}
