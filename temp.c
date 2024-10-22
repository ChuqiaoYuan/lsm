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
	//printf("origin %d levelsize %d runcount %d runsize %d \n", origin, levelsize, runcount, runsize);
	if(Current->next == NULL){
		LevelNode *New = (LevelNode *) malloc(sizeof(LevelNode));
		Level *destlevel = CreateLevel(levelsize, filtered);
		int start = sortedrun[0].key;
		int end = sortedrun[runcount - 1].key;

		//在子函数里分配的内存空间，出了子函数这部分内存空间就被free, 随便指向其他内容
		//所以我传的这个指针之后不再指向这个字符串，它被随便分配给其他程序了
		//所以不能在这里面分配内存啊TT

		char filename[14];
		sprintf(filename, "L%dN%d", (origin+1), destlevel->arrival);
		FILE *fp = fopen(filename, "wt");
		fwrite(sortedrun, sizeof(Node), runcount, fp);
		fclose(fp);
		free(sortedrun);

		InsertRun(destlevel, runcount, runsize, start, end, filtered, destlevel->arrival, bloom);
		New->level = destlevel;
		New->number = origin + 1;
		New->next = NULL;
		Current->next = New;

		//printf("test %d %d %d\n", Current->next->number, Current->next->level->count, Current->next->level->size);
		//printf("filename %s \n", Current->next->level->array[0].fencepointer);
	}else{
		int i;
		int j = 0;
		int min = INT_MAX;
		int minpos = -1;
		Level *destlevel = Current->next->level;
		//Level *destlevel = Dest->level;
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
		if(j == 0){
			//if there is no run with overlapping keys in the destination level
			if(destlevel->size < destlevel->count){
				//if there is still space for another run, insert this run directly
				int start = sortedrun[0].key;
				int end = sortedrun[runcount - 1].key;
				char filename[14];
				sprintf(filename, "L%dN%d", (origin+1), destlevel->arrival);
				FILE *fp = fopen(filename, "wt");
				fwrite(sortedrun, sizeof(Node), runcount, fp);
				fclose(fp);
				free(sortedrun);
				InsertRun(destlevel, runcount, runsize, start, end, filtered, destlevel->arrival, bloom);
			}else{
				//if there is no space for another run, merge this run with another existing run
				if(minpos != -1){
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
					free(newarray);
					oldrun.count = oldrun.count + runcount;
					oldrun.start = newarray[0].key;
					oldrun.end = newarray[oldrun.count + runcount - 1].key;
					//之后需要更新这个bloom
					oldrun.bloom = NULL;
					//}
				}else{
					//there is no space in this run to merge something into 
					// so minpos is -1 here
					//把最少访问的run直接扔下去
					Run pushtonext = PopRun(destlevel);
					Node *topush = (Node *) malloc(pushtonext.count * sizeof(Node));
					char name[14];
					sprintf(name, "L%dN%d", Current->next->number, pushtonext.fencepointer);
					FILE *fp = fopen(name, "rt");
					fread(topush, sizeof(Node), pushtonext.count, fp);
					fclose(fp);
					if(origin <= 1){
						//Merge(Dest->next, (origin + 2), levelsize, filtered,
						Merge(Current->next, (origin + 2), levelsize, filtered,
							pushtonext.count, (pushtonext.size * levelsize), topush, pushtonext.bloom);
					}else if (origin == 2){
						//Merge(Dest->next, (origin + 2), 1, filtered,
						Merge(Current->next, (origin + 2), 1, filtered,
							pushtonext.count, (pushtonext.size * levelsize * (levelsize - 1)), topush, pushtonext.bloom);
					}else{
						printf("No more entries in this LSM tree.");
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

					//插入新的run
					char filename[14];
					sprintf(filename, "L%dN%d", (origin+1), destlevel->arrival);
					FILE *fpw = fopen(filename, "wt");
					fwrite(&newarray[oldrun.size], sizeof(Node), (oldrun.count + runcount - oldrun.size), fpw);
					fclose(fpw);
					free(newarray);
					//bloom之后肯定要更新
					InsertRun(destlevel, (oldrun.count + runcount - oldrun.size), oldrun.size, 
						newarray[oldrun.size].key, newarray[oldrun.count + runcount - 1].key, filtered, destlevel->arrival, bloom);
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
					Run oldrun = destlevel->array[overlap[i]];
					char name[14];
					sprintf(name, "L%dN%d", Current->next->number, oldrun.fencepointer);
					FILE *fp = fopen(name, "wt");
					fwrite(&h->array[i * oldrun.size], sizeof(Node), oldrun.size, fp);
					fclose(fp);
					oldrun.count = oldrun.size;
					oldrun.start = h->array[i * oldrun.size].key;
					oldrun.end = h->array[(i + 1) * oldrun.size - 1].key;
					//之后要更新这个bloom
					oldrun.bloom = NULL;
				}
				Run oldrun = destlevel->array[overlap[numrun - 1]];
				char name[14];
				sprintf(name, "L%dN%d", Current->next->number, oldrun.fencepointer);
				FILE *fp = fopen(name, "wt");
				fwrite(&h->array[(numrun - 1) * oldrun.size], sizeof(Node), (h->count - (numrun - 1) * oldrun.size), fp);
				fclose(fp);
				oldrun.count = h->count - (numrun - 1) * oldrun.size;
				oldrun.start = h->array[(numrun - 1) * oldrun.size].key;
				oldrun.end = h->array[h->count - 1].key;
				//之后要更新这个bloom
				oldrun.bloom = NULL;
				if(numrun < j){
					for(i = numrun; i < j; i++){
						oldrun = destlevel->array[overlap[i]];
						oldrun.count = 0;
						oldrun.start = INT_MAX + 1;
						oldrun.end = INT_MAX;
						oldrun.fencepointer = -1;
						oldrun.bloom = NULL;
					}
				}
				ClearHeap(h);
			}else{
				//existing runs can not hold all the keys
				for(i = 0; i < j; i++){
					Run oldrun = destlevel->array[overlap[i]];
					char name[14];
					sprintf(name, "L%dN%d", Current->next->number, oldrun.fencepointer);
					FILE *fp = fopen(name, "wt");
					fwrite(&h->array[i * oldrun.size], sizeof(Node), oldrun.size, fp);
					fclose(fp);
					oldrun.count = oldrun.size;
					oldrun.start = h->array[i * oldrun.size].key;
					oldrun.end = h->array[(i + 1) * oldrun.size - 1].key;
					//之后要更新这个bloom
					oldrun.bloom = NULL;
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
					if(origin <= 1){
						//Merge(Dest->next, (origin + 2), levelsize, filtered,
						Merge(Current->next, (origin + 2), levelsize, filtered,
							pushtonext.count, (pushtonext.size * levelsize), topush, pushtonext.bloom);
					}else if (origin == 2){
						//Merge(Dest->next, (origin + 2), 1, filtered,
						Merge(Current->next, (origin + 2), 1, filtered,
							pushtonext.count, (pushtonext.size * levelsize * (levelsize - 1)), topush, pushtonext.bloom);
					}else{
						printf("No more entries in this LSM tree.");
					}					
				}
				//insert the run directly to the level then
				char filename[14];
				sprintf(filename, "L%dN%d", (origin+1), destlevel->arrival);
				FILE *fp = fopen(filename, "wt");
				Run oldrun = destlevel->array[0];
				fwrite(&h->array[j * oldrun.size], sizeof(Node), (h->count - j*oldrun.size), fp);
				//之后需要更新bloom
				InsertRun(destlevel, (h->count - j * oldrun.size), oldrun.size,
					h->array[j * oldrun.size].key, h->array[h->count - 1].key, filtered, destlevel->arrival, bloom);
				ClearHeap(h);
			}
		}
	}
}

