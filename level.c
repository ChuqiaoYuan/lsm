#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "lsm.h"

Level *CreateLevel(int size, bool filtered){
	Level *level = (Level *) malloc(sizeof(Level));
	if(level == NULL){
		printf("There is not enough memory for a new level.")
		return NULL;
	}
	level->size = size;
	level->filtered = filtered;
	level->count = 0;
	level->array = (Run *) malloc(size*sizeof(Run));
	if(level->array == NULL){
		printf("There is not enough memory for the array of runs.")
		return NULL;
	}
	return level;
}

void LevelHeapifyBottomTop(Level *level, int index){
	Run temp;
	int parent = (index-1) / 2;
	if(level->array[parent].visited > level->array[index].visited){
		temp = level->array[parent];
		level->array[parent] = level->array[index];
		level->array[index] = temp;
		LevelHeapifyBottomTop(level, parent);
	}
}

void LevelHeapifyTopBottom(Level *level, int parent){
	int left = parent * 2 + 1;
	int right = parent * 2 + 1;
	int min;
	Run temp;
	if(left >= level->count){
		left = -1;
	}
	if(right >= level->count){
		right = -1;
	}
	if((left > 0) && (level->array[left].visited < level->array[parent].visited)){
		min = left;
	}else{
		min = parent;
	}
	if((right > 0) && (level->array[right].visited < level->array[min].visited)){
		min = right;
	}
	if(min != parent){
		temp = level->array[min];
		level->array[min] = level->array[parent];
		level->array[parent] = temp;
		LevelHeapifyTopBottom(level, min);
	}
}

void InsertRun(Level *level, int count, int size, int start, int end, bool filtered, FILE *f, BloomFilter *b){
	level->array[level->count].visited = 0;
	level->array[level->count].count = count;
	level->array[level->count].size = size;
	level->array[level->count].start = start;
	level->array[level->count].end = end;
	level->array[level->count].filtered = filtered;
	level->array[level->count].fencepointer = f;
	level->array[level->count].bloom = b;
	LevelHeapifyBottomTop(level, level->count);
	level->count += 1;
}

Run PopRun(Level *level){
	Run run;
	run = level->array[0];
	level->array[0] = level->array[level->count - 1];
	level->count -= 1;
	LevelHeapifyTopBottom(level, 0);
	return run;
}

void IncreaseRunVisited(Level *level, int index, int visited){
	level->array[index].visited = visited;
	LevelHeapifyTopBottom(level, index);
}


