#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "lsm.h"

#define INT_MAX 2147483647

LSMtree *CreateLSM(int buffersize, int sizeratio, double fpr){
	LSMtree *lsm = (LSMtree *) malloc(sizeof(LSMtree));
	if(lsm == NULL){
		printf("There is not enough memory for an LSM-tree.");
		return NULL;
	}
	lsm->buffer = CreateHeap(buffersize);
	lsm->T = sizeratio;
	LevelNode *L0 = (LevelNode *) malloc(sizeof(LevelNode));
	L0->level = NULL;
	L0->number = 0;
	L0->next = NULL;
	lsm->L0 = L0; 
	//后面要加filtered runs with bloom filters
	//double threshold = 0.05; //这个地方后面要怎么设计可以好好想想，是否可以让它变成一个tunable knob
	//int filtered = int((log(threshold) - log(fpr))/log(T)) + 1;
	return lsm;
}

void Merge(LevelNode *Current, int origin, int levelsize, bool filtered,
	int runcount, int runsize, Node *sortedrun, BloomFilter *bloom){
	//if the destination level does not exist, initiate a new level and insert the run
	if(Current->next == NULL){
		LevelNode *New = (LevelNode *) malloc(sizeof(LevelNode));
		Level *destlevel = CreateLevel(levelsize, filtered);
		int start = sortedrun[0].key;
		int end = sortedrun[runcount - 1].key;
		char filename[14];
		sprintf(filename, "L%dN%d", (origin+1), destlevel->count);
		FILE *fp = fopen(filename, "wt");
		fwrite(sortedrun, sizeof(Node), runcount, fp);
		fclose(fp);
		InsertRun(destlevel, runcount, runsize, start, end, filtered, bloom);
		New->level = destlevel;
		New->number = origin + 1;
		New->next = NULL;
		Current->next = New;
	}else{
		int i;
		int j = 0;
		int min = INT_MAX;
		int minpos = -1;
		Level *destlevel = Current->next->level;
		int *distance = (int *) malloc(destlevel->count * sizeof(int));
		int *overlap = (int *) malloc(destlevel->count * sizeof(int));
		int start = sortedrun[0].key;
		int end = sortedrun[runcount - 1].key;
		for(i = 0; i < destlevel->count; i++){
			if(destlevel->array[i].start > end){
				distance[i] = destlevel->array[i].start - end;
			}else if(destlevel->array[i].end < start){
				distance[i] = start - destlevel->array[i].end;
			}else{
				distance[i] = 0;
				if(j == 0){
					overlap[j] = i;
					j += 1;
				}else{
					int k;
					int n;
					for(k = 0; k < j; k++){
						if(destlevel->array[overlap[k]].start > destlevel->array[i].start){
							break;
						}
					}
					for(n = j; n > k; n--){
						overlap[n] = overlap[n-1];
					}
					overlap[k] = i;
					j += 1;
				}
			}
			if((destlevel->array[i].count + runcount) < destlevel->array[i].size){
				if(distance[i] < min){
					min = distance[i];
					minpos = i;
				}
			}
		}
		//printf("j %d minpos %d start %d end %d levelsize %d levelcount %d \n", j, minpos, start, end, destlevel->size, destlevel->count);
		if(j == 0){
			//if there is no run with overlapping keys in the destination level
			if(destlevel->count < destlevel->size){
				//printf("If 6 \n");
				//if there is still space for another run, insert this run directly
				int start = sortedrun[0].key;
				int end = sortedrun[runcount - 1].key;
				char filename[14];
				sprintf(filename, "L%dN%d", (origin+1), destlevel->count);
				FILE *fp = fopen(filename, "wt");
				fwrite(sortedrun, sizeof(Node), runcount, fp);
				fclose(fp);
				InsertRun(destlevel, runcount, runsize, start, end, filtered, bloom);
			}else{
				//if there is no space for another run, merge this run with another existing run
				if(minpos != -1){
					//printf("If 1 \n");
					//there is still space in this run to merge something into
					Run oldrun = destlevel->array[minpos];
					Node *newarray = (Node *) malloc((oldrun.count + runcount) * sizeof(Node));
					if(oldrun.start > sortedrun[0].key){
						for(i = 0; i < runcount; i++){
							newarray[i] = sortedrun[i];
						}
						char name[14];
						sprintf(name, "L%dN%d", Current->next->number, oldrun.fencepointer);
						FILE *fp = fopen(name, "rt");
						fread(&newarray[runcount], sizeof(Node), oldrun.count, fp);
						fclose(fp);
					}else{
						char name[14];
						sprintf(name, "L%dN%d", Current->next->number, oldrun.fencepointer);
						FILE *fp = fopen(name, "rt");
						fread(newarray, sizeof(Node), oldrun.count, fp);
						fclose(fp);
						for(i = 0; i < runcount; i++){
							newarray[oldrun.count + i] = sortedrun[i];
						}
					}
					char name[14];
					sprintf(name, "L%dN%d", Current->next->number, oldrun.fencepointer);
					FILE *fp = fopen(name, "wt");
					fwrite(newarray, sizeof(Node), (oldrun.count + runcount), fp);
					fclose(fp);

					/*
					Node *new = (Node *) malloc((oldrun.count + runcount) * sizeof(Node));
					FILE *ftest = fopen(name, "rt");
					fread(new, sizeof(Node), (oldrun.count + runcount), ftest);
					fclose(ftest);
					printf("What is in the new array ");
					for(i = 0; i < (oldrun.count + runcount); i++){
						printf("%d ", new[i].key);
					}
					printf("\n");
					

					printf("original %d %d %d ", oldrun.count, oldrun.start,oldrun.end);
					*/

					oldrun.count += runcount;
					oldrun.start = newarray[0].key;
					oldrun.end = newarray[oldrun.count - 1].key;
					//之后需要更新这个bloom
					oldrun.bloom = NULL;

					destlevel->array[minpos] = oldrun;
					/*
					printf("updated %d %d %d \n", oldrun.count, oldrun.start, oldrun.end);
					*/

				}else{
					//there is no space in this run to merge something into 
					// so minpos is -1 here
					//printf("If 2\n");
					Run pushtonext = PopRun(destlevel);
					Node *topush = (Node *) malloc(pushtonext.count * sizeof(Node));
					char name[14];
					sprintf(name, "L%dN%d", Current->next->number, pushtonext.fencepointer);
					FILE *fp = fopen(name, "rt");
					fread(topush, sizeof(Node), pushtonext.count, fp);
					fclose(fp);
					if(origin <= 0){
						Merge(Current->next, (origin + 1), levelsize, filtered,
							pushtonext.count, pushtonext.size * (levelsize + 1), topush, pushtonext.bloom);
					}else if (origin == 1){
						Merge(Current->next, (origin + 1), 1, filtered,
							pushtonext.count, (pushtonext.size * (levelsize + 1) * levelsize), topush, pushtonext.bloom);
					}else{
						printf("No more entries in this LSM tree. Some data may be lost.");
					}

					Run oldrun = destlevel->array[destlevel->count - 1];
					Node *newarray = (Node *) malloc((oldrun.count + runcount) * sizeof(Node));
					if(oldrun.start > sortedrun[0].key){
						for(i = 0; i < runcount; i++){
							newarray[i] = sortedrun[i];
						}
						char name[14];
						sprintf(name, "L%dN%d", Current->next->number, oldrun.fencepointer);
						FILE *fp = fopen(name, "rt");
						fread(&newarray[runcount], sizeof(Node), oldrun.count, fp);
						fclose(fp);
					}else{
						char name[14];
						sprintf(name, "L%dN%d", Current->next->number, oldrun.fencepointer);
						FILE *fp = fopen(name, "rt");
						fread(newarray, sizeof(Node), oldrun.count, fp);
						fclose(fp);
						for(i = 0; i < runcount; i++){
							newarray[oldrun.count + i] = sortedrun[i];
						}
					}					

					//更新最末端的run
					char newname[14];
					sprintf(newname, "L%dN%d", Current->next->number, oldrun.fencepointer);
					fp = fopen(newname, "wt");
					fwrite(newarray, sizeof(Node), oldrun.size, fp);
					fclose(fp);
					oldrun.count = oldrun.size;
					oldrun.start = newarray[0].key;
					oldrun.end = newarray[oldrun.size - 1].key;
					//之后需要更新这个bloom
					oldrun.bloom = NULL;

					destlevel->array[destlevel->count - 1] = oldrun;

					//插入新的run
					char filename[14];
					sprintf(filename, "L%dN%d", (origin+1), destlevel->count);
					FILE *fpw = fopen(filename, "wt");
					fwrite(&newarray[oldrun.size], sizeof(Node), (oldrun.count + runcount - oldrun.size), fpw);
					fclose(fpw);
					//bloom之后肯定要更新
					InsertRun(destlevel, (oldrun.count + runcount - oldrun.size), oldrun.size, 
						newarray[oldrun.size].key, newarray[oldrun.count + runcount - 1].key, filtered, bloom);
				}
			}
		}else{
			//if there are runs with overlapping keys in the destination level
			//融合后不出现增加的run
			//融合后出现多的run
			//融合后出现多的run，扔哪个run下去？
			//overlap里的run已经按start排好序
			//所以直接先融合一个大array出来
			//再往这个新的array里插heap
			//把已有的runs都用新的array填满——问题是不一定能填满，这就非常尴尬
			//这个时候如果count == size (相当于原本的level就是满的)，就丢一个run下去，再插入最后的多出来的run
			//这个时候如果count < size,就直接插入多出来的这个run
			int oldcount = 0;
			for(i = 0; i < j; i++){
				oldcount += destlevel->array[overlap[i]].count;
			}

			Node *oldarray = (Node *) malloc(oldcount * sizeof(Node));

			oldcount = 0;
			for(i = 0; i < j; i++){
				char name[14];
				sprintf(name, "L%dN%d", Current->next->number, destlevel->array[overlap[i]].fencepointer);
				FILE *fp = fopen(name, "rt");
				fread(&oldarray[oldcount], sizeof(Node), destlevel->array[overlap[i]].count, fp);
				fclose(fp);
				oldcount += destlevel->array[overlap[i]].count;
			}

/*
			printf("What's in the old array %d in total\n", oldcount);
			for(i = 0; i < oldcount; i++){
				printf("%d ", oldarray[i].key);
			}
			printf("\n ");
*/

			Node *newarray = (Node *) malloc((oldcount + runcount) * sizeof(Node));
			int a = 0;
			int b = 0;
			int c = 0;
			while((a < oldcount) && (b < runcount)){
				if(oldarray[a].key < sortedrun[b].key){
					newarray[c] = oldarray[a];
					a += 1;
					c += 1;
				}else if(oldarray[a].key > sortedrun[b].key){
					newarray[c] = sortedrun[b];
					b += 1;
					c += 1;
				}else{
					newarray[c] = sortedrun[b];
					a += 1;
					b += 1;
					c += 1;
				}
			}
			while(a < oldcount){
				newarray[c] = oldarray[a];
				a += 1;
				c += 1;
			}
			while (b < runcount){
				newarray[c] = sortedrun[b];
				b += 1;
				c += 1;
			}
			//newarray长度是c，是一个sorted array of the runs
			//往旧run里填融合好的array
			//尴尬的是不一定能填满emmmmmmmmmmmmm
			//几种情况，压根填不满旧run
			//填满了旧run且无多余
			//填满了旧run还多出1个array

			//需要把heap里的node整个地pop out, 然后再往新run里面插入


			int numrun;
			if(c % destlevel->array[0].size == 0){
				numrun = c / destlevel->array[0].size;
			}else{
				numrun = c / destlevel->array[0].size + 1;
			}
			//printf("numrun %d j %d\n", numrun, j);
			if(numrun <= j){
				//printf("If 3\n");
				//existing runs can hold these keys
				for(i = 0; (i < (numrun - 1)); i++){
					Run oldrun = destlevel->array[overlap[i]];
					char name[14];
					sprintf(name, "L%dN%d", Current->next->number, oldrun.fencepointer);
					FILE *fp = fopen(name, "wt");
					fwrite(&newarray[i * oldrun.size], sizeof(Node), oldrun.size, fp);
					fclose(fp);
					oldrun.count = oldrun.size;
					oldrun.start = newarray[i * oldrun.size].key;
					oldrun.end = newarray[(i + 1) * oldrun.size - 1].key;
					//之后要更新这个bloom
					oldrun.bloom = NULL;
					destlevel->array[overlap[i]] = oldrun;
				}
				Run oldrun = destlevel->array[overlap[numrun - 1]];
				char name[14];
				sprintf(name, "L%dN%d", Current->next->number, oldrun.fencepointer);
				FILE *fp = fopen(name, "wt");
				fwrite(&newarray[(numrun - 1) * oldrun.size], sizeof(Node), (c - (numrun - 1) * oldrun.size), fp);
				fclose(fp);
				oldrun.count = c - (numrun - 1) * oldrun.size;
				oldrun.start = newarray[(numrun - 1) * oldrun.size].key;
				oldrun.end = newarray[c - 1].key;
				//之后要更新这个bloom
				oldrun.bloom = NULL;
				destlevel->array[overlap[numrun - 1]] = oldrun;

				if(numrun < j){
					for(i = numrun; i < j; i++){
						oldrun = destlevel->array[overlap[i]];
						oldrun.count = 0;
						oldrun.start = INT_MAX + 1;
						oldrun.end = INT_MAX;
						oldrun.fencepointer = -1;
						oldrun.bloom = NULL;
						destlevel->array[overlap[i]] = oldrun;
					}
				}
			}else{
				//printf("If 5\n");
				//existing runs can not hold all the keys
				for(i = 0; i < j; i++){
					Run oldrun = destlevel->array[overlap[i]];
					char name[14];
					sprintf(name, "L%dN%d", Current->next->number, oldrun.fencepointer);
					FILE *fp = fopen(name, "wt");
					fwrite(&newarray[i * oldrun.size], sizeof(Node), oldrun.size, fp);
					fclose(fp);

/*

					Node *b = (Node *) malloc(oldrun.size * sizeof(Node));
					FILE *f2 = fopen(name, "rt");
					fread(b, sizeof(Node), oldrun.size, f2);
					fclose(f2);
					printf("inserted %d num to the %d run on this level\n", oldrun.size, oldrun.fencepointer);
					for(m = 0; m < oldrun.size; m++){
						printf("%d ", b[m].key);
					}
					printf("\n");


*/

					oldrun.count = oldrun.size;
					oldrun.start = newarray[i * oldrun.size].key;
					oldrun.end = newarray[(i + 1) * oldrun.size - 1].key;
					//之后要更新这个bloom
					oldrun.bloom = NULL;
					destlevel->array[overlap[i]] = oldrun;
				}

				if(destlevel->count == destlevel->size){
					//if there is no space for the new array, push the least visited run to the next level
					Run pushtonext = PopRun(destlevel);
					Node *topush = (Node *) malloc(pushtonext.count * sizeof(Node));
					char name[14];
					sprintf(name, "L%dN%d", Current->next->number, pushtonext.fencepointer);
					FILE *fp = fopen(name, "rt");
					fread(topush, sizeof(Node), pushtonext.count, fp);
					fclose(fp);
					if(origin <= 0){
						Merge(Current->next, (origin + 1), levelsize, filtered,
							pushtonext.count, (pushtonext.size * (levelsize + 1)), topush, pushtonext.bloom);
					}else if (origin == 1){
						Merge(Current->next, (origin + 1), 1, filtered,
							pushtonext.count, (pushtonext.size * (levelsize + 1) * levelsize), topush, pushtonext.bloom);
					}else{
						printf("No more entries in this LSM tree. Some data may be lost. \n");
					}					
				}

				//insert the run directly to the level then
				char filename[14];
				sprintf(filename, "L%dN%d", (origin+1), destlevel->count);
				Run oldrun = destlevel->array[0];
				FILE *fp = fopen(filename, "wt");
				fwrite(&newarray[j * oldrun.size], sizeof(Node), (c - j*oldrun.size), fp);
				fclose(fp);

				/*
				Node *a = (Node *) malloc((c - j*oldrun.size) * sizeof(Node));
				FILE *f = fopen(filename, "rt");
				fread(a, sizeof(Node), (c - j*oldrun.size), f);
				fclose(f);
				printf("inserted %d num to the last run on this level\n", (c - j*oldrun.size));
				for(i = 0; i < (c - j*oldrun.size); i++){
					printf("%d ", a[i].key);
				}
				printf("\n");
				*/

				//之后需要更新bloom
				InsertRun(destlevel, (c - j * oldrun.size), oldrun.size,
					newarray[j * oldrun.size].key, newarray[c- 1].key, filtered, bloom);
				//ClearHeap(h);
			}
		}
	}
}

