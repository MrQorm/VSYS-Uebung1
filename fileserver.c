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
  char newline[2] = "\n";
  char temp[256];
  char filename[BUF-4];

  unsigned long int filesize, remain_data;
  FILE *received_file;
  unsigned long int len;

  char filepath[256];

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
          memset(&buffer[0], 0 , BUF);
          printf("Waiting for message\n");

        size = recv (new_socket, buffer, BUF-1, 0);

        if( size > 0)
        {
           buffer[size] = '\0';
           printf ("Message received: %s\n", buffer);

           if (strncmp(buffer, "list", 4) == 0)
           {
                DIR *dp;
                struct dirent *ep;
                dp = opendir (argv[2]);

                if (dp != NULL)
                {
                    /*send(new_socket, &newline, 1, 0);
                    send(new_socket, "Content of ", strlen("Content of "), 0);
                    send(new_socket, argv[2], strlen(argv[2]), 0);
                    send(new_socket, &newline, 1, 0);
                    send(new_socket, &newline, 1, 0);*/

                    int filecounter = 0;

                    while((ep = readdir(dp)) != NULL)
                    {
                         if(!(!strcmp(ep->d_name, ".") || !strcmp(ep->d_name, "..")))
                         {
                              filecounter++;
                         }
                    }

                    dp = opendir(argv[2]);

                    sprintf(temp, "%d", filecounter);

                    strcpy(buffer, newline);
                    strcat(buffer, "Content of ");
                    strcat(buffer, argv[2]);
                    strcat(buffer, ": ");
                    strcat(buffer, temp);
                    strcat(buffer, " files:");
                    strcat(buffer, newline);
                    strcat(buffer, newline);

                    send(new_socket, buffer, strlen(buffer), 0);

                    send(new_socket, temp, strlen(temp), 0);



                    while ((ep = readdir (dp)) != NULL)
                    {
                         //printf("%s\n", ep->d_name);
                         //st = (const struct stat){ 0 };
                         struct stat st;

                         strcpy(filepath, argv[2]);
                         strcat(filepath, ep->d_name);

                         if (stat(filepath, &st) == 0);
                         {
                              filesize = st.st_size;

                              if(!(!strcmp(ep->d_name, ".") || !strcmp(ep->d_name, "..")))
                              {
                                   strcpy(buffer, ep->d_name);
                                   strcat(buffer, newline);

                                   /*send(new_socket, buffer, strlen(buffer),0);
                                   send(new_socket, &newline, 1, 0);*/

                                   filesize = st.st_size;

                                   sprintf(temp, "%lu", filesize);
                                   //sprintf(buffer, "%lu", filesize);

                                   /*send(new_socket, "Size in Bytes: ", strlen("Size in Bytes: "), 0);
                                   send(new_socket, buffer, strlen(buffer),0);
                                   send(new_socket, &newline, 1, 0);
                                   send(new_socket, &newline, 1, 0);*/

                                   strcat(buffer, "Size in Bytes: ");
                                   strcat(buffer, temp);
                                   strcat(buffer, newline);
                                   strcat(buffer, newline);

                                   send(new_socket, buffer, strlen(buffer), 0);
                              }
                         }
                    }

                    (void) closedir (dp);
                }
                else
                {
                    perror ("Couldn't open the directory\n");
                }
           }
           else if(strncmp(buffer, "get ", 4) == 0)
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
           else if(strncmp(buffer, "put ", 4) == 0)
           {
               if(size > 4)
               {
                    for(int i = 0; i < size - 4; i++)
                    {
                         filename[i] = buffer[i+4];
                    }
                    filename[size-5] = '\0';
               }

               send(new_socket, "ready", sizeof("ready"), 0);

               size = recv(new_socket, buffer, BUF-1, 0);
               buffer[size] = '\0';
               filesize = atoi(buffer);
               printf("%lu bytes\n", filesize);

               strcpy(filepath, argv[2]);
               strcat(filepath, filename);

               printf("%s\n", filepath);

               received_file = fopen(filepath, "w");
               if(received_file == NULL)
               {
                    printf("Error while creating the file\n");
                    return EXIT_FAILURE;
               }
               remain_data = filesize;

               //while(((len = recv(new_socket, buffer, BUF-1, 0)) > 0) && (remain_data > 0))
               while(remain_data > 0)
               {
                    if((len = recv(new_socket, buffer, BUF-1, 0)) > 0)
                    {
                         printf("\n%lu bytes received\n", len);
                         buffer[len] = '\0';
                         fwrite(buffer, sizeof(char), len, received_file);
                         remain_data -= len;
                         printf("Wrote %lu bytes, %lu bytes remain", len, remain_data);

                    }
               }

               printf("\nReceived file\n");

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
