#include <inc/lib.h>
#include <lwip/sockets.h>
#include <lwip/inet.h>

#define SERVER_PORT 80
#define LISTENQ 8
#define bool int
#define false 0
#define true 1
const char* root="/home/yanni/webserver";

int service(int client);

void
umain(int argc, char** argv)
{
  int listener, conn;
  int pid;
  struct sockaddr_in servaddr, clientaddr;
  listener=socket(AF_INET, SOCK_STREAM, 0);
  if (listener<0)
  {
    cprintf("couldn't establish listening socket\n");
    return;
  }
  memset(&servaddr, 0, sizeof(struct sockaddr_in));
  servaddr.sin_family=AF_INET;
  servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
  servaddr.sin_port=htons(SERVER_PORT);
  if (bind(listener, (struct sockaddr*) &servaddr, sizeof(servaddr))<0)
  {
    cprintf("couldn't bind socket\n");
    return;
  }
  cprintf("bound socket\n");
  if (listen(listener, LISTENQ)<0)
  {
    cprintf("couldn't listen on socket\n");
    return;
  }
  cprintf("listening\n");
  while (1)
  {
    memset(&clientaddr, 0, sizeof(struct sockaddr_in));
    int len=sizeof(clientaddr);
    if ((conn=accept(listener, (struct sockaddr*) &clientaddr, (socklen_t*)&len))<0)
    {
      cprintf("error accepting connection...\n");
    }
    cprintf("connected client: %i\n", &clientaddr.sin_addr.s_addr);
    if ((pid=fork()) == 0)
    {
      //this is the child process
      //Service the request and quit
      close(listener);
      memset(&servaddr, 0, sizeof(struct sockaddr_in)); //clients should have no knowledge of the master
//      service(conn);
      close(conn);
      return;
    }
    close(conn);
  }
  return;
}

int service(int client)
{
    bool keepalive=false;
    char buf[256];
    do
    {
        memset(buf, 0, 256);
        int status=read(client, buf, 256); //fill buffer with user data
        if (status==0)
        {
          printf("closing connection\n");
          keepalive=false;
        }
        printf("%s\n", buf);
        char uri[250];
        memset(uri, 0, 250);
        //use sscanf with a max string length to avoid overflow
        if (sscanf(buf, "GET %249s HTTP/1.1\n", uri)==1)
        {
            //what is this crap?
            if (uri[0]==0 || strstr(uri, "..")!=NULL)
            {
                return 1;
            }
            //they want the index page
            if (uri[1]==0)
            {
                strcat(uri, "index.html");
            }
            char file[300];
            memset(file, 0, 300);
            strcat(file, root);
            strcat(file, uri);
            printf("serving %s\n", file);
            FILE *webpage;
            webpage=fopen(file, "r");
            //file didn't exist
            if (webpage==NULL)
            {
                return 1;
            }
            fseek(webpage, 0, SEEK_END);
            long fsize = ftell(webpage);
            fseek(webpage, 0, SEEK_SET);
            char response[25];
            int count=sprintf(response, "Content-length: %i", fsize);
            write(client, "HTTP/1.1 200 OK\n", 16);
            write(client, response, count);
            write(client, "Connection: Close\n", 18);
            write(client, "Content-Type: text/html\n\n", 25);
            //write(new_socket, "<html><body><H1>Hello world</H1></body></html>",46);
            //char *string = new char[fsize + 1];
            char *string = malloc(fsize+1);
            fread(string, fsize, 1, webpage);
            fclose(webpage);
            //write(client, string, fsize);
            int bytes=0;
            while (bytes<=fsize)
            {
                bytes+=write(client, string, fsize);
            }
            free(string);
            //delete string;
            sleep(1000);
        }
        //check if the browser wants to keep the connection alive
        if (strstr(buf, "Connection: keep-alive")!=NULL)
        {
            //I've decided I don't know what this means
            //keepalive=true;
        }
    } while (keepalive==true);
    return 1;
}
