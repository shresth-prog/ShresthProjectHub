/**
 * @file base.c
 * @author Shresth Mishra
 * @brief Contains logics to execute base station and its threads
 * @version 0.1
 * @date 2022-10-18
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "base.h"

extern int termination_flag, num_seafloor_sensors, balloon_front;
extern const int report_data_len;  // Size of report_data. 
extern float *report_data;  // An array holding report data. Used in base station and all sensors.
extern pthread_mutex_t g_Mutex;

/**
 * Creates two threads for generating balloon seismic data and receving reports from seafloor sensors.
 * Once a faulty seafloor sensor or user's termination input is detected, all threads will terminate and the base station will also terminate.
 * 
 * @return int 0 for no issue
 */
int base_station_io()
{
    // Variable declarations
    pthread_t balloon_thread_id;                    // thread id for balloon data generating thread
    pthread_t seafloor_reading_thread_id;           // thread id for report receiving thread
    struct balloon *pBalloon;                       // pointer to an array to hold balloon seismic data
    
    // =============== Initialisation ===============
    pthread_mutex_init(&g_Mutex, NULL);                                                     // Mutual exclusion initialisation
    pBalloon = (struct balloon*) malloc(num_seafloor_sensors * sizeof(struct balloon));     // Dynamic memory for balloon seismic data

    // =============== Spawns threads ===============
    pthread_create(&balloon_thread_id, NULL, balloonFunc, pBalloon);                        // Create balloon data generating thread
    pthread_create(&seafloor_reading_thread_id, NULL, seafloorReadingFunc, pBalloon);       // Create report recving thread

    // =============== Merges threads ===============
    pthread_join(balloon_thread_id, NULL);                                                  // Merge balloon data generating thread
    pthread_join(seafloor_reading_thread_id, NULL);                                         // Merge report recving thread

    free(pBalloon);     // Deallocate memory

    // =============== Print appropriate messages and exit ===============
    printf("Base station (Main thread) - safely exit\n");
    printf("\n");

    return 0;
}

/**
 * Periodically generates seismic data for balloon. The data is implemented as queue.
 *
 * @param pArg a pointer referencing to the balloon data in the base station
 * @return void*
 */
void* balloonFunc(void *pArg) {
    // Variables 
    struct balloon *pBalloon = (struct balloon*)pArg;   // A pointer to balloon data 
    time_t t = time(NULL);                      
    struct tm tm = *localtime(&t);                      // tm for holding time

    // =============== Periodically generate seismic data until termination flag is detected ===============
    while(1){
        getCurrentTime(&t, &tm);                        // Get current system time
        
        struct balloon newData = {
            .yr = tm.tm_year + 1900,
            .mon = tm.tm_mon + 1,
            .day = tm.tm_mday,
            .hr = tm.tm_hour,
            .min = tm.tm_min,
            .sec = tm.tm_sec,
            .lat = gen_rand_num(-16, -15),
            .lon = gen_rand_num(165, 168),
            .mag = gen_rand_num(2.5, 10),               // For the simulation purpose, it always has minimum of 2.5 magnitude
            .dep = gen_rand_num(0, 10)
        };
        
        pthread_mutex_lock(&g_Mutex);                   // Get mutex lock before apply changes to global and shared variables
        balloon_front++;
        balloon_front = balloon_front % num_seafloor_sensors;
        pBalloon[balloon_front] = newData;              // Push new seismic data
        pthread_mutex_unlock(&g_Mutex);                 // Unlock mutex
        
        // Check for termination flag
        if(termination_flag){
            // Print appropriate messages and break the loop
            printf("Base station (Balloon data generating thread - Detected termination flag.\n");
            break;
        }
        
        sleep(1);
    }
    
    // Print appropriate messages and exit
    printf("Base station (Balloon data generating thread - Safely exited.\n");
    printf("\n");
    return 0;
}

/**
 * Periodically receives report from seafloor sensors, but also checks for user's termination input and faulty sensor.
 * If no termination signal is detected, it logs into the file
 * 
 * @param pArg a pointer to balloon data 
 * @return void* 
 */
