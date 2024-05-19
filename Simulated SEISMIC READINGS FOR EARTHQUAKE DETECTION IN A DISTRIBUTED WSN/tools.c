/**
 * @file tools.c
 * @author Shresth Mishra
 * @brief contains helper functions
 * @version 0.1
 * @date 2022-10-18
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "tools.h"

/**
 * Generates and returns random float number in the range of given parameters.
 * 
 * @param range_low a lower bound
 * @param range_high a upper bound
 * @return float randomly selected number
 */
float gen_rand_num(float range_low, float range_high)
{
    return range_low + 1.0 * rand() / RAND_MAX * (range_high - range_low);
}

/**
 * Turns degree into radian
 * Reference : https://www.geodatasource.com/developers/c
 * 
 * @param deg a float to be converted into radian
 * @return float in radian
 */
float deg2rad(float deg) {
    return (deg * pi / 180);
}
 
/**
 * Turns radian into degree
 * Reference : https://www.geodatasource.com/developers/c
 * 
 * @param rad a float to be converted into degree
 * @return float in degree 
 */
float rad2deg(float rad) {
    return (rad * 180 / pi);
}

/**
 * Calculates the distance between (lat1, lon1) and (lat2, lon2) 
 * Reference : https://www.geodatasource.com/developers/c
 * 
 * @param lat1 a float representing latitude
 * @param lon1 a float representing longitude
 * @param lat2 a float representing latitude
 * @param lon2 a float representing longitude
 * @return float representing the distance between such coordinates
 */
float km_distance(float lat1, float lon1, float lat2, float lon2)
{
    float theta, dist;
    if ((lat1 == lat2) && (lon1 == lon2))
    {
        return 0;
    }
    else
    {
        theta = lon1 - lon2;
        dist = sin(deg2rad(lat1)) * sin(deg2rad(lat2)) + cos(deg2rad(lat1)) * cos(deg2rad(lat2)) * cos(deg2rad(theta));
        dist = acos(dist);
        dist = rad2deg(dist);
        dist = dist * 60 * 1.1515;
        dist = dist * 1.609344;
        return (dist);
    }
}

/**
 * @brief Get the Current Time and set into tm object
 * 
 * @param pT a pointer to time_t object
 * @param pTm a pointer to tm object
 */
void getCurrentTime(time_t *pT, struct tm *pTm ){
    *pT = time(NULL);
    *pTm = *localtime(&(*pT));      // Sets the current system time
}
