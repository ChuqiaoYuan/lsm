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

//int GetKeyPos(Heap *h, int key){
//	int left = 0;
//	int right = h->count - 1;
//	if((key < h->array[left].key) | (key > h->array[right].key)){
//		return -1;
//	}else{
//		int mid = (left + right) / 2;
//		if(key == h->array[left].key){
//			return left;
//		}else if(key == h->array[right].key){
//			return right;
//		}
//		while(left != mid){
//			if(key == h->array[mid].key){
//				return mid;
//			}else if(key > h->array[mid].key){
//				left = mid;
//				mid = (left + right) / 2;
//			}else{
//				right = mid;
//				mid = (left + right) / 2;
//			}
//		}
//		return -1;
//	}
//}

int GetKeyPos(Heap *h, int key){
	if(h->count == 0){
		return -1;
	}
	int stack[h->count];
	int pos;
	int left;
	int right;
	int top = 0;
	//use a stack to record the subtrees to search for the key
	stack[top] = 0;
	while(top != -1){
		pos = stack[top];
		top -= 1;
		if(h->array[pos].key == key){
			return pos;
		}else if(h->array[pos].key < key){
			left = 2 * pos + 1;
			if(left < h->count){
				top += 1;
				stack[top] = left;
			}
			right = 2 * pos + 2;
			if(right < h->count){
				top += 1;
				stack[top] = right;
			}
		}
	}
	return -1;
}

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

//int main(){
//	Heap *heap = CreateHeap(10);
//	InsertKey(heap, 1, 0, true);
//	InsertKey(heap, 2, 3, false);
//	InsertKey(heap, 3, 4, true);
//	InsertKey(heap, 7, 12, true);
//	InsertKey(heap, 8, 15, false);
//	PrintNode(heap);
//	int i;
//	for(i=-2; i<10; i++){
//		printf("key %d position %d\n", i, GetKeyPos(heap, i));
//	}
//	InsertKey(heap, 0, -1, true);
//	InsertKey(heap, 5, 8, true);
//	InsertKey(heap, 1, 1, true);
//	InsertKey(heap, 8, 14, true);
//	InsertKey(heap, 4, 7, false);
//	PrintNode(heap);
//	for(i=-2; i<10; i++){
//		printf("key %d position %d\n", i, GetKeyPos(heap, i));
//	}	
//	ClearHeap(heap);
//	return 0;
//}