//Server fuer Socketuebung
//Tobias Nemecek/Verena Poetzl

#include <errno.h>
#include <limits.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define BUF 1024
//#define PORT 6543

int main (int argc, char **argv)
{
  int create_socket, new_socket;
  socklen_t addrlen;
  char buffer[BUF];
  int size, PORT;
  struct sockaddr_in address, cliaddress;
  char newline = '\n';
  char filename[BUF-4];
  struct stat st;
  unsigned long int filesize, remain_data;
  FILE *received_file;
  unsigned long int len;

  if (argc < 3)
  {
       printf("Usage: %s Port Downloadverzeichnis\n", argv[0]);
       exit(EXIT_FAILURE);
  }

  errno = 0;
  PORT = strtol(argv[1], &argv[1], 10);

  if(errno != 0 || *argv[1] != '\0' || PORT > INT_MAX)
  {
       perror("Input error");
       return EXIT_FAILURE;
  }

  create_socket = socket (AF_INET, SOCK_STREAM, 0);

  memset(&address,0,sizeof(address));
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons (PORT);

  if (bind ( create_socket, (struct sockaddr *) &address, sizeof (address)) != 0)
  {
     perror("bind error");
     return EXIT_FAILURE;
  }
  listen (create_socket, 5);

  addrlen = sizeof (struct sockaddr_in);

  while (1)
  {
     printf("Waiting for connections...\n");
     new_socket = accept ( create_socket, (struct sockaddr *) &cliaddress, &addrlen );
     if (new_socket > 0)
     {
        printf ("Client connected from %s:%d...\n", inet_ntoa (cliaddress.sin_addr),ntohs(cliaddress.sin_port));
        strcpy(buffer,"Welcome to myserver, Please enter your command:\n");
        send(new_socket, buffer, strlen(buffer),0);
     }
     do
     {
        size = recv (new_socket, buffer, BUF-1, 0);
        if( size > 0)
        {
           buffer[size] = '\0';
           printf ("Message received: %s\n", buffer);

           int i = 0;

           while(buffer[i])
           {
                buffer[i] = tolower(buffer[i]);
                i++;
           }

           if (strncmp(buffer, "list", size-1) == 0)
           {
                DIR *dp;
                struct dirent *ep;
                dp = opendir (argv[2]);

                if (dp != NULL)
                {
                    send(new_socket, &newline, 1, 0);
                    send(new_socket, "Content of ", strlen("Content of "), 0);
                    send(new_socket, argv[2], strlen(argv[2]), 0);
                    send(new_socket, &newline, 1, 0);
                    send(new_socket, &newline, 1, 0);

                    while ((ep = readdir (dp)) != NULL)
                    {
                         //printf("%s\n", ep->d_name);
                         //st = (const struct stat){ 0 };

                         strcpy(buffer, ep->d_name);

                         send(new_socket, buffer, strlen(ep->d_name),0);
                         send(new_socket, &newline, 1, 0);

                         stat(ep->d_name, &st);
                         filesize = st.st_size;
                         sprintf(buffer, "%lu", filesize);

                         send(new_socket, "Size in Bytes: ", strlen("Size in Bytes: "), 0);
                         send(new_socket, buffer, strlen(buffer),0);
                         send(new_socket, &newline, 1, 0);
                         send(new_socket, &newline, 1, 0);
                    }

                    (void) closedir (dp);
                }
                else
                {
                    perror ("Couldn't open the directory\n");
                }
           }
           else if(strncmp(buffer, "get", 3) == 0)
           {
               if(size > 4)
               {
                    for(int i = 0; i < size - 4; i++)
                    {
                         filename[i] = buffer[i+4];
                    }

                    printf("%s", filename);
               }
           }
           else if(strncmp(buffer, "put", 3) == 0)
           {
               if(size > 4)
               {
                    for(int i = 0; i < size - 4; i++)
                    {
                         filename[i] = buffer[i+4];
                    }
                    filename[size-5] = '\0';
               }

               recv(new_socket, buffer, BUF-1, 0);
               filesize = atoi(buffer);
               printf("%lu\n", filesize);

               char filepath[256];

               strcpy(filepath, argv[2]);
               strcat(filepath, "/");
               strcat(filepath, filename);

               printf("%s\n", filepath);

               received_file = fopen(filepath, "w");
               if(received_file == NULL)
               {
                    printf("Error while opening the file\n");
                    return EXIT_FAILURE;
               }
               remain_data = filesize;

               while(((len = recv(new_socket, buffer, BUF-1, 0)) > 0) && (remain_data > 0))
               {
                    fwrite(buffer, sizeof(char), len, received_file);
                    remain_data -= len;
                    printf("\nReceived %lu bytes, %lu bytes remain", len, remain_data);
               }

               fclose(received_file);

           }
           else if(strncmp(buffer, "quit", size-1) == 0)
           {
                printf("User terminated connection\n");
           }
        }
        else if (size == 0)
        {
           printf("Client closed remote socket\n");
           break;
        }
        else
        {
           perror("recv error");
           return EXIT_FAILURE;
        }
     } while (strncmp (buffer, "quit", 4)  != 0);
     close (new_socket);
  }
  close (create_socket);
  return EXIT_SUCCESS;
}
