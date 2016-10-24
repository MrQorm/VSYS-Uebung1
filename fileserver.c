//Server fuer Socketuebung
//Tobias Nemecek/Verena Poetzl

#include <errno.h>
#include <limits.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <fcntl.h>

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
  char file_size[256];

  struct stat st;
  long offset;

  unsigned long int filesize, remain_data, sent_bytes;
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

//LIST BEFEHL
           if (strncmp(buffer, "list", 4) == 0)
           {
                DIR *dp;
                struct dirent *ep;
                dp = opendir (argv[2]);

                if (dp != NULL)
                {
                    int filecounter = 0;
                    strcpy(buffer, "\n");

                    while((ep = readdir(dp)) != NULL)
                    {
                         struct stat st;

                         strcpy(filepath, argv[2]);
                         strcat(filepath, "/");
                         strcat(filepath, ep->d_name);

                         if (stat(filepath, &st) == 0);
                         {
                              if(!(!strcmp(ep->d_name, ".") || !strcmp(ep->d_name, "..")))
                              {
                                   filecounter++;
                                   strcat(buffer, ep->d_name);
                                   strcat(buffer, newline);

                                   filesize = st.st_size;

                                   sprintf(temp, "%lu", filesize);

                                   strcat(buffer, "Size in Bytes: ");
                                   strcat(buffer, temp);
                                   strcat(buffer, newline);
                              }
                         }
                    }

                    dp = opendir(argv[2]);

                    sprintf(temp, "%d", filecounter);

                    strcat(buffer, newline);
                    strcat(buffer, "Content of ");
                    strcat(buffer, argv[2]);
                    strcat(buffer, ": ");
                    strcat(buffer, temp);
                    strcat(buffer, " files");
                    strcat(buffer, newline);
                    strcat(buffer, newline);

                    send(new_socket, buffer, strlen(buffer), 0);

                    (void) closedir (dp);
                }
                else
                {
                    perror ("Couldn't open the directory\n");
                }
           }

//GET BEFEHL
           else if(strncmp(buffer, "get ", 4) == 0)
           {
             if(size > 4)
             {
                  for(int i = 4; i < size; i++)
                  {
                       filename[i-4] = buffer[i];
                  }
                  filename[size-5] = '\0';
             }

             //send ready to client
             send(new_socket, "ready\n", sizeof("ready\n"), 0);

             int fd = open(filename, O_RDONLY);

             if(fd == -1)
             {
                  printf("Error while opening file\n");
                  return EXIT_FAILURE;
             }

             if(fstat(fd, &st) < 0)
             {
                  printf("Error fstat\n");
                  return EXIT_FAILURE;
             }

            sprintf(file_size, "%li", st.st_size);

             len = send(new_socket, file_size, sizeof(file_size), 0);
             if(len < 0)
             {
                  printf("Error while sending filesize\n");
                  return EXIT_FAILURE;
             }

             offset = 0;
             sent_bytes = 0;
             remain_data = st.st_size;

             while(((sent_bytes = sendfile(new_socket, fd, &offset, BUF-1)) > 0) && (remain_data > 0))
             {
                  remain_data -= sent_bytes;
                  printf("Sent %lu bytes of data, offset ist now %li and %lu bytes remain\n", sent_bytes, offset, remain_data);
             }

             printf("Finished sending\n\n");

             close (fd);
           }

//PUT BEFEHL
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

               send(new_socket, "ready\n", sizeof("ready\n"), 0);

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

               while(remain_data > 0)
               {
                    if((len = recv(new_socket, buffer, BUF-1, 0)) > 0)
                    {
                         printf("\n%lu bytes received\n", len);
                         buffer[len] = '\0';
                         fwrite(buffer, sizeof(char), len, received_file);
                         remain_data -= len;
                         printf("Wrote %lu bytes, %lu bytes remain\n", len, remain_data);

                    }
               }

               printf("Received file\n\n");

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
