/*
这个小小的服务器主要的功能是用来练习在linux下的编程和熟悉与网络编程相关的知识。
这个服务器支持get和post两种请求方式。
支持cgi
支持图片、文档等等的静态内容。
完成于2016.8.2
*/

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
	//服务器socket号
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
	
	//初始化服务器socket号
	server_sock=start_up(port);
	client_addr_len=sizeof(client_addr);
	
	//不断循环，不断接收客户端的链接请求
	while(1){
		//accept函数返回值，初始化客户socket号，以后获取客户端的内容和向客户端发送信息都
		//通过这个端口号
		client_sock = accept(server_sock,(struct sockaddr *)&client_addr,&client_addr_len);

		if(client_sock == -1){
			error_die("accept error\n");
		}
		
		//创建新的线程来为客户端服务，主要pthread_create这个函数的参数
		if(pthread_create(&thread_id,NULL,(void *)deal_request,(void *)&client_sock)!=0)
			error_die("pthread_create error\n");
	}
	
	close(server_sock);
	return 0;
}
/*
这个函数主要是初始化服务器端的socket，以准备链接客户端
*/

int start_up(int port){
	
	int sock_id;
	struct sockaddr_in name;
	
	//AF_INET表示ipv4,SOCK_STREAM表示有链接传输流，具体查看socket函数手册
	if((sock_id=socket(AF_INET,SOCK_STREAM,0))==-1){
		error_die("socket error\n");
	}
	
	memset(&name,0,sizeof(name));
	//这是必须要设置的值
	name.sin_family=AF_INET;
	//使用这个函数把port转换为网络端存储形式
	name.sin_port=htons(port);
	//这个自动获取主机ip
	name.sin_addr.s_addr=htonl(INADDR_ANY);
	
	//把sock_id和sockaddr进行绑定
	if(bind(sock_id,(struct sockaddr *)&name,sizeof(name))==-1){
		error_die("bind error\n");
	}
	
	//启动监听
	if(listen(sock_id,10)==-1){
		error_die("listen error\n");
	}

	return sock_id;

}

//出错时在终端打印错误信息
void error_die(const char *msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

/*
处理客户端发送过来的请求，主要是判断请求是get还是post方法
把 请求方法 url protocol 进行分离
并判断是请求静态内容还是cgi
分别调用deal_static和deal_cgi进行处理
*/
void deal_request(void *client_sock){
	
	//客户端sock号
	int cli_sock =*(int*)client_sock;
	int charnum=0;
	char buf[1024];
	//请求方法
	char method[10];
	char url[255];
	char protocol[50];
	bool cgi=false;
	//cgi程序的参数
	char *query_string=NULL;
	char *path=NULL;
	//文件的各种信息，具体可以查看stat函数
	struct stat st;
	
	//读取一行
	charnum=get_line(cli_sock,buf,sizeof(buf));
	if(charnum==0){
		error_die("get_line error\n");
	}
	
	//进行分离
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
		//找出？字符的位置
		query_string=strchr(url,'?');
		if(query_string){
			cgi=true;
			*query_string='\0';
			query_string++;
		}

		//如果是get方法，后面的内容没有用，就丢弃
		charnum=get_line(cli_sock,buf,sizeof(buf));
		while((charnum>0)&&strcmp("\n",buf)){
			//printf("%s",buf);
			charnum=get_line(cli_sock,buf,sizeof(buf));

		}
	}
	//url的形式为：/aisi.jpg，而path是存取路径，把第一个/去掉，变为aisi.jpg
	path=url+1;
	//判断文件是否存在
	if(stat(path,&st)==-1){
		not_found(cli_sock,path);
	}else{
		//如果访问的是目录，默认为访问主页
		if((st.st_mode&S_IFMT)==S_IFDIR){
			sprintf(path,"index.html");
		}
		//查看是否有执行权限，如果有，就是cgi
		if((st.st_mode&S_IXOTH)||(st.st_mode&S_IXUSR)||(st.st_mode&S_IXGRP)){
			cgi=true;
		}
		//分别调用对应的处理函数
		if(cgi){
			deal_cgi(cli_sock,path,method,query_string);
		}else{
			deal_static(cli_sock,path,(int)st.st_size);
		}
	
	}

	close(cli_sock);

}

