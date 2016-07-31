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
	
}
