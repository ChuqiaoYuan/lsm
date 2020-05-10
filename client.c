// reference: https://www.geeksforgeeks.org/tcp-server-client-implementation-in-c/

#include "lsm.h"
#define PORT 8080
#define SA struct sockaddr 

static char buff[80];
static char result[4096];

void Query(int sockfd, char *filename){
	FILE *fp = fopen(filename, "rt");
	while(!feof(fp)){
		fgets(buff, 80, fp);
		if(strlen(buff) == 0.0){
			break;
		}
		printf("%s\n", buff);
		write(sockfd, buff, sizeof(buff));
		bzero(buff, sizeof(buff));
		read(sockfd, result, sizeof(result));
		printf("%s\n", result);
		bzero(result, sizeof(result));
	}
	fclose(fp);
}

int main(int argc, char **argv){
	clock_t start_t, finish_t;
	double total_t = 0;
	int sockfd, connfd;
	struct sockaddr_in servaddr, cli;
	char *filename;
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

	if(connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) { 
		printf("Connection with the server failed.\n"); 
		exit(0); 
	}else{
		printf("Connected to the server.\n"); 
	}
	start_t = clock();
	filename = (argc > 1) ? argv[1] : "workload.txt";
	Query(sockfd, filename);
	finish_t = clock();
	total_t = (double)(finish_t - start_t) / CLOCKS_PER_SEC;
	printf("Query ended.\n");
	printf("Latency: %f seconds \n", total_t);
	close(sockfd);
	return 0;
}