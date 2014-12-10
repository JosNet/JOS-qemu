
#include <inc/lib.h>
#include <lwip/sockets.h>
#include <lwip/inet.h>

#define SERVER_PORT 514
#define LISTENQ 8
#define bool int
#define false 0
#define true 1

int service(int client);

void wait_ms(int time)
{
  int curtime=sys_time_msec();
  while ((sys_time_msec()-curtime)<time)
  {
    sys_yield();
  }
}

void
umain(int argc, char** argv)
{
  binaryname="telnet_console";
  int listener, conn;
  int pid;
  struct sockaddr_in servaddr, clientaddr;
  listener=socket(AF_INET, SOCK_STREAM, 0);
  if (listener<0)
  {
    printf("couldn't establish listening socket\n");
    return;
  }
  memset(&servaddr, 0, sizeof(struct sockaddr_in));
  servaddr.sin_family=AF_INET;
  servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
  servaddr.sin_port=htons(SERVER_PORT);
  if (bind(listener, (struct sockaddr*) &servaddr, sizeof(servaddr))<0)
  {
    printf("couldn't bind socket\n");
    return;
  }
  printf("bound socket\n");
  if (listen(listener, LISTENQ)<0)
  {
    printf("couldn't listen on socket\n");
    return;
  }
  printf("listening\n");
  while (1)
  {
    memset(&clientaddr, 0, sizeof(struct sockaddr_in));
    int len=sizeof(clientaddr);
    if ((conn=accept(listener, (struct sockaddr*) &clientaddr, (socklen_t*)&len))<0)
    {
      printf("error accepting connection...\n");
    }
    printf("connected client: %i\n", &clientaddr.sin_addr.s_addr);
    if ((pid=fork()) == 0)
    {
      //this is the child process
      //Service the request and quit
      close(listener);
      memset(&servaddr, 0, sizeof(struct sockaddr_in)); //clients should have no knowledge of the master
      service(conn);
      close(conn);
      return;
    }
    close(conn);
  }
  return;
}

int service(int client)
{
    //dup stdin and stdout
    dup(client, 1);
    dup(client, 0);

    //spawn sh
    int id=0;
    id=spawnl("/sh", "sh", (char*)0);
	//id = spawnl("/init", "init", "initarg1", "initarg2", (char*)0);
    if (id<0)
    {
			printf("spawn sh: %e\n", id);
      return -1;
    }
    wait(id);
    return 0;
}
