#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(void){

	char *str="x-8&y=9";
	char string[30];
	//sprintf(str,"%s","x=8&y=9");
	sprintf(string,"%s",str);
	
	char buf[1024];
	printf("string:%s\nstrlen:%d\n",string,(int)strlen(string));
	char str1[20]="";
	char str2[20]="";

	sscanf("123456","%s",buf);
	sscanf(string,"%[^&]&%s",str1,str2);
	printf("str1:%s\nstr2:%s\n",str1,str2);
	printf("buf:%s\n",buf);
	//printf("string:%s\ntr1:%s\nstr2:%s\n",string,str1,str1);
	return 0;
}


