#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <unistd.h>
#include <netdb.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <sys/types.h>
#include <time.h>
#include <pthread.h>

typedef struct Node{
	int key;
	int value;
	bool flag;
} Node;

typedef struct Heap{
	Node *array;
	int count;
	int size;
} Heap;

typedef struct BloomFilter{
	void *bits;
	int k;
	size_t size;
} BloomFilter;

typedef struct Run{
	int count;
	int size;
	int start;
	int end;
} Run;

typedef struct Level{
	Run *array;
	int count;
	int size;
	double targetfpr;
	BloomFilter *filters;
} Level;

typedef struct LevelNode{
	Level *level;
	int number;
	struct LevelNode *next;
} LevelNode;

typedef struct ChainNode{
	int key;
	struct ChainNode *next;
} ChainNode;

typedef struct HashTable{
	int count;
	int *array;
} HashTable;

typedef struct LSMtree{
	Heap *buffer;
	int T;
	LevelNode *L0;
	double fpr1;
	pthread_mutex_t lock;
} LSMtree;

typedef struct Worker{
	void *(*process) (void *arg);
	void *arg;
	struct Worker *next;
} Worker;

typedef struct ThreadPool{
	pthread_mutex_t lock;
	pthread_cond_t ready;
	int threadnumber;
	pthread_t *threadid;
	Worker *head;
	int count;
	bool shutdown;
} ThreadPool;

typedef struct ThreadArg{
	int sockfd;
	int first;
	int second;
} ThreadArg;

//declaration for heap.c
Heap *CreateHeap(int size);
int GetKeyPos(Heap *h, int key);
void HeapifyBottomTop(Heap *h, int index);
void HeapifyTopBottom(Heap *h, int parent);
void InsertKey(Heap *h, int key, int value, bool flag);
Node PopMin(Heap *h);
void PrintNode(Heap *h);
void ClearHeap(Heap *h);

//declaration for level.c
Level *CreateLevel(int size, double fpr);
void InsertRun(Level *level, int count, int size, int start, int end);
Run PopRun(Level *level);
void ClearLevel(Level *l);

//declaration for bloom.c
unsigned int djb2(int key);
unsigned int jenkins(int key);
void InsertEntry(BloomFilter filter, int key);
bool LookUp(BloomFilter filter, int key);

//declaration for hashtable.c
HashTable *CreateHashTable(int size);
void AddToTable(HashTable *table, int key);
bool CheckTable(HashTable *table, int key);
void ClearTable(HashTable *table);

//declaration for lsm.c
LSMtree *CreateLSM(int buffersize, int sizeratio, double fpr);
void Merge(LevelNode *Current, int origin, int levelsize,
	int runcount, int runsize, Node *sortedrun, double targetfpr);
void Put(LSMtree *lsm, int key, int value, bool flag);
void Get(LSMtree *lsm, int key, char *result);
void Range(LSMtree *lsm, int start, int end, char *result);
void Load(LSMtree *lsm, char *binaryfile);
void PrintStats(LSMtree *lsm);

//declaration for server.c
bool Respond(int sockfd, LSMtree *lsm);

//declaration for client.c
void Query(int sockfd, char *filename);

//declaration for parallelizedserver.c
void CreateThreadPool(int threadnumber);
void AddToPool(void *(*process) (void *arg), void *arg);
int ClearPool();
void *ThreadRoutine(void *arg);
void *ParallelizedPut(void *arg);
void *ParallelizedGet(void *arg);
void *ParallelizedRange(void *arg);
void *ParallelizedDelete(void *arg);
bool ParallelizedRespond(int sockfd, LSMtree *lsm);
