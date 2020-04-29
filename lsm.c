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
	if(DestLevel == NULL){
		DestLevel = (LevelNode *) malloc(sizeof(LevelNode));
		Level *level = CreateLevel(levelsize, filtered);
		InsertRun(level, runcount, runsize, start, end, filtered, file, bloom);
		DestLevel->level = level;
		DestLevel->number = origin + 1;
		DestLevel->next = NULL;
	}else{
		
	}
}


