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
	lsm->L0 = (LevelNode *) malloc(sizeof(LevelNode));
	lsm->L0->level = NULL;
	lsm->L0->number = 0;
	lsm->L0->next = NULL;
	lsm->fpr1 = fpr;
	pthread_mutex_init(&(lsm->lock), NULL);
	return lsm;
}

void Merge(LevelNode *Current, int origin, int levelsize,
	int runcount, int runsize, Node *sortedrun, double targetfpr){
	//if the destination level does not exist, initiate a new level and insert the run
	if(Current->next == NULL){
		Current->next = (LevelNode *) malloc(sizeof(LevelNode));
		Current->next->level = CreateLevel(levelsize, targetfpr);
		int start = sortedrun[0].key;
		int end = sortedrun[runcount - 1].key;
		char filename[14];
		sprintf(filename, "data/L%dN%d", (origin+1), Current->next->level->count);
		FILE *fp = fopen(filename, "wt");
		fwrite(sortedrun, sizeof(Node), runcount, fp);
		fclose(fp);
		if(targetfpr < 0.3){
			double numbits = - 2 * runcount * log(targetfpr);
			size_t m = (size_t)numbits;
			double numhashes = 0.7 * numbits / runcount;
			int k = (int)numhashes;
			Current->next->level->filters[Current->next->level->count].k = k;
			Current->next->level->filters[Current->next->level->count].size = m;
			Current->next->level->filters[Current->next->level->count].bits = malloc(m);
			int i;
			for(i = 0; i < runcount; i++){
				InsertEntry(&Current->next->level->filters[Current->next->level->count], sortedrun[i].key);
			}
		}
		InsertRun(Current->next->level, runcount, runsize, start, end);
		Current->next->number = origin + 1;
		Current->next->next = NULL;
	}else{
		//do some statistics to enable partial compaction
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
		if(j == 0){
			//if there is no run with overlapping keys in the destination level
			if(destlevel->count < destlevel->size){
				//if there is still space for another run, insert this run directly
				int start = sortedrun[0].key;
				int end = sortedrun[runcount - 1].key;
				char filename[14];
				sprintf(filename, "data/L%dN%d", (origin+1), destlevel->count);
				FILE *fp = fopen(filename, "wt");
				fwrite(sortedrun, sizeof(Node), runcount, fp);
				fclose(fp);

				if(targetfpr < 0.3){
					double numbits = - 2 * runcount * log(targetfpr);
					size_t m = (size_t)numbits;
					double numhashes = 0.7 * numbits / runcount;
					int k = (int)numhashes;
					destlevel->filters[destlevel->count].k = k;
					destlevel->filters[destlevel->count].size = m;
					destlevel->filters[destlevel->count].bits = malloc(m);
					for(i = 0; i < runcount; i++){
						InsertEntry(&destlevel->filters[destlevel->count], sortedrun[i].key);
					}
				}
				InsertRun(destlevel, runcount, runsize, start, end);
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
						sprintf(name, "data/L%dN%d", Current->next->number, minpos);
						FILE *fp = fopen(name, "rt");
						fread(&newarray[runcount], sizeof(Node), oldrun.count, fp);
						fclose(fp);
					}else{
						char name[14];
						sprintf(name, "data/L%dN%d", Current->next->number, minpos);
						FILE *fp = fopen(name, "rt");
						fread(newarray, sizeof(Node), oldrun.count, fp);
						fclose(fp);
						for(i = 0; i < runcount; i++){
							newarray[oldrun.count + i] = sortedrun[i];
						}
					}
					char name[14];
					sprintf(name, "data/L%dN%d", Current->next->number, minpos);
					FILE *fp = fopen(name, "wt");
					fwrite(newarray, sizeof(Node), (oldrun.count + runcount), fp);
					fclose(fp);

					if(targetfpr < 0.3){
						double numbits = - 2 * (oldrun.count + runcount) * log(targetfpr);
						size_t m = (size_t)numbits;
						double numhashes = 0.7 * numbits / (oldrun.count + runcount);
						int k = (int)numhashes;
						destlevel->filters[minpos].k = k;
						destlevel->filters[minpos].size = m;
						destlevel->filters[minpos].bits = malloc(m);
						for(i = 0; i < (oldrun.count + runcount); i++){
							InsertEntry(&destlevel->filters[minpos], newarray[i].key);
						}
					}
					oldrun.count += runcount;
					oldrun.start = newarray[0].key;
					oldrun.end = newarray[oldrun.count - 1].key;
					destlevel->array[minpos] = oldrun;
					free(newarray);
				}else{
					//there is no space in this run to merge something into 
					// so minpos is -1 here
					Run pushtonext = PopRun(destlevel);
					Node *topush = (Node *) malloc(pushtonext.count * sizeof(Node));
					char name[14];
					sprintf(name, "data/L%dN%d", Current->next->number, destlevel->count);
					FILE *fp = fopen(name, "rt");
					fread(topush, sizeof(Node), pushtonext.count, fp);
					fclose(fp);
					if(origin <= 7){
						Merge(Current->next, (origin + 1), levelsize, 
							pushtonext.count, pushtonext.size * (levelsize + 1), topush, (targetfpr * (levelsize + 1)));
					}else if (origin == 8){
						Merge(Current->next, (origin + 1), 1,
							pushtonext.count, (pushtonext.size * (levelsize + 1) * levelsize), topush, (targetfpr * (levelsize + 1) * levelsize));
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
						sprintf(name, "data/L%dN%d", Current->next->number, (destlevel->count - 1));
						FILE *fp = fopen(name, "rt");
						fread(&newarray[runcount], sizeof(Node), oldrun.count, fp);
						fclose(fp);
					}else{
						char name[14];
						sprintf(name, "data/L%dN%d", Current->next->number, (destlevel->count - 1));
						FILE *fp = fopen(name, "rt");
						fread(newarray, sizeof(Node), oldrun.count, fp);
						fclose(fp);
						for(i = 0; i < runcount; i++){
							newarray[oldrun.count + i] = sortedrun[i];
						}
					}					

					char newname[14];
					sprintf(newname, "data/L%dN%d", Current->next->number, (destlevel->count - 1));
					fp = fopen(newname, "wt");
					fwrite(newarray, sizeof(Node), oldrun.size, fp);
					fclose(fp);

					if(targetfpr < 0.3){
						double numbits = - 2 * oldrun.size * log(targetfpr);
						size_t m = (size_t)numbits;
						double numhashes = 0.7 * numbits / oldrun.size;
						int k = (int)numhashes;
						destlevel->filters[destlevel->count - 1].k = k;
						destlevel->filters[destlevel->count - 1].size = m;
						destlevel->filters[destlevel->count - 1].bits = malloc(m);
						for(i = 0; i < oldrun.size; i++){
							InsertEntry(&destlevel->filters[destlevel->count - 1], newarray[i].key);
						}
					}

					oldrun.count = oldrun.size;
					oldrun.start = newarray[0].key;
					oldrun.end = newarray[oldrun.size - 1].key;

					destlevel->array[destlevel->count - 1] = oldrun;

					char filename[14];
					sprintf(filename, "data/L%dN%d", (origin+1), destlevel->count);
					FILE *fpw = fopen(filename, "wt");
					fwrite(&newarray[oldrun.size], sizeof(Node), (oldrun.count + runcount - oldrun.size), fpw);
					fclose(fpw);

					if(targetfpr < 0.3){
						double numbits = - 2 * (oldrun.count + runcount - oldrun.size) * log(targetfpr);
						size_t m = (size_t)numbits;
						double numhashes = 0.7 * numbits / (oldrun.count + runcount - oldrun.size);
						int k = (int)numhashes;
						destlevel->filters[destlevel->count].k = k;
						destlevel->filters[destlevel->count].size = m;
						destlevel->filters[destlevel->count].bits = malloc(m);
						for(i = 0; i < (oldrun.count + runcount - oldrun.size); i++){
							InsertEntry(&destlevel->filters[destlevel->count], newarray[oldrun.size + i].key);
						}
					}

					InsertRun(destlevel, (oldrun.count + runcount - oldrun.size), oldrun.size, 
						newarray[oldrun.size].key, newarray[oldrun.count + runcount - 1].key);
					free(newarray);
				}
			}
		}else{
			//if there are runs with overlapping keys in the destination level
			int oldcount = 0;
			for(i = 0; i < j; i++){
				oldcount += destlevel->array[overlap[i]].count;
			}

			Node *oldarray = (Node *) malloc(oldcount * sizeof(Node));

			oldcount = 0;
			for(i = 0; i < j; i++){
				char name[14];
				sprintf(name, "data/L%dN%d", Current->next->number, overlap[i]);
				FILE *fp = fopen(name, "rt");
				fread(&oldarray[oldcount], sizeof(Node), destlevel->array[overlap[i]].count, fp);
				fclose(fp);
				oldcount += destlevel->array[overlap[i]].count;
			}

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

			int numrun;
			if(c % destlevel->array[0].size == 0){
				numrun = c / destlevel->array[0].size;
			}else{
				numrun = c / destlevel->array[0].size + 1;
			}
			if(numrun <= j){
				//existing runs can hold these keys
				for(i = 0; (i < (numrun - 1)); i++){
					Run oldrun = destlevel->array[overlap[i]];
					char name[14];
					sprintf(name, "data/L%dN%d", Current->next->number, overlap[i]);
					FILE *fp = fopen(name, "wt");
					fwrite(&newarray[i * oldrun.size], sizeof(Node), oldrun.size, fp);
					fclose(fp);

					if(targetfpr < 0.3){
						double numbits = - 2 * oldrun.size * log(targetfpr); 
						size_t m = (size_t)numbits;
						double numhashes = 0.7 * numbits / oldrun.size;
						int k = (int)numhashes;
						destlevel->filters[overlap[i]].k = k;
						destlevel->filters[overlap[i]].size = m;
						destlevel->filters[overlap[i]].bits = malloc(m);
						int ii;
						for(ii = 0; ii < oldrun.size; ii++){
							InsertEntry(&destlevel->filters[overlap[i]], newarray[i * oldrun.size + ii].key); 
						}
					}

					oldrun.count = oldrun.size;
					oldrun.start = newarray[i * oldrun.size].key;
					oldrun.end = newarray[(i + 1) * oldrun.size - 1].key;
					destlevel->array[overlap[i]] = oldrun;
				}
				Run oldrun = destlevel->array[overlap[numrun - 1]];
				char name[14];
				sprintf(name, "data/L%dN%d", Current->next->number, overlap[numrun - 1]);
				FILE *fp = fopen(name, "wt");
				fwrite(&newarray[(numrun - 1) * oldrun.size], sizeof(Node), (c - (numrun - 1) * oldrun.size), fp);
				fclose(fp);

				if(targetfpr < 0.3){
					double numbits = - 2 * (c - (numrun - 1) * oldrun.size) * log(targetfpr); 
					size_t m = (size_t)numbits;
					double numhashes = 0.7 * numbits / (c - (numrun - 1) * oldrun.size);
					int k = (int)numhashes;
					destlevel->filters[overlap[numrun - 1]].k = k;
					destlevel->filters[overlap[numrun - 1]].size = m;
					destlevel->filters[overlap[numrun - 1]].bits = malloc(m);
					for(i = 0; i < (c - (numrun - 1) * oldrun.size); i++){
						InsertEntry(&destlevel->filters[overlap[numrun - 1]], newarray[(numrun - 1) * oldrun.size + i].key); 
					}
				}

				oldrun.count = c - (numrun - 1) * oldrun.size;
				oldrun.start = newarray[(numrun - 1) * oldrun.size].key;
				oldrun.end = newarray[c - 1].key;
				destlevel->array[overlap[numrun - 1]] = oldrun;

				if(numrun < j){
					for(i = numrun; i < j; i++){
						oldrun = destlevel->array[overlap[i]];
						oldrun.count = 0;
						oldrun.start = INT_MAX + 1;
						oldrun.end = INT_MAX;
						destlevel->array[overlap[i]] = oldrun;
					}
				}
			}else{
				for(i = 0; i < j; i++){
					Run oldrun = destlevel->array[overlap[i]];
					char name[14];
					sprintf(name, "data/L%dN%d", Current->next->number, overlap[i]);
					FILE *fp = fopen(name, "wt");
					fwrite(&newarray[i * oldrun.size], sizeof(Node), oldrun.size, fp);
					fclose(fp);

					if(targetfpr < 0.3){
						double numbits = - 2 * oldrun.size * log(targetfpr); 
						size_t m = (size_t)numbits;
						double numhashes = 0.7 * numbits / oldrun.size;
						int k = (int)numhashes;
						destlevel->filters[overlap[i]].k = k;
						destlevel->filters[overlap[i]].size = m;
						destlevel->filters[overlap[i]].bits = malloc(m);
						int ii;
						for(ii = 0; ii < oldrun.size; ii++){
							InsertEntry(&destlevel->filters[overlap[i]], newarray[i * oldrun.size + ii].key); //注意array其实
						}
					}

					oldrun.count = oldrun.size;
					oldrun.start = newarray[i * oldrun.size].key;
					oldrun.end = newarray[(i + 1) * oldrun.size - 1].key;
					destlevel->array[overlap[i]] = oldrun;
				}

				if(destlevel->count == destlevel->size){
					//if there is no space for the new array, push the last run to the next level
					Run pushtonext = PopRun(destlevel);
					Node *topush = (Node *) malloc(pushtonext.count * sizeof(Node));
					char name[14];
					sprintf(name, "data/L%dN%d", Current->next->number, destlevel->count);
					FILE *fp = fopen(name, "rt");
					fread(topush, sizeof(Node), pushtonext.count, fp);
					fclose(fp);
					if(origin <= 7){
						Merge(Current->next, (origin + 1), levelsize, 
							pushtonext.count, (pushtonext.size * (levelsize + 1)), topush, (targetfpr * (levelsize + 1)));
					}else if (origin == 8){
						Merge(Current->next, (origin + 1), 1, 
							pushtonext.count, (pushtonext.size * (levelsize + 1) * levelsize), topush, (targetfpr * (levelsize + 1) * levelsize));
					}else{
						printf("No more entries in this LSM tree. Some data may be lost. \n");
					}					
				}

				//insert the run directly to the current level 
				char filename[14];
				sprintf(filename, "data/L%dN%d", (origin+1), destlevel->count);
				Run oldrun = destlevel->array[0];
				FILE *fp = fopen(filename, "wt");
				fwrite(&newarray[j * oldrun.size], sizeof(Node), (c - j*oldrun.size), fp);
				fclose(fp);

				if(targetfpr < 0.3){
					double numbits = - 2 * (c - j*oldrun.size) * log(targetfpr); 
					size_t m = (size_t)numbits;
					double numhashes = 0.7 * numbits / (c - j*oldrun.size);
					int k = (int)numhashes;
					destlevel->filters[destlevel->count].k = k;
					destlevel->filters[destlevel->count].size = m;
					destlevel->filters[destlevel->count].bits = malloc(m);
					for(i = 0; i < (c - j*oldrun.size); i++){
						InsertEntry(&destlevel->filters[destlevel->count], newarray[j * oldrun.size + i].key); //注意array其实
					}
				}
				InsertRun(destlevel, (c - j * oldrun.size), oldrun.size,
					newarray[j * oldrun.size].key, newarray[c- 1].key);
			}
			free(newarray);
			free(oldarray);
		}
		free(overlap);
		free(distance);
	}
}

