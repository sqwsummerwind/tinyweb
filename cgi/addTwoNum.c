/*
一个小小的cgi程序
把两个数相加，返回运行后的结果
注意：
参数形式：x=num&y=num
2016.8.2
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void send_result(int,int,int);
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
	method=getenv("REQUEST_METHOD");
	
	if(strcasecmp(method,"GET")==0){

		query_str=getenv("QUERY_STRING");	
		sprintf(query_string,"%s",query_str);

	}else{
		con_len=getenv("CONTENT_LENGTH");
		content_len=atoi(con_len);
		//从管道中读取参数内容，因为管道已经重定向到stdin，0为stdin
		read(0,query_string,content_len);
	}
	
	//进行参数分离，特别要主要这个函数的参数
	//参数只能是 数组类型 不能是 char * 类型
	sscanf(query_string,"%[^&]&%s",strnum1,strnum2);
	index=strchr(strnum1,'=');
	num1=atoi(++index);

	index=strchr(strnum2,'=');
	num2=atoi(++index);

	sum=num1+num2;
	
	send_result(num1,num2,sum);
	
	exit(0);

}

void send_result(int num1,int num2,int sum){
	char buffer[1024];
	
	sprintf(buffer,"Content-Type:text/html\r\n\r\n");
	sprintf(buffer,"%s<html><title>sum of two numbers.</title>\r\n",buffer);
	sprintf(buffer,"%s<body><p>the sum of two numbers:%d + %d = %d\r\n",buffer,num1,num2,sum);
	sprintf(buffer,"%s</body></html>\r\n",buffer);
	printf("%s",buffer);
	
}
