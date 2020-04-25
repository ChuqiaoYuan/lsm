#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "lsm.h"

Heap *CreateHeap(int size){
	//allocate memory to heap
	Heap *h = (Heap *) malloc(sizeof(Heap));
	//check if there is enough memory for heap
	if(h == NULL){
		printf("There is not enough memory for heap.");
		return NULL;
	}
	//initiate components in heap
	h->size = size;
	h->count = 0;
	h->array = (Node *) malloc(size*sizeof(Node));
	//check if there is enough memory for the array of nodes
	if(h->array == NULL){
		printf("There is not engouth memory for the array of nodes.");
		return NULL;
	}
	return h;
}

int GetKeyPos(Heap *h, int key){
	int left = 0;
	int right = h->count;
	if((key < h->array[left].key) | ((key > h->array[right].key))){
		return -1;//压根不在这个range里
	}else{
		int mid = (left + right) / 2;
		while(mid != right){
			if(key == h->array[mid].key){
				return mid;
			}else if(key > h->array[mid].key){
				left = mid;
				mid = (left + right) / 2;
			}else{
				right = mid;
				mid = (left + right) / 2;
			}
		}
		return -1; //在这个range里但是不在这个array里面
	}
}
//建议插入后把每个插入的key都test一遍，同时也要test在range里但不在array里的每个元素

void HeapifyBottomTop(Heap *h, int index){
	Node temp;
	int parent = (index-1) / 2;
	if(h->array[parent].key > h->array[index].key){
		temp = h->array[parent];
		h->array[parent] = h->array[index];
		h->array[index] = temp;
		HeapifyBottomTop(h, parent);
	}
}

void InsertKey(Heap *h, int key, int value, bool flag){
	int position = GetKeyPos(h, key);
	if(position != -1){
		h->array[position].key = key;
		h->array[position].value = value;
		h->array[position].flag = flag;
	}else{
		if(h->count < h->size){
			h->array[h->count].key = key;
			h->array[h->count].value = value;
			h->array[h->count].flag = flag;
			HeapifyBottomTop(h, h->count);
			h->count += 1;
		}
	}
}

void PrintNode(Heap *h){
	int i;
	for(i=0; i<h->count; i++){
		printf("key %d value %d\n", h->array[i].key, h->array[i].value);
	}
}
void ClearHeap(Heap *h){
	free(h->array);
	free(h);
}

int main(){
	Heap *heap = CreateHeap(10);
	InsertKey(heap, 1, 0, true);
	InsertKey(heap, 2, 3, false);
	InsertKey(heap, 3, 4, true);
	InsertKey(heap, 5, 9, true);
	InsertKey(heap, 7, 12, true);
	PrintNode(heap);
	int i;
	for(i=0; i<9; i++){
		printf("key %d position %d", i, GetKeyPos(heap, i));
	}
	InsertKey(heap, 0, -1, true);
	InsertKey(heap, 1, 1, true);
	InsertKey(heap, 4, 7, false);
	InsertKey(heap, 5, 8, true);
	InsertKey(heap, 8, 15, true);
	PrintNode(heap);
	for(i=0; i<9; i++){
		printf("key %d position %d", i, GetKeyPos(heap, i));
	}	
	ClearHeap(heap);
	return 0;
}