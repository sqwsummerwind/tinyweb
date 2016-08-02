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
#include <sys/stat.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>

#define SERVER_STRING "Server: summerwind/0.1.0\r\n"

int start_up(int port);
void error_die(const char *msg);
void deal_request(void *server_sock);
int get_line(int,char *,int);
void uncompleted(int,char *);
void not_found(int,char *);
void deal_cgi(int,char *,char *,char *);
void deal_static(int,char *,int);
void send_head(int,char *);
int  get_content(char *,int,int);
void get_mime(char *content_type,char *path);
void cgi_error(int);
void invalid_req(int);

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
	char *query_string=NULL;
	char *path=NULL;
	struct stat st;

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
		return;
	}

	if(strcasecmp(method,"POST")==0){	
		cgi=true;
	}

	if(strcasecmp(method,"GET")==0){
		query_string=strchr(url,'?');
		if(query_string){
			cgi=true;
			*query_string='\0';
			query_string++;
		}
		//printf("url:%s\n query_stirng:%s\n",url,query_string);

		charnum=get_line(cli_sock,buf,sizeof(buf));
		while((charnum>0)&&strcmp("\n",buf)){
			//printf("%s",buf);
			charnum=get_line(cli_sock,buf,sizeof(buf));

		}
	}

	path=url+1;
	if(stat(path,&st)==-1){
		
		//printf("path:%s\n",path);
		not_found(cli_sock,path);
	}
	else{
		if((st.st_mode&S_IFMT)==S_IFDIR){
			sprintf(path,"index.html");
		}

		if((st.st_mode&S_IXOTH)||(st.st_mode&S_IXUSR)||(st.st_mode&S_IXGRP)){
			cgi=true;
		}

		if(cgi){
			deal_cgi(cli_sock,path,method,query_string);
		}else{
			deal_static(cli_sock,path,(int)st.st_size);
		}
	
	}

	close(cli_sock);

}

