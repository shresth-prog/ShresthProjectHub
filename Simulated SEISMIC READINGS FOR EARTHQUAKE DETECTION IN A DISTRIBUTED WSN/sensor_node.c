/**
 * @file sensor_node.c
 * @author Shresth Mishra
 * @brief Implements seafloor sensors
 * @version 0.1
 * @date 2022-10-18
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "sensor_node.h"

// Extern the gloable variable from the main function
extern float *report_data;
extern int report_data_len;

/**
 * this fucntion working as the sdlave process in the main file, simulate the earthquake sensor node, it will communicate message with the base station by MPI,
 * Each  sensor node will report the message itgenerate, or report the fault information signal to shut dow the whole system and receive the shut down
 * signal from the base station
 * 
 * @param new_comm A MPI communicator splitted 
 * @param nrows 0 or user specified number of rows
 * @param ncols 0 or user specified number of columns
 * @return int 0 for no issue
 */
int sensor_node_io(MPI_Comm new_comm, int nrows, int ncols)
{
    //  Variable declaration 
    int world_size, size, current_rank, reorder = 1, cart_rank, count_sensor_node, termination_signal = 0, iteration_count = 0;
    int coord[NDIMS], wrap_around[NDIMS], dims[NDIMS], nbr_list[NBR_NUM];
    float *node_data;                                       // A poiter to an array holing its own seismic data
    float **recv_data;                                      // A poiter to an array holing adjacent sensors' data
    float dist_to_nbr, mag_diff, dep_diff;

    MPI_Comm Comm2D;                                        // A MPI communicator for virtual topology of 2D Cartesian grid
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    MPI_Request termination_signal_request;
    
    // These are used when sharing data with adjacent sensors
    MPI_Request send_request[NBR_NUM];
    MPI_Request receive_request[NBR_NUM];
    MPI_Status send_status[NBR_NUM];
    MPI_Status receive_status[NBR_NUM];


    // =============== MPI Environment initialisation for seafloor sensors ===============
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);                                             // size of COMM_WORLD
    MPI_Comm_size(new_comm, &size);                                                         // size of sensors
    MPI_Comm_rank(new_comm, &current_rank);                                                 // rank within sensors

    
    // =============== Waits to receive termination request from the base station in the back (Non-blocking)  ===============
    MPI_Ibcast(&termination_signal, 1, MPI_INT, world_size-1, MPI_COMM_WORLD, &termination_signal_request);

    // =============== Virtual Topology for 2D grid ===============
    dims[ROW] = nrows; dims[COL] = ncols;
    MPI_Dims_create(size, NDIMS, dims);

    wrap_around[ROW] = wrap_around[COL] = 0;                                                // Sets Whether to form a ring

    if ( MPI_Cart_create(new_comm, NDIMS, dims, wrap_around, reorder, &Comm2D) != 0 ) {     // Create Cartesian grid
        printf("ERROR creating CART\n");
        return -1;
    }

    MPI_Cart_coords(Comm2D, current_rank, NDIMS, coord);                                    // Get coordinates
    MPI_Cart_rank(Comm2D, coord, &cart_rank);                                               // Get rank from Cartesian grid

    MPI_Cart_shift(Comm2D, SHIFT_ROW, DISP, &nbr_list[UP], &nbr_list[DOWN]);                // Get ranks from UP and DOWN
    MPI_Cart_shift(Comm2D, SHIFT_COL, DISP, &nbr_list[LEFT], &nbr_list[RIGHT]);             // Get ranks from LEFT and RIGHT


    // =============== Dynamic memory allocation ===============
    node_data = (float *) malloc(INFO_LEN * sizeof(float));                                 // Holds seafloor data of itself

    recv_data = (float **) malloc(NBR_NUM * sizeof(float *));                               // Holds seafloor data of adjacent sensors
    for(int nbr = 0; nbr < NBR_NUM; nbr++){
        recv_data[nbr] = (float *)malloc(INFO_LEN * sizeof(float));                         // Dynamic memory allocation
    }

    // =============== Generate seismic data for every 5 Sec (sleep(5) is at the end) ===============
    while(1){
        iteration_count++;
        MPI_Barrier(new_comm);                                  // Waits for all other seafloor sensors
        
        // =============== Checks for termination signal. If true, exit ===============
        MPI_Test(&termination_signal_request, &termination_signal, MPI_STATUS_IGNORE);

        // If termination signal is received
        if (termination_signal == 1){
            printf("Rank : %d - Detected termination signal from the base station.\n", current_rank);
            break;
        }

        // =============== Get node data and store in an array ===============
        sleep(cart_rank);
        getCurrentTime(&t, &tm);                        // Get current system time

        node_data[ROW] = coord[ROW];
        node_data[COL] = coord[COL];
        node_data[YEAR] = tm.tm_year + 1900;
        node_data[MONTH] = tm.tm_mon + 1;
        node_data[DAY] = tm.tm_mday;
        node_data[HOUR] = tm.tm_hour;
        node_data[MIN] = tm.tm_min;
        node_data[SEC] = tm.tm_sec;
        
        srand((unsigned)time(NULL) * current_rank);

        node_data[LAT] = gen_rand_num(-16, -15);
        node_data[LON] = gen_rand_num(165, 168);
        node_data[MAG] = gen_rand_num(0, 10);
        node_data[DEP] = gen_rand_num(0, 10);

        // Checks for the Magnitude. Set -1 if it does not exceeds, otherwise leave as it is.
        if (node_data[MAG] < MAGNITUDE_THRESHOLD){
            node_data[MAG] = -1;
        }
        
        // =============== Share data between neighbours ===============
        #pragma omp parallel for num_threads(NBR_NUM) schedule(dynamic)
        for (int nbr = 0; nbr < NBR_NUM; nbr++) {
            MPI_Isend(node_data, INFO_LEN, MPI_FLOAT, nbr_list[nbr], 0, Comm2D, &send_request[nbr]);                        // non-blocking send to adjacent sensors
            MPI_Irecv(recv_data[nbr], INFO_LEN, MPI_FLOAT, nbr_list[nbr], 0, Comm2D, &receive_request[nbr]);                // non-blocking recv to adjacent sensors
        }

        // Wait for the non-blocking operations
        MPI_Waitall(NBR_NUM, send_request, send_status);
        MPI_Waitall(NBR_NUM, receive_request, receive_status);
        

        /************************************************************************************************************
         *           At this point, each node has its own data and datasets received from the neighbours            *
         ************************************************************************************************************/

        // =============== Count neighbours with magnitude > MAGNITUDE_THRESHOLD ===============
        count_sensor_node = 0;
        for (int nbr = 0; nbr < NBR_NUM; nbr++){
            if (recv_data[nbr][MAG] > MAGNITUDE_THRESHOLD){
                count_sensor_node++;
            }
        }
        
        // Only if the bellow conditions are met, report to the base station
        // 1. There at least 2 adjacent sensors with magnitude > MAGNITUDE_THRESHOLD
        // 2. Magnitude of its own data > MAGNITUDE_THRESHOLD
        if ( 2 <= count_sensor_node && MAGNITUDE_THRESHOLD <= node_data[MAG] ) {

            // =============== Copy own data into report and some additional data ===============
            for (int i = 0; i < INFO_LEN; i++){
                report_data[i] = node_data[i];
            }

            // Put additional useful information
            report_data[NODE_RANK] = current_rank;
            report_data[CART_RANK] = cart_rank;
            report_data[COUNTER_OVER_THRESHOLD] = count_sensor_node;
            report_data[ITER_NUM] = iteration_count;

            // =============== Pre-process data for adjacent sensors and copy into report ===============
            for (int nbr = 0; nbr < NBR_NUM; nbr++)
            {
                dist_to_nbr = -1.0, mag_diff = -1.0, dep_diff = -1.0;
                
                // If adjacent exist in 'nbr' direction.
                // 0 == UP, 1 == DOWN, 2 == LEFT and 3 == RIHGT
                if (nbr_list[nbr] >= 0 && recv_data[nbr][MAG] >= MAGNITUDE_THRESHOLD)
                {
                    dist_to_nbr = fabs(km_distance(recv_data[nbr][LAT], recv_data[nbr][LON], node_data[LAT], node_data[LON]));    // Distance
                    mag_diff = fabs(fabs(recv_data[nbr][MAG]) - fabs(node_data[MAG]));                                            // Differences in magnitude
                    dep_diff = fabs(fabs(recv_data[nbr][DEP]) - fabs(node_data[DEP]));                                            // Differences in depth
                }
                else {
                    recv_data[nbr][MAG] = -1;       // If neighbour does not exist in nbr direction, set recv_data[MAG] to -1
                }

                // =============== Push adjacent sensors' data into report ===============
                switch(nbr){
                    case UP:
                        report_data[U_RANK] = nbr_list[nbr];
                        report_data[U_MAG] = recv_data[nbr][MAG];
                        report_data[U_DIST] = dist_to_nbr;
                        report_data[U_MAG_DIFF] = mag_diff;
                        report_data[U_DEP_DIFF] = dep_diff;
                        break;
                    case DOWN:
                        report_data[D_RANK] = nbr_list[nbr];
                        report_data[D_MAG] = recv_data[nbr][MAG];
                        report_data[D_DIST] = dist_to_nbr;
                        report_data[D_MAG_DIFF] = mag_diff;
                        report_data[D_DEP_DIFF] = dep_diff;
                        break;
                    case LEFT:
                        report_data[L_RANK] = nbr_list[nbr];
                        report_data[L_MAG] = recv_data[nbr][MAG];
                        report_data[L_DIST] = dist_to_nbr;
                        report_data[L_MAG_DIFF] = mag_diff;
                        report_data[L_DEP_DIFF] = dep_diff;
                        break;
                    default: //RIGHT
                        report_data[R_RANK] = nbr_list[nbr];
                        report_data[R_MAG] = recv_data[nbr][MAG];
                        report_data[R_DIST] = dist_to_nbr;
                        report_data[R_MAG_DIFF] = mag_diff;
                        report_data[R_DEP_DIFF] = dep_diff;
                        break;
                }
            }
            
            // =============== Push own status ===============
            // NOTE : For simulation purpose, if report_data[STATUS] == FAULTY_SEAFLOOR_SENSOR == 1, it is faulty
            report_data[STATUS] = gen_rand_num(1, 50);
            
            MPI_Send(report_data, report_data_len, MPI_FLOAT, world_size-1, 0, MPI_COMM_WORLD);

            printf("Rank : %d - status : %d\n", current_rank, (int)report_data[STATUS]);
        }   // if ( 2 <= count_sensor_node && MAGNITUDE_THRESHOLD <= node_data[MAG] )  ends
    }   // while ends

    // Deallocate memory spaces
    for(int nbr = 0; nbr < NBR_NUM; nbr++){
        free(recv_data[nbr]);
    }
    free(recv_data);
    free(node_data);

    MPI_Comm_free(&Comm2D);
    
    // print appropriate messages and exit
    printf("Rank : %d - Safely exited.\n", current_rank);
    
    return 0;
}