void Put(LSMtree *lsm, int key, int value, bool flag){
	//check if the key is already in the buffer
	//update the key directly in the buffer if it is here to save time and space
	int position = GetKeyPos(lsm->buffer, key);
	if(position >= 0){
		lsm->buffer->array[position].value = value;
		lsm->buffer->array[position].flag = flag;
	}else{
		//insert key directly if the buffer is not full
		if(lsm->buffer->count < lsm->buffer->size){
			InsertKey(lsm->buffer, key, value, flag);
		}else if(lsm->buffer->count == lsm->buffer->size){
			//clear the buffer and get the sorted run
			int i;
			Node *sortedrun = (Node *) malloc(lsm->buffer->size * sizeof(Node));
			for(i = 0; i < lsm->buffer->size; i++){
				sortedrun[i] = PopMin(lsm->buffer);
			}
			//merge this sorted run into level 1
			Merge(lsm->L0, 0, (lsm->T - 1), 
				lsm->buffer->size, lsm->buffer->size, sortedrun, lsm->fpr1);
			InsertKey(lsm->buffer, key, value, flag);
		}
	}
}

void Get(LSMtree *lsm, int key, char *result){
	int position = GetKeyPos(lsm->buffer, key);
	int i;
	if(position != -1){
		if(lsm->buffer->array[position].flag){
			int val = lsm->buffer->array[position].value;
			sprintf(result, "%d", val);
		}else{
		}
	}else{
		LevelNode *current = lsm->L0->next;
		bool find = false;
		while(current != NULL){
			Level *exploringlevel = current->level;
			for(i = 0; i < exploringlevel->count; i++){
				if((exploringlevel->array[i].start <= key) && (exploringlevel->array[i].end >= key)){
					if(LookUp(&exploringlevel->filters[i], key)){
						//binary search through the run to search key
						Node *currentarray = (Node *) malloc(exploringlevel->array[i].count * sizeof(Node));
						char filename[14];
						sprintf(filename, "data/L%dN%d", current->number, i);
						FILE *fp = fopen(filename, "rt");
						fread(currentarray, sizeof(Node), exploringlevel->array[i].count, fp);
						fclose(fp);
						int left = 0;
						int right = exploringlevel->array[i].count - 1;
						int mid = (left + right) / 2;
						if(key == currentarray[left].key){
							if(currentarray[left].flag){
								int val = currentarray[left].value;
								sprintf(result, "%d", val);
								find = true;
								break;
							}else{
								return;
							}
						}else if(key == currentarray[right].key){
							if(currentarray[right].flag){
								int val = currentarray[right].value;
								sprintf(result, "%d", val);
								find = true;
								break;
							}else{
								return;
							}
						}
						while(left != mid){
							if(key == currentarray[mid].key){
								if(currentarray[mid].flag){
									int val = currentarray[mid].value;
									sprintf(result, "%d", val);
									find = true;
									break;
								}else{
									return;
								}
							}else if(key > currentarray[mid].key){
								left = mid;
								mid = (left + right) / 2;
							}else{
								right = mid;
								mid = (left + right) / 2;
							}
						}
					}
				}
			}
			if(find){
				break;
			}
			current = current->next;
		}
		return;
	}
}

