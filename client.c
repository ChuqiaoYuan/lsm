//主要任务是读完workload.txt然后把每一行逐条发给server，并且读server的回执，当每一条都收到回执后再关上

#include <netdb.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <time.h>
#include <sys/socket.h> 
#define PORT 8080 
#define SA struct sockaddr 

void query(int sockfd){
	char buff[80];
	char result[1000];
	FILE *fp = fopen("workload.txt", "rt");
	while(!feof(fp)){
		fgets(buff, 80, fp);
		write(sockfd, buff, sizeof(buff));
		bzero(buff, sizeof(buff));
		read(sockfd, result, sizeof(result));
		printf("%s\n", result);
		bzero(result, sizeof(result));
	}
	fclose(fp);
}

int main(){
	clock_t start_t, finish_t;
	double total_t = 0;
	int sockfd, connfd;
	struct sockaddr_in servaddr, cli;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1){
		printf("Socket creation failed.\n");
		exit(0);
	}else{
		printf("Socket successfully created.\n");
	}
	bzero(&servaddr, sizeof(servaddr));

    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
    servaddr.sin_port = htons(PORT);

    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) { 
        printf("connection with the server failed...\n"); 
        exit(0); 
    } 
    else
        printf("connected to the server..\n"); 
    start_t = clock();
    query(sockfd);
    finish_t = clock();
    total_t = (double)(finish_t - start_t) / CLOCKS_PER_SEC;
    printf("Latency %f seconds \n", total_t);
    close(sockfd);
    return 0;
}