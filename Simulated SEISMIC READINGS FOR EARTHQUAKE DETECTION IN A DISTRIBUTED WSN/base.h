

#ifndef BASE_H
#define BASE_H

#include <stdio.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include "tools.h"

struct balloon {
    int yr, mon, day, hr, min, sec;
    float lat, lon, mag, dep;
};

struct balloon* getFront(struct balloon*);
void* balloonFunc(void*);
void* seafloorReadingFunc(void*);
int base_station_io();
void logIntoFile(FILE*, struct balloon*, char*, int*);


#endif /* base_h */