void Range(LSMtree *lsm, int start, int end, char *result){
	int i;
	int j;
	int find = 0;
	//use hash table to record keys found
	HashTable *table = CreateHashTable(101);
	char str[32];
	for(i = 0; i < lsm->buffer->count; i++){
		if((lsm->buffer->array[i].key >= start) && (lsm->buffer->array[i].key < end)){
			if(!CheckTable(table, lsm->buffer->array[i].key)){
				find += 1;
				AddToTable(table, lsm->buffer->array[i].key);
				if(lsm->buffer->array[i].flag){
					bzero(str, 32);
					sprintf(str, "%d:%d ", lsm->buffer->array[i].key, lsm->buffer->array[i].value);
					strcat(result, str);
				}
			}
		}
		if(find == (end - start)){
			break;
		}
	}
	LevelNode *currentlevelnode = lsm->L0->next;
	while(currentlevelnode != NULL){
		if(find == (end - start)){
			break;
		}
		int levelnum = currentlevelnode->number;
		for(i = 0; i < currentlevelnode->level->count; i++){
			if((currentlevelnode->level->array[i].end >= start) || (currentlevelnode->level->array[i].start < end)){
				Node *currentarray = (Node *) malloc(currentlevelnode->level->array[i].count * sizeof(Node));
				char filename[14];
				sprintf(filename, "data/L%dN%d", levelnum, i);
				FILE *fp = fopen(filename, "rt");
				fread(currentarray, sizeof(Node), currentlevelnode->level->array[i].count, fp);
				fclose(fp);
				for(j = 0; j < currentlevelnode->level->array[i].count; j++){
					if(currentarray[j].key >= end){
						break;
					}else if(currentarray[j].key >= start){
						if(!CheckTable(table, currentarray[j].key)){
							find += 1;
							AddToTable(table, currentarray[j].key);
							if(currentarray[j].flag){
								bzero(str, 32);
								sprintf(str, "%d:%d ", currentarray[j].key, currentarray[j].value);
								strcat(result, str);
							}
						}
					}
				}
			}
		}
		currentlevelnode = currentlevelnode->next;
	}
	ClearTable(table);
}

