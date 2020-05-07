#include "lsm.h"

static ThreadPool *pool = NULL;

void CreateThreadPool(int threadnumber){
	pool = (ThreadPool *) malloc(sizeof(ThreadPool));
	pthread_mutex_init(&(pool->lock), NULL);
	pthread_cond_init(&(pool->ready), NULL);
	pool->threadnumber = threadnumber;
	pool->head = NULL;
	pool->count = 0;
	pool->shutdown = false;
	pool->threadid = (pthread_t *) malloc(threadnumber * sizeof(pthread_t));
	int i;
	for(i = 0; i < threadnumber; i++){
		pthread_create(&(pool->threadid[i]), NULL, ThreadRoutine , NULL);
	}
}

void AddToPool(void *(*process) (void *arg), void *arg){
	Worker *newworker = (Worker *) malloc(sizeof(Worker));
	newworker->process = process;
	newworker->arg = arg;
	newworker->next = NULL;
	pthread_mutex_lock(&(pool->lock));
	Worker *previous = pool->head;
	if(previous != NULL){
		while(previous->next != NULL){
			previous = previous->next;
		}
		previous->next = newworker;
	}else{
		pool->head = newworker;
	}
	//assert(pool->head != NULL);
	pool->count += 1;
	pthread_mutex_unlock(&(pool->lock));
	pthread_cond_signal(&(pool->ready));
}

int ClearPool(){
	if(pool->shutdown){
		return -1;
	}
	pool->shutdown = true;
	pthread_cond_broadcast(&(pool->ready));
	int i;
	for(i = 0; i < pool->threadnumber; i++){
		pthread_join(pool->threadid[i], NULL);
	}
	free(pool->threadid);
	Worker *head;
	while(pool->head != NULL){
		head = pool->head;
		pool->head = pool->head->next;
		free(head);
	}
	pthread_mutex_destroy(&(pool->lock));
	pthread_cond_destroy(&(pool->ready));
	free(pool);
	return 0;
}

void *ThreadRoutine(void *arg){
	while(1){
		pthread_mutex_lock(&(pool->lock));
		while((pool->count == 0) && (!pool->shutdown)){
			pthread_cond_wait(&(pool->ready), &(pool->lock));
		}
		if(pool->shutdown){
			pthread_mutex_unlock(&(pool->lock));
			pthread_exit(NULL);
		}
		pool->count -= 1;
		Worker *worker = pool->head;
		pool->head = worker->next;
		pthread_mutex_unlock(&(pool->lock));
		(*(worker->process)) (worker->arg);
	}
}

void *process(void *arg){
	printf("Thread 0x%x is working on task %d \n", pthread_self(), *(int *) arg);
	sleep(1);
	return NULL;
}

int main(){
	CreateThreadPool(10);
	int *tasks = (int *) malloc(10 * sizeof(int));
	int i;
	for(i = 0; i < 10; i++){
		tasks[i] = i;
		AddToPool(process, &tasks[i]);
	}
	sleep(5);
	ClearPool();
	return 0;
}