void Put(LSMtree *lsm, int key, int value, bool flag){
	//check if the key is already in the buffer
	//update the key directly in the buffer if it is here to save time and space
	int position = GetKeyPos(lsm->buffer, key);
	Heap *buffer = lsm->buffer;
	//printf("Here it is, key %d position %d \n", key, position);
	if(position >= 0){
		buffer->array[position].value = value;
		buffer->array[position].flag = flag;
	}else{
		//insert key directly if the buffer is not full
		//printf("Here it is 1 %d \n", key);
		if(buffer->count < buffer->size){
			InsertKey(buffer, key, value, flag);
		}else if(buffer->count == buffer->size){
			//clear the buffer and get the sorted run
			//printf("Here it is 2 %d\n", key);
			int i;
			Node *sortedrun = (Node *) malloc(buffer->size * sizeof(Node));
			for(i = 0; i < buffer->size; i++){
				sortedrun[i] = PopMin(buffer);
			}
			//merge this sorted run into level 1

			/*
			for(i = 0; i < buffer->size; i++){
				printf("Here it is 3 ");
				printf("%d ", sortedrun[i].key);
			}
			printf("\n");
			*/
			

			Merge(lsm->L0, 0, (lsm->T - 1), false, 
				buffer->size, buffer->size, sortedrun, NULL);
			InsertKey(buffer, key, value, flag);
		}
	}
}

