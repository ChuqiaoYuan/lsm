#include "lsm.h"

HashTable *CreateHashTable(int size){
	HashTable *table = (HashTable *) malloc(sizeof(HashTable));
	if(table == NULL){
		printf("There is not enough memory for a hash table.");
		return NULL;
	}
	table->size = size;
	table->array = (ChainNode *) malloc(size * sizeof(ChainNode));
	int i;
	for(i = 0; i < size; i++){
		table->array[i].next = NULL;
	}
	return table;
}

void AddToTable(HashTable *table, int key){
	int pos = key % table->size;
	ChainNode *previous = &table->array[pos];
	ChainNode *node = table->array[pos].next;
	while(node != NULL){
		previous = node;
		node = node->next;
	}
	previous->next = (ChainNode *) malloc(sizeof(ChainNode));
	previous->next->key = key;
	previous->next->next = NULL;
	return;
}

bool CheckTable(HashTable *table, int key){
	int pos = key % table->size;
	ChainNode *node = table->array[pos].next;
	while(node != NULL){
		if(node->key == key){
			return true;
		}else{
			node = node->next;
		}
	}
	return false;
}

void ClearTable(HashTable *table){
	free(table->array);
	free(table);
}

/*
int main(){
	HashTable *t = CreateHashTable(7);
	AddToTable(t, 6);
	AddToTable(t, 20);
	AddToTable(t, 22);
	AddToTable(t, 14);
	AddToTable(t, 36);
	int a[10] = {6, 20, 7, 10, 14, 22, 36, 50, 18, 4};
	int i;
	for(i = 0; i < 10; i++){
		if(CheckTable(t, a[i])){
			printf("%d is in the table \n", a[i]);
		}else{
			printf("%d is not in the table \n", a[i]);
		}
	}
	return 0;
}
*/