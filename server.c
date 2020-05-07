#include <stdio.h> 
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include "lsm.h"
#define PORT 8080 
#define SA struct sockaddr 
  
bool Respond(int sockfd, LSMtree *lsm){ 
	char buff[80]; 
	char result[4096];

	while (1){
		bzero(buff, 80);
		bzero(result, 4096);
		read(sockfd, buff, sizeof(buff));
		printf("Query from client %s \n", buff);
		if(buff[0] == 'p'){
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
			printf("key %d value %d \n", key, value);
			Put(lsm, key, value, true);
			write(sockfd, result, sizeof(result));
		}else if(buff[0] == 'g'){
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
			printf("key %d \n", key);
			Get(lsm, key, result);
			write(sockfd, result, sizeof(result));
		}else if(buff[0] == 'r'){
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
			printf("start %d end %d \n", start, end);
			Range(lsm, start, end, result);
			write(sockfd, result, sizeof(result));
		}else if(buff[0] == 'd'){
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
			printf("key %d \n", key);
			Put(lsm, key, 0, false);
			write(sockfd, result, sizeof(result));
		}else if(buff[0] == 'e'){
			printf("Here it is 1\n");
			return true;
		}else{
			return false;
		}
		printf("Here it is 2\n");
	}
	printf("Here it is 3\n");
}
  
int main(){ 
	int sockfd, connfd, len; 
	struct sockaddr_in servaddr, cli; 
  
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1){ 
		printf("socket creation failed...\n"); 
		exit(0); 
	}else{
		printf("Socket successfully created..\n"); 
	}
	bzero(&servaddr, sizeof(servaddr)); 
  
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
	servaddr.sin_port = htons(PORT); 
  
	if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0){ 
		printf("socket bind failed...\n"); 
		exit(0); 
	} 
	else{
		printf("Socket successfully binded..\n"); 
	}
  
	if ((listen(sockfd, 5)) != 0){ 
		printf("Listen failed...\n"); 
		exit(0); 
	} 
	else{
		printf("Server listening..\n"); 
	}

	int buffersize = 100;
	int sizeratio = 10;
	double fprlevel1 = 0.00000001;
	LSMtree *lsm = BuildLSMTree(buffersize, sizeratio, fprlevel1, false, "data/load_file");

	while(1){
		len = sizeof(cli); 
		connfd = accept(sockfd, (SA*)&cli, &len); 
		if(connfd < 0){ 
			printf("server acccept failed...\n"); 
			exit(0); 
		}else{
			printf("server acccept the client...\n"); 
		}
		bool shutdown = Respond(connfd, lsm);
		if(shutdown){
			break;
		}
	}
  
	//respond(connfd); 
	printf("Here it is 4\n");
	close(sockfd);
	printf("Here it is 5\n");
	return 0;
} 