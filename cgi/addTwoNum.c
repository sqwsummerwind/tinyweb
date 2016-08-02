#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


int main(void){
	
	char *query_str;
	char query_string[255];
	char *method;
	char buf[255];
	char *con_len;
	int content_len;
	
	char strnum1[20];
	char strnum2[20];
	int num1;
	int num2;
	char *index;
	int sum;

	char buffer[1024];
	//printf("this is addTwoNum\n");
	method=getenv("REQUEST_METHOD");
	
	//printf("method:%s\n",method);
	if(strcasecmp(method,"GET")==0){

		query_str=getenv("QUERY_STRING");	
		sprintf(query_string,"%s",query_str);

	}else{
		con_len=getenv("CONTENT_LENGTH");
		content_len=atoi(con_len);
		//maybe error
		/*while(read(0,buf,1)){
			strcat(query_string,buf);
		}*/

		read(0,query_string,content_len);

	}
	
	sscanf(query_string,"%[^&]&%s",strnum1,strnum2);
	index=strchr(strnum1,'=');
	num1=atoi(++index);

	index=strchr(strnum2,'=');
	num2=atoi(++index);

	sum=num1+num2;

	sprintf(buffer,"Content-Type:text/html\r\n\r\n");
	sprintf(buffer,"%s<html><title>sum of two numbers.</title>\r\n",buffer);
	sprintf(buffer,"%s<body><p>the sum of two numbers:%d + %d = %d\r\n",buffer,num1,num2,sum);
	//sprintf(buffer,"%s<body><p>the sum of two numbers\n",buffer);
	//sprintf(buffer,"%s<p>query string:%s\n",buffer,query_string);
	sprintf(buffer,"%s</body></html>\r\n",buffer);

	//write(1,buffer,strlen(buffer));
	printf("%s",buffer);
	
//	fflush(1);
	exit(0);

}
