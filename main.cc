/****************************************************************** 
The Main
 * program with the two functions. A simple example of creating and using a
 * thread is provided.
 ******************************************************************/

#include "helper.h"

void *producer (void *id); 
void *consumer (void *id);
void *consumerTimeOut (void *time_out);

struct job { // initialise values here? 
	int jobs;
	int* producer_id; // for producer and consumer
	int* consumer_id;
	int duration;
	int* queue;
	int* tail;
	int* head;
	int queue_size;
	clock_t* time_last_produced = new clock_t;
	clock_t* time_last_consumed = new clock_t;
};

struct TimeOut {

	clock_t* time_stamp;
	bool* expired;
};

using namespace std::chrono;
// need 3 semaphores
int sem = sem_create(SEM_KEY, 6); 
/* note: could also be put into main and have pointer pointing 
to the sem (?)
don't need pointer actually as this is like a constant.
*/

int main (int argc, char **argv) {

	/* 
	Input parameters:
		* size of queue
		* number of jobs per producer
		* number of producers
		* number of consumers
	*/	
	int queue_size = 3;
	int jobs = 3;
	int producers = 10;
	int consumers = 1;

	// check if init worked
	int returns = sem_init(sem, 0, 1); // mutex
	if (returns)
		cout << "SEMAPHORE NOT CREATED" << endl;
	returns = sem_init(sem, 1, queue_size); // queue not full
	if (returns)
		cout << "SEMAPHORE NOT CREATED" << endl;
	returns = sem_init(sem, 2, 0); // queue not empty
	if (returns)
		cout << "SEMAPHORE NOT CREATED" << endl;
	returns = sem_init(sem, 3, 1); // printing
	if (returns)
		cout << "SEMAPHORE NOT CREATED" << endl;
	returns = sem_init(sem, 4, 1); // producer_id allocation
	if (returns)
		cout << "SEMAPHORE NOT CREATED" << endl;
	returns = sem_init(sem, 5, 1); // consumer_id allocation
	if (returns)
		cout << "SEMAPHORE NOT CREATED" << endl;

	pthread_t producerid[producers];
	pthread_t consumerid[consumers];

	job next_job;

	next_job.jobs = jobs;
	next_job.queue = new int[queue_size];
	next_job.tail = new int(0);
	next_job.head = new int(0);
	next_job.queue_size = queue_size;
	next_job.producer_id = new int(1);
	next_job.consumer_id = new int(1);
	

	/* NOTE: potential issue: we will always pass the same struct, so whatever
	 * change we make to the struct will be global for all the other structs.
	 * Thus we need to make sure that I actually pass different structs every
	 * time. E.g. by creating new structs within the for loop and changing the
	 * value of id.
	 * Maybe can do this more memory lightweight by having producer_id (need to split)
	 * as a pointer to something on the heap that every producer will read and
	 * then increment by one (could use semaphore to block) and then every
	 * producer have their own id. 
	*/

//	cout << "trying clock stuff" << endl;

//	cout << "It took " << nseconds << " seconds. " << endl;

//	cout << " end trying clock stuff ---------------" << endl;

	for (int i = 0; i < producers; i++) {
		pthread_create (&producerid[i], NULL, producer, (void *) &next_job);
	}
	for (int i = 0; i < consumers; i++) {
		pthread_create (&consumerid[i], NULL, consumer, (void *) &next_job);
	}

	for (int i = 0; i < producers; i++) {
  		pthread_join (producerid[i], NULL);
		cout << " PRODUCER " << i << " JOINED " << endl;
	}
	for (int i = 0; i < consumers; i++){ 
		pthread_join (consumerid[i], NULL);
		cout << " CONSUMER " << i << " JOINED " << endl;
	}
	sem_close(sem); // alternatively/manually ipcrm -s [number from ipcs]

	// don't forget to clear the memory from heap;
  	return 0; 
}

