#include <stdio.h>
#include <stdlib.h>

int main(){
	int data[200];
	int i;
	for(i = 0; i < 200; i++){
		data[i] = - 50 + rand() % 200;
	}
	FILE *fp = fopen("data/load_file", "wb");
	fwrite(data, sizeof(int), 200, fp);
	fclose(fp);
	return 0;
}