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
			//把已有的runs都用新的array填满
			//这个时候如果count == size (相当于原本的level就是满的)，就丢一个run下去，再插入最后的多出来的run
			//这个时候如果count < size,就直接插入多出来的这个run








					Run oldrun = destlevel->array[minpos];
					Node *oldarray = (Node *) malloc(oldrun->count * sizeof(Node));
					FILE *fp = fopen(oldrun->fencepointer, "rt");
					fread(oldarray, sizeof(Node), oldrun->count, fp);
					fclose(fp);

					Heap *h = (Heap *) malloc(sizeof(Heap));
					h->array = oldarray;
					h->count = oldrun->count;
					h->size = oldrun->count + runcount;

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

					//if(h->count <= oldrun->size){//这个判断在找minpos的时候就做过了
					FILE *fp = fopen(oldrun->fencepointer, "wt");
					fwrite(h->array, sizeof(Node), h->count, fp);
					fclose(fp);
					oldrun->count = h->count;
					oldrun->start = h->array[0].key;
					oldrun->end = h->array[h->count - 1].key;
					//之后需要更新这个bloom
					oldrun->bloom = NULL;
		}
	}
}