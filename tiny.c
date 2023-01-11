#include "csapp.h"

int main(int argc, char **argv){
   int listenfd, connfd, port, clientlen;
   struct sockaddr_in clientaddr;

   if(argc != 2){
     fpringf(stderr, "usage: %s <port>\n", argv[0]);
     exit(1);
   }

   port = atoi(argv[1]);
   
   //创建一个服务端的监听描述符
   listenfd = Open_listenfd(port);

    /**
     * 等待与客户端的连接请求
    */
   while(1) {

        clientlen = sizeof(clientaddr);
        //等待与客户端连接， 建立连接成功后，返回一个已连接描述符connfd
        connfd  = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        doit(connfd);
        close(connfd);

   }

}

/*
*@desc 根据描述符，解析出url,method等参数， 区分动态请求和静态请求。 分别做处理。
*@param fd 已连接描述符
*@琦 2023-01-07
*/
void doit(int fd)
{
  int is_static
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE],uri[MAXLINE],version[MAXLINE];
  char filename[MAXLINE],cgiargs[MAXLINE];
  rio_t rio;

   //将描述符与缓冲区关联起来
  Rio_readinitb(&rio, fd);  
  Rio_readinitb(&rio, buf, MAXLINE);
  sscanf(buf, "%s %s %s", method, uri, version);

  //只支持get请求， 非get请求，报501
  if(strcasecmp(method, "GET")) {
     clienterror(fd, method, "501", "Not Implemented", "Tiny does not implement this method");
     return;   
  }  

  //将缓冲区rp中的字符写入到buf中
  read_requesthdrs(&rio);
  
  is_static = parse_uri(uri, filename, cgiargs);
  if(stat(filename, &sbuf) < 0) {
      clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
      return;  
  }

  if(is_static){
    if (!S_ISREG(sbuf.st_mode) || !(S_IXUSR & sbuf.st_mode)){
        clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
        return;
    }

    serve_static(fd, filename, sbuf.st_size);
  }else{

    if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)){
        clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
        return;
    }

    serve_dynamic(fd, filename, cgiargs);
  }

}

/**
 * @details 组装错误信息报文
 * @param fd 已连接描述符
 * @param cause 文件名
 * @param errnum http响应码
 * @param shortmsg 错误码对应的信息（短）
 * @param longmsg  错误码对应的信息(长)
 * 
*/
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
    char buf[MAXLINE], body[MAXBUF];

    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=\"fffff\">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);


}


/**
 * @details 判断请求内容是动态还是静态
 * @param uri 请求url
 * @param filename 请求文件名
 * @author 琦 2023-01-07
*/

int parse_uri(char *uri, char *filename, char *cigargs)
{
    char *ptr;

    //uri中包含cgi-bin的字符判断为请求动态内容，否则为静态
    if(!strstr(uri, "cgi-bin")){
        strcpy(cigargs, "");
        strcpy(filename, ".");
        strcat(filename, uri);
        if(uri[strlen(uri-1)] == '/')
          strcat(filename, "home.html");
        return 1;
    }else{
       ptr = index(uri, '?');
       if(ptr){
        strcpy(cigargs, ptr+1);
        *ptr = '\0';
       }else
        strcpy(cigargs, "");

      strcpy(filename, ".");
      strcat(filename, uri);
      return 0;
    }

}

/**
 * @details 将缓冲区rp中的字符写入到buf中
 * @param rp rp是已连接描述符写入的，接受到的数据
 * @author 琦 2023-01-11
*/
void read_requesthdrs(rio_t *rp){

    char buf[MAXLINE];
    
    Rio_readlineb(rp, buf, MAXLINE);

    //遇到空文本行则打印出buf
    while (strcmp(buf, "\r\n"))
    {
      Rio_readlineb(rp, buf, MAXLINE);
      printf("%s", buf);
    }
    
    return;

}

/**
 * @details 处理静态请求
 * 
*/
void serve_static(int fd, char *filename, int filesize)
{
   int srcfd;
   char *srcp, filetype[MAXLINE], buf[MAXBUF];
   
   //根据文件名确定文件类型
   get_filetype(filename, filetype);

   //响应内容
   sprintf(buf, "HTTP/1.0 200 OK \r\n");
   sprintf(buf, "%s Server: Tiny web Server \r\n", buf);
   sprintf(buf, "%s Content-length: %d \r\n", buf, filesize);
   sprintf(buf, "%s Content-type: %s \r\n\r\n", buf, filetype);
   Rio_writen(fd, buf, strlen(buf));
  
   srcfd = Open(filename , O_RDONLY, 0);
   srcfd = Mmap(0 , filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
   close(srcfd);
   rio_writen(fd, srcp, filesize);
   Munmap(srcp, filesize);
   
}

void get_filetype(char *filename, char *filetype)
{
   if (strstr(filename, ".html"))
   {
    strcpy(filetype, "text/html");
   }
   else if (strstr(filename , ".gif"))
   {
    strcpy(filetype, "image/gif");
   }
   else if(strstr(filename, ".jpg"))
    strcpy(filetype, "iamge/jpeg");
   else  
     strcpy(filetype, "text/plain");

}

void serve_dynamic(int fd, char *filename, char *cgiargs)
{
  char buf[MAXLINE], *emptylist[] = {  NULL };
  
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny web server \r\n");
  Rio_writen(fd, buf, strlen(buf));

  if(fork() == 0){
      setenv("QUERY_STRING", cgiargs, 1);
      dup2(fd, STDOUT_FILENO);
      execve(filename, emptylist, environ);
  }

  wait(NULL);

}