//load data if there is a directory
void Load(LSMtree *lsm, char *binaryfile){
	int data[1024];
	FILE *fp = fopen(binaryfile, "rb");
	int count = fread(data, sizeof(int), 1024, fp);
	fclose(fp);
	int i;
	for(i = 0; i < (count / 2); i++){
		Put(lsm, data[2 * i], data[2 * i + 1], true);
	}
}

void PrintStats(LSMtree *lsm){
	int i;
	int j;
	int total = 0;

	LevelNode *Current = lsm->L0;
	LevelNode *currentlevelnode = lsm->L0->next;

	while(currentlevelnode != NULL){
		int levelnum = currentlevelnode->number;
		int currentcount = 0;
		for(i = 0; i < currentlevelnode->level->count; i++){
			currentcount += currentlevelnode->level->array[i].count;
			Node *currentarray = (Node *) malloc(currentlevelnode->level->array[i].count * sizeof(Node));
			char filename[14];
			sprintf(filename, "data/L%dN%d", levelnum, i);
			FILE *fp = fopen(filename, "rt");
			fread(currentarray, sizeof(Node), currentlevelnode->level->array[i].count, fp);
			fclose(fp);
			for(j = 0; j < currentlevelnode->level->array[i].count; j++){
				printf("%d:%d:L%d  ", currentarray[j].key, currentarray[j].value, levelnum);
				if(!currentarray[j].flag){
					total -= 1;
				}
			}
			printf("a run has ended. \n");
		}
		printf("There are %d pairs on Level %d. \n\n", currentcount, levelnum);
		total += currentcount;
		currentlevelnode = currentlevelnode->next;
	}
	printf("There are %d pairs on the LSM-tree in total. \n", total);
}

/*
int main(){
	LSMtree *lsm = CreateLSM(4, 4, 0.0000001);
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
	Put(lsm, -39, -78, true);

	Load(lsm, "data/load_file");
	
	char val[16];
	int key = -39;
	Get(lsm, key, val);
	printf("value of key %d is %s \n", key, val);
	printf("\n");

	int start = 90;
	int end = 103;
	char result[1000] = {""};
	Range(lsm, start, end, result);
	printf("range query result for [%d, %d) is %s\n", start, end, result);
	printf("\n");

	PrintNode(lsm->buffer);
	printf("\n");
	PrintStats(lsm);
	ClearLSM(lsm);
	return 0;
}
*/