void* seafloorReadingFunc(void *pArg) {
    // Variable declaration 
    FILE *logFile;                                          // Reference to the log file                                          
    FILE *userInputFile;                                    // Reference to the user input file                                          
    struct balloon *pBalloonData;                           // Corresponding balloon data when a report is received
    struct balloon *pBalloon = (struct balloon*)pArg;       // Pointer to balloon data
    int world_size, conclusive_counter = 0, userInput, termination_bcast = -1;
    MPI_Request termination_bcast_request;
    
    char currentTimeBuffer[26];                             // Holds current system time in a formatted structure.
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);                          // tm for holding current system time

    MPI_Comm_size(MPI_COMM_WORLD, &world_size);             // Get the number of all processes

    // Clean up the files and open it    
    fclose(fopen("logs.txt", "w"));         // Erase log.txt first
    logFile = fopen("logs.txt", "a");       // Then write information
    fclose(fopen("input.txt", "w"));        // Erase input.txt first


    // =============== Periodically receives report, checks user's sentinel value and faulty seafloor sensor ===============
    while(1){
        // =============== Prepare datasets for comparison ===============
        MPI_Recv(report_data, report_data_len, MPI_FLOAT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);      // Receive report
        pBalloonData = getFront(pBalloon);                                  // Retrieve the corresponding balloon data

        // =============== Checks for user's sentinel value ===============
        userInputFile = fopen("input.txt", "r");                            // Opens user input file
        fscanf(userInputFile, "%d ", &userInput);                           // Scans user input
        
        // If user wants to terminate the program
        if(userInput == USER_SENTINEL_VALUE){    
            // Print appropriate messages                       
            printf("Base station (Seafloor Reading Thread) - Detected user's sentinel value\n");
            
            // Set termination flag
            pthread_mutex_lock(&g_Mutex);                                   // Get mutex lock
            termination_flag = USER_SENTINEL_VALUE;                         // Set the termination flag
            pthread_mutex_unlock(&g_Mutex);                                     

            fclose(userInputFile);                                          // Close user input file
            break;
        }
        fclose(userInputFile);                                              // Close user input file


        // =============== Fault Detection ===============
        // If faulty seafloor sensor is detected
        if( ((int)report_data[STATUS]) == FAULTY_SEAFLOOR_SENSOR ){
            // Print appropriate messages                       
            printf("Base station (Seafloor Reading Thread) - Detected faulty seafloor sensor.\n");
             
            // Set termination flag
            pthread_mutex_lock(&g_Mutex);                                   // Get mutex lock
            termination_flag = USER_SENTINEL_VALUE;                         // Set the termination flag
            pthread_mutex_unlock(&g_Mutex);                                 // Set the termination flag
            break;
        }
        

        // =============== No user input or faulty sensor, log into the file ===============
        getCurrentTime(&t, &tm);                                            // Current system time
        strftime(currentTimeBuffer, 26, "%d-%m-%Y %H:%M:%S", &tm);          // Format the time
        
        // =============== Log into file ===============
        logIntoFile(logFile, pBalloonData, currentTimeBuffer, &conclusive_counter);
    }

     // =============== Log final information ===============
    // Log the cause of program termination
    if( userInput == USER_SENTINEL_VALUE ) {
        fprintf(logFile, ">> User stopped the program!!\n");
    } else {
        fprintf(logFile, ">> Seafloor sensor with rank %d has been detected as faulty sensor!!!\n", (int)report_data[NODE_RANK]);
    }
    printf("\n");

    fprintf(logFile, "Magnitude threashold for reporting : %.2f\n",MAGNITUDE_THRESHOLD);
    fprintf(logFile, "Magnitude Difference Threadshold: %d\n",MAG_DIFF_THRESHOLD);
    fprintf(logFile, "Depth Difference Threadshold: %d\n",DEP_DIFF_THRESHOLD);
    printf("\n");
    fprintf(logFile, "Total iteration of the system process: %d\n", (int)report_data[ITER_NUM]);
    fprintf(logFile, "Overall Conclusive Node Number: %d\n",conclusive_counter);
    fclose(logFile);


    // =============== Broadcast all seafloor sensors to terminate ===============
    printf("Base station (Seafloor Reading Thread) - Broadcast to all seafloor sensors to terminate.\n");
    MPI_Ibcast(&termination_bcast, 1, MPI_INT, world_size-1, MPI_COMM_WORLD, &termination_bcast_request);   // Broadcast to all seafloor sensors to terminate
    MPI_Wait(&termination_bcast_request, MPI_STATUS_IGNORE);

    // Print appropriate message and exit
    printf("Base station (Seafloor Reading Thread) - Safely exit.\n");
    printf("\n");

    return 0;
}


/**
 * Get the corresponding balloon data
 * 
 * @param pBalloon a pointer to an array holding balloon seismic data
 * @return struct balloon* a pointer to one balloon object
 */
struct balloon* getFront(struct balloon *pBalloon){
    pthread_mutex_lock(&g_Mutex);                                   // Get mutex lock
    struct balloon *pFrontBalloon = &pBalloon[balloon_front];       // Get one data
    pthread_mutex_unlock(&g_Mutex);                                 // Unlock mutex

    return pFrontBalloon;
}

/**
 * Pre-process useful information, determines whether conclusive or not and log into the file
 * 
 * @param logFile a reference to the log file
 * @param pBalloonData a pointer to one balloon data
 * @param currentTimeBuffer an array holding current time in a formatted structure
 * @param conclusive_counter a int pointer holding the total number of conclusive results
 */
