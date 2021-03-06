#include "track_position.h"

extern int keepRunning;
extern pthread_mutex_t compute_pos_mux;
extern pthread_mutex_t track_pos_mux;

//handler for a signal
void intHandlerThread3(int sig){
	keepRunning=0;
	printf("CTRL+C signal in track_position\n");
}

/**
 *	@brief	Print current position of the emitter on standard output
 */
void print_position(void)
{
	time_t rawtime;
	struct tm * timeinfo;
	char buffer [80];

	time (&rawtime);
	timeinfo = localtime (&rawtime);
	strftime (buffer, 80, "%X", timeinfo);

	// @todo make a mutex especially for the global variable pos
	if(pos.signalDetected)
		printf("%s > Angle : %d - Distance : %d \n", buffer, pos.angle, pos.distance);
	else
		printf("%s > No signal\n", buffer);
}

/**
 * @brief	function designed to be the main of a thread
 *			Sends movement commands to the drone in order to follow the beacon
 */
void * track_position(void * arg){

	// check time interval
    struct timeval old_tv = {0};
    struct timeval tv = {0};
    long unsigned int elapsed_time = 0; // in microsecondss
    
    // moves
    char message [512];
	int tps = 1;
	int wait =1;

	//handle the ctrl -c to make the drone land
	struct sigaction act;
	memset(&act,0,sizeof(act));
	act.sa_handler = intHandlerThread3;
	sigaction(SIGINT, &act, NULL);

    gettimeofday(&old_tv, NULL); 

	if (init_socket() != 0)
    {
        printf("[FAILED] Socket initialization failed\n");
    }
    else
    {

    	//////////////////////////////////////////////////////////
    	//	TAKING OFF 
    	//////////////////////////////////////////////////////////
		sleep(1);
        printf("Drone starts flying...\n");
		set_trim(message, wait);
		
		printf("Taking off...\n");
		while(tps < 167)
		{
			take_off(message, wait);
			tps++;
		}
		
		//stop waiting 40 us after a command send
		wait = 0;

		// Go up to be at shoulder level
		printf("Going up...\n");
		gettimeofday(&old_tv, NULL);
		elapsed_time = 0;

		while(elapsed_time < 2)
		{
			set_simple_move(message, UP, 1, wait);
			gettimeofday(&tv, NULL);
			elapsed_time = (tv.tv_sec-old_tv.tv_sec);
		}

		set_simple_move(message, UP, 0.0, wait);
		elapsed_time = 35000;

		while(keepRunning){
		    while (elapsed_time < 35000)
			{
				gettimeofday(&tv, NULL);
				elapsed_time = (tv.tv_sec-old_tv.tv_sec)*1000000 + (tv.tv_usec-old_tv.tv_usec);
			}

			pthread_mutex_lock(&track_pos_mux);

			print_position();
			
			///////////////////////////////////////////////////////////////////////
			// MOVES TO HAVE THE RIGHT ANGLE AND RIGHT DISTANCE FROM THE EMIITER
			///////////////////////////////////////////////////////////////////////
			reset_com(message, wait);

			//If no signal has been detected
			if(!pos.signalDetected)
			{
				// stop moving
				set_simple_move(message, FRONT, 0, wait);
			}
			// If a signal has been detected, move !
			else
			{

				if(pos.angle >= -ANGLE_PRECISION/2 && pos.angle <= ANGLE_PRECISION/2)
				{
					// For now, always move forward when the source in front of the drone
					set_simple_move(message, FRONT, 0.05, wait);

					// And now manage distance
					// if(pos.distance > 200) // in cm
					// 	set_simple_move(message, FRONT, 0.05, wait);
					// else if(pos.distance < 180) // in cm
					// 	set_simple_move(message, BACK, 0.05, wait);
					// else
					// 	set_simple_move(message, FRONT, 0, wait);	
				}				
	      	 	else if (pos.angle > ANGLE_PRECISION/2)
					set_simple_move(message, CLKWISE, 0.5, wait);
				else
					set_simple_move(message, ANTI_CLKWISE, 0.5,wait);
				
			}
			pthread_mutex_unlock(&compute_pos_mux);

			gettimeofday(&old_tv, NULL);
			elapsed_time=0;
		}

		///////////////////////////////////////////
		// LANDING
		///////////////////////////////////////////
		landing(message, wait);
		sleep(1);

	}
	pthread_exit(NULL);
}
