#include "csapp.h"

int main(int argc, char **argv){
   int listenfd, connfd, port, clientlen;
   struct sockaddr_in clientaddr;

   if(argc != 2){
     fpringf(stderr, "usage: %s <port>\n", argv[0]);
     exit(1);
   }

   port = atoi(argv[1]);
   
   listenfd = Open_listenfd(port);

   while(1) {

        clientlen = sizeof(clientaddr);
        connfd  = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        doit(connfd);
        close(connfd);

   }

}

void doit(int fd)
{
  int is_static
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE],uri[MAXLINE],version[MAXLINE];
  char filename[MAXLINE],cgiargs[MAXLINE];
  rio_t rio;

  Rio_readinitb(&rio, fd);
  Rio_readinitb(&rio, buf, MAXLINE);
  sscanf(buf, "%s %s %s", method, uri, version);
  if(strcasecmp(method, "GET")) {
     clienterror(fd, method, "501", "Not Implemented", "Tiny does not implement this method");
     return;   
  }  

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