//文件不存在时调用，向客户端发送文件不存在的信息
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

//从sock读取内容，一次设置成读取一行
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

//如果请求的方法不是get也不是post，返回错误信息
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

/*
处理静态内容
*/
void deal_static(int client_sock,char *path,int file_size){
	
	//send_head(client_sock,path);

	//FILE *file=fopen(path,"r");
	char *buf;
	char file[file_size];
	//对buf进行内存分配
	buf = (char *)malloc(file_size);
	//打开文件
	int fd = open(path,O_RDONLY);
	if(fd==-1){
		not_found(client_sock,path);
	}else{
		//发送头部
		send_head(client_sock,path);
		//获取内容
		get_content(buf,fd,file_size);
		//给客户端发送文件内容
		send(client_sock,buf,file_size,0);
	}
	
	//释放内容，这很重要
	free(buf);
	close(fd);
	return;

}

/*
处理cgi的内容
*/
void deal_cgi(int client_sock,char *path,char *method,char *query_string){
	
	int charnum=-1;
	char buf[1024];
	//请求的cgi的参数长度
	int content_len=-1;
	//也是参数长度
	char con_len[10];
	int cgi_output[2];
	int cgi_input[2];
	//进程号
	pid_t pid;
	//参数的内容
	char content[255];
	char c;
	int status;

	buf[0]='a';
	buf[1]='\0';
	//方法如果为post，不断读取剩下的内容并丢弃
	if(strcasecmp(method,"POST")==0){
		charnum=get_line(client_sock,buf,sizeof(buf));
		while(charnum>0 && strcmp(buf,"\n")){
			//找到content-length,获取参数内容的长度
			buf[15]='\0';
			if(strcmp(buf,"Content-Length:")==0){
				content_len=atoi(&buf[16]);
			}

			charnum=get_line(client_sock,buf,sizeof(buf));
		}

		if(content_len==-1){
			invalid_req(client_sock);
			return;
		}
		

	}
	
	//创建管道
	if(pipe(cgi_output)==-1){
		cgi_error(client_sock);
		return;
	}
	//创建管道
	if(pipe(cgi_input)==-1){
		cgi_error(client_sock);
		return;
	}
	//fork一个新的进程处理cgi
	if((pid=fork())==-1){
		cgi_error(client_sock);
		return;
	}

	sprintf(buf,"HTTP/1.0 200 OK\r\n");
	send(client_sock,buf,strlen(buf),0);

	//子进程
	if(pid==0){
		
		//把cgi_input的输出端重定向为stdin
		dup2(cgi_input[0],0);
		//把客户sock重定向为stdout
		dup2(client_sock,1);
		//分别关闭不使用的两个管道端口
		close(cgi_input[1]);
		close(cgi_output[0]);
		
		//设置环境变量，在cgi程序中可以获取
		setenv("REQUEST_METHOD",method,1);
		if(strcasecmp(method,"GET")==0)
			setenv("QUERY_STRING",query_string,1);
		else
			setenv("CONTENT_LENGTH",con_len,1);
		
		//使用exec系列函数调用cgi程序
		execl(path,path,NULL);
		exit(0);
	}
	else
	{
		
		close(cgi_input[0]);
		close(cgi_output[1]);
		
		if(strcasecmp(method,"POST")==0){
			//读取参数的内容，输出到管道中，cgi程序可以在管道中获取
			recv(client_sock,content,content_len,0);
			write(cgi_input[1],content,content_len);
			
		}
		//while(read(cgi_output[0],&c,1)>0)
			//send(client_sock,&c,1,0);

		close(cgi_output[0]);
		close(cgi_input[1]);

		waitpid(pid,&status,0);

	}

}

//管道或者fork不成功
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

//post方法没有content-length
void invalid_req(int client_sock){

	char buf[1024];

	sprintf(buf,"HTTP/1.0 400 INVALID REQUEST\r\n");
	sprintf(buf,"%sContent-Type:text/html\r\n\r\n",buf);
	sprintf(buf,"%s<html><title>bad request</title>\r\n",buf);
	sprintf(buf,"%s<body><p>this post method does not has content length",buf);
	sprintf(buf,"%s</p></body></html>\r\n",buf);
	send(client_sock,buf,strlen(buf),0);
}

//获取文件的内容
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

//发送头部
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

//获取mime类型
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
