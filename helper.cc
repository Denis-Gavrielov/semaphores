/******************************************************************
 * The helper file that contains the following helper functions:
 * check_arg - Checks if command line input is a number and returns it
 * sem_create - Create number of sempahores required in a semaphore array
 * sem_init - Initialise particular semaphore in semaphore array
 * sem_wait - Waits on a semaphore (akin to down ()) in the semaphore array
 * sem_signal - Signals a semaphore (akin to up ()) in the semaphore array
 * sem_close - Destroy the semaphore array
 ******************************************************************/

# include "helper.h"
#include <stdio.h>
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

void thread_error_handler (const int &id, const int &sem, string type) {
	
	type[0] = toupper(type[0]);
		
	if (errno == EAGAIN && type == "Consumer") {
		printf("Consumer(%i): No more jobs left.\n", id);
	} else if (errno == EAGAIN && type == "Producer") {
		printf("Producer(%i): Quitting, because no space to add jobs.\n", id);
	} else {
		printf("%s(%i): quit because of error: %s\n", type.c_str(), id, strerror(errno));
	}
	pthread_exit(0);
}

int sem_create (key_t key, int num)
{
  int id;
  if ((id = semget (key, num,  0666 | IPC_CREAT | IPC_EXCL)) < 0)
    return -1;
  return id;
}

int sem_init (int id, int num, int value)
{
  union semun semctl_arg;
  semctl_arg.val = value;
  if (semctl (id, num, SETVAL, semctl_arg) < 0)
    return -1;
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

/* Overloaded sem_wait function returning 0 if signal was received, 
else -1.*/
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
  if (semctl (id, 0, IPC_RMID, 0) < 0)
    return -1;
  return 0;
}
