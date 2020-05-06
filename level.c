#include "lsm.h"

Level *CreateLevel(int size, double fpr){
	Level *level = (Level *) malloc(sizeof(Level));
	if(level == NULL){
		printf("There is not enough memory for a new level.");
		return NULL;
	}
	level->size = size;
	level->count = 0;
	level->array = (Run *) malloc(size * sizeof(Run));
	if(level->array == NULL){
		printf("There is not enough memory for the array of runs.");
		return NULL;
	}
	level->filters = (BloomFilter *) malloc(size * sizeof(BloomFilter));
	level->targetfpr = fpr;
	return level;
}

void InsertRun(Level *level, int count, int size, int start, int end){
	level->array[level->count].count = count;
	level->array[level->count].size = size;
	level->array[level->count].start = start;
	level->array[level->count].end = end;
	/*
	//level->array[level->count].filtered = filtered;
	level->array[level->count].fencepointer = level->count;
	//level->array[level->count].bloom = bloom;
	BloomFilter *filter = CreateBloomFilter(int k, size_t size);
	level->filters[level->count] = *filter;
	*/



	level->count += 1;
}

Run PopRun(Level *level){
	Run run;
	run = level->array[level->count - 1];
	level->count -= 1;
	return run;
}


