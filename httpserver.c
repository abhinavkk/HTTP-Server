//simple http server
//AUTHOR: Abhinav Kaushal Keshari 

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>

#define BACKLOG 10 		//allows maximum 10 connections together
#define BYTES 1024
char PORT[5];
char *DIRECTORY;
strcpy(PORT,"3500"); 	//use the PORT 3500
DIRECTORY = getenv("PWD"); 	//starts the server in the current directory
int listenfd, clients[BACKLOG];

void error(char *);
void startServer(char *);
void respond(int);

int main(int argc, char* argv[])
{
	struct sockaddr_in clientaddr;
	socklen_t addrlen;
	int slot = 0;

	printf("Server started at port no. %s%s%s in the directory %s%s%s\n","\033[92m",PORT,"\033[0m","\033[92m",DIRECTORY,"\033[0m");

	for(int i=0; i<BACKLOG; i++)
	{
		clients[i] = -1; 	//sets all elements to -1 signifying there is no client connected yet
	}

	startServer(PORT);

	while(1) 	//accept connections
	{
		addrlen = sizeof(clientaddr);
		clients[slot] = accept (listenfd, (struct sockaddr *) &clientaddr, &addrlen);

		if (clients[slot]<0)
		    error ("error in accepting");
		else
		{
		    if ( fork()==0 ) 	//creates new process id
		    {
		        respond(slot); 	//gives a new slot out of 10 available
		        exit(0);
		    }
		}
		while (clients[slot]!=-1) slot = (slot+1)%BACKLOG;
	}

	return 0;
}

void startServer(char *port)
{
	struct addrinfo hints, *res, *p;

    memset (&hints, 0, sizeof(hints)); 	// getaddrinfo for host
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if (getaddrinfo( NULL, port, &hints, &res) != 0)
    {
        perror ("gerror in fetting address of the host");
        exit(1);
    }
    for (p = res; p!=NULL; p=p->ai_next)
    {
        listenfd = socket (p->ai_family, p->ai_socktype, 0); 	//opens a socket
        if (listenfd == -1) continue;
        if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0) break; 		//binds to a port
    }
    if (p==NULL)
    {
        perror ("error while calling socket or bind function");
        exit(1);
    }

    freeaddrinfo(res);
    if ( listen (listenfd, 1000000) != 0 )	 //listen for 1000000 incoming connections
    {
        perror("listen() error");
        exit(1);
    }
}

void respond(int n)
{
    char mesg[99999], *reqline[3], data_to_send[BYTES], path[99999];
    int rcvd, fd, bytes_read;

    memset( (void*)mesg, (int)'\0', 99999 );

    rcvd=recv(clients[n], mesg, 99999, 0);

    if (rcvd<0)    // receive error
        fprintf(stderr,("recv() error\n"));
    else if (rcvd==0)    // receive socket closed
        fprintf(stderr,"Client disconnected upexpectedly.\n");
    else    // message received
    {
        printf("%s", mesg);
        reqline[0] = strtok (mesg, " \t\n");
        if ( strncmp(reqline[0], "GET\0", 4)==0 )
        {
            reqline[1] = strtok (NULL, " \t");
            reqline[2] = strtok (NULL, " \t\n");
            if ( strncmp( reqline[2], "HTTP/1.0", 8)!=0 && strncmp( reqline[2], "HTTP/1.1", 8)!=0 )
            {
                write(clients[n], "HTTP/1.0 400 Bad Request\n", 25);
            }
            else
            {
                if ( strncmp(reqline[1], "/\0", 2)==0 )
                    reqline[1] = "/index.html";        //Because if no file is specified, index.html will be opened by default (like it happens in APACHE...

                strcpy(path, DIRECTORY);
                strcpy(&path[strlen(DIRECTORY)], reqline[1]);
                printf("file: %s\n", path);

                if ( (fd=open(path, O_RDONLY))!=-1 )    //FILE FOUND
                {
                    send(clients[n], "HTTP/1.0 200 OK\n\n", 17, 0);
                    while ( (bytes_read=read(fd, data_to_send, BYTES))>0 )
                        write (clients[n], data_to_send, bytes_read);
                }
                else    write(clients[n], "HTTP/1.0 404 Not Found\n", 23); //FILE NOT FOUND
            }
        }
    }

    //Closing SOCKET
    shutdown (clients[n], SHUT_RDWR);         //All further send and recieve operations are DISABLED
    close(clients[n]);
    clients[n]=-1;
}