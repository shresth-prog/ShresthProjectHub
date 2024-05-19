

#ifndef TOOLS_H
#define TOOLS_H
#define SHIFT_ROW 0
#define SHIFT_COL 1
#define DISP 1
#define MAGNITUDE_THRESHOLD 2.5
#define pi 3.14159265358979323846
#define NBR_NUM 4
#define INFO_LEN 12
#define NDIMS 2
#define NBR_PROCESS_LEN 5

#define DIST_THESHOLD 100
#define DEP_DIFF_THRESHOLD 5
#define MAG_DIFF_THRESHOLD 5

#define USER_SENTINEL_VALUE 9
#define FAULTY_SEAFLOOR_SENSOR 1

#include <stdio.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define pi 3.14159265358979323846

enum node_data {
    // Seafloor seismic data for itself
    YEAR = 2, MONTH, DAY, HOUR, MIN, SEC, LAT, LON, MAG, DEP,
    NODE_RANK, CART_RANK, COUNTER_OVER_THRESHOLD, ITER_NUM, STATUS,

    // Adjacent seafloor sensors' data
    U_RANK, U_MAG, U_DIST, U_MAG_DIFF, U_DEP_DIFF,
    D_RANK, D_MAG, D_DIST, D_MAG_DIFF, D_DEP_DIFF,
    L_RANK, L_MAG, L_DIST, L_MAG_DIFF, L_DEP_DIFF,
    R_RANK, R_MAG, R_DIST, R_MAG_DIFF, R_DEP_DIFF
};

enum grid {
    ROW, COL
};
enum direction {
    UP, DOWN, LEFT, RIGHT
};


float gen_rand_num(float , float );
float deg2rad(float );
float rad2deg(float );
float km_distance(float , float , float , float );
void getCurrentTime(time_t*, struct tm* );

#endif /* tools_h */
