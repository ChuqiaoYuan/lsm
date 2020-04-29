#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "lsm.h"

LSMtree *CreateLSM(int buffersize, int sizeratio, double fpr){
	LSMtree *lsm = (LSMtree *) malloc(sizeof(LSMtree));
	if(lsm == NULL){
		printf("There is not enough memory for an LSM-tree.")
		return NULL;
	}
	lsm->buffer = CreateHeap(buffersize);
	lsm->T = sizeratio;
	lsm->L1 = NULL;
	//后面要加filtered runs with bloom filters
	//double threshold = 0.05; //这个地方后面要怎么设计可以好好想想
	//int filtered = int((log(threshold) - log(fpr))/log(T)) + 1;
}

void Put(int key, int value, bool flag){
	//check if the key is already in the buffer
	//update the key directly in the buffer if it is here to save time and space
	int position = GetKeyPos(lsm->buffer, key);
	if(position >= 0){
		lsm->buffer.array[position].value = value;
		lsm->buffer.array[position].flag = flag;
	}else{
		//insert key directly if the buffer is not full
		if(lsm->buffer.count < lsm->buffer.size){
			InsertKey(lsm->buffer, key, value, flag);
		//firstly clear the buffer and merge it into level 1
		//then insert key to the cleared buffer
		}else if(lsm->buffer.count == lsm->buffer.size){
			//clear the buffer and get the sorted run
			//merge the sorted run to level 1
			Node *sortedrun = HeapSort(lsm->buffer);
			int start = sortedrun[0].key;
			int end = sortedrun[lsm->buffer.size - 1].key;
			FILE *fptr = fopen("a.txt", "w");//关于每个run的文件名也要好好想想
			Merge(lsm->L1, 0, (lsm->T - 1), false, lsm->buffer.size, 
				lsm->buffer.size, start, end, fptr, NULL);
			//insert key in buffer
			InsertKey(lsm->buffer, key, value, flag);
		}
	}
}

//destlevel is null
//how to decide if destlevel is full
//what if it is full
//if it is not full, which runs to merge together
//注意初始化一个level之后，如何往里面插入run，操作是插入run而不是直接插入Node
//写完merge就能搞定put
//然后写bloom filters搞定get
void Merge(LevelNode *DestLevel, int origin, int levelsize, bool filtered, 
	int runcount, int runsize, int start, int end, FILE *file, BloomFilter *bloom){
	//check if the level exists
	//if it does not exist, initiate a new level and insert the run
	if(DestLevel == NULL){
		DestLevel = (LevelNode *) malloc(sizeof(LevelNode));
		Level *level = CreateLevel(levelsize, filtered);
		InsertRun(level, runcount, runsize, start, end, filtered, file, bloom);
		DestLevel->level = level;
		DestLevel->number = origin + 1;
		DestLevel->next = NULL;
	//if the level exists, find out if there are runs with overlapping key ranges  
	}else{
		int i;
		int j = 0;
		Level *level = DestLevel->level;
		int *distance = (int *) malloc(level->count * sizeof(int));
		int *overlap = (int *) malloc(level->count * sizeof(int));
		for(i = 0; i < level->count; i++){
			if((level->array[i].start > end) || (level->array[i].end < start)){
				if(level->array[i].start > end){
					distance[i] = level->array[i].start - end;
				}else{
					distance[i] = start - level->array[i].end;
				}
			}else{
				distance[i] = 0;
				overlap[j] = i;
				j += 1;
			}
		}
		if(j != 0){
			//有重叠的，所有的run都pop出来，一起sort再写出去
			//中间有可能触发新的merge,先判断是否会产生第T个run
			//如果会的话就先把目前第1个run merge进一层，再把这层新的run都插进去
			//如果不会的话就直接插入新的run
			//注意新run和旧run一起更新的时候，如果遇到相同key以新run为准
		}else{
			//跟已有run merge还是另起一run???
			//能另起一run就另起一run，避免这个level里有一些run key range过大
			//不能另起一run时先判断有没有还有剩余空间的run，有的话merge在一起
			//没有有剩余空间的run，那就扔一个run下去，然后插这个新的run进入level
		}
	}
}