void *producer (void *next_job) {
	/*	
	Initialise parameters 
	add jobs into the jobs (randomly every 1-5 seconds) 
	If job is taken by consumer -> can add another one with same id (so basically
	if we free up the space) 
		* duration of job: 1-10s
	
	Block if the queue is full -> quit if not available after 20s. 
	* print the status:
		* Producer(1): Job id 0 duration 9
		* Producer(1): No more jobs to generate.


	quit if no more jobs left to produce.
	*/

	job *current_job = (job *) next_job; // call job_p later 
	int *head = current_job->head;

	// produce duration and ID of the job before we put it into the queue we
	// sleep.

	int duration, job, id, jobs;
	clock_t time_stamp;
	
	jobs = current_job->jobs;
	sem_wait(sem, 4);
	id = *(current_job->producer_id);
	*(current_job->producer_id) = *(current_job->producer_id) + 1;
	sem_signal(sem, 4);
	
	while (jobs) {
		jobs--;
		sleep(rand() % 5 + 1);
		duration = rand() % 10 + 1; 
		
		while (1) {
			time_stamp = *(current_job->time_last_consumed); // timestamp 
			steady_clock::time_point clock_begin = steady_clock::now();
			
			sem_wait(sem, 1, 20); // end this wait if wait for 20s and quit consumer
			
			steady_clock::time_point clock_end = steady_clock::now();
			steady_clock::duration time_span = clock_end - clock_begin;
			double nseconds = double(time_span.count()) * steady_clock::period::num / steady_clock::period::den;
//			cout << (int) *(current_job->time_last_produced) << " timestamp clock_t " << endl;
			if (time_stamp == *(current_job->time_last_consumed) && nseconds >=20 ) {
				cout << "Consumer(" << id << "): No more jobs left" << endl;
				pthread_exit (0); 
			} else if (time_stamp != *(current_job->time_last_consumed) && nseconds >= 20) 
				continue;
			else 
				break;
		}
		
		
		sem_wait(sem, 0);
			
		current_job->queue[*head] = duration;
		*(current_job->time_last_produced) = clock(); 
		job = *head; // maybe change to plus one 
		*(current_job->head) = (*head + 1) % (current_job->queue_size);
		sem_signal(sem, 0);
		sem_signal(sem, 2);

		sem_wait(sem, 3);
		cout << "Producer(" << id << "): Job id " << job << " duration " << duration << endl;
		sem_signal(sem, 3);
// add cout No more jobs to generate. 
	}
	pthread_exit(0);
}

void *consumer (void *next_job) {
	/*
	* take job from circular queue -> 'sleep' for duration.
	* block if queue is empty. If blocked for 20s -> quit.
	* print the status:
		* Consumer(1): Job id 0 executing sleep duration 9
		* Consumer(3): No more jobs left. 

	* if no jobs -> wait for 20s and then quit.  ( NOTE: implement this delay as
	* part of a semaphore ). 
	*/

	job *current_job = (job *) next_job;
	int *tail = current_job->tail;
	int duration, job, id;
	bool* expired = new bool(0);
	clock_t time_stamp;
	
	sem_wait(sem, 5);
	id = *(current_job->consumer_id);
	*(current_job->consumer_id) = *(current_job->consumer_id) + 1;
	sem_signal(sem, 5);

	while (1) {
	
		while (1) {	
			time_stamp = *(current_job->time_last_produced); // timestamp 
			steady_clock::time_point clock_begin = steady_clock::now();
			
			sem_wait(sem, 2, 20); // end this wait if wait for 20s and quit consumer
			
			steady_clock::time_point clock_end = steady_clock::now();
			steady_clock::duration time_span = clock_end - clock_begin;
			double nseconds = double(time_span.count()) * steady_clock::period::num / steady_clock::period::den;
//			cout << (int) *(current_job->time_last_produced) << " timestamp clock_t " << endl;
			if (time_stamp == *(current_job->time_last_produced) && nseconds >=20 ) {
				cout << "Consumer(" << id << "): No more jobs left" << endl;
				pthread_exit (0); 
			} else if (time_stamp != *(current_job->time_last_produced) && nseconds >= 20) 
				continue;
			else 
				break;
		}

		sem_wait(sem, 0);
		duration = current_job->queue[*tail];
		*(current_job->time_last_consumed) = clock(); // timestamp
		job = *tail;
		*(current_job->tail) = (*tail + 1) % (current_job->queue_size); 
		sem_signal(sem, 0);
		sem_signal(sem, 1);

		sem_wait(sem, 3);
		cout << "Consumer(" << id << "): Job id " << job << " executing sleep"
			<< " duration " << duration << endl;
		sem_signal(sem, 3);

		sleep(duration);
		
		sem_wait(sem, 3);
		cout << "Consumer(" << id << "): Job id " << job << " completed" << endl;
		sem_signal(sem, 3);
		
	}
	pthread_exit (0);

}

