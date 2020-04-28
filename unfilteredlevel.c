#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "lsm.h"

//UnfilteredRun *CreateUnfilteredRun(int count, int size, int start, int end, File *f){
//	UnfilteredRun *run = (UnfilteredRun *) malloc(sizeof(UnfilteredRun));
//	if(run == NULL){
//		printf("There is not enough memory for a new unfiltered run.");
//		return NULL;
//	}
//	run->visited = 0;
//	run->count = count;
//	run->size = size;
//	run->start = start;
//	run->end = end;
//	run->f = f;
//	return run;
//}

UnfilteredLevel *CreateUnfilteredLevel(int size){
	UnfilteredLevel *level = (UnfilteredLevel *) malloc(sizeof(UnfilteredLevel));
	if(level == NULL){
		printf("There is not enough memory for a new unfiltered level.")
		return NULL;
	}
	level->size = size;
	level->count = 0;
	level->array = (UnfilteredRun *) malloc(size*sizeof(UnfilteredRun));
	if(level->array == NULL){
		printf("There is not enough memory for the array of unfiltered runs.");
		return NULL;
	}
	return level;
}

void UnfilteredLevelHeapifyBottomTop(UnfilteredLevel *level, int index){
	UnfilteredRun temp;
	int parent = (index-1) / 2;
	if(level->array[parent].visited > level->array[index].visited){
		temp = level->array[parent];
		level->array[parent] = level->array[index];
		level->array[index] = temp;
		UnfilteredLevelHeapifyBottomTop(level, parent);
	}
}

void InsertUnfilteredRun(UnfilteredLevel *level, int count, int size, int start, int end, FILE *f){
	//if(level->count < level->size){ 在插入前就要确认
	level->array[level->count].visited = 0;
	level->array[level->count].count = count;
	level->array[level->count].size = size;
	level->array[level->count].start = start;
	level->array[level->count].end = end;
	level->array[level->count].filepointer = f;
	UnfilteredLevelHeapifyBottomTop(level, level->count);
	level->count += 1;
	//}
}

void UnfilteredLevelHeapifyTopBottom(UnfilteredLevel *level, int parent){
	int left = parent * 2 + 1;
	int right = parent * 2 + 1;
	int min;
	UnfilteredRun temp;
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
		UnfilteredLevelHeapifyTopBottom(level, min);
	}
}

UnfilteredRun PopUnfilteredRun(UnfilteredLevel *level){
	UnfilteredRun run;
	run = level->array[0];
	level->array[0] = level->array[level->count - 1];
	level->count -= 1;
	UnfilteredLevelHeapifyTopBottom(level, 0);
	return run;
}

void IncreaseUnfilteredRunVisited(UnfilteredLevel *level, int index, int visited){
	level->array[index].visited = visted;
	UnfilteredLevelHeapifyTopBottom(level, index);
}

