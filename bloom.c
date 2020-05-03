//reference https://drewdevault.com/2016/04/12/How-to-write-a-better-bloom-filter-in-C.html


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "lsm.h"

unsigned int djb2(int key){
	char _str[16];
	sprintf(_str, "%d", key);
	const char *str = &_str[0];
	unsigned int hash = 5381;
	char c;
	while((c = *str++)){
		hash = ((hash << 5) + hash) + c;
	}
	return hash;
}

unsigned int jenkins(int key) {
	char _str[16];
	sprintf(_str, "%d", key);
	const char *str = &_str[0];
	unsigned int hash = 0;
	char c;
	while((c = *str++)){
		hash += c;
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}
	
	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);
	
	return hash;
}

BloomFilter *CreateBloomFilter(int k, size_t size){
	BloomFilter *filter = calloc(1, sizeof(BloomFilter));
	filter->k = k;
	filter->size = size;
	filter->bits = malloc(size);
	return filter;
}

void InsertEntry(BloomFilter *filter, int key){
	uint8_t *bits = filter->bits;
	unsigned int hash[filter->size];
	unsigned int h1 = djb2(key) % filter->size;
	unsigned int h2 = jenkins(key) % filter->size;
	int i;
	for(i = 0; i < filter->k; i++){
		hash[i] = h1 + i * h2;
		//printf("%d ", hash[i]);
	}
	//printf("input\n");
	for(i = 0; i < filter->k; i++){
		hash[i] %= filter->size * 8;
		bits[hash[i] / 8] |= (1 << hash[i] % 8);
	}
}

bool LookUp(BloomFilter *filter, int key){
	uint8_t *bits = filter->bits;
	unsigned int hash[filter->size];
	unsigned int h1 = djb2(key) % filter->size;
	unsigned int h2 = jenkins(key) % filter->size;
	int i;
	for(i = 0; i < filter->k; i++){
		hash[i] = h1 + i * h2;
		//printf("%d ", hash[i]);
	}
	//printf("lookup \n");
	
	for(i = 0; i < filter->k; i++){
		hash[i] %= filter->size * 8;
		if (!(bits[hash[i] / 8] & 1 << hash[i] % 8)){
			return false;
		}
	}
	
	return true;
}

void ClearBloomFilter(BloomFilter *filter){
	free(filter->bits);
	free(filter);
}

int main(){
	int k = 8;
	size_t m = 10;
	BloomFilter *filter = CreateBloomFilter(k, m);
	int search = 40;
	InsertEntry(filter, 40);
	InsertEntry(filter, 30);
	InsertEntry(filter, 100);
	if (LookUp(filter, search)){
		printf("%d is in the run. \n", search);
	}else{
		printf("%d is not in the run. \n", search);
	}
	return 0;
}