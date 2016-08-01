#include <arpa/inet.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <pthread.h>

int start_up(int port);
void error_die(const char *msg);
void deal_request(void *server_sock);
int get_line(int,char *,int);
void uncompleted(int,char *);

int main(int argc,char **argv)
{
	int server_sock;
	int client_sock;
	//端口号
	int port;
	int client_addr_len;
	struct sockaddr_in client_addr;
	pthread_t thread_id;
	if(argc!=2){
		perror("please run it like: ./tinyweb port");
		exit(EXIT_FAILURE);
	}
	
	//把字符串类型端口号转为整型
	port = atoi(argv[1]);
	
	server_sock=start_up(port);
	client_addr_len=sizeof(client_addr);

	while(1){

		client_sock = accept(server_sock,(struct sockaddr *)&client_addr,&client_addr_len);

		if(client_sock == -1){
			
			error_die("accept error\n");
		}
		
		
		if(pthread_create(&thread_id,NULL,(void *)deal_request,(void *)&client_sock)!=0)
			error_die("pthread_create error\n");
	}
	
	close(server_sock);
	return 0;
}

//start_up function
//socket
//bind
//listen

int start_up(int port){
	
	int sock_id;
	struct sockaddr_in name;
	
	//socket
	if((sock_id=socket(AF_INET,SOCK_STREAM,0))==-1){
		error_die("socket error\n");
	}
	
	memset(&name,0,sizeof(name));
	name.sin_family=AF_INET;
	name.sin_port=htons(port);
	name.sin_addr.s_addr=htonl(INADDR_ANY);

	if(bind(sock_id,(struct sockaddr *)&name,sizeof(name))==-1){
		error_die("bind error\n");
	}

	if(listen(sock_id,10)==-1){
		error_die("listen error\n");
	}

	return sock_id;

}

//error_die
void error_die(const char *msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

//deal_request
void deal_request(void *client_sock){
	
	int cli_sock =*(int*)client_sock;
	int charnum=0;
	char buf[1024];
	char method[10];
	char url[255];
	char protocol[50];
	bool cgi=false;

	charnum=get_line(cli_sock,buf,sizeof(buf));
	if(charnum==0){
		error_die("get_line error\n");
	}

	if(EOF==sscanf(buf,"%[^ ] %[^ ] %[^ ]",method,url,protocol)){
		error_die("sscanf error");
	}

	//method is not get and post
	if((strcasecmp(method,"GET")!=0)&&(strcasecmp(method,"POST")!=0)){
		
		uncompleted(cli_sock,method);	
	}


	close(cli_sock);

}

//get a line
/*int get_line(int client_sock,char *buf){

	int i=0;
	int n=1;
	char c='\0';
	while(n>0&&c!='\n'){
		
		n=recv(client_sock,&c,1,MSG_DONTWAIT);
		if(c=='\r'){
			n=recv(client_sock,&c,1,MSG_PEEK);
			if(n&&c=='\n'){
				n=recv(client_sock,&c,1,0);
			}else{
				c='\n';
			}

		}
		buf[i++]=c;
	}
	return i;
}*/

int get_line(int sock, char *buf, int size)
{
	int i = 0;
	char c = '\0';
	int n;
	
	while ((i < size - 1) && (c != '\n')){
		n = recv(sock, &c, 1, 0);
		/* DEBUG printf("%02X\n", c); */
		if (n > 0){
			if (c == '\r'){
				n = recv(sock, &c, 1, MSG_PEEK);
				/* DEBUG printf("%02X\n", c); */
				if ((n > 0) && (c == '\n'))
					recv(sock, &c, 1, 0);
				else
					c = '\n';
			}
			
			buf[i] = c;
			i++;
		} else
			c = '\n';
	}
	buf[i] = '\0';
	
	return(i);
 }

//uncompleted();
void uncompleted(int client_sock,char *method){
	char buf[1024];

	sprintf(buf,"HTTP/1.0 501 METHOD NOT COMPLETED\r\n");
	send(client_sock,buf,strlen(buf),0);
	
	sprintf(buf,"Content-Type:text/html\r\n");
	send(client_sock,buf,strlen(buf),0);
	

	sprintf(buf,"\r\n");
	send(client_sock,buf,strlen(buf),0);

	sprintf(buf,"<html><title>METHOD NOT COMPLETED</title>\r\n");
	send(client_sock,buf,strlen(buf),0);

	sprintf(buf,"<body><p>This method :%s is not completed</p></body></html>\r\n",method);
	send(client_sock,buf,strlen(buf),0);

}
