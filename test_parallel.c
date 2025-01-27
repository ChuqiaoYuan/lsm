#include <stdio.h> 
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include "lsm.h"

#define PORT 8080 
#define SA struct sockaddr 
#define buffersize 100
#define sizeratio 10
#define fprlevel1 0.0000001
#define loaddirectory false
#define filename "data/load_file"


static ThreadPool *pool = NULL;

static LSMtree *lsm = NULL; 

//void BuildLSMTree(int buffersize, int sizeratio, double fprlevel1, bool loaddirectory, char *filename){
void BuildLSMTree(){
	lsm = (LSMtree *) malloc(sizeof(LSMtree));
	if(lsm == NULL){
		printf("There is not enough memory for an LSM-tree.");
	}
	lsm->buffer = CreateHeap(buffersize);
	lsm->T = sizeratio;
	lsm->L0 = (LevelNode *) malloc(sizeof(LevelNode));
	lsm->L0->level = NULL;
	lsm->L0->number = 0;
	lsm->L0->next = NULL;
	lsm->fpr1 = fprlevel1;
	pthread_mutex_init(&(lsm->lock), NULL);
	if(loaddirectory){
		Load(lsm, filename);
	}
}

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

void *ParallelizedPut(void *arg){
	pthread_mutex_lock(&(lsm->lock));
	printf("Thread 0x%x is working on put\n", pthread_self());
	char result[16];
	bzero(result, 16);
	ThreadArg *arguments = (ThreadArg *) arg;
	Put(lsm, arguments->first, arguments->second, true);
	//Put(lsm, key, value, true);
	write(arguments->sockfd, result, sizeof(result));
	sleep(5);
	pthread_mutex_unlock(&(lsm->lock));
}

void *ParallelizedGet(void *arg){
	//Get(lsm, key, result);
	printf("Thread 0x%x is working on get\n", pthread_self());
	char result[16];
	bzero(result, 16);
	ThreadArg *arguments = (ThreadArg *) arg;
	Get(lsm, arguments->first, result);
	write(arguments->sockfd, result, sizeof(result));
	sleep(5);
}

void *ParallelizedRange(void *arg){
	//Range(lsm, start, end, result);
	printf("Thread 0x%x is working on range\n", pthread_self());
	char result[4096];
	bzero(result, 4096);
	ThreadArg *arguments = (ThreadArg *) arg;
	Range(lsm, arguments->first, arguments->second, result);
	write(arguments->sockfd, result, sizeof(result));
	sleep(5);
}

void *ParallelizedDelete(void *arg){
	pthread_mutex_lock(&(lsm->lock));
	printf("Thread 0x%x is working on delete\n", pthread_self());
	char result[16];
	bzero(result, 16);
	ThreadArg *arguments = (ThreadArg *) arg;
	//Put(lsm, key, 0, false);
	Put(lsm, arguments->first, 0, false);
	write(arguments->sockfd, result, sizeof(result));
	sleep(5);
	pthread_mutex_unlock(&(lsm->lock));	
}
  