void not_found(int client_sock,char *path){
	
	char buf[1024];
	
	sprintf(buf,"HTTP/1.0 404 NOT FOUND\r\n");
	send(client_sock,buf,strlen(buf),0);
	sprintf(buf,SERVER_STRING);
	send(client_sock,buf,strlen(buf),0);
	sprintf(buf,"Content-Type:text/html\r\n");
	send(client_sock,buf,strlen(buf),0);
	sprintf(buf,"\r\n");
	send(client_sock,buf,strlen(buf),0);
	sprintf(buf,"<html><title>Resource not found</title>\r\n");
	send(client_sock,buf,strlen(buf),0);
	sprintf(buf,"<body><h1>The resource of the path:\r\n");
	send(client_sock,buf,strlen(buf),0);
	sprintf(buf,"%s\n",path);
	send(client_sock,buf,strlen(buf),0);
	sprintf(buf,"not found\r\n");
	send(client_sock,buf,strlen(buf),0);
	sprintf(buf,"</h1></body></html>");
	send(client_sock,buf,strlen(buf),0);

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

void deal_static(int client_sock,char *path,int file_size){
	
	//send_head(client_sock,path);

	//FILE *file=fopen(path,"r");
	char *buf;
	char file[file_size];

	buf = (char *)malloc(file_size);
	int fd = open(path,O_RDONLY);
	if(fd==-1){
		not_found(client_sock,path);
	}else{
	
		send_head(client_sock,path);
		get_content(buf,fd,file_size);
		//sprintf(file,"%s",buf);
		send(client_sock,buf,file_size,0);
	}

	free(buf);
	close(fd);

}

void deal_cgi(int client_sock,char *path,char *method,char *query_string){
	int charnum=-1;
	char buf[1024];
	int content_len=-1;
	char con_len[10];
	int cgi_output[2];
	int cgi_input[2];
	pid_t pid;
	char content[255];
	char c;
	int status;

	buf[0]='a';
	buf[1]='\0';
	if(strcasecmp(method,"POST")==0){
		charnum=get_line(client_sock,buf,sizeof(buf));
		while(charnum>0 && strcmp(buf,"\n")){
			buf[15]='\0';
			if(strcmp(buf,"Content-Length:")==0){
				content_len=atoi(&buf[16]);
			//	con_len=&buf[16];
			}

			charnum=get_line(client_sock,buf,sizeof(buf));
		}

		if(content_len==-1){
			invalid_req(client_sock);
			return;
		}
		sprintf(con_len,"%d",content_len);

	}

	if(pipe(cgi_output)==-1){
		cgi_error(client_sock);
		return;
	}

	if(pipe(cgi_input)==-1){
		cgi_error(client_sock);
		return;
	}

	if((pid=fork())==-1){
		cgi_error(client_sock);
		return;
	}

	sprintf(buf,"HTTP/1.0 200 OK\r\n");
	send(client_sock,buf,strlen(buf),0);

	//child process
	if(pid==0){
		
		printf("this a deal_cgi,child process\n");
		printf("content-length:%s\n",con_len);
		dup2(cgi_input[0],0);
		dup2(client_sock,1);
		close(cgi_input[1]);
		close(cgi_output[0]);

		setenv("REQUEST_METHOD",method,1);
		if(strcasecmp(method,"GET")==0)
			setenv("QUERY_STRING",query_string,1);
		else
			setenv("CONTENT_LENGTH",con_len,1);
		
		execl(path,path,NULL);
		exit(0);
	}
	else
	{
		
		close(cgi_input[0]);
		close(cgi_output[1]);
		
		if(strcasecmp(method,"POST")==0){
			
			recv(client_sock,content,content_len,0);
			write(cgi_input[1],content,content_len);
			
		}

		printf("this is parent process\n");
		printf("content:%s\n",content);
		//while(read(cgi_output[0],&c,1)>0)
			//send(client_sock,&c,1,0);

		close(cgi_output[0]);
		close(cgi_input[1]);

		waitpid(pid,&status,0);

	}

}

void cgi_error(int client_sock){
	
	char buf[1024];

	sprintf(buf,"HTTP/1.0 501 CGI EXEC ERROR\r\n");
	sprintf(buf,"%sContent-Type:text/html\r\n\r\n",buf);
	sprintf(buf,"%s<html><title>cgi error</title>\r\n",buf);
	sprintf(buf,"%s<body><p>cgi exec error\r\n",buf);
	sprintf(buf,"%s<p>maybe pipe or fork\r\n",buf);
	sprintf(buf,"%s</body></html>\r\n",buf);
	send(client_sock,buf,strlen(buf),0);

}

void invalid_req(int client_sock){

	char buf[1024];

	sprintf(buf,"HTTP/1.0 400 INVALID REQUEST\r\n");
	sprintf(buf,"%sContent-Type:text/html\r\n\r\n",buf);
	sprintf(buf,"%s<html><title>bad request</title>\r\n",buf);
	sprintf(buf,"%s<body><p>this post method does not has content length",buf);
	sprintf(buf,"%s</p></body></html>\r\n",buf);
	send(client_sock,buf,strlen(buf),0);
}

int get_content(char *usrbuf,int fd,int file_size){
	/*char buf[1024];
	//fgets(buf,sizeof(buf),file);
	while(read(fd,buf,sizeof(buf))){
		//fgets(buf,sizeof(buf),file);
		send(client_sock,buf,strlen(buf),0);
	}*/

	int nleft = file_size;
	int nread;
	char *bufp=usrbuf;
	while (nleft > 0) {
		if ((nread = read(fd, bufp, nleft)) < 0) {
			if (errno == EINTR) /* interrupted by sig handler return */
				nread = 0;      /* and call read() again */
			else
				return -1;      /* errno set by read() */ 
		}else if (nread == 0)
			break;              /* EOF */
	nleft -= nread;
	bufp += nread;
	
	}
	
	return(file_size-nleft);

}

void send_head(int client_sock,char *path){
	
	char content_type[30];
	get_mime(content_type,path);
	char buf[1024];
	sprintf(buf,"HTTP/1.0 200 OK\r\n");
	send(client_sock,buf,strlen(buf),0);

	sprintf(buf,SERVER_STRING);
	send(client_sock,buf,strlen(buf),0);

	sprintf(buf,"Content-Type:%s\r\n",content_type);
	send(client_sock,buf,strlen(buf),0);

	sprintf(buf,"\r\n");
	send(client_sock,buf,strlen(buf),0);
}

void get_mime(char *content_type,char *path){
	char *index=strchr(path,'.');
	
	if((strcasecmp(index,".jpeg")==0)||(strcasecmp(index,".jpg")==0))
		strcpy(content_type,"image/jpeg");
	else if(strcasecmp(index,".pdf")==0)	
		strcpy(content_type,"application/pdf");
	else if(strcasecmp(index,".mp4")==0)
		strcpy(content_type,"video/mp4");
	else if(strcasecmp(index,".gif")==0)
		strcpy(content_type,"image/gif");
	else if(strcasecmp(index,".html")==0)
		strcpy(content_type,"text/html");
	else 
		strcpy(content_type,"text/plain");
	printf("content-type:%s\n",content_type);
	return;
	//return content_type;
}
