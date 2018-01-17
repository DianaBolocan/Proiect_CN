#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>

/* codul de eroare returnat de anumite apeluri */
extern int errno;

/* portul de conectare la server*/
int port;

int main (int argc, char *argv[])
{
  int loggedIn = 0;
  int sd;			// descriptorul de socket
  struct sockaddr_in server;	// structura folosita pentru conectare 
  char msg[2000];		// mesajul trimis
  port = 11111;
  char cmd[50];
  int quit;

  /* cream socketul */
  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("[ERROR]: socket().\n");
      return errno;
    }
  

  /* umplem structura folosita pentru realizarea conexiunii cu serverul */
  /* familia socket-ului */
  server.sin_family = AF_INET;
  /* adresa IP a serverului */
  server.sin_addr.s_addr = inet_addr("127.0.0.1");
  /* portul de conectare */
  server.sin_port = htons (port);
  
  /* ne conectam la server */
  if (connect (sd, (struct sockaddr *) &server,sizeof (struct sockaddr)) == -1)
    {
      perror ("[ERROR]: connect().\n");
      return errno;
    }
  
  bzero(cmd,50);

  while(quit == 0){
	//read from server message
	if(read(sd,msg,2000) < 0){
		perror ("[ERROR]: read() from server.\n");
		return errno;
		  }
	printf("%s",msg);
	fflush(stdout);
	bzero(msg,2000);
	bzero(cmd,50);
	read (0,cmd,50);
	cmd[strcspn(cmd,"\n")] = '\0';
	//write to server command
	if (write (sd, cmd,50) <= 0)
	{
		perror ("[ERROR]: write() to server.\n");
		return errno;
	}else if(strcmp(cmd,"quit") == 0) quit = 1;
  }
  /* inchidem conexiunea, am terminat */
  close (sd);
}