void Put(LSMtree *lsm, int key, int value, bool flag){
	//check if the key is already in the buffer
	//update the key directly in the buffer if it is here to save time and space
	int position = GetKeyPos(lsm->buffer, key);
	Heap *buffer = lsm->buffer;
	if(position >= 0){
		buffer->array[position].value = value;
		buffer->array[position].flag = flag;
	}else{
		//insert key directly if the buffer is not full
		if(buffer->count < buffer->size){
			InsertKey(buffer, key, value, flag);
		}else if(buffer->count == buffer->size){
			//clear the buffer and get the sorted run
			int i;
			Node *sortedrun = (Node *) malloc(buffer->size * sizeof(Node));
			for(i = 0; i < buffer->size; i++){
				sortedrun[i] = PopMin(buffer);
			}
			//merge this sorted run into level 1
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
		printf("\n");
		printf("There are %d pairs on Level %d. \n\n", currentcount, levelnum);
		total += currentcount;
		currentlevelnode = currentlevelnode->next;
	}
	printf("There are %d pairs on the LSM-tree in total. \n", total);
}


		if(targetfpr < 0.3){
			double numbits = - 2 * () * log(targetfpr); //注意runcount变化
			size_t m = (size_t)numbits;
			double numhashes = 0.7 * numbits / ();
			int k = (int)numhashes;
			BloomFilter *filter = CreateBloomFilter(k, m);
			(int i;)
			for(i = 0; i < (); i++){
				InsertEntry(filter, ()[()].key); //注意array其实
			}
			destlevel->filters[()] = *filter;
		}



int main(){
	//需要一句一句debug, 注意测试到尽可能多的函数，尽可能多的if分句
	LSMtree *lsm = CreateLSM(3, 3, 0.001);
	Put(lsm, 1, 2, true);
	Put(lsm, 5, 10, true);
	Put(lsm, 3, 6, true);
	Put(lsm, 4, 8, true);
	printf("Pairs on buffer \n");
	PrintNode(lsm->buffer);
	printf("\n");
	PrintStats(lsm);
	return 0;
}
