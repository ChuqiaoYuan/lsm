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

//要不要改成统一的run
//设置参数bool filtered
//bloom filter pointer可以为空
//最后一个level也可以是size是1的level,相当于只有1个run
//生成level的时候确认参数，保证每个level的初始化参数是对的
//linked list or array?

typedef struct BloomFilter{
	int *array;
} BloomFilter;

typedef struct Run{
	int count;
	int size;
	int start;
	int end;
	bool filtered;
	int fencepointer;
	BloomFilter *bloom;
} Run;

typedef struct Level{
	Run *array;
	int count;
	int size;
	bool filtered;
} Level;

typedef struct LevelNode{
	Level *level;
	int number;
	struct LevelNode *next;
} LevelNode;

//typedef struct FilteredRun{
//	int visited;
//	int count;
//	int size;
//	int start;
//	int end;
	//BloomFilter bloom;
//	FILE *f;
//} FilteredRun;

//typedef struct UnfilteredRun{
//	int visited;
//	int count;
//	int size;
//	int start;
//	int end;
//	FILE *filepointer;
//} UnfilteredRun;

//typedef struct FilteredLevel{
//	FilteredRun *array;
//	int count;
//	int size;
//} FilteredLevel;

//typedef struct UnfilteredLevel{
//	UnfilteredRun *array;
//	int count;
//	int size;
//} UnfilteredLevel;

typedef struct LSMtree{
	Heap *buffer;
	int T;
	LevelNode *L0;
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
Level *CreateLevel(int size, bool filtered);
//void LevelHeapifyBottomTop(Level *level, int index);
//void LevelHeapifyTopBottom(Level *level, int parent);
void InsertRun(Level *level, int count, int size, int start, int end, bool filtered, BloomFilter *bloom);
Run PopRun(Level *level);
//void IncreaseRunVisited(Level *level, int index, int visited);

//declaration for lsm.c
LSMtree *CreateLSM(int buffersize, int sizeratio, double fpr);
void Merge(LevelNode *Dest, int origin, int levelsize, bool filtered,
	int runcount, int runsize, Node *sortedrun, BloomFilter *bloom);
void Put(LSMtree *lsm, int key, int value, bool flag);
void PrintStats(LSMtree *lsm);




