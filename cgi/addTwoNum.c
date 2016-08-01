#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


int main(int argc,char **argv){
	
	char *query_string=NULL;
	char *method;
	char *buf=NULL;
	
	char *strnum1;
	char *strnum2;
	int num1;
	int num2;
	char *index;
	int sum;

	char buffer[1024];

	method=getenv("METHOD");
	
	if(strcasecmp(method,"GET")==0){

		query_string=getenv("QUERY_STRING");	
		
	}else{
		//maybe error
		while(read(0,buf,1)){
			strcat(query_string,buf);
		}
	}
	
	sscanf(query_string,"%[^&]&%[^&]",strnum1,strnum2);
	index=strchr(strnum1,'=');
	num1=atoi(++index);

	index=strchr(strnum2,'=');
	num2=atoi(++index);

	sum=num1+num2;

	sprintf(buffer,"Content-Type:text/html\r\n\r\n");
	sprintf(buffer,"%s<html><title>sum of two numbers</title>\r\n",buffer);
	sprintf(buffer,"%s<body><p>the sum of two numbers:%d + %d = %d\r\n",buffer,num1,num2,sum);
	sprintf(buffer,"%s</body></html>\r\n",buffer);

	write(1,buffer,strlen(buffer));

	return 0;

}
