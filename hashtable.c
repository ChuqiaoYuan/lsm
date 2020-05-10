#include "lsm.h"

HashTable *CreateHashTable(int size){
	HashTable *table = (HashTable *) malloc(sizeof(HashTable));
	if(table == NULL){
		printf("There is not enough memory for a hash table.");
		return NULL;
	}
	table->count = 0;
	table->array = (int *) malloc(size * sizeof(int));
	if(table->array == NULL){
		printf("There is not enough memory for an array in the hash table.");
	}
	return table;
}

void AddToTable(HashTable *table, int key){
	table->array[table->count] = key;
	table->count += 1;
	return;
}

bool CheckTable(HashTable *table, int key){
	int i;
	for(i = 0; i < table->count; i++){
		if(table->array[i] == key){
			return true;
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
	int a[10] = {6, 20, 7, 27, 14, 10, 36, 50, 18, 4};
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