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


void Merge(LevelNode *DestLevel, int origin, int levelsize, bool filtered,
	int runcount, int runsize, int start, int end, char *file, BloomFilter *bloom){
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