void logIntoFile(FILE *logFile, struct balloon *pBalloonData, char *currentTimeBuffer, int *conclusive_counter){
    // Variable declarations
    float dist_to_node, mag_diff, dep_diff;     // Holding pre-processed information
    int isConclusive;

    // =============== Pre-process log data ===============
    dist_to_node = km_distance(pBalloonData -> lat, pBalloonData -> lon, report_data[LAT], report_data[LON]);   // Compute distance to the reporting seafloor sensor
    mag_diff = fabs(fabs(pBalloonData -> mag)-fabs(report_data[MAG]));                                          // Compute Magnitude difference to the reporting seafloor sensor
    dep_diff = fabs(fabs(pBalloonData -> dep)-fabs(report_data[DEP]));                                          // Compute Depth differnce to the reporting seafloor sensor
    
    // =============== Detemines whether the report is conclusive or not ===============
        // If the below conditions are met, it is considered to be the conclusive result
        if( dist_to_node < DIST_THESHOLD && mag_diff < MAG_DIFF_THRESHOLD && dep_diff < DEP_DIFF_THRESHOLD) {
            isConclusive = 1;
        } else {
            isConclusive = 0;
        }

    // ============================== Log into the logFile  ==============================
        // ============== Heading ==============
        fprintf(logFile, "--------------------------------------------------------------------------------------------------------------\n");
        fprintf(logFile, "Iterations : %d\n", (int)report_data[ITER_NUM]);
        fprintf(logFile, "Logged Time : %s\n", currentTimeBuffer);
        if(isConclusive) {
            (*conclusive_counter)++;
            fprintf(logFile, "Alert Type : Conclusive\n");
        } else {
            fprintf(logFile, "Alert Type : Inconclusive\n");
        }
        fprintf(logFile, "\n");


        // ============== Balloon seismic data ==============
        fprintf(logFile, ">> Balloon Seismic Reading\n");
        fprintf(logFile, "\tTime : %d-%d-%d %d:%d:%d\n",
            pBalloonData -> day, pBalloonData -> mon, pBalloonData -> yr, pBalloonData -> hr, pBalloonData -> min, pBalloonData -> sec
        );
        fprintf(logFile, "\n");
        fprintf(logFile, "\t%16s%20s%20s\n", "Coordinates", "Magnitude", "Depth");
        fprintf(logFile, "\t(%.2f, %.2f)%20.2f%20.2f\n",
            pBalloonData -> lat, pBalloonData -> lon, pBalloonData -> mag, pBalloonData -> dep
        );

        fprintf(logFile, "\n");
        fprintf(logFile, "%20s%30s%30s\n", "Dist to Node", "Mag Diff with Node", "Depth Diff with Node");
        fprintf(logFile, "%17.2f km%30.2f%30.2f\n",
            dist_to_node, mag_diff, dep_diff
        );
        fprintf(logFile, "\n");


        // ============== Seafloor sensor data ==============
        fprintf(logFile, ">> Seafloor Seismic Reading\n");
        fprintf(logFile, "\tTime : %.f-%.f-%.f %.f:%.f:%.f\n",
            report_data[DAY], report_data[MONTH], report_data[YEAR], report_data[HOUR], report_data[MIN], report_data[SEC]
        );
        fprintf(logFile, "%16s%20s%16s%16s%40s\n", "Rank", "Coordinates", "Magnitude", "Depth", "Number of NBRs with mag > threshold");
        fprintf(logFile, "%9.f(%.f, %.f)\t\t(%.2f, %.2f)%16.2f%16.2f%40.f\n",
            report_data[NODE_RANK], report_data[ROW], report_data[COL], report_data[LAT], report_data[LON], report_data[MAG], report_data[DEP], report_data[COUNTER_OVER_THRESHOLD]
        );
        
        fprintf(logFile, "\n");
        
        // ============== Seafloor sensor data - Adjacent sensors ==============
        fprintf(logFile, "\tAdjacent Nodes\n");
        fprintf(logFile, "\t\t%12s%20s%20s%20s%20s\n", "Rank", "Distance", "Magnitude", "Mag Difference", "Dep Difference");
        
        fprintf(logFile, "\t\tUP%10.f%20.2f%20.2f%20.2f%20.2f\n",
            report_data[U_RANK], report_data[U_DIST], report_data[U_MAG], report_data[U_MAG_DIFF], report_data[U_DEP_DIFF]
        );

        fprintf(logFile, "\t\tDOWN%8.f%20.2f%20.2f%20.2f%20.2f\n",
            report_data[D_RANK], report_data[D_DIST], report_data[D_MAG], report_data[D_MAG_DIFF], report_data[D_DEP_DIFF]
        );

        fprintf(logFile, "\t\tLEFT%8.f%20.2f%20.2f%20.2f%20.2f\n",
            report_data[L_RANK], report_data[L_DIST], report_data[L_MAG], report_data[L_MAG_DIFF], report_data[L_DEP_DIFF]
        );

        fprintf(logFile, "\t\tRIGHT%7.f%20.2f%20.2f%20.2f%20.2f\n",
            report_data[R_RANK], report_data[R_DIST], report_data[R_MAG], report_data[R_MAG_DIFF], report_data[R_DEP_DIFF]
        );

        fprintf(logFile, "--------------------------------------------------------------------------------------------------------------\n");
        fprintf(logFile, "\n\n");
        
        fflush(logFile);
}

