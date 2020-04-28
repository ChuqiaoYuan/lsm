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
	//后面要加filtered runs with bloom filters
	//double threshold = 0.05; //这个地方后面要怎么设计可以好好想想
	//int filtered = int((log(threshold) - log(fpr))/log(T)) + 1;
}