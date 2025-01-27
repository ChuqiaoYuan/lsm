// reference: https://www.geeksforgeeks.org/tcp-server-client-implementation-in-c/

#include "lsm.h"

#define PORT 8080
#define SA struct sockaddr 
#define buffersize 32
#define sizeratio 3
#define fprlevel1 0.01
#define loaddirectory false
#define filename "data/load_file"

static LSMtree *lsm = NULL; 

void BuildLSMTree(){
	lsm = (LSMtree *) malloc(sizeof(LSMtree));
	if(lsm == NULL){
		printf("There is not enough memory for an LSM-tree.");
	}
	lsm->buffer = CreateHeap(buffersize);
	lsm->T = sizeratio;
	lsm->L0 = (LevelNode *) malloc(sizeof(LevelNode));
	lsm->L0->level = NULL;
	lsm->L0->number = 0;
	lsm->L0->next = NULL;
	lsm->fpr1 = fprlevel1;
	pthread_mutex_init(&(lsm->lock), NULL);
	if(loaddirectory){
		Load(lsm, filename);
	}
}

void ClearLSMTree(){
	ClearHeap(lsm->buffer);
	LevelNode *current = lsm->L0->next;
	while(current != NULL){
		ClearLevel(current->level);
		free(current);
		current = current->next;
	}
	pthread_mutex_destroy(&(lsm->lock));
}

static char buff[80];
static char result[4096];
  
bool Respond(int sockfd, LSMtree *lsm){ 

	while (1){
		bzero(buff, 80);
		read(sockfd, buff, sizeof(buff));
		printf("Query from client %s \n", buff);
		if(buff[0] == 'p'){
			bzero(result, 4096);
			int pos = 2;
			int key = 0;
			int sign = 1;
			if(buff[pos] == '-'){
				sign = -1;
				pos += 1;
			}
			while((buff[pos] >= '0') && (buff[pos] <= '9')){
				key = key * 10 + (buff[pos] - '0');
				pos += 1;
			}
			key = key * sign;

			int value = 0;
			pos += 1;
			sign = 1;
			if(buff[pos] == '-'){
				sign = -1;
				pos += 1;
			}
			while((buff[pos] >= '0') && (buff[pos] <= '9')){
				value = value * 10 + (buff[pos] - '0');
				pos += 1;
			}
			value = value * sign;
			Put(lsm, key, value, true);
			write(sockfd, result, sizeof(result));
		}else if(buff[0] == 'g'){
			bzero(result, 16);
			int pos = 2;
			int key = 0;
			int sign = 1;
			if(buff[pos] == '-'){
				sign = -1;
				pos += 1;
			}
			while((buff[pos] >= '0') && (buff[pos] <= '9')){
				key = key * 10 + (buff[pos] - '0');
				pos += 1;
			}
			key = key * sign;
			Get(lsm, key, result);
			write(sockfd, result, sizeof(result));
		}else if(buff[0] == 'r'){
			bzero(result, 4096);
			int pos = 2;
			int start = 0;
			int sign = 1;
			if(buff[pos] == '-'){
				sign = -1;
				pos += 1;
			}
			while((buff[pos] >= '0') && (buff[pos] <= '9')){
				start = start * 10 + (buff[pos] - '0');
				pos += 1;
			}
			start = start * sign;

			int end = 0;
			pos += 1;
			sign = 1;
			if(buff[pos] == '-'){
				sign = -1;
				pos += 1;
			}
			while((buff[pos] >= '0') && (buff[pos] <= '9')){
				end = end * 10 + (buff[pos] - '0');
				pos += 1;
			}
			end = end * sign;
			Range(lsm, start, end, result);
			write(sockfd, result, sizeof(result));
		}else if(buff[0] == 'd'){
			bzero(result, 4096);
			int pos = 2;
			int key = 0;
			int sign = 1;
			if(buff[pos] == '-'){
				sign = -1;
				pos += 1;
			}
			while((buff[pos] >= '0') && (buff[pos] <= '9')){
				key = key * 10 + (buff[pos] - '0');
				pos += 1;
			}
			key = key * sign;
			Put(lsm, key, 0, false);
			write(sockfd, result, sizeof(result));
		}else if(buff[0] == 'e'){
			printf("Exit request from client has been received.\n");
			return true;
		}else{
			printf("Queries from client has been finished.\n");
			return false;
		}
	}
}
  
int main(){ 
	int sockfd, connfd, len; 
	struct sockaddr_in servaddr, cli; 
  
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1){ 
		printf("Socket creation failed.\n"); 
		exit(0); 
	}else{
		printf("Socket is successfully created.\n"); 
	}
	bzero(&servaddr, sizeof(servaddr)); 
  
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
	servaddr.sin_port = htons(PORT); 
  
	if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0){ 
		printf("Socket binding failed.\n"); 
		exit(0); 
	} 
	else{
		printf("Socket is successfully binded.\n"); 
	}
  
	if ((listen(sockfd, 5)) != 0){ 
		printf("Server listening failed.\n"); 
		exit(0); 
	} 
	else{
		printf("Server is listening.\n"); 
	}

	BuildLSMTree();

	while(1){
		len = sizeof(cli); 
		connfd = accept(sockfd, (SA*)&cli, &len); 
		if(connfd < 0){ 
			printf("Server acceptance failed.\n"); 
			exit(0); 
		}else{
			printf("Server accepts the client.\n"); 
		}
		bool shutdown = Respond(connfd, lsm);
		if(shutdown){
			ClearLSMTree();
			printf("LSM is cleared.\n");
			break;
		}
	}
	printf("Server is shut down.\n");
	close(sockfd);
	return 0;
} 