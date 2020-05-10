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

//the structure of the level is a stack, 
//both insertion and deletion happen at the end of the array.

void InsertRun(Level *level, int count, int size, int start, int end){
	level->array[level->count].count = count;
	level->array[level->count].size = size;
	level->array[level->count].start = start;
	level->array[level->count].end = end;
	level->count += 1;
}

Run PopRun(Level *level){
	Run run = level->array[level->count - 1];
	level->count -= 1;
	return run;
}

void ClearLevel(Level *l){
	int i;
	free(l->filters);
	free(l->array);
	free(l);
}
