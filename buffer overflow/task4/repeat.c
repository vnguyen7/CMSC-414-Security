#include <stdio.h>
#include <string.h>

int main(void){
	char buff[21];
	FILE *input = fopen("output.txt","r");
	if(!input){
		printf("output.txt does not exist");
	}
	else {
		fgets(buff,sizeof(buff),input);
		fwrite(buff,strlen(buff),1,stdout);
		fclose(input);
	}
return 0;
}