void PrintStats(LSMtree *lsm){
	int i;
	int j;
	int total = lsm->buffer->count;

	LevelNode *Current = lsm->L0;

	//printf("test %d %d %d\n", Current->next->number, Current->next->level->count, Current->next->level->size);
	//printf("filename %d \n", Current->next->level->array[0].fencepointer);

	LevelNode *currentlevelnode = lsm->L0->next;
	while(currentlevelnode != NULL){
		int levelnum = currentlevelnode->number;
		int currentcount = 0;
		for(i = 0; i < currentlevelnode->level->count; i++){
			currentcount += currentlevelnode->level->array[i].count;
			Node *currentarray = (Node *) malloc(currentlevelnode->level->array[i].count * sizeof(Node));
			char filename[14];
			sprintf(filename, "L%dN%d", levelnum, currentlevelnode->level->array[i].fencepointer);
			FILE *fp =fopen(filename, "rt");
			fread(currentarray, sizeof(Node), currentlevelnode->level->array[i].count, fp);
			fclose(fp);
			for(j = 0; j < currentlevelnode->level->array[i].count; j++){
				printf("%d:%d:L%d  ", currentarray[j].key, currentarray[j].value, levelnum);
			}
			printf("a run has ended. \n");
		}
		printf("There are %d pairs on Level %d. \n\n", currentcount, levelnum);
		total += currentcount;
		currentlevelnode = currentlevelnode->next;
	}
	printf("There are %d pairs on the LSM-tree in total. \n", total);
}

