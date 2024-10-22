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
	if(h->count == 0){
		return -1;
	}else{
		int i;
		for(i = 0; i < h->count; i++){
			if(h->array[i].key == key){
				return i;
			}
		}
		return -1;
	}
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

void HeapifyTopBottom(Heap *h, int parent){
	int left = parent * 2 + 1;
	int right = parent * 2 + 1;
	int min;
	Node temp;
	if(left >= h->count){
		left = -1;
	}
	if(right >= h->count){
		right = -1;
	}
	if((left > 0) && (h->array[left].key < h->array[parent].key)){
		min = left;
	}else{
		min = parent;
	}
	if((right > 0) && (h->array[right].key < h->array[min].key)){
		min = right;
	}
	if(min != parent){
		temp = h->array[min];
		h->array[min] = h->array[parent];
		h->array[parent] = temp;
		HeapifyTopBottom(h, min);
	}
}

void InsertKey(Heap *h, int key, int value, bool flag){
	h->array[h->count].key = key;
	h->array[h->count].value = value;
	h->array[h->count].flag = flag;
	HeapifyBottomTop(h, h->count);
	h->count += 1;
}

Node PopMin(Heap *h){
	Node pair;
	pair = h->array[0];
	h->array[0] = h->array[h->count - 1];
	h->count -= 1;
	HeapifyTopBottom(h, 0);
	return pair;
}

void PrintNode(Heap *h){
	int i;
	for(i=0; i<h->count; i++){
		printf("%d:%d:L0 ", h->array[i].key, h->array[i].value);
	}
	printf("\nThere are %d pairs on buffer. \n", h->count);
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