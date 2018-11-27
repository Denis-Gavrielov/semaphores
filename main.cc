/****************************************************************** 
The Main
 * program with the two functions. A simple example of creating and using a
 * thread is provided.
 ******************************************************************/

#include "helper.h"

void *producer (void *id); 
void *consumer (void *id);

struct job {
	int job_id;
	int duration;
};

// need 3 semaphore
int sem = sem_create(SEM_KEY, 3); 

int main (int argc, char **argv) {

	/* 
	Input parameters:
		* size of queue
		* number of jobs per producer
		* number of producers
		* number of consumers
	*/	
	int queue_size = 5;
	int jobs = 20;
	int producers = 10;
	int consumers = 10;

	// mutex, buffer not full, buffer not empty
	sem_init(sem, 0, 1); // mutex
	sem_init(sem, 1, queue_size); // queue not full
	sem_init(sem, 2, 0); // queue not empty

	pthread_t producerid;
	job next_job;
	next_job.duration = 6; // should be random number

	pthread_create (&producerid, NULL, producer, (void *) &next_job);

  	pthread_join (producerid, NULL);

  	return 0; 
}

void *producer (void *next_job) {
	/*	
	Initialise parameters 
	add jobs into the jobs (randomly every 1-5 seconds) 
	If job is taken by consumer -> can ad another one with same id (so basically
	if we free up the space) 
		* duration of job: 1-10s
	
	Block if the queue is full -> quit if not available after 20s. 
	* print the status:
		* Producer(1): Job id 0 duration 9
		* Producer(1): No more jobs to generate.


	quit if no more jobs left to produce.
	*/

	job *current_job = (job *) next_job;

	cout << "Parameter = " << current_job->duration << endl;

	pthread_exit(0);
}

void *consumer (void *id) {
	/*
	* take job from circular queue -> 'sleep' for duration.
	* block if queue is empty. If blocked for 20s -> quit.
	* print the status:
		* Consumer(1): Job id 0 executing sleep duration 9
		* Consumer(3): No more jobs left. 

	* if no jobs -> wait for 20s and then quit.  ( NOTE: implement this delay as
	* part of a semaphore ). 
	*/

	pthread_exit (0);

}
