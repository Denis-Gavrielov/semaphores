/****************************************************************** 
The Main
 * program with the two functions. A simple example of creating and using a
 * thread is provided.
 ******************************************************************/

#include "helper.h"

void *producer (void *id); 
void *consumer (void *id);

struct job { 
	int sem;
	int jobs;
	int* producer_id = new int(1);
	int* consumer_id = new int(1);
	int* queue;
	int* tail = new int(0);
	int* head = new int(0);
	int queue_size; 
};

const int MAX_WAIT = 5; // change to 20

int main (int argc, char **argv) {

	if (argc != 5) {
		cerr << "Invalid amount of input parameters specified (need 4)." << endl;
		return -1;
	} 
	for (int i = 1; i <= 4; i++) {
		if (check_arg(argv[i]) == -1) {
			cout << "Wrong input parameter! (For number " << i << ")" << endl;
			return -1;
		}
	}
	int queue_size = check_arg(argv[1]);
	int jobs = check_arg(argv[2]);
	int producers = check_arg(argv[3]);
	int consumers = check_arg(argv[4]);

	int sem = sem_create(SEM_KEY, 6); 

	// test if init worked
	if (sem_init(sem, 0, 1)) // mutex
		cerr << "SEMAPHORE 0 NOT CREATED" << endl;
	if (sem_init(sem, 1, queue_size)) // queue not full
		cerr << "SEMAPHORE 1 NOT CREATED" << endl;
	if (sem_init(sem, 2, 0)) // queue not empty
		cerr << "SEMAPHORE 2 NOT CREATED" << endl;
	if (sem_init(sem, 3, 1)) // printing
		cerr << "SEMAPHORE 3 NOT CREATED" << endl;
	if (sem_init(sem, 4, 1)) // producer_id allocation
		cerr << "SEMAPHORE 4 NOT CREATED" << endl;
	if (sem_init(sem, 5, 1)){ // consumer_id allocation
		cerr << "SEMAPHORE 5 NOT CREATED" << endl;
		cerr << "-----------------NOTE: if all 6 semaphores are not "
			<< "created, try to delete the "
			<< "last semaphore with ipcrm -s [id].---------------" << endl;
	}

	pthread_t producerid[producers];
	pthread_t consumerid[consumers];

	job my_job;

	my_job.sem = sem;
	my_job.jobs = jobs;
	my_job.queue = new int[queue_size];
	my_job.queue_size = queue_size;
	
	
	for (int i = 0; i < producers; i++) {
		if (pthread_create (&producerid[i], NULL, producer, (void *) &my_job))
			main_error_handler(i, "Creating producer thread");
	}	

	for (int i = 0; i < consumers; i++) {
		if (pthread_create (&consumerid[i], NULL, consumer, (void *) &my_job)) 
			main_error_handler(i, "Creating consumer thread");
	}

	for (int i = 0; i < producers; i++) {
  		if (pthread_join (producerid[i], NULL)) 
			main_error_handler(i, "Joining producer thread");
	}
	for (int i = 0; i < consumers; i++) {
		if (pthread_join (consumerid[i], NULL))
			main_error_handler(i, "Joining consumer thread");
	}
	sem_close(sem);
	// try two sem_close in a row. 

	delete my_job.producer_id;
	delete my_job.consumer_id;
	delete [] my_job.queue;
	delete my_job.tail;
	delete my_job.head;

  	return 0; 
}

/*
idea: have the sem_wait function calling the handler immediately, if 
semop returns -1. then the handler prints an informative message and exits
from the thread. Need to overload the sem_wait then to take in an int for just 
sem in the case of the sem_wait around the id allocation, and a struct with
additionally the job id. 

*/
void *producer (void *my_job) {

	job *job_p = (job *) my_job;
	int *head = job_p->head;
	int duration, job, id, jobs, sem;
	
	id = -1; 

	sem = job_p->sem;
	jobs = job_p->jobs;
	
	sem_wait(sem, id, 4);
	id = *(job_p->producer_id);
	*(job_p->producer_id) = *(job_p->producer_id) + 1;
	sem_signal(sem, id, 4);
	
	while (jobs) {
		jobs--;
		sleep(rand() % 5 + 1);
		duration = rand() % 10 + 1; 
		
		sem_wait(sem, id, 1, MAX_WAIT);
		sem_wait(sem, id, 0);

		job_p->queue[*head] = duration;
		job = *head; 
		*(job_p->head) = (*head + 1) % (job_p->queue_size);
		
		sem_signal(sem, id, 0);
		sem_signal(sem, id, 2);

	//	cout << "Producer(" << id << "): Job id " << job + 1 
	//		<< " duration " << duration << endl;
		printf("Producer(%d): Job id %d duration %d\n", id, job + 1, duration);	
	}
//	if (jobs) {
//		if (sem_wait(sem, 3)) {
//			thread_error_handler (id, sem);
//			pthread_exit (0);
//		}
//		cout << "Producer(" << id << "): Quitting, because no space to add jobs."
//		<< endl;
//		sem_signal(sem, 3);
//	} else {
//		sem_wait(sem, 3);
//		cout << "Producer(" << id << "): No more jobs to generate." << endl;
//		sem_signal(sem, 3);
//	}
	printf("Producer(%d): No more jobs to generate.\n", id);

	pthread_exit(0);
}

void *consumer (void *my_job) {

	job *job_p = (job *) my_job;
	int *tail = job_p->tail;
	int duration, job, id, sem;
	
	id = -1;

	sem = job_p->sem;
	sem_wait(sem, id, 5); // extra error handler for without id
	id = *(job_p->consumer_id);
	*(job_p->consumer_id) = *(job_p->consumer_id) + 1;
	sem_signal(sem, id, 5);

	while (1) {
	
		sem_wait(sem, id, 2, MAX_WAIT);
		sem_wait(sem, id, 0);

		duration = job_p->queue[*tail];
		job = *tail;
		*(job_p->tail) = (*tail + 1) % (job_p->queue_size); 
		
		sem_signal(sem, id, 0);
		sem_signal(sem, id, 1);

		printf("Consumer(%d): Job id %d executing sleep\n", id, job + 1);
		
		sleep(duration);
		
		printf("Consumer(%d): Job id %d completed\n", id, job + 1);	
		
	}
}

