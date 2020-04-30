#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "lsm.h"

#define INT_MAX 2147483647

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
	//double threshold = 0.05; //这个地方后面要怎么设计可以好好想想，是否可以让它变成一个tunable knob
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
		}else if(lsm->buffer.count == lsm->buffer.size){
			//clear the buffer and get the sorted run
			Node *sortedrun = HeapSort(lsm->buffer);
			//merge this sorted run into level 1
			Merge(lsm->L1, 0, (lsm->T - 1), false, 
				lsm->buffer.size, lem->buffer.size, sortedrun, NULL);


			/*discard


			//int start = sortedrun[0].key;
			//int end = sortedrun[lsm->buffer.size - 1].key;
			//注意文件名的处理和命名
			//char filename[] = "L0";
			//FILE *fp = fopen(filename, "wt");
			//fwrite(sortedrun, sizeof(Node), lsm->buffer.size, fp);
			//fclose(fp)
			//之后加入filter这里还要再变动
			//Merge(lsm->L1, 0, (lsm->T - 1), false,
				//lsm->buffer.size, lsm->buffer.size, start, end, filename, NULL);


			discard*/

			//insert key in buffer
			InsertKey(lsm->buffer, key, value, flag);
		}
	}
}

void Merge(LevelNode *Dest, int origin, int levelsize, bool filtered,
	int runcount, int runsize, Node *sortedrun, BloomFilter *bloom){
	//if the destination level does not exist, initiate a new level and insert the run
	if(Dest == NULL){
		Dest = (LevelNode *) malloc(sizeof(LevelNode));
		Level *destlevel = CreateLevel(levelsize, filtered);
		int start = sortedrun[0].key;
		int end = sortedrun[runcount - 1].key;
		char filename[14] = "L";
		char levelnum[2];
		itoa((origin + 1), levelnum, 10);
		strcat(filename, levelnum);
		strcat(filename, "N");
		char runnum[10];
		itoa(destlevel->arrival, runnum, 10);
		strcat(filename, runnum);
		FILE *fp = fopen(filename, "wt");
		fwrite(sortedrun, sizeof(Node), runcount, fp);
		fclose(fp);
		InsertRun(destlevel, runcount, runsize, start, end, filtered, filename, bloom);
		Dest->level = destlevel;
		Dest->number = origin + 1;
		Dest->next = NULL;
	}else{
		int i;
		int j = 0;
		int min = INT_MAX;
		int minpos = -1;
		Level *destlevel = Dest->level;
		int distance = (int *) malloc(destlevel->count * sizeof(int));
		int overlap = (int *) malloc(destlevel->count * sizeof(int));
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
		if(j == 0){
			//if there is no run with overlapping keys in the destination level
			if(level->size < level->count){
				//if there is still space for another run, insert this run directly
				int start = sortedrun[0].key;
				int end = sortedrun[runcount - 1].key;
				char filename[14] = "L";
				char levelnum[2];
				itoa((origin + 1), levelnum, 10);
				strcat(filename, levelnum);
				strcat(filename, "N");
				char runnum[10];
				itoa(destlevel->arrival, runnum, 10);
				strcat(filename, runnum);
				FILE *fp = fopen(filename, "wt");
				fwrite(sortedrun, sizeof(Node), runcount, fp);
				fclose(fp);
				InsertRun(destlevel, runcount, runsize, start, end, filtered, filename, bloom);
			}else{
				//if there is no space for another run, merge this run with another existing run
				if(minpos != -1){
					//there is still space in this run to merge something into
					Run oldrun = destlevel->array[minpos];
					Node newarray = (Node *) malloc((oldrun->count + runcout) * sizeof(Node));
					if(oldrun->start > sortedrun[0].key){
						for(i = 0; i < runcount; i++){
							newarray[i] = sortedrun[i];
						}
						FILE *fp = fopen(oldrun->fencepointer, "rt");
						fread(newarray[runcount], sizeof(Node), oldrun->count, fp);
						fclose(fp);
					}else{
						FILE *fp = fopen(oldrun->fencepointer, "rt");
						fread(newarray, sizeof(Node), oldrun->count, fp);
						fclose(fp);
						for(i = 0; i < runcount; i++){
							newarray[oldrun->count + i] = sortedrun[i];
						}
					}

					FILE *fp = fopen(oldrun->fencepointer, "wt");
					fwrite(newarray, sizeof(Node), (oldrun->count + runcount), fp);
					fclose(fp);
					oldrun->count = oldrun->count + runcount;
					oldrun->start = newarray[0].key;
					oldrun->end = newarray[oldrun->count + runcount - 1].key;
					//之后需要更新这个bloom
					oldrun->bloom = NULL;
					//}
				}else{
					//there is no space in this run to merge something into 
					// so minpos is -1 here
					//把最少访问的run直接扔下去
					Run pushtonext = PopRun(destlevel);
					if(origin <= 8){
						Merge(Dest.next, (origin + 2), levelsize, filtered,
							pushtonext->count, (pushtonext->size * levelsize), pushtonext->array, pushtonext->bloom);
					}else if (orgin == 9){
						Merge(Dest.next, (origin + 2), 1, filtered,
							pushtonext->count, (pushtonext->size * levelsize), pushtonext->array, pushtonext->bloom);
					}else{
						printf("No more entries in this LSM tree.");
					}

					Run oldrun = destlevel->array[destlevel->count - 1];
					Node newarray = (Node *) malloc((oldrun->count + runcout) * sizeof(Node));
					if(oldrun->start > sortedrun[0].key){
						for(i = 0; i < runcount; i++){
							newarray[i] = sortedrun[i];
						}
						FILE *fp = fopen(oldrun->fencepointer, "rt");
						fread(newarray[runcount], sizeof(Node), oldrun->count, fp);
						fclose(fp);
					}else{
						FILE *fp = fopen(oldrun->fencepointer, "rt");
						fread(newarray, sizeof(Node), oldrun->count, fp);
						fclose(fp);
						for(i = 0; i < runcount; i++){
							newarray[oldrun->count + i] = sortedrun[i];
						}
					}					


					//更新最末端的run
					FILE *fp = fopen(oldrun->fencepointer, "wt");
					fwrite(newarray, sizeof(Node), oldrun->size, fp);
					fclose(fp);
					oldrun->count = oldrun->size;
					oldrun->start = newarray[0].key;
					oldrun->end = newarray[oldrun->size - 1].key;
					//之后需要更新这个bloom
					oldrun->bloom = NULL;

					//插入新的run
					char filename[14] = "L";
					char levelnum[2];
					itoa((origin + 1), levelnum, 10);
					strcat(filename, levelnum);
					strcat(filename, "N");
					char runnum[10];
					itoa(destlevel->arrival, runnum, 10);
					strcat(filename, runnum);
					FILE *fp = fopen(filename, "wt");
					fwrite(newarray[oldrun->size], sizeof(Node), (oldrun->count + runcout - oldrun->size), fp);
					fclose(fp);
					//bloom之后肯定要更新
					InsertRun(destlevel, (oldrun->count + runcout - oldrun->size), oldrun->size, 
						newarray[oldrun->size].key, newarray[oldrun->count + runcout - 1].key, filtered, filename, bloom);
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
				FILE *fp = fopen(destlevel->array[overlap[i]].fencepointer, "rt");
				fread(oldarray[oldcount], sizeof(Node), destlevel->array[overlap[i]].count, fp);
				fclose(fp);
				oldcount += destlevel->array[overlap[i]].count;
			}

			Heap *h = (Heap *) malloc(sizeof(Heap));
			h->array = oldarray;
			h->count = oldcount;
			h->size = oldcount + runcount;

			int position;
			for(i = 0; i < runcount; i++){
				position = GetKeyPos(h, sortedrun[i].key);
				if(position != -1){
					h->array[position].value = sortedrun[i].value;
					h->array[position].flag = sortedrun[i].flag;
				}else{
					InsertKey(h, sortedrun[i].key, sortedrun[i].value, sortedrun[i].flag);
				}
			}

			//往旧run里填融合好的array
			//尴尬的是不一定能填满emmmmmmmmmmmmm
			//几种情况，压根填不满旧run
			//填满了旧run且无多余
			//填满了旧run还多出1个array

			int numrun;
			if(h->count % destlevel->array[0].size == 0){
				numrun = h->count / destlevel->array[0].size;
			}else{
				numrun = h->count / destlevel->array[0].size + 1;
			}
			if(numrun <= j){
				//existing runs can hold these keys
				for(i = 0; (i < (numrun - 1)); i++){
					oldrun = destlevel->array[overlap[i]];
					FILE *fp = fopen(oldrun->fencepointer, "wt");
					fwrtie(h->array[i * oldrun->size], sizeof(Node), oldrun->size, fp);
					fclose(fp);
					oldrun->count = oldrun->size;
					oldrun->start = h->array[i * oldrun->size].key;
					oldrun->end = h->array[(i + 1) * oldrun->size - 1].key;
					//之后要更新这个bloom
					oldrun->bloom = NULL;
				}
				oldrun = destlevel->array[overlap[numrun - 1]];
				FILE *fp = fopen(oldrun->fencepointer, "wt");
				fwrite(h->array[(numrun - 1) * oldrun->size], sizeof(Node), (h->count - (numrun - 1) * oldrun->size), fp);
				fclose(fp);
				oldrun->count = h->count - (numrun - 1) * oldrun->size;
				oldrun->start = h->array[(numrun - 1) * oldrun->size].key;
				oldrun->end = h->array[h->count - 1].key;
				//之后要更新这个bloom
				oldrun->bloom = NULL;
				if(numrun < j){
					for(i = numrun; i < j; i++){
						oldrun = destlevel->array[overlap[i]];
						oldrun->count = 0;
						oldrun->start = INT_MAX + 1;
						oldrun->end = INT_MAX;
						oldrun->fencepointer = "NULL";
						oldrun->bloom = NULL;
					}
				}
			}else{
				//existing runs can not hold all the keys
				for(i = 0; i < j; i++){
					oldrun = destlevel->array[overlap[i]];
					FILE *fp = fopen(oldrun->fencepointer, "wt");
					fwrtie(h->array[i * oldrun->size], sizeof(Node), oldrun->size, fp);
					fclose(fp);
					oldrun->count = oldrun->size;
					oldrun->start = h->array[i * oldrun->size].key;
					oldrun->end = h->array[(i + 1) * oldrun->size - 1].key;
					//之后要更新这个bloom
					oldrun->bloom = NULL;
				}
				if(destlavel->count == destlevel->size){
					//if there is no space for the new array, push the least visited run to the next level
					Run pushtonext = PopRun(destlevel);
					if(origin <= 8){
						Merge(Dest.next, (origin + 2), levelsize, filtered,
							pushtonext->count, (pushtonext->size * levelsize), pushtonext->array, pushtonext->bloom);
					}else if (orgin == 9){
						Merge(Dest.next, (origin + 2), 1, filtered,
							pushtonext->count, (pushtonext->size * levelsize), pushtonext->array, pushtonext->bloom);
					}else{
						printf("No more entries in this LSM tree.");
					}					
				}
				//insert the run directly to the level then
				char filename[14] = "L";
				char levelnum[2];
				itoa((origin + 1), levelnum, 10);
				strcat(filename, levelnum);
				strcat(filename, "N");
				char runnum[10];
				itoa(destlevel->arrival, runnum, 10);
				strcat(filename, runnum);
				FILE *fp = fopen(filename, "wt");
				fwrite(h->array[j*oldrun->size], sizeof(Node), (h->count - j*oldrun->size), fp);
				//之后需要更新bloom
				InsertRun(destlevel, (h->count - j*oldrun->size), oldrun->size,
					h->array[j*oldrun->size].key, h->array[h->count - 1].key, filename, bloom);
			}
		}
	}
}

void PrintStats(LSMtree *lsm){
	int i;
	int j;
	total = 0;
	LevelNode *currentlevelnode = lsm->L1;
	while(currentlevelnode != NULL){
		int levelnum = currentlevelnode->number;
		currentcount = 0
		Level *currentlevel = currentlevelnode->level;
		for(i = 0; i < currentlevel->count; i++){
			Run currentrun = currentlevel->array[i];
			currentcount += currentrun->count;
			Node *currentarray = (Node *) malloc(currentrun->count * sizeof(Node));
			FILE *fp =fread(currentrun->fencepointer, "rt");
			fread(currentarray, sizeof(Node), currentrun->count, fp);
			fclose(fp);
			for(j = 0; j < currentrun->count; j++){
				printf("%d:%d:L%d  ", currentarray[j].key, currentarray[j].value, levelnum);
			}
			//看看这部分打算如何处置，是否去掉
			printf("a run has ended. \n")
		}
		printf("\n")
		printf("There are %d pairs on Level %d. \n\n", currentcount, levelnum);
		total += currentcount;
		currentlevelnode = currentlevelnode->next;
	}
	printf("There are %d pairs on the LSM-tree in total. \n", total);
}
