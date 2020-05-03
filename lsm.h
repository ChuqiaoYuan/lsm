#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

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

//bloom filter从Run里面去掉，应该是每个level有自己的一个bloom filter array
//每个run对应着自己的bloom filter
//然后每个level有自己的target fpr的记录
//Run里的fencepointer最好也都去掉

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

typedef struct LSMtree{
	Heap *buffer;
	int T;
	LevelNode *L0;
	double fpr1;
} LSMtree;

//declaration for heap.c
Heap *CreateHeap(int size);
int GetKeyPos(Heap *h, int key);
void HeapifyBottomTop(Heap *h, int index);
void InsertKey(Heap *h, int key, int value, bool flag);
Node PopMin(Heap *h);
void PrintNode(Heap *h);
void ClearHeap(Heap *h);

//declaration for level.c
Level *CreateLevel(int size, double fpr);
//void LevelHeapifyBottomTop(Level *level, int index);
//void LevelHeapifyTopBottom(Level *level, int parent);
void InsertRun(Level *level, int count, int size, int start, int end);
Run PopRun(Level *level);
//void IncreaseRunVisited(Level *level, int index, int visited);

//declaration for lsm.c
LSMtree *CreateLSM(int buffersize, int sizeratio, double fpr);
void Merge(LevelNode *Current, int origin, int levelsize,
	int runcount, int runsize, Node *sortedrun, double targetfpr);
void Put(LSMtree *lsm, int key, int value, bool flag);
void PrintStats(LSMtree *lsm);
void ClearLSM(LSMtree *lsm);

//declaration for bloom.c
unsigned int djb2(int key);
unsigned int jenkins(int key);
BloomFilter *CreateBloomFilter(int k, size_t size);
void InsertEntry(BloomFilter *filter, int key);
bool LookUp(BloomFilter *filter, int key);
void ClearBloomFilter(BloomFilter *filter);


