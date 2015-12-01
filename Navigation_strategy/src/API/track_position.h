#ifndef TRACK_POSITION
#define TRACK_POSITION_H

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>
#include "find_position.h"

//shared variable of position of the beacon
extern t_position pos;

//mutex arbitrating computing and tracking
extern pthread_mutex_t compute_pos_mux;
extern pthread_mutex_t track_pos_mux;

//used to prototype track_position
//prints the position of the beacon anytime the thread is active
void * print_position(void * arg);

//function designed to be the main of a thread
//send the movement orders of the drone in order to follow the beacon
void * track_position(void * arg);

#endif // TRACK_POSITION_H