void ClearLSM(LSMtree *lsm){
	ClearHeap(lsm->buffer);
	free(lsm->L0);
}

int main(){
	LSMtree *lsm = CreateLSM(2, 3, 0.001);
	Put(lsm, 1, 2, true);
	Put(lsm, 5, 10, true);
	Put(lsm, 3, 6, true);
	Put(lsm, 10, 20, true);
	Put(lsm, 4, 8, true);
	Put(lsm, 43, 86, true);
	Put(lsm, 20, 40, true);
	Put(lsm, 2, 4, true);
	Put(lsm, 9, 18, true);
	Put(lsm, 52, 104, true);
	Put(lsm, 103, 206, true);
	Put(lsm, 94, 188, true);
	Put(lsm, 5, 11, true);
	Put(lsm, 11, 22, true);
	Put(lsm, 30, 60, true);
	Put(lsm, 40, 80, true);
	Put(lsm, 120, 240, true);
	Put(lsm, 39, 78, true);
	Put(lsm, 10, 21, true);
	Put(lsm, 44, 88, true);
	Put(lsm, 6, 12, true);
	Put(lsm, 34, 68, true);
	Put(lsm, 106, 212, true);
	Put(lsm, 41, 82, true);
	Put(lsm, 14, 28, true);
	Put(lsm, 23, 46, true);
	Put(lsm, 30, 61, true);
	Put(lsm, 17, 34, true);
	Put(lsm, 57, 114, true);
	Put(lsm, 66, 132, true);
	Put(lsm, 2, 5, true);
	Put(lsm, 22, 44, true);
	Put(lsm, 29, 58, true);
	Put(lsm, 18, 36, true);
	Put(lsm, 31, 62, true);
	Put(lsm, 67, 134, true);
	Put(lsm, 55, 110, true);
	Put(lsm, 27, 54, true);
	Put(lsm, 90, 180, true);
	Put(lsm, 4, 9, true);
	Put(lsm, 88, 176, true);
	Put(lsm, 53, 106, true);
	Put(lsm, 61, 132, true);
	Put(lsm, 93, 186, true);
	Put(lsm, 13, 26, true);
	Put(lsm, 71, 142, true);
	Put(lsm, 59, 118, true);
	Put(lsm, 97, 194, true);
	Put(lsm, 11, 22, true);
	Put(lsm, 110, 220, true);
	Put(lsm, 47, 94, true);
	Put(lsm, 93, 187, true);
	Put(lsm, 39, 78, true);
	Put(lsm, 56, 112, true);
	Put(lsm, 58, 116, true);
	Put(lsm, 85, 170, true);
	Put(lsm, 95, 190, true);
	Put(lsm, 19, 38, true);
	//Put(lsm, 3, 7, true);

	PrintNode(lsm->buffer);
	printf("\n");
	PrintStats(lsm);
	ClearLSM(lsm);
	return 0;
}