bool ParallelizedRespond(int sockfd, LSMtree *lsm){ 
	char buff[80]; 
	//char result[4096];

	//加入线程池后主要是把所有查询都挨个往里扔，注意当进行写操作的时候LSM要加锁，进行读操作的时候开始调线程
	while (1){
		bzero(buff, 80);
		//bzero(result, 4096);
		read(sockfd, buff, sizeof(buff));
		printf("Query from client %s \n", buff);
		if(buff[0] == 'p'){
			int pos = 2;
			int key = 0;
			int sign = 1;
			if(buff[pos] == '-'){
				sign = -1;
				pos += 1;
			}
			while((buff[pos] >= '0') && (buff[pos] <= '9')){
				key = key * 10 + (buff[pos] - '0');
				pos += 1;
			}
			key = key * sign;
			int value = 0;
			pos += 1;
			sign = 1;
			if(buff[pos] == '-'){
				sign = -1;
				pos += 1;
			}
			while((buff[pos] >= '0') && (buff[pos] <= '9')){
				value = value * 10 + (buff[pos] - '0');
				pos += 1;
			}
			value = value * sign;
			printf("key %d value %d \n", key, value);
			//Put(lsm, key, value, true);
			ThreadArg *arguments = (ThreadArg *) malloc(sizeof(ThreadArg));
			arguments->sockfd = sockfd;
			arguments->first = key;
			arguments->second = value;
			AddToPool(ParallelizedPut, arguments);
			//write(sockfd, result, sizeof(result));
		}else if(buff[0] == 'g'){
			int pos = 2;
			int key = 0;
			int sign = 1;
			if(buff[pos] == '-'){
				sign = -1;
				pos += 1;
			}
			while((buff[pos] >= '0') && (buff[pos] <= '9')){
				key = key * 10 + (buff[pos] - '0');
				pos += 1;
			}
			key = key * sign;
			printf("key %d \n", key);
			//Get(lsm, key, result);
			ThreadArg *arguments = (ThreadArg *) malloc(sizeof(ThreadArg));
			arguments->sockfd = sockfd;
			arguments->first = key;
			arguments->second = 0;
			AddToPool(ParallelizedGet, arguments);
			//write(sockfd, result, sizeof(result));
		}else if(buff[0] == 'r'){
			int pos = 2;
			int start = 0;
			int sign = 1;
			if(buff[pos] == '-'){
				sign = -1;
				pos += 1;
			}
			while((buff[pos] >= '0') && (buff[pos] <= '9')){
				start = start * 10 + (buff[pos] - '0');
				pos += 1;
			}
			start = start * sign;

			int end = 0;
			pos += 1;
			sign = 1;
			if(buff[pos] == '-'){
				sign = -1;
				pos += 1;
			}
			while((buff[pos] >= '0') && (buff[pos] <= '9')){
				end = end * 10 + (buff[pos] - '0');
				pos += 1;
			}
			end = end * sign;
			printf("start %d end %d \n", start, end);
			//Range(lsm, start, end, result);
			ThreadArg *arguments = (ThreadArg *) malloc(sizeof(ThreadArg));
			arguments->sockfd = sockfd;
			arguments->first = start;
			arguments->second = end;
			AddToPool(ParallelizedRange, arguments);

			//write(sockfd, result, sizeof(result));
		}else if(buff[0] == 'd'){
			int pos = 2;
			int key = 0;
			int sign = 1;
			if(buff[pos] == '-'){
				sign = -1;
				pos += 1;
			}
			while((buff[pos] >= '0') && (buff[pos] <= '9')){
				key = key * 10 + (buff[pos] - '0');
				pos += 1;
			}
			key = key * sign;
			printf("key %d \n", key);
			//Put(lsm, key, 0, false);
			ThreadArg *arguments = (ThreadArg *) malloc(sizeof(ThreadArg));
			arguments->sockfd = sockfd;
			arguments->first = key;
			arguments->second = 0;
			AddToPool(ParallelizedDelete, arguments);

			//write(sockfd, result, sizeof(result));
		}else if(buff[0] == 'e'){
			printf("Here it is 1\n");
			return true;
		}else{
			return false;
		}
		printf("Here it is 2\n");
	}
	printf("Here it is 3\n");
}

  
int main(){ 
	int sockfd, connfd, len; 
	struct sockaddr_in servaddr, cli; 
  
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1){ 
		printf("socket creation failed...\n"); 
		exit(0); 
	}else{
		printf("Socket successfully created..\n"); 
	}
	bzero(&servaddr, sizeof(servaddr)); 
  
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
	servaddr.sin_port = htons(PORT); 
  
	if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0){ 
		printf("socket bind failed...\n"); 
		exit(0); 
	} 
	else{
		printf("Socket successfully binded..\n"); 
	}
  
	if ((listen(sockfd, 5)) != 0){ 
		printf("Listen failed...\n"); 
		exit(0); 
	} 
	else{
		printf("Server listening..\n"); 
	}

	//LSMtree *lsm = BuildLSMTree(buffersize, sizeratio, fprlevel1, false, "data/load_file");
	//BuildLSMTree(buffersize, sizeratio, fprlevel1, loaddirectory, filename);
	BuildLSMTree();

	//加线程池
	CreateThreadPool(4);

	while(1){
		len = sizeof(cli); 
		connfd = accept(sockfd, (SA*)&cli, &len); 
		if(connfd < 0){ 
			printf("server acccept failed...\n"); 
			exit(0); 
		}else{
			printf("server acccept the client...\n"); 
		}
		//=client还是挨个handle，但是同一个client有多个线程去handle
		bool shutdown = ParallelizedRespond(connfd, lsm);
		if(shutdown){
			ClearPool();
			ClearLSM(lsm);
			break;
		}
	}
  
	//respond(connfd); 
	printf("Here it is 4\n");
	close(sockfd);
	printf("Here it is 5\n");
	return 0;
} 