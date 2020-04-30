#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
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
		//firstly clear the buffer and merge it into level 1
		//then insert key to the cleared buffer
		}else if(lsm->buffer.count == lsm->buffer.size){
			//clear the buffer and get the sorted run
			//merge the sorted run to level 1
			Node *sortedrun = HeapSort(lsm->buffer);
			int start = sortedrun[0].key;
			int end = sortedrun[lsm->buffer.size - 1].key;
			FILE *fptr = fopen("a.txt", "w");//关于每个run的文件名也要好好想想
			//把sorted run给写进去啊！！！
			fwrite(sortedrun, sizeof(Node), (lsm->buffer.size), fptr);
			fclose(fptr);
			Merge(lsm->L1, 0, (lsm->T - 1), false,
				lsm->buffer.size, lsm->buffer.size, start, end, fptr, NULL);
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
		int min = INT_MAX;
		int minpos = -1;
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
				if((level->array[i].count + runcount) < level->array[i].size){
					distance[i] = INT_MAX;
				}
			}else{
				distance[i] = 0;
				//overlap[j] = i; 
				int k;
				int n;
				for(k = 0; k < j; k++){
					if(level->array[overlap[k]].start>level->array[i].start){
						break;
					}
				}
				for(n = j; n > k; n--){
					overlap[n] = overlap[n-1];
				}
				overlap[k] = i;
				j += 1;
			}
			if(distance[i] < min){
				min = distance[i];
				minpos = i;
			}
		}
		if(j != 0){
			//获取想融的run
			//融好，
			//能放入原位就直接塞入原位
			//不能放入原位，就把一些run塞入原位后再insert一个新run
			//insert一个新run时可能会出现这个level没有位置的情况
			//需要判断是否触发新的merge再insert
			//注意新run和旧run一起更新的时候，如果遇到相同key以新run为准
			//TODO
			//overlapping runs的size和这个run的size
			//overlapping runs的file和这个run的file
			//应该先按overlapping runs的key range把它们的sorted run都接成1个大array
			//怎么能让这个overlapping runs的key range都排好序？？？前面直接用了insertion sort的做法排好了
			//怎么拼这个超大array???
			//再在这个大的array里插入这个新run
			//最后把这个大array里的sorted nodes分成大小合适的新run再插回去
			int oldcount = 0;
			for(i = 0; i < j; i++){
				oldcount += level->array[overlap[i]].count;
			}
			Node *oldarray = (Node *) malloc(oldcount * sizeof(Node));
			oldcount = 0;
			for(i = 0; i < j; i++){
				fread(oldarray[oldcount], sizeof(Node), level->array[overlap[i]].size, level->array[overlap[i]].fencepointer);
				oldcount += level->array[overlap[i]].count;
			}
			Heap *h = (Heap *) malloc(sizeof(Heap));
			h->array = oldarray;
			h->count = oldcount;
			h->size = oldcount + runcount;
			Node *inserting = (Node *) malloc(runcount * sizeof(Node));
			fread(inserting, sizeof(Node), runcount, file);
			for(i = 0; i < runcount; i++){
				int position = GetKeyPos(h, inserting[i].key);
				if(position != -1){
					h->array[position].value = inserting[i].value;
					h->array[position].flag = inserting[i].flag;
				}else{
					InsertKey(h, inserting[i].key, inserting[i].value, inserting[i].flag);
				}
			}
			//能够把所有的东西填回原来的runs
			if(h->count < (level->array[0].size * j)){
				for(i = 0; i < j; i++){
					FILE *fptr = fopen("a.txt", "w"); //重写文件名TT
					fwrite(h->array[level->array[0].size * i], sizeof(Node), level->array[0].size, fptr);//等等，这里应该不一定会全部填同样size的吧？？最后一个chunk可能大小不一样啊！！！TODO
					fclose(fptr);
					level->array[i].count = level->array[0].size;
					level->array[i].start = h->array[level->array[0].size * i].key;
					level->array[i].end = h->array[level->array[0].size * (i+1) - 1].key;
					level->array[i].fencepointer = fptr;
				}
			}else{
				//不能把所有的东西填回原来的runs
				for(i = 0; i < j; i++){
					FILE *fptr = fopen("a.txt", "w"); //重写文件名TT
					fwrite(h->array[level->array[0].size * i], sizeof(Node), level->array[0].size, fptr);
					fclose(fptr);
					level->array[i].count = level->array[0].size;
					level->array[i].start = h->array[level->array[0].size * i].key;
					level->array[i].end = h->array[level->array[0].size * (i+1) - 1].key;
					level->array[i].fencepointer = fptr;
				}
				FILE *fptr = fopen("a.txt", "w");//重写文件名TT
				fwrite(h->array[level->array[0].size * (j-1)], sizeof(Node), (h->count - level->array[0].size * (j-1)), fptr);
				fclose(fptr);
				if(level->count < level->size){
					InsertRun(level, (h->count - level->array[0].size * (j-1)), level->array[0].size, 
						h->array[level->array[0].size * (j-1)].key, h->array[h->count - 1].key, filtered, fptr, NULL);
				}else{
					Run puttonext = PopRun(level);
					//注意level->size可能会是1，对于最后一层
					Merge(DestLevel.next, (origin + 1), level->size, filtered,
						puttonext->count, puttonext->size, puttonext->start, puttonext->end, puttonext->fencepointer, puttonext->bloom);
					InsertRun(level, (h->count - level->array[0].size * (j-1)), level->array[0].size, 
						h->array[level->array[0].size * (j-1)].key, h->array[h->count - 1].key, filtered, fptr, NULL);
				}			
			}
		}else{
			//跟已有run merge还是另起一run???
			//能另起一run就另起一run，避免这个level里有一些run key range过大
			//不能另起一run时先判断有没有还有剩余空间的run，有的话merge在一起
			//没有有剩余空间的run，那就扔一个run下去，然后插这个新的run进入level
			//TODO
			//if there is still space for another run, insert this run into the next level directly
			if(level->size < level->count){
				InsertRun(level, runcount, runsize, start, end, filtered, file, bloom);
			//if there is no space for another run, insert this run into an existing run with space
			}else{
				if(minpos != -1){
					//有可以插入的run
					//先把旧run里面的东西都给读出来
					Run oldrun = level->array[minpos];
					Node *oldarray = (Node *) malloc(oldrun->count * sizeof(Node));
					fread(oldarray, sizeof(Node), oldrun->count, oldrun->fencepointer);
					//搞1个heap放旧run里的东西
					Heap *h = (Heap *) malloc(sizeof(Heap));
					h->array = oldarray;
					h->count = oldrun->count;
					//注意如何设置heap的大小可以把新run里的nodes也都插进去
					h->size = oldrun->count + runcount;
					//把这个run里面的东西也都给读出来
					Node *inserting = (Node *) malloc(runcount * sizeof(Node));
					fread(inserting, sizeof(Node), runcount, file);
					//把这个run里的node都插入heap，后插入新run以保证run里的东西会被更新
					for(i = 0; i < runcount; i++){
						int position = GetKeyPos(h, inserting[i].key);
						if(position != -1){
							h->array[position].value = inserting[i].value;
							h->array[position].flag = inserting[i].flag;
						}else{
							InsertKey(h, inserting[i].key, inserting[i].value, inserting[i].flag);
						}
					}
					//获得超大array后，看怎么把这个run给塞回去
					if(h->count <= oldrun->size){
						//直接更新oldrun里的一些参数
						FILE *fptr = fopen("a.txt", "w");//关于每个run的文件名也要好好想想TODO
						fwrite(h->array, sizeof(Node), h->count, fptr);
						fclose(fptr);
						oldrun->count = h->count;
						oldrun->start = h->array[0].key;
						oldrun->end = h->array[h->count - 1].key;
						oldrun->fencepointer = fptr;
						//之后还要更新bloom
						oldrun->bloom = NULL;
					}else{
						//如果融合后的run size已经超过了这层的run size
						//把这一层的旧run先填好，因为没法直接把它pop出来，然后再插多出来的那部分
						FILE *fptr = fopen("a.txt", "w");//这个名字肯定要重写TODO
						fwrite(h->array, sizeof(Node), oldrun->size, fptr);
						fclose(fptr);
						oldrun->count = oldrun->size;
						oldrun->start = h->array[0].key;
						oldrun->end = h->array[oldrun->size - 1].key;
						oldrun->fencepointer = fptr;
						//之后还要更新bloom
						oldrun->bloom = NULL;
						FILE *fptr = fopen("a.txt", "w");//这个名字肯定要重写TODO
						fwrite(h->array[oldrun->size], sizeof(Node), (h->count - oldrun->size), fptr);
						fclose(fptr);
						//还有位置就直接插入，没有位置就把这个run 放入下层
						//之后还要更新bloom
						if(DestLevel->count < DestLevel.size){
							InsertRun(DestLevel->level, (h->count - oldrun->size), oldrun->size, 
								h->array[oldrun->size].key, h->array[h->count - 1].key, filtered, fptr, NULL);
						}else{
							//是直接把这个run放到下层吗？？？难道不是pop min出去放到下层吗 TODO要改！！！
							Merge(DestLevel->next, (origin + 1), level->size, filtered,
								(h->count - oldrun->size), (oldrun->size * (level->size + 1)),
								h->array[oldrun->size].key, h->array[h->count - 1].key, fptr, NULL);
						}
					}

				//if no run can be inserted into, push an old run to the next level and then merge the new arrival
				}else{
					//每个run都没位置的时候直接把一个旧run流放到下层去
					Run oldrun = PopRun(level);
					//注意如果level超过一定值，levelsize这个参数应该写1，注意每个参数的真正含义
					Merge(DestLevel->next, (origin + 1), level->size, filtered,
						oldrun->count, (oldrun->size * (level->size + 1)), oldrun->start, oldrun->end, oldrun->fencepointer, oldrun->bloom);
				}
			}
		}
	}
}


