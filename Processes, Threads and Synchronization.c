#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "a2_helper.h"
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/sem.h>

pthread_t t5[6], t6[44], t7[5];
pthread_mutex_t lock, lock1;
pthread_cond_t cond, cond1;
int id5[6], id6[44], id7[5];
int condProcess7 = 0, condProcess6 = 0, th = 0, th_no = 0, th_wait = 0;
int sem_id;

void P(int semId, int semNr)
{
    struct sembuf op = {semNr, -1, 0};

    semop(semId, &op, 1);
}

void V(int semId, int semNr)
{
    struct sembuf op = {semNr, +1, 0};

    semop(semId, &op, 1);
}

void* thread7(void* arg){
	int *i = (int*)arg;

	if(*i == 4 && condProcess7 == 0){
		while(condProcess7 == 0){}
	}
	if(*i == 3){
		P(sem_id, 0);
	}
	info(BEGIN, 7, *i);

	if(*i == 2){
		condProcess7 = 1;
		pthread_join(t7[4], NULL);
	}

	if(*i == 4){
		while(condProcess7 == 0){}
	}


	info(END, 7, *i);

	if(*i == 3){
		V(sem_id, 1);
	}
	return NULL;
}

void* thread6(void* arg){
	int i = (*(int*)arg);

	pthread_mutex_lock(&lock);
	while(th >= 4){
		pthread_cond_wait(&cond, &lock);
	}
	th++;
	th_no++;
	pthread_mutex_unlock(&lock);
	info(BEGIN, 6, i);
	if(i == 10){
		pthread_mutex_lock(&lock);
		condProcess6 = 1;
		pthread_mutex_unlock(&lock);
		while(th_wait!=3){
			if(th_no == 43){
				th_wait = 3;
			}
		}
	}
	if(condProcess6 == 1 && i != 10){
		pthread_mutex_lock(&lock1);
		th_wait++;
		printf("%d - %d\n", th_wait, i);
		pthread_cond_wait(&cond1, &lock1);
		pthread_mutex_unlock(&lock1);
	}
	if(condProcess6 == 0 && th_no >= 41 && i != 10){
		pthread_mutex_lock(&lock1);
		th_wait++;
		pthread_cond_wait(&cond1, &lock1);
		pthread_mutex_unlock(&lock1);
	}

	info(END, 6, i);

	pthread_mutex_lock(&lock);
	th--;
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&lock);
	if(i == 10){
		condProcess6 = 2;
		pthread_cond_signal(&cond1);
		pthread_cond_signal(&cond1);
		pthread_cond_signal(&cond1);
	}
	return NULL;
}

void* thread5(void* arg){
	int i = (*(int*)arg);
	if(i == 2){
		P(sem_id, 1);
	}
	info(BEGIN, 5, i);
	info(END, 5, i);
	if(i == 3){
		V(sem_id, 0);
	}
	return NULL;
}

int main(){

	for(int i = 1; i <= 4; i++){
		id7[i] = i;
	}
    init();

    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&cond, NULL);
    pthread_mutex_init(&lock1, NULL);
    pthread_cond_init(&cond1, NULL);

    sem_id = semget(IPC_PRIVATE, 2, IPC_CREAT | 0600);
    if (sem_id < 0) {
        perror("Error creating the semaphore set");
        exit(2);
    }
    semctl(sem_id, 0, SETVAL, 0);
    semctl(sem_id, 1, SETVAL, 0);

    info(BEGIN, 1, 0);

    switch(fork()){
    	case -1: printf("Error creating process 2");
    	case 0:{
    		info(BEGIN, 2, 0);
    		switch(fork()){
    			case -1: printf("Error creating process 5");
    			case 0:{
    				info(BEGIN, 5, 0);
    				switch(fork()){
    					case -1: printf("Error creating process 6");
    					case 0:{
    						id6[10] = 10;
    						info(BEGIN, 6, 0);
    						for(int i = 1; i <= 43; i++){
    							id6[i] = i;
   								if (pthread_create(&t6[i], NULL, thread6, (void *)(id6 + i))!=0) {
									perror("Error creating a new thread");
   								}
							}
    						for(int i = 1; i <= 43; i++){
    							pthread_join(t6[i], NULL);
    						}
    						info(END, 6, 0);
    						_exit(1);
    						break;
    					}
    					default:{
    						for(int i = 1; i <= 5; i++){
    							id5[i] = i;
    							pthread_create(&t5[i], NULL, thread5, (void *)(id5 + i));
    						}
    						for(int i = 1; i <= 5; i++){
    							pthread_join(t5[i], NULL);
    						}
    						wait(NULL);
    						info(END, 5, 0);
    						_exit(1);
    					}
    				}
    				break;
    			}
    			default:{
    				wait(NULL);
    				info(END, 2, 0);
    				_exit(1);
    			}
    		break;
    		}
    	}
    	default:{
    		switch(fork()){
    			case -1: printf("Error creating process 3");
    			case 0:{
    				info(BEGIN, 3, 0);
    				switch(fork()){
    					case -1: printf("Error creating process 4");
    					case 0:{
    						info(BEGIN, 4, 0);
    						info(END, 4, 0);
    						_exit(1);
    						break;
    					}
    					default:{
    						switch(fork()){
    							case -1: printf("Error creating process 7");
    							case 0:{
    								info(BEGIN, 7, 0);
    								for(int i = 1; i <= 4; i++){
    									if (pthread_create(&t7[i], NULL, thread7, (void *)(id7 + i))!=0) {
											perror("Error creating a new thread");
    									}
    								}
    								for(int i = 1; i <= 4; i++){
    									pthread_join(t7[i], NULL);
    								}
    								info(END, 7, 0);
    								_exit(1);
    								break;
    							}
    							default:{}
    						}
    					}
    				}
    				wait(NULL);
    				wait(NULL);
    				info(END, 3, 0);
    				_exit(1);
    				break;
    			}
    			default:{
    				wait(NULL);
    				wait(NULL);
    				info(END, 1, 0);
    			}
    		}
    	}
    }

    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&lock1);
    pthread_cond_destroy(&cond1);
    semctl(sem_id, 0, IPC_RMID, 0);

    return 0;
}
