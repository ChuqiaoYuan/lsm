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

typedef struct FilteredRun{
	int visited;
	int count;
	int size;
	int start;
	int end;
	//BloomFilter bloom;
	FILE *f;
} FilteredRun;

typedef struct UnfilteredRun{
	int visited;
	int count;
	int size;
	int start;
	int end;
	FILE *f;
} UnfilteredRun;

typedef struct FilteredLevel{
	FilteredRun *array;
	int count;
	int size;
} FilteredLevel;

typedef struct UnfilteredLevel{
	UnfilteredRun *array;
	int count;
	int size;
} UnfilteredLevel;

typedef struct LSMtree{
	Heap *buffer;
	int T;
	FilteredLevel *filtered;
	UnfilteredLevel *unfiltered;
}

//declaration for heap.c
Heap *CreateHeap(int size);
int GetKeyPos(Heap *h, int key);
void HeapifyBottomTop(Heap *h, int index);
void InsertKey(Heap *h, int key, int value, bool flag);
void PrintNode(Heap *h);
void ClearHeap(Heap *h);