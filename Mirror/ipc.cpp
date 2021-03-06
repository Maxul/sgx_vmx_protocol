#include "svmos.h"

int sem_id;

int set_semvalue(int sem_id) {
  union semun sem_union;

  sem_union.val = 1;
  if (semctl(sem_id, 0, SETVAL, sem_union) == -1)
    return 0;
  return 1;
}

int semaphore_p(int sem_id) {
  struct sembuf sem_b;

  sem_b.sem_num = 0;
  sem_b.sem_op = -1;
  sem_b.sem_flg = SEM_UNDO;
  if (semop(sem_id, &sem_b, 1) == -1) {
    printf("semaphore_p failed\n");
    return 0;
  }
  return 1;
}

int semaphore_v(int sem_id) {
  struct sembuf sem_b;

  sem_b.sem_num = 0;
  sem_b.sem_op = 1;
  sem_b.sem_flg = SEM_UNDO;
  if (semop(sem_id, &sem_b, 1) == -1) {
    printf("semaphore_v failed\n");
    return 0;
  }
  return 1;
}

