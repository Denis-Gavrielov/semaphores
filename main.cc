/****************************************************************** 
The Main
 * program with the two functions. A simple example of creating and using a
 * thread is provided.
 ******************************************************************/

#include "helper.h"

using namespace std::chrono;

void *producer (void *id); 
void *consumer (void *id);
void *consumerTimeOut (void *time_out);

struct job { // initialise values here?
	int sem;
	int jobs;
	int* producer_id = new int(1);
	int* consumer_id = new int(1);
	int duration;
	int* queue;
	int* tail = new int(0);
	int* head = new int(0);
	int queue_size; // needed?
};

const int MAX_SLEEP = 20;

int main (int argc, char **argv) {

	int queue_size = check_arg(argv[1]);
	int jobs = check_arg(argv[2]);
	int producers = check_arg(argv[3]);
	int consumers = check_arg(argv[4]);
	int sem = sem_create(SEM_KEY, 6); 

	// check if init worked
	if (sem_init(sem, 0, 1)) // mutex
		cout << "SEMAPHORE NOT CREATED" << endl;
	if (sem_init(sem, 1, queue_size)) // queue not full
		cout << "SEMAPHORE NOT CREATED" << endl;
	if (sem_init(sem, 2, 0)) // queue not empty
		cout << "SEMAPHORE NOT CREATED" << endl;
	if (sem_init(sem, 3, 1)) // printing
		cout << "SEMAPHORE NOT CREATED" << endl;
	if (sem_init(sem, 4, 1)) // producer_id allocation
		cout << "SEMAPHORE NOT CREATED" << endl;
	if (sem_init(sem, 5, 1)) // consumer_id allocation
		cout << "SEMAPHORE NOT CREATED" << endl;

	pthread_t producerid[producers];
	pthread_t consumerid[consumers];

	job next_job;

	next_job.sem = sem;
	next_job.jobs = jobs;
	next_job.queue = new int[queue_size];
	next_job.queue_size = queue_size;
	
	
	for (int i = 0; i < producers; i++) {
		pthread_create (&producerid[i], NULL, producer, (void *) &next_job);
	}
	
	for (int i = 0; i < consumers; i++) {
		pthread_create (&consumerid[i], NULL, consumer, (void *) &next_job);
	}


	for (int i = 0; i < producers; i++) {
  		pthread_join (producerid[i], NULL);
//		cout << " PRODUCER " << i << " JOINED " << endl; // to delete
	}
	
	for (int i = 0; i < consumers; i++){ 
		pthread_join (consumerid[i], NULL);
//		cout << " CONSUMER " << i << " JOINED " << endl; // to delete
	}
	sem_close(sem); // alternatively/manually ipcrm -s [number from ipcs]

	delete next_job.producer_id;
	delete next_job.consumer_id;
	delete [] next_job.queue;
	delete next_job.tail;
	delete next_job.head;

  	return 0; 
}

void *producer (void *next_job) {

	job *current_job = (job *) next_job; // call job_p later 
	int *head = current_job->head;

	int duration, job, id, jobs, sem;
	
	sem = current_job->sem;
	jobs = current_job->jobs;
	
	sem_wait(sem, 4);
	id = *(current_job->producer_id);
	*(current_job->producer_id) = *(current_job->producer_id) + 1;
	sem_signal(sem, 4);
	
	while (jobs) {
		jobs--;
		sleep(rand() % 5 + 1);
		duration = rand() % 10 + 1; 
		
		if (sem_wait(sem, 1, MAX_SLEEP))
			break;		
		
		sem_wait(sem, 0);
		current_job->queue[*head] = duration;
		job = *head; // maybe change to plus one 
		*(current_job->head) = (*head + 1) % (current_job->queue_size);
		sem_signal(sem, 0);
		sem_signal(sem, 2);

		sem_wait(sem, 3);
		cout << "Producer(" << id << "): Job id " << job << " duration " << duration << endl;
		sem_signal(sem, 3);
	
	}
	sem_wait(sem, 3);
	cout << "Producer(" << id << "): No more jobs to generate." << endl;
	sem_signal(sem, 3);
	pthread_exit(0);
}

void *consumer (void *next_job) {

	job *current_job = (job *) next_job;
	int *tail = current_job->tail;
	int duration, job, id, sem;

	sem = current_job->sem;
	sem_wait(sem, 5);
	id = *(current_job->consumer_id);
	*(current_job->consumer_id) = *(current_job->consumer_id) + 1;
	sem_signal(sem, 5);

	while (1) {
	
		if (sem_wait(sem, 2, MAX_SLEEP))
			break;

		sem_wait(sem, 0);
		duration = current_job->queue[*tail];
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
	
	sem_wait(sem, 3);
	cout << "Consumer(" << id << "): No more jobs left." << endl;
	sem_signal(sem, 3);

	pthread_exit (0);
}

