#include <stdio.h>
#include <string.h>

int main(void) {
char buff[21];

FILE *output =fopen("output.txt","w");
fgets(buff,21,stdin);
fwrite(buff,strlen(buff),1,output);
fclose(output);
return 0;
}
