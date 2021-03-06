/******************************************************************
 * The helper file that contains the following helper functions:
 * thread_error_handler - handles errors within sem_wait and sem_signal 
 * sem_error_handler - handles erros within sem_create, sem_init, and sem_close
 * main_error_handler - handles errors related to pthread creation or join
 * check_arg - Checks if command line input is a number and returns it
 * sem_create - Create number of sempahores required in a semaphore array
 * sem_init - Initialise particular semaphore in semaphore array
 * sem_wait - Waits on a semaphore (akin to down ()) in the semaphore array
 * sem_signal - Signals a semaphore (akin to up ()) in the semaphore array
 * sem_close - Destroy the semaphore array
 ******************************************************************/

# include "helper.h"
#include <stdio.h>

void thread_error_handler (const int &id, const int &sem, string type) {
	/* Print an informative message depending on the errno flag and 
	exit the pthread.
	*/
	
	type[0] = toupper(type[0]);
	
	if (errno == EAGAIN && type == "Consumer") {
		fprintf(stderr, "Consumer(%i): No more jobs left.\n", id);
	} else if (errno == EAGAIN && type == "Producer") {
		fprintf(stderr, "Producer(%i): Quitting, because no space to add jobs.\n", id);
	} else if (id == -1) {
		fprintf(stderr, "%s: quit because of error: %s\n", type.c_str(), strerror(errno));	
	} else {
		fprintf(stderr, "%s(%i): quit because of error: %s\n", type.c_str(), id, strerror(errno));
	}
	pthread_exit(0);
}

void main_error_handler (const int i, string type) {
	/* Print an informative message about the errno flag raised. */
	fprintf(stderr, "%s number %d returns error: %s\n", type.c_str(), i, strerror(errno));
}

void sem_error_handler (const int sem, string func) {
	/* Print an informative message about the errno flag raised.
	close the sem safely and exit the pthread (main thread in all cases).
	*/

	fprintf(stderr, "%s exited with error: %s\n", func.c_str(), strerror(errno));
	if (func == "sem_init") 
		sem_close(sem);
	
	pthread_exit(0);
}

int check_arg (char *buffer)
{
  int i, num = 0, temp = 0;
  if (strlen (buffer) == 0)
    return -1;
  for (i=0; i < (int) strlen (buffer); i++)
  {
    temp = 0 + buffer[i];
    if (temp > 57 || temp < 48)
      return -1;
    num += pow (10, strlen (buffer)-i-1) * (buffer[i] - 48);
  }
  return num;
}

int sem_create (key_t key, int num)
{
  int id;
  if ((id = semget (key, num,  0666 | IPC_CREAT | IPC_EXCL)) < 0) {
  	sem_error_handler (id, __FUNCTION__);
    return -1;
  }
  return id;
}

int sem_init (int id, int num, int value)
{
  union semun semctl_arg;
  semctl_arg.val = value;
  if (semctl (id, num, SETVAL, semctl_arg) < 0) {
  	sem_error_handler (id, __FUNCTION__);
  	return -1;
  }
  return 0;
}

void sem_wait (int sem, int id, short unsigned int num, string caller)
{
  struct sembuf op[] = {
    {num, -1, SEM_UNDO}
  };
  if (semop (sem, op, 1))
  	thread_error_handler(id, sem, caller);
}

/* Overloaded sem_wait function timing out if no signal is received within 20s. 
thread_error_handler will print an informative message if semtimedop returned 
because of a time out. 
*/
void sem_wait (int sem, int id, short unsigned int num, int seconds, string caller)
{
  struct sembuf op[] = {
    {num, -1, SEM_UNDO}
  };
  const timespec time_struct = {seconds};
  if (semtimedop (sem, op, 1, &time_struct)) 
  	thread_error_handler(id, sem, caller); 
}

void sem_signal (int sem, int id, short unsigned int num, string caller)
{
  struct sembuf op[] = {
    {num, 1, SEM_UNDO}
  };
  if (semop (sem, op, 1))
  	thread_error_handler(id, sem, caller);
}

int sem_close (int id)
{
  if (semctl (id, 0, IPC_RMID, 0) < 0){
  	sem_error_handler (id, __FUNCTION__);
  	return -1;
  }
  return 0;
}
